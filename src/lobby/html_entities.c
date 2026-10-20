/* The HTML parser's reset, its character entities (&lt; ...) and the
 * handler of the tags it skips. */
#include "common.h"
#include "html.h"
s32 strncmp(const char *, const char *, s32);

s32 strlen(const char *s);
s32 strncmp(const char *a, const char *b, s32 n);
char *strstr(const char *s, const char *w); /* strstr */

extern s8 gHtmlParseStep;
#define HTML_ENTITY_COUNT 9
extern char *gHtmlEntityNames[]; /* "lt;", "gt;", "quot;", "amp;", "nbsp;", "#034;", "#34;", "#039;", "#39;" */
extern char gHtmlEntityChars[];  /* '<', '>', '"', '&', ' ', '"', '"', '\'', '\'' */
extern char gHtmlAmpersand[];  /* "&" */

/* Sends Html_Parse back to its first step for the next page. */
void Html_ResetLoadStep(void)
{
    gHtmlParseStep = 0;
}

/* Replaces the character entities of s in place (&lt; &gt; &quot; &amp;
 * &nbsp; &#34; &#39;): <IMG ALT=...> and <INPUT ALT=...>'s text. */
void Html_DecodeEntities(char *str)
{
    char *src;
    char *entity;
    s32 i;
    char **name;
    char *next;
    char *amp;

    if ((amp = strstr(str, gHtmlAmpersand)) != NULL) {
        do {
            next = amp + 1;
            for (i = 0, name = gHtmlEntityNames; i < HTML_ENTITY_COUNT; i++, name++) {
                entity = *name;
                if (strncmp(amp + 1, entity, strlen(entity)) == 0) {
                    *amp = gHtmlEntityChars[i];
                    src = amp + strlen(entity);
                    do {
                        src++;
                        amp++;
                        *amp = *src;
                    } while (*src != 0);
                    break;
                }
            }
        } while ((amp = strstr(next, gHtmlAmpersand)) != NULL);
    }
}

/* <!--, <HTML ...>, <MAP ...> and ends that do nothing. */
s32 Html_TagIgnore(u8 **cur, u8 *text)
{
    return 0;
}
