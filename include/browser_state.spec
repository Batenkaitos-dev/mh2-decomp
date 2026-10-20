# Fields of BrowserState (include/browser.h): the browser's state, at
# BrowserWork + 4 (D_00895258 points to it). Regenerate with:
#   python tools/gen_struct.py include/browser_state.spec include/browser.h
name BrowserState
size 0x5C4
0x000 u8 prevPage // the page shown before (set from page)
0x001 u8 mode // index into the running top mode's table of modes
0x002 u8 step // index into the mode's table of steps
0x003 u8 sub
0x004 s32 x
0x008 s32 y
0x00C s32 pageW // the page's width (x scrolls up to pageW - PAGE_VIEW_W)
0x010 s32 pageH // the page's height (y scrolls up to pageH - PAGE_VIEW_H)
0x014 u32 bgColor // the page's background colour (Html_SetBgColor; BROWSER_BLACK at start); Page_DrawObjBack draws bevels' lit edges grey on white (-1)
0x018 u16 style // the page's style flags (Page_SetStyle; 0x2E7 at start; 1, 2, 4, 8, 0x20 ...)
0x01A s16 marginTop // the margins (Page_SetMargins)
0x01C s16 marginBottom
0x01E s16 marginLeft
0x020 s16 marginRight
0x022 s16 vThumbLen // the vertical scroll bar's thumb: length (0x20490 / pageH)
0x024 s16 vThumbY // and position
0x026 s16 hThumbLen // the horizontal one's: length (0x4EB40 / pageW)
0x028 s16 hThumbX // and position
0x02A u8 vScroll // 1: the page is taller than the screen (vertical scroll bar)
0x02B u8 hScroll // 1: wider (horizontal scroll bar)
0x02C u8 histDir // the history move asked for: 1 back, 2 forward
0x02D u8 imgCount // images requested for the page (at most 20)
0x02E u8 page // where the page on show is in its load: enum PageLoad
0x02F u8 prevPageMode // pageMode before the last Page_SetDataMode
0x030 u8 pageMode // the page's <CSV> block kind, CSV_KIND_* (Page_SetDataMode); the session, server-list and result kinds are data pages without form fields (Page_IsDataPage)
0x031 u8 menuReached // 1 from CRS-MENU or COMP-SIGNUP (Page_SetKind)
0x032 u8 unk32
0x033 u8 fromTop // 1 from CRS-TOP until the login id dialog (Page_SetKind)
0x034 u8 plain // 1: the portal's pages 1 and 2 (no frame colour, no back button)
0x035 s8 pageKind // what the page is to the game: enum PageKind (Page_SetKind; PAGE_KIND_NONE before)
0x036 s8 initMode // the argument of Browser_Init (1: ...)
0x037 u8 styled // 1: Page_SetStyle ran for the page
0x038 u8 styleWait // 0x1E frames from Page_SetStyle (no reload meanwhile)
0x039 u8 forceStyle8 // 1: the reply was status 14 or 2 (Page_SetStyle adds style flag 8)
0x03A s8 fade // the fade: 16 in, -16 out (no reload meanwhile)
0x03B char url[0x100] // the URL asked for (Browser_Go)
0x13B char baseUrl[0x100] // the page's base URL, that relative links resolve against (Html_SetBase)
0x33B char msg[0x100] // the page's message (Page_SetMode)
0x43B char urlAnchor[0x100] // the URL's #fragment: the page scrolls to the anchor of that name (Portal_PageReady)
0x55D char formLoginId[0x11] // the login id typed in the page's form (its field 1; Form_SetLoginId, Form_Submit)
0x56E char field2[0x11] // form field 2's
0x57F u8 loginId[0x11] // the MMBB login id (from the NET file)
0x590 u8 password[0x11]
0x5A1 char loginIdSave[0x11] // a login id copy (restored into loginId)
0x5B2 char field3[0x11] // form field 3's
0x5C3 s8 styleMode // Page_SetStyle's mode
