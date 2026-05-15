#ifndef CGFX_EXAMPLES_PROFESSIONAL_APP_THEME_SETUP_H
#define CGFX_EXAMPLES_PROFESSIONAL_APP_THEME_SETUP_H

#include <cgfx/cgfx_api.h>

#include "app_types.h"

void prof_app_font_setup(cgfx_context *ctx);
cgfx_result prof_app_theme_apply(cgfx_window *window,
                                 const prof_app_handles *handles);

#endif
