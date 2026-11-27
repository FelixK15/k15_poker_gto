#include "zstd_stream.h"

#include <cstdint>
#include <random>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace gto::server {
namespace {

/// Several zstd blocks of floats that compress somewhat, like a solver state.
std::string sample_data() {
  std::mt19937 random(7);
  std::normal_distribution<float> value(0.0f, 1.0f);
  std::string data;
  for (int i = 0; i < 400'000; ++i) {
    const float f = i % 3 == 0 ? 0.0f : value(random);
    data.append(reinterpret_cast<const char*>(&f), sizeof(f));
  }
  return data;
}

std::string compress(const std::string& data) {
  std::ostringstream out(std::ios::binary);
  EXPECT_TRUE(write_compressed(out, [&](std::ostream& s) {
    s.write(data.data(), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(s);
  }));
  return std::move(out).str();
}

/// Reads up to `expected` bytes; returns what came out.
std::string read_back(const std::string& stored, std::size_t expected, bool* complete) {
  std::istringstream in(stored, std::ios::binary);
  std::string result(expected, '\0');
  read_maybe_compressed(in, [&](std::istream& s) {
    s.read(result.data(), static_cast<std::streamsize>(result.size()));
    result.resize(static_cast<std::size_t>(s.gcount()));
    *complete = static_cast<bool>(s);
    return *complete;
  });
  return result;
}

TEST(ZstdStream, RoundTripsAndShrinks) {
  const std::string data = sample_data();
  const std::string stored = compress(data);
  EXPECT_LT(stored.size(), data.size());
  bool complete = false;
  EXPECT_EQ(read_back(stored, data.size(), &complete), data);
  EXPECT_TRUE(complete);
}

TEST(ZstdStream, RoundTripsOddSizes) {
  // Sizes around the 1 MiB chunk and not multiples of 4.
  const std::string all = sample_data();
  for (const std::size_t size : {std::size_t{0}, std::size_t{3}, std::size_t{1'048'575},
                                 std::size_t{1'048'576}, std::size_t{1'048'579}}) {
    const std::string data = all.substr(0, size);
    bool complete = false;
    EXPECT_EQ(read_back(compress(data), data.size(), &complete), data) << size;
  }
}

TEST(ZstdStream, ReadsUncompressedDataAsItIs) {
  const std::string data = "K15GTOS1 a state saved before compression";
  bool complete = false;
  EXPECT_EQ(read_back(data, data.size(), &complete), data);
  EXPECT_TRUE(complete);
}

TEST(ZstdStream, EmptyInput) {
  const std::string stored = compress("");
  bool complete = true;
  EXPECT_EQ(read_back(stored, 1, &complete), "");
  EXPECT_FALSE(complete);
}

TEST(ZstdStream, TruncatedFrameIsAShortRead) {
  const std::string data = sample_data();
  const std::string stored = compress(data);
  bool complete = true;
  const std::string got = read_back(stored.substr(0, stored.size() / 2), data.size(), &complete);
  EXPECT_FALSE(complete);
  EXPECT_LT(got.size(), data.size());
  EXPECT_EQ(got, data.substr(0, got.size()));  // Whole chunks only.
}

TEST(ZstdStream, CorruptFrameIsAShortRead) {
  const std::string data = sample_data();
  std::string stored = compress(data);
  stored[stored.size() / 2] = static_cast<char>(stored[stored.size() / 2] ^ 0x5a);
  bool complete = true;
  read_back(stored, data.size(), &complete);
  EXPECT_FALSE(complete);
}

TEST(ZstdStream, ReportsAFailingWriter) {
  std::ostringstream out(std::ios::binary);
  EXPECT_FALSE(write_compressed(out, [](std::ostream&) { return false; }));
}

}  // namespace
}  // namespace gto::server
