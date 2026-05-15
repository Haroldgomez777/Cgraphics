#include "text/font_registry.hpp"

namespace cgfx {

bool FontRegistry::is_valid(cgfx_font_id id) const noexcept {
  (void)this;
  return id == kBuiltinMonoStub;
}

} // namespace cgfx
