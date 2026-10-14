# Fields of BrowserObj (include/browser.h): an object of the page (a text
# run, a link, an image, a form field ...), 0x70 bytes, from the pool at
# BrowserWork + 0xC590 (PageObj_Alloc, PageObj_Free); work->obj[] lists the page's
# objects in draw-command order. Regenerate with:
#   python tools/gen_struct.py include/browser_obj.spec include/browser.h
name BrowserObj
size 0x70
0x00 u8 active // update is called while set
0x01 u8 visible // draw is called while set
0x02 u8 kind // the draw command's kind (Draw_Cmd: 1 text, 2 link, 6 text input ...)
0x03 u8 layer // the list it is in (PageObj_Alloc)
0x04 u8 size // the text size (Draw_Cmd)
0x05 u8 sub // the command's sub-kind; a form's method; an option's index ...
0x06 s8 state
0x07 s8 boxStep // the dialog box's step (DlgBox_Update)
0x0A s16 blink // the cursor's blink alpha (0 to 0xFF)
0x0C s16 blinkStep // its step (15, -15)
0x0E s16 unkE
0x10 s16 unk10
0x14 BrowserObjFunc{4} update // called each frame while active (PageObj lists, unk_730AE0.c)
0x18 BrowserObjFunc{4} draw // called each frame while visible
0x1C struct BrowserObj *prev
0x20 struct BrowserObj *next
0x30 u8 timer
0x34 f32 x
0x38 f32 y
0x40 f32 w
0x44 f32 h
0x4C s32 unk4C
0x50 s32 unk50
0x58 u32 color // the draw command's color (Draw_Cmd)
0x5C u8 unk05C
0x5D u8 field // 1 the login id, 2 the password, 3 ... (Form_Submit keeps their values)
0x5E u8 group // a form's number; a radio button's group (Draw_Cmd kind 8)
0x5F u8 tex // texture slot; a radio button's index in its group
0x60 char *s1 // href, name ...
0x64 char *s2 // value ...
0x68 char *s3 // the draw command's third string
