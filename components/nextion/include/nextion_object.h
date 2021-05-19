#pragma once

#include "nextion_display.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif
    nextion_err_t nextion_set_text(const nextion_display_t *display, const char *obj_name, const char *text);
#ifdef __cplusplus
}
#endif