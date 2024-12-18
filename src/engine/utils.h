#pragma once

#include <cinttypes>
#include <string>

// Sized type aliases
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using i8 = char;
using i16 = short;
using i32 = int;
using i64 = long long;

using f32 = float;
using f64 = double;

using usize = size_t;
using isize = ptrdiff_t;

namespace ste {

inline std::string getAssetPath(const std::string &filename) noexcept {
  return std::string(ASSET_PATH) + "/" + filename;
}

} // namespace ste