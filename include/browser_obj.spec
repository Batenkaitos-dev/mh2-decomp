# Fields of BrowserObj (include/browser.h): an object of the page (a text
# run, a link, an image, a form field ...), 0x70 bytes, from the pool at
# BrowserWork + 0xC590 (PageObj_Alloc, PageObj_Free); work->obj[] lists the page's
# objects in draw-command order. Regenerate with:
#   python tools/gen_struct.py include/browser_obj.spec include/browser.h
name BrowserObj
size 0x70
0x00 u8 active // update is called while set; a page object's draw-command sub-kind (PAGE_IMG_*, PAGE_SELECT_*), nonzero while its slot is used
0x01 u8 visible // draw is called while set; a page object's: on the page view this frame (Page_DrawObj)
0x02 u8 kind // a page object's kind (enum PageObjKind, from Draw_Cmd)
0x03 u8 layer // the list it is in (PageObj_Alloc)
0x04 u8 size // the text size (Draw_Cmd)
0x05 u8 sub // a page object's own state: a button pressed, a radio button or check box checked (1), an image's load state (PAGE_IMG_REQUESTED, _LOADED, _FAILED), a cell's part (PAGE_CELL_*); Draw_Cmd first stores its maxlen there
0x06 s8 state // a dialog object's state: BOBJ_STATE_*, or the dialog box's BDLG_BOX_*; its update sets it, its draw follows it
0x07 s8 glowDir // the dialog box's glow: GLOW_RISING or GLOW_FALLING (DlgBox_Draw)
0x0A s16 blink // the pointer object's: the focus frame's pulse, 0 to 0xFF by glow a frame; the dialog box's: a frame counter for the loading mark, 0 to 600
0x0C s16 glow // the dialog box's: its glow, 0 to 0xFF in steps of 15; the pointer object's: blink's step, +15 or -15 (the focus frame is red while it rises)
0x0E s16 unkE // seen: only cleared, by the dialog objects' create functions
0x10 s16 unk10 // seen: only cleared, as unkE
0x14 BrowserObjFunc{4} update // called each frame while active (PageObj lists, browser_urls.c)
0x18 BrowserObjFunc{4} draw // called each frame while visible
0x1C struct BrowserObj *prev
0x20 struct BrowserObj *next
0x30 u8 timer // a scroll bar's: frames an arrow has shown pressed
0x34 f32 x
0x38 f32 y
0x40 f32 w
0x44 f32 h
0x4C s32 unk4C // seen: only cleared, as unkE
0x50 s32 unk50 // seen: only cleared, as unkE
0x58 s32 color // the draw command's color (Draw_Cmd); the draws test it signed: negative, none
0x5C u8 unk05C // seen: Page_DrawObj passes it to Page_DrawTextLine, which doesn't read it
0x5D u8 field // by kind: a text box's login field (1 the id, 2 the password, 3 ...; Form_Submit keeps their values); text's bold flag; a table's or cell's border width; an image button's kind (low 4 bits, 0x10 its second highlight, 0x80 no frame while it loads)
0x5E u8 group // a form's number; a radio button's group (Draw_Cmd kind 8)
0x5F u8 tex // texture slot; a radio button's index in its group; an option's index in its list (from 1)
0x60 char *s1 // href, name ...
0x64 char *s2 // value ...
0x68 char *s3 // the draw command's third string
