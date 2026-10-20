# Fields of BrowserWork (include/browser.h): the browser's work area,
# 0x1D2C0 bytes at gBrowserWorkMem (gBrowserWork points to it; cleared by
# Browser_Init). Regenerate with:
#   python tools/gen_struct.py include/browser_work.spec include/browser.h
name BrowserWork
size 0x1D2C0
0x00000 char *url // the page's URL (Browser_SetUrl)
0x00004 BrowserState{1476} state // gBrowserState
0x005C8 BrowserSub{36} sub // gBrowserSub
0x005EC struct BrowserObj *obj[500] // the page's objects, in draw-command order
0x00DEC u8 freedObjs // objects freed by their update (state 99; cleared when the dialogs close)
0x00DF0 s32 texHandle[20] // the page textures (0: none), for Prim_DrawSprite
0x00E40 u16 texW[20] // the page textures' widths (by BrowserObj.tex)
0x00E68 u16 texH[20] // and heights
0x00E90 u16 focus
0x00E92 s8 loginSave // what the login dialog's answer keeps: LOGIN_SAVE(the page's kind, LOGIN_KEEP_* or LOGIN_CLEAR); Browser_KeepLogin
0x00E93 s8 keepLoginStep
0x00E94 s8 endReason // why the browser ends: 1 network error, 2 left by the user, 3 the retry failed (>0: only the dialog boxes draw)
0x00E95 u8 netError
0x00E96 s8 endGoOn // 1: the end goes on to gMmbbData->nextUrl (PortalEnd_Step0), 0: Browser_Exit(-1)
0x00E97 u8 formGoLink // set: a form goes to its link object's URL and skips fields named URL (only cleared in the C so far)
0x00E98 s8 result // Browser_Update's result: 0 running, -1 ...
0x00E99 u8 atMmbbUrl
0x00E9A u8 loginPosted
0x00E9B s8 gameMenuNo
0x00E9C u8 firstPage // 1 until the first page is shown (Browser_GoStart only then)
0x00E9D u8 tries // page tries: the second failure gives up (Browser_Retry, Browser_GoStart; 2: give up)
0x00E9E u8 startPosts // posts to gMmbbData->f4 (more than 1 gives up)
0x00E9F u8 texFailOnce // 1: a failed image may fail the page once (Browser_Init's mode 1)
0x00EA0 u8 texCount // page textures in texHandle/texW/texH
0x00EA1 u8 cmdBuf[0x8000] // the draw commands of the page
0x08EA4 char * pageData // the reply data: the page HTML (Html_StartLoad)
0x08EA8 u8 *cmdPtr // write position in cmdBuf
0x08EAC u8 stickReleased // 1: the left stick came back to neutral
0x08EAD u8 stickFirst // 1: this is the first frame of a tilt (moves at once, then every 8 frames)
0x08EAE u8 reqBusy // 1 from a request (BrowserReq_Page, _Post) until its reply (BrowserReq_Poll)
0x08EAF u8 dlgBackImgState // the page's background image object (DlgBackImg_Update): BDLG_BACK_*
0x08EB0 u8 dlgPageState // dialog kind 1's state (DlgPage_Update; 5 close)
0x08EB1 u8 dlgVScrollState // dialog kind 2's (DlgVScroll_Update)
0x08EB2 u8 dlgHScrollState // dialog kind 3's (DlgHScroll_Update)
0x08EB3 u8 dlgStatusState // dialog kind 4's (DlgStatus_Update)
0x08EB4 u8 dlgMenuState // the menu of three (DlgMenu_Update): BDLG_MENU_*
0x08EB5 u8 dlgPointerState // the pointer and focus (DlgPointer_Update): BDLG_POINTER_*
0x08EB6 u8 dlgInputState // the software keyboard (DlgInput_Update): BDLG_INPUT_*
0x08EB7 u8 dlgBoxKind // the dialog box shown (Browser_OpenDialog's kind): BDLG_KIND_*
0x08EB8 u8 errorKind // Browser_SetErrorState's kind
0x08EB9 s8 cursorOnLink // 1: the selection moved onto a link (restarts the cursor blink)
0x08EC0 char kbdText[0x100] // the software keyboard's text
0x08FC0 void *memPages // the name lists (browser_cache_draw.c: pages, images, history, requests): the page nodes' memory
0x08FC4 void *memBufs
0x08FC8 void *memImgs
0x08FCC void *memConnBufs
0x08FD0 void *memHist
0x08FD4 void *memReqs
0x08FD8 BrowserPageNode *pages
0x08FDC BrowserImgNode *imgs
0x08FE0 BrowserPageNode{276} download // the one node of downloads (BrowserReq_StartDownload)
0x090F4 BrowserNode *hist
0x090F8 BrowserReqNode *reqs
0x090FC u8 *connBuf[8]
0x0911C BrowserNode{268} pageList
0x09228 BrowserNode{268} imgList
0x09334 BrowserNode{268} histList
0x09440 BrowserNode *histCur
0x09444 BrowserNode{268} reqList
0x09550 u8 lastNav // the last navigation, enum BrowserNav (BrowserHist_Undo undoes it)
0x09554 BrowserResult{16} reqResult // BrowserReq_Poll result
0x09564 u8 loadSerial // counts the page loads; a node's tag keeps it (BrowserNodeFlags.serial)
0x09568 ArenaBlk{12} heapBlk[0x400] // the records of heap
0x0C568 Arena{40} heap // the page heap, on buf512k
0x0C590 BrowserObj{112} objPool[512] // PageObj_Alloc's pool
0x1A590 u32 tex[24] // textures and palettes (Tex_Add)
0x1A5F0 s8 nTex
0x1A5F4 struct BrowserObj *freeObj // PageObj_Alloc's free list
0x1A5F8 struct BrowserObj *layerHead[9]
0x1A61C struct BrowserObj *layerTail[9]
0x1A640 s32 unk1A640 // seen: only cleared, by PageObj_InitPool
0x1A644 s32 unk1A644 // seen: only cleared, as unk1A640
0x1A648 u16 padHeld // gPadState[0], copied each frame (Browser_ReadPad)
0x1A64A u16 padPrevHeld // the buttons held the frame before (Browser_ReadPad)
0x1A64C u16 padPressed
0x1A64E u16 padReleased
0x1A650 u8 stickTilt // the left stick: tilt (0 to 3)
0x1A651 u8 stickDir // and direction (16 steps)
0x1A652 u8 stick2Tilt // the right stick
0x1A653 u8 stick2Dir
0x1A660 u8 *buf64k // 0x10000 bytes (Mem_Alloc)
0x1A664 u8 *buf512k // 0x80000 bytes (Mem_Alloc)
0x1A668 void * mem64k // the Mem_Alloc handle of buf64k
0x1A66C void * mem512k // the Mem_Alloc handle of buf512k
0x1A670 char formData[0x1000] // the form's query: name=value pairs joined by & (BrowserForm_Add escapes them), cleared before a page's fields are added
0x1B670 char baseUrl[0x100] // the base for relative URLs (Url_SetBase)
0x1B770 char urlLower[0x100] // the URL in lower case (Url_SpecialScheme)
0x1B870 char errorText[0x200] // a failed request's message (BrowserReq_Poll copies Browser_GetText(2, k) there: the result's data)
0x1BA70 u8 errorPage[0x800] // the error page built in memory (Browser_BuildErrorPage), the memory:// page 0
0x1C270 u8 *memPage[3] // the pages of memory://N URLs (HttpReq_Open, URL_MEMORY); [0] is errorPage, [1] and [2] are cleared
0x1C27D u8 exitArgs // the arguments of mmbb://EXIT:n (n: 0-9)
0x1C27E s16 refreshTimer // frames until the META REFRESH URL loads (Html_Refresh)
0x1C280 s16 pressTimer // 3 after a move (Browser_Move, form buttons): the pressed arrow or button is drawn pressed until it ends, and gBrowserSub->moveKind is then cleared
0x1C282 char savedId[0xB] // the kept login id before Browser_SaveLogin wrote it (restored on failure)
0x1C28D char savedPass[9] // and its password
0x1C296 s8 dlgQuestion // the question the message box asked last: 1 dialog kind 4's, 2 kind 6's (BrowserDlg_DrawMsgBox; Browser_OpenDialog clears it)
0x1C298 s32 loadState // 1 while a page is being parsed (Html_StartLoad)
0x1C29C s16 loadResult // what Html_LoadStep returns a pointer to: 0, 1 parsing, 2 done, 3 failed
0x1C2A0 char *loadSrc // the page's HTML
0x1C2A4 s32 loadPos
0x1C2A8 u8 *cert0 // certificate 0x84 (func_8FD770), Http_Init passes it with Http_GetCertSize(0)
0x1C2AC u8 *cert1 // certificate 0x85, 0x800 bytes after cert0 (Http_GetCertSize(1))
0x1C2C0 u8 afsPartInfo[0x1000] // the info area of AFS partition 5, the browser's data files (Http_LoadAfsPartition)
