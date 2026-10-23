/* SPDX-License-Identifier: AGPL-3.0-only */
#pragma once
#include <stdint.h>
#include "plat.h"
#include "dsp/acid.h"
#define APP_NAME "ACID FM1"
#define APP_SPLASH_RGB RGB(185, 241, 90)
#define APP_BLOCK 128u
#define APP_CANVAS_PX (240u * 240u)
#define APP_REDRAW
#define APP_HOST_EXPECT
enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_PATCH0, OBJ_PATCH1, OBJ_PATCH2, OBJ_PATCH3, OBJ_NOBJ };
#define APP_STORE_MAP {0xFC000u, 0xDC000u, 0x97000u, 0x99000u, 0x9B000u, 0x9D000u}
void app_init(void);
void app_render(int32_t *lr,uint32_t n,uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a,const char *b);
void app_redraw(void);
int app_expect(const char *what,int *got);
