#include <assert.h>
#include <stdbool.h>

#include <cgfx/cgfx_api.h>

int main(void) {
  assert(cgfx_context_get_input_routing_trace_enabled(NULL) == false);

  cgfx_context *ctx = NULL;
  assert(cgfx_context_create(&ctx) == CGFX_OK && ctx != NULL);
  assert(cgfx_context_get_input_routing_trace_enabled(ctx) == false);
  cgfx_context_set_input_routing_trace_enabled(ctx, true);
  assert(cgfx_context_get_input_routing_trace_enabled(ctx) == true);
  cgfx_context_set_input_routing_trace_enabled(ctx, false);
  assert(cgfx_context_get_input_routing_trace_enabled(ctx) == false);

  cgfx_context_destroy(ctx);
  return 0;
}
