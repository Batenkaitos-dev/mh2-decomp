/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "browser.h"

char *strcpy(void *dst, const void *src);

void Form_SetField1(u8 *id) {
    strcpy(gBrowserState->field1, id);
    if ((id == NULL) || (*id == 0)) {
        return;
    }
    strcpy(gBrowserState->loginIdSave, id);
}
