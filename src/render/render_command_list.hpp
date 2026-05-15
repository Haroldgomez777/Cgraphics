#pragma once

#include <cgfx/cgfx_api.h>

#include <vector>

namespace cgfx {

enum class RenderCommandType {
  ClearColor = 1,
};

struct RenderClearColorCommand {
  float red{0.0f};
  float green{0.0f};
  float blue{0.0f};
  float alpha{1.0f};
};

struct RenderCommand {
  RenderCommandType type{RenderCommandType::ClearColor};
  RenderClearColorCommand clear{};
};

class RenderCommandList {
public:
  RenderCommandList() = default;

  void reset();
  cgfx_result append_clear_color(float red, float green, float blue, float alpha);

  const std::vector<RenderCommand> &commands() const { return commands_; }
  bool empty() const { return commands_.empty(); }

private:
  std::vector<RenderCommand> commands_{};
};

} // namespace cgfx
