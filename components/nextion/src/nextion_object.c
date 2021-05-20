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