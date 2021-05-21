#include "nextion_object.h"
#include <string.h>

nextion_err_t nextion_set_text(const nextion_display_t *display, const char *obj_name, const char *text)
{
    char *cmd = malloc(40 * sizeof(char));
    bzero(cmd, 40);
    sprintf(cmd, "%s.txt=\"%s\"", obj_name, text);
    nextion_err_t result = display->send_cmd(display, cmd);
    free(cmd);

    return result;
}

nextion_err_t nextion_set_pic(const nextion_display_t *display, const char *obj_name, uint8_t pic)
{
    char *cmd = malloc(40 * sizeof(char));
    bzero(cmd, 40);
    sprintf(cmd, "%s.pic=%i", obj_name, pic);
    nextion_err_t result = display->send_cmd(display, cmd);
    free(cmd);

    return result;
}

nextion_err_t nextion_set_pco(const nextion_display_t *display, const char *obj_name, uint32_t pco)
{
    char *cmd = malloc(40 * sizeof(char));
    bzero(cmd, 40);
    sprintf(cmd, "%s.pco=%i", obj_name, pco);
    nextion_err_t result = display->send_cmd(display, cmd);
    free(cmd);

    return result;
}