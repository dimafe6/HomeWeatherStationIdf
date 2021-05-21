#pragma once

#include "nextion_display.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif
    nextion_err_t nextion_set_text(const nextion_display_t *display, const char *obj_name, const char *text);
    nextion_err_t nextion_set_pic(const nextion_display_t *display, const char *obj_name, uint8_t pic);
    nextion_err_t nextion_set_pco(const nextion_display_t *display, const char *obj_name, uint32_t pco);
#ifdef __cplusplus
}
#endif