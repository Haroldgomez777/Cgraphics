#pragma once

#include <cgfx/cgfx_api.h>

#include <cstdint>

namespace cgfx {

/** Owns font identity validation for the context. Phase 7 ships one built-in deterministic face
 *  used for stub metrics; later phases map handles to platform fonts / bundled TTF. */
class FontRegistry {
public:
  static constexpr cgfx_font_id kBuiltinMonoStub = CGFX_FONT_ID_BUILTIN_DEFAULT;

  bool is_valid(cgfx_font_id id) const noexcept;
};

} // namespace cgfx
