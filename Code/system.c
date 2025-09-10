///includes
#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <devices/input.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <hardware/adkbits.h>
#include <hardware/cia.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/keymap.h>

#include "audio.h"
#include "color.h"
#include "diskio.h"
#include "system.h"
#include "display.h"
#include "keyboard.h"

#include "SDI_headers/SDI_interrupt.h"
///
///defines
#define VPOSMASK  0x01FF00
#define INPUT_BUFFER_SIZE 16
/******************************************************************************
 * The defines below set up the key repeat treshold and key repeat interval   *
 * delays for held keys. Values are number of display frames to wait.         *
 ******************************************************************************/
#define HELD_KEY_TRESHOLD 25
#define HELD_KEY_INTERVAL  2
/******************************************************************************
 * Held key states.                                                           *
 ******************************************************************************/
#define HELD_KEY_NONE   0
#define HELD_KEY_DELAY  1
#define HELD_KEY_REPEAT 2
#define IECODE_NONE     0xFFFF
///
///structs
struct InputBuffer {
	UBYTE* write_ptr; //write pointer
	UBYTE* read_ptr;  //read pointer
	UBYTE* end;       //buffer + INPUT_BUFFER_SIZE
	UBYTE buffer[INPUT_BUFFER_SIZE];
};
///
///globals
ULONG chipset = 0; //will get one of the values from CHIPSET_OCS, CHIPSET_ECS, CHIPSET_AGA
volatile LONG new_frame_flag = 1;
volatile ULONG g_frame_counter = 0;
volatile ULONG g_get_input = FALSE;
struct InputBuffer g_input_buffer;
extern struct Custom custom;
extern struct CIA ciaa, ciab;

//private globals
STATIC UWORD old_dmacon;
STATIC UWORD old_intena;
STATIC UWORD old_adkcon;
STATIC UWORD old_intreq;
STATIC struct View    *old_view;
STATIC struct Window  *old_winptr;
STATIC struct Process *self;
STATIC BPTR initial_lock = NULL; //Lock to current directory before system take over
STATIC BPTR fonts_lock = NULL;   //Lock to the fonts drawer in game directory (if it exists)
STATIC BPTR data_lock = NULL;    //Lock to the data drawer in game directory

STATIC struct MsgPort  *input_mp;
STATIC struct IOStdReq *input_ioreq;
STATIC struct Interrupt input_handler;
STATIC struct Interrupt vblank_handler;
STATIC VOID (*volatile vblankEvents)(VOID);
STATIC volatile ULONG held_key_timer = 0;
STATIC volatile UWORD held_key_code  = IECODE_NONE;
STATIC volatile UBYTE held_key_char  = 0;
STATIC volatile UBYTE held_key_state = HELD_KEY_NONE;
STATIC BOOL	device_ok = FALSE;
///

///setVBlankEvents(function)
/******************************************************************************
 * Every display shall define a function that takes care of the events that   *
 * will be carried out during every vblank. This function sets that function  *
 * to be called at every vblank interrupt in customVBlankHandler().           *
 ******************************************************************************/
VOID setVBlankEvents(VOID (*function)(VOID))
{
	vblankEvents = function;
}
///
///removeVBlankEvents()
VOID nullEvent(VOID) {}

VOID removeVBlankEvents(VOID)
{
	vblankEvents = nullEvent;
}
///

///inputEventToInputBuffer(event)
/******************************************************************************
 * Maps system's input events to ASCII characters and pushes them into the    *
 * input buffer (g_input_buffer).                                             *
 ******************************************************************************/
STATIC INLINE VOID inputEventToInputBuffer(struct InputEvent* inputevent) {
	while (inputevent) {
		if (inputevent->ie_Class == IECLASS_RAWKEY) {
			if (inputevent->ie_Code & IECODE_UP_PREFIX) {
				if (held_key_code == (inputevent->ie_Code & ~IECODE_UP_PREFIX)) {
					held_key_state = HELD_KEY_NONE;
				}
			}
			else {
				UBYTE ch = 0;

				switch (inputevent->ie_Code) {
					case RAW_BACKSPACE:
						if (inputevent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							ch = ASCII_BACKSPACE_ALL;
						else
							ch = ASCII_BACKSPACE;
					break;
					case RAW_TAB:
						ch = ASCII_TAB;
					break;
					case RAW_NUM_ENTER:
					case RAW_RETURN:
						ch = ASCII_RETURN;
					break;
					case RAW_ESC:
						ch = ASCII_ESC;
					break;
					case RAW_DEL:
						if (inputevent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							ch = ASCII_DEL_ALL;
						else
							ch = ASCII_DEL;
					break;
					case RAW_UP:
						ch = ASCII_UP;
					break;
					case RAW_DOWN:
						ch = ASCII_DOWN;
					break;
					case RAW_RIGHT:
						if (inputevent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							ch = ASCII_END;
						else
							ch = ASCII_RIGHT;
					break;
					case RAW_LEFT:
						if (inputevent->ie_Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							ch = ASCII_HOME;
						else
							ch = ASCII_LEFT;
					break;
					default:
						MapRawKey(inputevent, (STRPTR)&ch, 1, NULL);
						if (ch < 32) ch = 0;
					break;
				}

				if (ch) {
					held_key_timer = initDelayFrames(HELD_KEY_TRESHOLD);
					held_key_code  = inputevent->ie_Code;
					held_key_char  = ch;
					held_key_state = HELD_KEY_DELAY;
					pushInputBuffer(ch);
				}
			}
		}
		inputevent = inputevent->ie_NextEvent;
	}
}
///
///heldKeyRepeat()
/******************************************************************************
 * Repeats held keyboard keys by pushing the ASCII equivalent of the last     *
 * pressed (and held key) to the g_input_buffer, respecting the "key repeat   *
 * treshold" and the "key repeat interval" settings.                          *
 ******************************************************************************/
VOID heldKeyRepeat()
{
	switch (held_key_state) {
		case HELD_KEY_DELAY:
			if (testDelay(held_key_timer)) {
				held_key_timer = initDelayFrames(HELD_KEY_INTERVAL);
				held_key_state = HELD_KEY_REPEAT;
			}
		break;
		case HELD_KEY_REPEAT:
			if (testDelay(held_key_timer)) {
				held_key_timer = initDelayFrames(HELD_KEY_INTERVAL);
				pushInputBuffer(held_key_char);
			}
		break;
		case HELD_KEY_NONE:
		break;
	}
}
///
///customInputHandler()
HANDLERPROTO(customInputHandler, ULONG, struct InputEvent* inputevent, APTR userdata)
{
	if (g_get_input) {
		inputEventToInputBuffer(inputevent);
	}

	return 0;
}
///
///customVBlankHandler()
HANDLERPROTO(customVBlankHandler, LONG, struct InputEvent *inputevent, APTR userdata)
{
	new_frame_flag--;
	g_frame_counter++;
	//WARNING: system's VBL handler may set some values on bplcon3 (e.g sprite resolution)
	//WARNING: system's VBL handler also messes with COLOR00 register (sets it to 0)

	vblankEvents();

	if (g_get_input) {
		heldKeyRepeat();
	}

	// Clear the z flag of the CPU to break interrupt server chain!
	return 1;
}
///

///detectChipset()
VOID detectChipset()
{
	ULONG i;
	UBYTE rev_reg = custom.deniseid & 0xFF;

	//Check 30 times to make sure this register does not hold random values
	//NOTE: Getting random values means this is an OCS machine
	for (i = 0; i < 30; i++) {
		if (rev_reg != (custom.deniseid & 0xFF)) {
			chipset = CHIPSET_OCS;
			return;
		}
	}

	//This is either an ECS or an AGA machine
	//NOTE: Upper nibble holds chipset revision so we discard that part
	rev_reg = rev_reg & 0xF;

	switch (rev_reg) {
		case 0xC:
			chipset = CHIPSET_ECS;
		return;
		case 0x8:
			chipset = CHIPSET_AGA;
		return;
		default:
			chipset = CHIPSET_OCS;
	}
}
///
///takeOverSystem()
BOOL takeOverSystem()
{
  self = (struct Process*)FindTask(0);

	if ((fonts_lock = Lock("fonts", SHARED_LOCK))) {
		struct FileInfoBlock* fib = AllocDosObject(DOS_FIB, NULL);
		if (fib) {
			Examine(fonts_lock, fib);
			if (fib->fib_DirEntryType > 0) {
				AssignAdd("FONTS", fonts_lock);
			}
			FreeDosObject(DOS_FIB, fib);
		}
		else {
			UnLock(fonts_lock);
			fonts_lock = NULL;
		}
	}

	data_lock = Lock("data", SHARED_LOCK);
	if (data_lock) initial_lock = CurrentDir(data_lock);

	if (setKeyboardAccess()) {
	  if (openNullDisplay()) {

      // save the current view
      old_view = GfxBase->ActiView;

      // display an empty view
      LoadView(0);
      WaitTOF();
      WaitTOF();

      // disable window pointer
      old_winptr = self->pr_WindowPtr;
      self->pr_WindowPtr = (APTR)-1;

      // save old custom register states
      old_dmacon = custom.dmaconr;
      old_intena = custom.intenar;
      old_adkcon = custom.adkconr;
      old_intreq = custom.intreqr;

      // setup the vertical blank interrupt handler
      vblankEvents = nullEvent;
      vblank_handler.is_Node.ln_Type = NT_INTERRUPT;
      vblank_handler.is_Node.ln_Pri = 127;
      vblank_handler.is_Node.ln_Name = "VBlank_Interrupt";
      vblank_handler.is_Data = 0;
      vblank_handler.is_Code = (APTR)customVBlankHandler;
      AddIntServer(INTB_VERTB, &vblank_handler);

      // setup a custom input handler
      input_mp = CreateMsgPort();
      if (input_mp) {
        input_ioreq = CreateIORequest(input_mp, sizeof(struct IOStdReq));
        if (input_ioreq) {
          if (OpenDevice("input.device", 0, (struct IORequest*)input_ioreq, 0) == 0) {
            device_ok = TRUE;

            input_handler.is_Node.ln_Type = NT_INTERRUPT;
            input_handler.is_Node.ln_Pri = 127;
            input_handler.is_Data = 0;
            input_handler.is_Code = (APTR)customInputHandler;

            input_ioreq->io_Command = IND_ADDHANDLER;
            input_ioreq->io_Data = &input_handler;

            DoIO((struct IORequest*)input_ioreq);
          }
        }
      }

      // Disable interrupts (except i/o port interrupts so we can read the keyboard)
      custom.intena = CLR_ALL ^ (INTF_INTEN | INTF_PORTS);

      PT_InitPTPlayer();

      activateNullCopperList();
      return TRUE;
    }
    else endKeyboardAccess();
  }

	if (data_lock) {
		UnLock(data_lock);
		data_lock = NULL;
	}

	if (fonts_lock) {
		RemAssignList("FONTS", fonts_lock);
		fonts_lock = NULL;
	}

  return FALSE;
}
///
///giveBackSystem()
VOID giveBackSystem()
{
	deactivateNullCopperList();

	PT_TerminatePTPlayer();

	// remove custom input handler
	if (device_ok) {
		input_ioreq->io_Command = IND_REMHANDLER;
		input_ioreq->io_Data    = &input_handler;
		DoIO((struct IORequest*)input_ioreq);
		CloseDevice((struct IORequest*)input_ioreq);
	}

	if (input_ioreq) DeleteIORequest(input_ioreq);
	if (input_mp) DeleteMsgPort(input_mp);

	// restore the old vertical blank interrupt
	RemIntServer(INTB_VERTB, &vblank_handler);

	// restore old custom register states
	custom.dmacon = CLR_ALL;
	custom.intena = CLR_ALL;
	custom.adkcon = CLR_ALL;
	custom.intreq = CLR_ALL;

	custom.dmacon = old_dmacon | DMAF_SETCLR;
	custom.intena = old_intena | INTF_SETCLR;
	custom.adkcon = old_adkcon | ADKF_SETCLR;
	custom.intreq = old_intreq | INTF_SETCLR;

	// enable window pointer
	self->pr_WindowPtr = old_winptr;

	// restore old view
	LoadView(old_view);
	WaitTOF();
	WaitTOF();

	closeNullDisplay();
	endKeyboardAccess();

	if (data_lock) {
		CurrentDir(initial_lock);
		UnLock(data_lock);
		data_lock = NULL;
	}

	if (fonts_lock) {
		RemAssignList("FONTS", fonts_lock);
		fonts_lock = NULL;
	}
}
///
///WaitVBL()
/******************************************************************************
 * Busy waits until the video beam reaches vertical 297 (0x12900 is 297 << 8) *
 * 297 is 256 + 41 (SCREEN_HEIGHT + (DIWSTRT >> 8)).                          *
 * NOTE: This should be coded as so!?                                         *
 * Reads both VPOSR and VPOSHR, masks out the bits that represent horizontal  *
 * beam position and compares it with beam value 297 left shifted to match    *
 * the bit positions on VPOSR+VPOSHR combined.                                *
 * NOTE: This only works on PAL Amigas. Because on NTSC vertical beam pos     *
 *       will never reach 297.                                                *
 ******************************************************************************/
VOID WaitVBL()
{
	while (((*(ULONG*)&custom.vposr) & VPOSMASK) < 0x12900L);
}
///
///waitTOF()
/******************************************************************************
 * Old implementation:                                                        *
 * Busy waits until the 50hz video tick counter in CIAA gets a new value      *
 * (which means the beam has flew back to the top of the frame).              *
 * New implementation:                                                        *
 * VBlank_Interrupt sets the global new_frame_flag to 0. Waiting for this     *
 * event guarantees the sync between program flow and display.                *
 ******************************************************************************/
INLINE VOID waitTOF()
{
	/**** Old implementation ****
	static UBYTE todlo = 0;
	todlo = ciaa.ciatodlow;

	while(ciaa.ciatodlow == todlo);
	*/

	while (new_frame_flag > 0);
}
///
///waitVBeam(line)
/******************************************************************************
 * Busy waits until the video beam reaches the given raster line on the       *
 * current frame. Returns immediately if it has already passed that line      *
 * (being aware that the beam could have flewback to start a new frame).      *
 * Reads both VPOSR and VPOSHR, masks the bits that represent vertical beam   *
 * position and compares it with the given value left shifted to match the    *
 * bit positions for vertical beam position on VPOSR+VPOSHR combined.         *
 ******************************************************************************/
VOID waitVBeam(ULONG line)
{
	ULONG vposr;
	line <<= 8;

	do { vposr = *(ULONG*)&custom.vposr; }
	while (new_frame_flag && (vposr & VPOSMASK) < line);
}
///
///WaitVBeam(line)
/******************************************************************************
 * This version of the function returns control exactly at the vertical beam  *
 * passed. This wait may pass over a vblank so it resets new_frame_flag.      *
 * This version of the function is to be used for timing switchToCopperList() *
 * functions. Consider using the waitVBeam() variant if you plan to call it   *
 * from a display loop.                                                       *
 ******************************************************************************/
VOID WaitVBeam(ULONG line)
{
	ULONG vposr;
	line <<= 8;

	do { vposr = *(ULONG*)&custom.vposr; }
	while ((vposr & VPOSMASK) == line);
	new_frame_flag = 1;
}
///
///busyWaitBlit()
VOID busyWaitBlit()
{
	if (custom.dmaconr & DMAF_BLTDONE) // NOTE:  Because of timing problems with '020+
	{ 											           // processors and/or old chip-sets, you should wait a
	} 											           // short while before testing BBUSY.  A "tst.w DMACONR"
	                                   // before the test will provide the appropriate delay.

	while (custom.dmaconr & DMAF_BLTDONE)
	{
	}
}
///

///initInputBuffer()
VOID initInputBuffer()
{
	g_input_buffer.write_ptr = g_input_buffer.buffer;
	g_input_buffer.read_ptr = g_input_buffer.buffer;
	g_input_buffer.end = g_input_buffer.buffer + INPUT_BUFFER_SIZE;
}
///
///turnInputBufferOn()
VOID turnInputBufferOn()
{
	initInputBuffer();
	g_get_input = TRUE;
}
///
///turnInputBufferOff()
VOID turnInputBufferOff()
{
	g_get_input = FALSE;
	held_key_state = HELD_KEY_NONE;
}
///
///pushInputBuffer(UBYTE ch)
VOID pushInputBuffer(UBYTE ch)
{
	*g_input_buffer.write_ptr = ch;
	g_input_buffer.write_ptr++;
	if (g_input_buffer.write_ptr >= g_input_buffer.end) {
		g_input_buffer.write_ptr = g_input_buffer.buffer;
	}
}
///
///popInputBuffer()
UBYTE popInputBuffer()
{
	UBYTE ch = 0;

	if (g_input_buffer.write_ptr != g_input_buffer.read_ptr) {
		ch = *g_input_buffer.read_ptr;
		g_input_buffer.read_ptr++;
		if (g_input_buffer.read_ptr >= g_input_buffer.end) {
			g_input_buffer.read_ptr = g_input_buffer.buffer;
		}
	}

	return ch;
}
///

/*
// Some utility busy wait functions if ever you need them
VOID busyWaitFrames(ULONG frames);
VOID busyWaitSeconds(ULONG seconds);

///busyWaitFrames()
VOID busyWaitFrames(ULONG frames)
{
	ULONG r = initDelayFrames(frames);
	while (!testDelay(r)) {
	}
	new_frame_flag = 1;
}
///
///busyWaitSeconds()
VOID busyWaitSeconds(ULONG seconds)
{
	ULONG r = initDelaySeconds(seconds);
	while (!testDelay(r)) {
	}
	new_frame_flag = 1;
}
///
*/
