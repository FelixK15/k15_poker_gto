#pragma once

#include <functional>
#include <istream>
#include <ostream>

namespace gto::server {

/// Runs `write` on a stream whose output reaches `sink` as one zstd frame.
/// Returns false if `write` does, or if compressing or writing fails.
bool write_compressed(std::ostream& sink, const std::function<bool(std::ostream&)>& write);

/// Runs `read` on the contents of `source`, decompressed if they start with a
/// zstd frame and as they are otherwise (files from before compression). A
/// corrupt or truncated frame ends the stream early, which `read` sees as a
/// short read. `source` must be seekable.
bool read_maybe_compressed(std::istream& source, const std::function<bool(std::istream&)>& read);

}  // namespace gto::server
