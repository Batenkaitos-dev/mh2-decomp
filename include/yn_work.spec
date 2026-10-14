# Fields of YnWork (include/yn.h), the yn overlay's work area (at 0xAD9C80,
# gYnWork points to it). Regenerate with:
#   python tools/gen_struct.py include/yn_work.spec include/yn.h
name YnWork
size 0x1AF00
0x0000 u32 texPal // the texture handle | the palette handle << 16 (released by Yn_Quit's path)
0x0008 s8 unk8
0x000B s8 cursor // the settings list's cursor (an entry)
0x000C s8 listTop // its first entry shown (four shown)
0x000D s8 proxyCursor // the proxy menu's cursor (0 use, 1 not: proxy.use = 1 - it)
0x000E s8 menuCursor // the menu's cursor (of four, wraps)
0x0017 s8 quitChoice // the quit dialog's choice (left/right toggle; 0: quit)
0x0018 s8 step // the current screen's step
0x0019 s8 backStep // the step to go back to (from a message)
0x001A s8 textMode // Yn_GetTextMode()
0x001C s8 entryCount // the settings file's entries (YnNetcnf_GetCount)
0x001D u8 cardSlots // memory card slots with a card (bit per slot; Yn_ScanCards)
0x001E u8 slot // the memory card slot in use
0x001F u8 unk1F
0x0021 u8 timer
0x0024 u8 scanStep // Yn_ScanCards' state (0-5)
0x0025 u8 triedSlots // slots tried by the connect scan (bit per slot; 0x80 after the first)
0x0026 u8 changed // 1: the chosen settings differ from the saved ones (gNetMode.ynResult = changed + 1)
0x0027 u8 scanSlot // the slot Yn_ScanCards looks at
0x0028 s32 msgTimer // 60 frames for the connect message
0x002C s32 repeatPad
0x0030 u32 repeat // frames the same pad state has been held (Yn_UpdatePadRepeat)
0x003C s8 numDigitPos // the number editor: the digit's position (0 ones .. 4)
0x003D s8 numEditing // 2 while a number is being edited
0x003E u16 numBackup // the value before the edit (put back on cancel)
0x0040 u16 * numTarget // the number being edited (Yn_BeginNumberEdit)
0x0044 u16 numStep // the digit being changed (1, 10, .. 10000)
0x0046 s8 unk46
0x0047 s8 unk47
0x0048 s8 unk48
0x0049 s8 screen // index into gYnScreenItems / gYnScreenItemCounts
0x004A u8 msg // index into gYnMsgItems / gYnMsgItemCounts
0x004B s8 help // index into gYnHelpItems / gYnHelpItemCounts
0x004C s8 helpVariant // added to help for the help line (2 at most)
0x0050 char * editTarget // the string being edited with the keyboard (Yn_OpenKeyboard)
0x0054 char editBuf[0x200]
0x0254 YnCombo{20} combos[30]
0x0ECC YnProxy{520} proxy // copied from and to sub_main (D_5BE6C4, D_5BF400)
0x10D4 YnCardCheck{6} card
0x1180 YnNetcnf{105792} netcnf
0x1AEC0 s32 state // the main state machine (Yn_Update)
0x1AEC4 s32 initArg // Yn_Init's argument (nonzero: no wait for the circle)
0x1AEC8 s32 result
0x1AECC s32 nameScroll // 1: the cursor entry's name is too long and scrolls
0x1AED0 s32 nameOfs // the scroll's offset in the name
0x1AED4 s32 nameTimer // frames to the next scroll step (60, 90, 40)
0x1AED8 s32 nameAtEnd // 1: the scroll reached the end (starts over)
0x1AEDC s32 quitAsked // 1 while the quit dialog shows
0x1AEE0 s32 helpBar // the help bar's kind (Yn_DrawHelpBar)
0x1AEE4 s8 scanResult // the card scan's result (0, -1, -2 card changed, -3)
0x1AEE5 s8 prevSlot // the card slot before the scan
0x1AEE6 u8 okSlots // slots with a readable card (bit per slot)
0x1AEE7 s8 listBusy // nonzero: the list takes no input (only cleared in the C)
0x1AEE8 s8 cardGone // the current card went away (Yn_IsCurrentCardGone)
0x1AEEC s32 cardError[2] // per memory card slot: the last load error (Yn_ScanCardsForConnect)
