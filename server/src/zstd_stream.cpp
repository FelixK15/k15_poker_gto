#include "zstd_stream.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <streambuf>
#include <thread>
#include <vector>

#include <zstd.h>

namespace gto::server {

namespace {

/// Solver states are mostly floats, which compress little beyond the first
/// levels; level 3 (zstd's default) keeps saving close to disk speed.
constexpr int kLevel = 3;

/// The data is cut into chunks of this size (the last one shorter), and the
/// bytes of each chunk are regrouped by their position in a 4-byte value: all
/// first bytes, then all second bytes, and so on. Signs and exponents of
/// floats then sit next to each other and compress, where zstd finds next to
/// nothing in the floats as they are. A multiple of 4.
constexpr std::size_t kChunk = std::size_t{1} << 20;

void shuffle(const char* in, std::size_t size, char* out) {
  const std::size_t values = size / 4;
  for (std::size_t i = 0; i < values; ++i) {
    for (std::size_t b = 0; b < 4; ++b) out[b * values + i] = in[4 * i + b];
  }
  std::memcpy(out + 4 * values, in + 4 * values, size % 4);
}

void unshuffle(const char* in, std::size_t size, char* out) {
  const std::size_t values = size / 4;
  for (std::size_t i = 0; i < values; ++i) {
    for (std::size_t b = 0; b < 4; ++b) out[4 * i + b] = in[b * values + i];
  }
  std::memcpy(out + 4 * values, in + 4 * values, size % 4);
}

class CompressingBuffer : public std::streambuf {
 public:
  explicit CompressingBuffer(std::ostream& sink)
      : sink_(sink), context_(ZSTD_createCCtx()), in_(kChunk), shuffled_(kChunk),
        out_(ZSTD_CStreamOutSize()) {
    ZSTD_CCtx_setParameter(context_, ZSTD_c_compressionLevel, kLevel);
    ZSTD_CCtx_setParameter(context_, ZSTD_c_checksumFlag, 1);
    // Ignored (with an error code) by a zstd built without threads.
    const unsigned workers = std::clamp(std::thread::hardware_concurrency() / 2, 1u, 8u);
    ZSTD_CCtx_setParameter(context_, ZSTD_c_nbWorkers, static_cast<int>(workers));
    setp(in_.data(), in_.data() + in_.size());
  }
  ~CompressingBuffer() override { ZSTD_freeCCtx(context_); }
  CompressingBuffer(const CompressingBuffer&) = delete;
  CompressingBuffer& operator=(const CompressingBuffer&) = delete;

  /// Ends the frame. Nothing may be written afterwards.
  bool finish() { return compress(ZSTD_e_end); }

 protected:
  /// Called only when the chunk is full.
  int_type overflow(int_type c) override {
    if (!compress(ZSTD_e_continue)) return traits_type::eof();
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
      *pptr() = traits_type::to_char_type(c);
      pbump(1);
    }
    return traits_type::not_eof(c);
  }

 private:
  bool compress(ZSTD_EndDirective mode) {
    if (failed_) return false;
    const auto size = static_cast<std::size_t>(pptr() - pbase());
    shuffle(pbase(), size, shuffled_.data());
    ZSTD_inBuffer input{shuffled_.data(), size, 0};
    for (;;) {
      ZSTD_outBuffer output{out_.data(), out_.size(), 0};
      const std::size_t left = ZSTD_compressStream2(context_, &output, &input, mode);
      if (ZSTD_isError(left)) return fail();
      sink_.write(out_.data(), static_cast<std::streamsize>(output.pos));
      if (!sink_) return fail();
      const bool done = mode == ZSTD_e_end ? left == 0 : input.pos == input.size;
      if (done) break;
    }
    setp(in_.data(), in_.data() + in_.size());
    return true;
  }

  bool fail() {
    failed_ = true;
    return false;
  }

  std::ostream& sink_;
  ZSTD_CCtx* context_;
  std::vector<char> in_;
  std::vector<char> shuffled_;
  std::vector<char> out_;
  bool failed_ = false;
};

class DecompressingBuffer : public std::streambuf {
 public:
  explicit DecompressingBuffer(std::istream& source)
      : source_(source), context_(ZSTD_createDCtx()), in_(ZSTD_DStreamInSize()),
        shuffled_(kChunk), out_(kChunk) {}
  ~DecompressingBuffer() override { ZSTD_freeDCtx(context_); }
  DecompressingBuffer(const DecompressingBuffer&) = delete;
  DecompressingBuffer& operator=(const DecompressingBuffer&) = delete;

 protected:
  int_type underflow() override {
    if (gptr() < egptr()) return traits_type::to_int_type(*gptr());
    // Decompress one whole chunk, or what is left of the frame.
    std::size_t filled = 0;
    while (filled < kChunk && !failed_ && !frame_done_) {
      if (input_.pos == input_.size) {
        source_.read(in_.data(), static_cast<std::streamsize>(in_.size()));
        const auto got = static_cast<std::size_t>(source_.gcount());
        if (got == 0) {
          failed_ = true;  // Truncated: the frame never ended.
          break;
        }
        input_ = {in_.data(), got, 0};
      }
      ZSTD_outBuffer output{shuffled_.data() + filled, kChunk - filled, 0};
      const std::size_t left = ZSTD_decompressStream(context_, &output, &input_);
      if (ZSTD_isError(left)) {
        failed_ = true;
        break;
      }
      filled += output.pos;
      if (left == 0) frame_done_ = true;
    }
    // A chunk cut short by damage is not whole, so it cannot be unshuffled.
    if (failed_ || filled == 0) return traits_type::eof();
    unshuffle(shuffled_.data(), filled, out_.data());
    setg(out_.data(), out_.data(), out_.data() + filled);
    return traits_type::to_int_type(*gptr());
  }

 private:
  std::istream& source_;
  ZSTD_DCtx* context_;
  std::vector<char> in_;
  std::vector<char> shuffled_;
  std::vector<char> out_;
  ZSTD_inBuffer input_{nullptr, 0, 0};
  bool frame_done_ = false;
  bool failed_ = false;
};

}  // namespace

bool write_compressed(std::ostream& sink, const std::function<bool(std::ostream&)>& write) {
  CompressingBuffer buffer(sink);
  std::ostream out(&buffer);
  if (!write(out)) return false;
  out.flush();
  return static_cast<bool>(out) && buffer.finish();
}

bool read_maybe_compressed(std::istream& source,
                           const std::function<bool(std::istream&)>& read) {
  const std::istream::pos_type start = source.tellg();
  std::array<unsigned char, 4> magic{};
  source.read(reinterpret_cast<char*>(magic.data()), magic.size());
  const bool compressed =
      source.gcount() == 4 && (static_cast<unsigned>(magic[0]) |
                               static_cast<unsigned>(magic[1]) << 8 |
                               static_cast<unsigned>(magic[2]) << 16 |
                               static_cast<unsigned>(magic[3]) << 24) == ZSTD_MAGICNUMBER;
  source.clear();
  source.seekg(start);
  if (!source) return false;
  if (!compressed) return read(source);
  DecompressingBuffer buffer(source);
  std::istream in(&buffer);
  return read(in);
}

}  // namespace gto::server
