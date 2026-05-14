#include <cgfx/cgfx_api.h>
#include <stddef.h>
#include <stdio.h>

int main(void) {
  cgfx_context *ctx = NULL;

  if (cgfx_context_create(&ctx) != CGFX_OK || ctx == NULL) {
    fprintf(stderr, "cgfx_context_create failed\n");
    return 1;
  }

  cgfx_poll_events(ctx);
  cgfx_context_destroy(ctx);
  return 0;
}
