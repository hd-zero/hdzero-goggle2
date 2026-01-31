#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl/lvgl.h>

#include "ui/ui_main_menu.h"

extern page_pack_t pp_source;

void source_status_timer();
void source_toggle();
void source_cycle();
void source_toggle_cam();

#ifdef __cplusplus
}
#endif
