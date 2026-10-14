/* The HTML parser's start and character entities. */
#include "common.h"
#include "html.h"
s32 strncmp(const char *, const char *, s32);

s32 strlen(const char *s);
s32 strncmp(const char *a, const char *b, s32 n);
char *strstr(const char *s, const char *w); /* strstr */

extern s8 gHtmlParseStep;
extern char *gHtmlEntityNames[]; /* "lt;", "gt;", "quot;", "amp;", "nbsp;", "#034;", "#34;", "#039;", "#39;" */
extern char gHtmlEntityChars[];  /* '<', '>', '"', '&', ' ', '"', '"', '\'', '\'' */
extern char D_00839ED8[];  /* "&" */

void Html_ResetLoadStep(void) {
    gHtmlParseStep = 0;
}

/* Replaces the character entities of s in place (&lt; &gt; &quot; &amp;
 * &nbsp; &#34; &#39;): <IMG ALT=...> and <INPUT ALT=...>'s text. */
void Html_DecodeEntities(char *s) {
    char *q;
    char *e;
    s32 i;
    char **name;
    char *next;
    char *p;

    if ((p = strstr(s, D_00839ED8)) != NULL) {
        do {
            next = p + 1;
            for (i = 0, name = gHtmlEntityNames; i < 9; i++, name++) {
                e = *name;
                if (strncmp(p + 1, e, strlen(e)) == 0) {
                    *p = gHtmlEntityChars[i];
                    q = p + strlen(e);
                    do {
                        q++;
                        p++;
                        *p = *q;
                    } while (*q != 0);
                    break;
                }
            }
        } while ((p = strstr(next, D_00839ED8)) != NULL);
    }
}

/* <!--, <HTML ...>, <MAP ...> and ends that do nothing. */
s32 Html_TagIgnore(u8 **cur, u8 *text) {
    return 0;
}
