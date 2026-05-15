#pragma once

#include <cstddef>
#include <cstdint>

namespace cgfx {

/** Invalid UTF-8 advances one scalar mapped to replacement for parity with README. */
constexpr char32_t kTextUtf8Replacement = char32_t{0xFFFD};

inline bool text_utf8_next_codepoint(const char *&p, const char *end,
                                     char32_t &out) noexcept {
  if (!p || p >= end) {
    return false;
  }
  const auto u0 = static_cast<unsigned char>(*p);
  if (u0 < 0x80U) {
    out = static_cast<char32_t>(u0);
    p += 1;
    return true;
  }
  if ((u0 & 0xE0U) == 0xC0U) {
    if (end - p < 2) {
      out = kTextUtf8Replacement;
      p += 1;
      return true;
    }
    const auto u1 = static_cast<unsigned char>(p[1]);
    if ((u1 & 0xC0U) != 0x80U) {
      out = kTextUtf8Replacement;
      p += 1;
      return true;
    }
    const char32_t cp = (static_cast<char32_t>(u0 & 0x1FU) << 6) |
                        static_cast<char32_t>(u1 & 0x3FU);
    if (cp < 0x80U) {
      out = kTextUtf8Replacement;
      p += 1;
      return true;
    }
    out = cp;
    p += 2;
    return true;
  }
  if ((u0 & 0xF0U) == 0xE0U) {
    if (end - p < 3) {
      out = kTextUtf8Replacement;
      p += 1;
      return true;
    }
    const auto u1 = static_cast<unsigned char>(p[1]);
    const auto u2 = static_cast<unsigned char>(p[2]);
    if (((u1 & 0xC0U) != 0x80U) || ((u2 & 0xC0U) != 0x80U)) {
      out = kTextUtf8Replacement;
      p += 1;
      return true;
    }
    const char32_t cp = (static_cast<char32_t>(u0 & 0x0FU) << 12) |
                        (static_cast<char32_t>(u1 & 0x3FU) << 6) |
                        static_cast<char32_t>(u2 & 0x3FU);
    if (cp < 0x800U || (cp >= 0xD800U && cp <= 0xDFFF)) {
      out = kTextUtf8Replacement;
      p += 1;
      return true;
    }
    out = cp;
    p += 3;
    return true;
  }
  if ((u0 & 0xF8U) == 0xF0U) {
    if (end - p < 4) {
      out = kTextUtf8Replacement;
      p += 1;
      return true;
    }
    const auto u1 = static_cast<unsigned char>(p[1]);
    const auto u2 = static_cast<unsigned char>(p[2]);
    const auto u3 = static_cast<unsigned char>(p[3]);
    if (((u1 & 0xC0U) != 0x80U) || ((u2 & 0xC0U) != 0x80U) ||
        ((u3 & 0xC0U) != 0x80U)) {
      out = kTextUtf8Replacement;
      p += 1;
      return true;
    }
    const char32_t cp =
        (static_cast<char32_t>(u0 & 0x07U) << 18) |
        (static_cast<char32_t>(u1 & 0x3FU) << 12) |
        (static_cast<char32_t>(u2 & 0x3FU) << 6) |
        static_cast<char32_t>(u3 & 0x3FU);
    if (cp < 0x10000U || cp > 0x10FFFFU) {
      out = kTextUtf8Replacement;
      p += 1;
      return true;
    }
    out = cp;
    p += 4;
    return true;
  }
  out = kTextUtf8Replacement;
  p += 1;
  return true;
}

} // namespace cgfx
