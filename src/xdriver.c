/****************************************************************/
/*			Apple IIgs emulator			*/
/*			Copyright 1996 Kent Dickey		*/
/*								*/
/*	This code may not be used in a commercial product	*/
/*	without prior written permission of the author.		*/
/*								*/
/*	You may freely distribute this code.			*/ 
/*								*/
/*	You can contact the author at kentd@cup.hp.com.		*/
/*	HP has nothing to do with this software.		*/
/****************************************************************/

const char rcsid_xdriver_c[] = "@(#)$Header: xdriver.c,v 1.149 99/07/19 00:58:28 kentd Exp $";

#define X_SHARED_MEM

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <time.h>
#include <stdlib.h>

#ifdef X_SHARED_MEM
# include <sys/ipc.h>
# include <sys/shm.h>
# include <X11/extensions/XShm.h>
#endif

int XShmQueryExtension(Display *display);

#include "defc.h"

#include "protos_xdriver.h"


#define FONT_NAME_STATUS	"8x13"

extern int Verbose;

extern int g_limit_speed;

extern int g_fast_disk_emul;

extern int g_warp_pointer;
extern int g_screen_depth;
extern int g_force_depth;
int g_screen_mdepth = 0;

extern int g_swap_paddles;
extern int g_invert_paddles;
extern int _Xdebug;

extern int g_send_sound_to_file;

int	g_has_focus = 0;
int	g_auto_repeat_on = -1;
int	g_x_shift_control_state = 0;


Display *display = 0;
Window a2_win;
GC a2_winGC;
XFontStruct *text_FontSt;
Colormap g_a2_colormap = 0;
Colormap g_default_colormap = 0;
int	g_needs_cmap = 0;

#ifdef X_SHARED_MEM
int g_use_shmem = 1;
#else
int g_use_shmem = 0;
#endif

byte *data_text[2];
byte *data_hires[2];
byte *data_superhires;
byte *data_border_special;
byte *data_border_sides;

byte *dint_border_sides;
byte *dint_border_special;
byte *dint_main_win;

XImage *ximage_text[2];
XImage *ximage_hires[2];
XImage *ximage_superhires;
XImage *ximage_border_special;
XImage *ximage_border_sides;

XImage *xint_border_sides;
XImage *xint_border_special;
XImage *xint_main_win;

#ifdef X_SHARED_MEM
XShmSegmentInfo shm_text_seginfo[2];
XShmSegmentInfo shm_hires_seginfo[2];
XShmSegmentInfo shm_superhires_seginfo;
XShmSegmentInfo shm_border_special_seginfo;
XShmSegmentInfo shm_border_sides_seginfo;

XShmSegmentInfo shmint_border_sides_seginfo;
XShmSegmentInfo shmint_border_special_seginfo;
XShmSegmentInfo shmint_main_win_seginfo;
#endif

int Max_color_size = 256;

XColor xcolor_a2vid_array[256];
XColor xcolor_superhires_array[256];
XColor dummy_xcolor;
int g_palette_internal[200];
word32 g_palette_8to16[256];
word32 g_a2palette_8to16[256];
word32 g_palette_8to24[256];
word32 g_a2palette_8to24[256];

int	g_alt_left_up = 1;
int	g_alt_right_up = 1;

extern word32 g_full_refresh_needed;

extern int g_border_sides_refresh_needed;
extern int g_border_special_refresh_needed;
extern int g_status_refresh_needed;

extern int lores_colors[];
extern int hires_colors[];
extern int g_cur_a2_stat;

extern int g_a2vid_palette;

extern int g_installed_full_superhires_colormap;

extern int g_screen_redraw_skip_amt;

extern word32 a2_screen_buffer_changed;
extern byte *a2_line_ptr[];
extern void *a2_line_xim[];
extern int a2_line_stat[];
extern int a2_line_left_edge[];
extern int a2_line_right_edge[];
extern int a2_line_full_left_edge[];
extern int a2_line_full_right_edge[];

Cursor	g_cursor;
Pixmap	g_cursor_shape;
Pixmap	g_cursor_mask;

XColor	g_xcolor_black = { 0, 0x0000, 0x0000, 0x0000, DoRed|DoGreen|DoBlue, 0 };
XColor	g_xcolor_white = { 0, 0xffff, 0xffff, 0xffff, DoRed|DoGreen|DoBlue, 0 };

int g_depth_attempt_list[] = { 8, 15, 24, 16 };


#define X_EVENT_LIST_ALL_WIN						\
	(ExposureMask | ButtonPressMask | ButtonReleaseMask |		\
	 OwnerGrabButtonMask | KeyPressMask | KeyReleaseMask |		\
	 KeymapStateMask | ColormapChangeMask | FocusChangeMask)

#define X_BASE_WIN_EVENT_LIST						\
	(X_EVENT_LIST_ALL_WIN | PointerMotionMask | ButtonMotionMask)

#define X_A2_WIN_EVENT_LIST						\
	(X_BASE_WIN_EVENT_LIST)

int	g_num_a2_keycodes = 0;

int a2_key_to_xsym[][3] = {
	{ 0x35,	XK_Escape,	0 },
	{ 0x7a,	XK_F1,	0 },
	{ 0x7b,	XK_F2,	0 },
	{ 0x63,	XK_F3,	0 },
	{ 0x76,	XK_F4,	0 },
	{ 0x60,	XK_F5,	0 },
	{ 0x61,	XK_F6,	0 },
#if 0
	/* these keys have special KEGS functions */
	{ 0x62,	XK_F7,	0 },
	{ 0x64,	XK_F8,	0 },
	{ 0x65,	XK_F9,	0 },
	{ 0x6d,	XK_F10,	0 },
	{ 0x67,	XK_F11,	0 },
	{ 0x6f,	XK_F12,	0 },
	{ 0x69,	XK_F13,	0 },
	{ 0x6b,	XK_F14,	0 },
	{ 0x71,	XK_F15,	0 },
#endif
	{ 0x7f, XK_Pause, XK_Break },
	{ 0x32,	'`', '~' },		/* Key number 18? */
	{ 0x12,	'1', '!' },
	{ 0x13,	'2', '@' },
	{ 0x14,	'3', '#' },
	{ 0x15,	'4', '$' },
	{ 0x17,	'5', '%' },
	{ 0x16,	'6', '^' },
	{ 0x1a,	'7', '&' },
	{ 0x1c,	'8', '*' },
	{ 0x19,	'9', '(' },
	{ 0x1d,	'0', ')' },
	{ 0x1b,	'-', '_' },
	{ 0x18,	'=', '+' },
	{ 0x33,	XK_BackSpace, 0 },
	{ 0x72,	XK_Insert, 0 },		/* Help? */
/*	{ 0x73,	XK_Home, 0 },		alias XK_Home to be XK_KP_Equal! */
	{ 0x74,	XK_Page_Up, 0 },
	{ 0x47,	XK_Num_Lock, XK_Clear },	/* Clear */
	{ 0x51,	XK_KP_Equal, XK_Home },		/* Note XK_Home alias! */
	{ 0x4b,	XK_KP_Divide, 0 },
	{ 0x43,	XK_KP_Multiply, 0 },

	{ 0x30,	XK_Tab, 0 },
	{ 0x0c,	'q', 'Q' },
	{ 0x0d,	'w', 'W' },
	{ 0x0e,	'e', 'E' },
	{ 0x0f,	'r', 'R' },
	{ 0x11,	't', 'T' },
	{ 0x10,	'y', 'Y' },
	{ 0x20,	'u', 'U' },
	{ 0x22,	'i', 'I' },
	{ 0x1f,	'o', 'O' },
	{ 0x23,	'p', 'P' },
	{ 0x21,	'[', '{' },
	{ 0x1e,	']', '}' },
	{ 0x2a,	0x5c, '|' },	/* backslash, bar */
	{ 0x75,	XK_Delete, 0 },
	{ 0x77,	XK_End, 0 },
	{ 0x79,	XK_Page_Down, 0 },
	{ 0x59,	XK_KP_7, XK_KP_Home },
	{ 0x5b,	XK_KP_8, XK_KP_Up },
	{ 0x5c,	XK_KP_9, XK_KP_Page_Up },
	{ 0x4e,	XK_KP_Subtract, 0 },

	{ 0x39,	XK_Caps_Lock, 0 },
	{ 0x00,	'a', 'A' },
	{ 0x01,	's', 'S' },
	{ 0x02,	'd', 'D' },
	{ 0x03,	'f', 'F' },
	{ 0x05,	'g', 'G' },
	{ 0x04,	'h', 'H' },
	{ 0x26,	'j', 'J' },
	{ 0x28,	'k', 'K' },
	{ 0x25,	'l', 'L' },
	{ 0x29,	';', ':' },
	{ 0x27,	0x27, '"' },	/* single quote */
	{ 0x24,	XK_Return, 0 },
	{ 0x56,	XK_KP_4, XK_KP_Left },
	{ 0x57,	XK_KP_5, 0 },
	{ 0x58,	XK_KP_6, XK_KP_Right },
	{ 0x45,	XK_KP_Add, 0 },

	{ 0x38,	XK_Shift_L, XK_Shift_R },
	{ 0x06,	'z', 'Z' },
	{ 0x07,	'x', 'X' },
	{ 0x08,	'c', 'C' },
	{ 0x09,	'v', 'V' },
	{ 0x0b,	'b', 'B' },
	{ 0x2d,	'n', 'N' },
	{ 0x2e,	'm', 'M' },
	{ 0x2b,	',', '<' },
	{ 0x2f,	'.', '>' },
	{ 0x2c,	'/', '?' },
	{ 0x3e,	XK_Up, 0 },
	{ 0x53,	XK_KP_1, XK_KP_End },
	{ 0x54,	XK_KP_2, XK_KP_Down },
	{ 0x55,	XK_KP_3, XK_KP_Page_Down },

	{ 0x36,	XK_Control_L, XK_Control_R },
	{ 0x3a,	XK_Print, XK_Sys_Req },		/* Option */
	{ 0x37,	XK_Scroll_Lock, 0 },		/* Command */
	{ 0x31,	' ', 0 },
	{ 0x3b,	XK_Left, 0 },
	{ 0x3d,	XK_Down, 0 },
	{ 0x3c,	XK_Right, 0 },
	{ 0x52,	XK_KP_0, XK_KP_Insert },
	{ 0x41,	XK_KP_Decimal, XK_KP_Separator },
	{ 0x4c,	XK_KP_Enter, 0 },
	{ -1, -1, -1 }
};


void
update_color_array(int col_num, int a2_color)
{
	XColor	*xcol, *xcol2;
	int	palette;
	int	doit;
	int	full;

	if(col_num >= 256 || col_num < 0) {
		printf("update_color_array called: col: %03x\n", col_num);
		set_halt(1);
		return;
	}

	full = g_installed_full_superhires_colormap;

	xcol = &xcolor_superhires_array[col_num];
	xcol2 = &xcolor_a2vid_array[col_num];
	palette = col_num >> 4;
	doit = 1;
	if(!full && palette == g_a2vid_palette) {
		xcol2 = &dummy_xcolor;
		doit = 0;
	}

	if(g_screen_depth != 8) {
		/* redraw whole superhires for now */
		g_full_refresh_needed = -1;
	}

	convert_to_xcolor(xcol, xcol2, col_num, a2_color, doit);
}

#define MAKE_4(val)	( (val << 12) + (val << 8) + (val << 4) + val)
#define	MAKE_COL16(red, green, blue)				\
		(((red) << 11) + \
		((green) << 6) + \
		((blue) << 1))
#define	MAKE_COL24(red, green, blue)				\
		(((red) << 20) + ((green) << 12) + ((blue) << 4))

void
convert_to_xcolor(XColor *xcol, XColor *xcol2, int col_num,
			int a2_color, int doit)
{
	int	red, green, blue;

	red = (a2_color >> 8) & 0xf;
	green = (a2_color >> 4) & 0xf;
	blue = (a2_color) & 0xf;
	if(g_screen_depth == 8 || !doit) {
		xcol->red = MAKE_4(red);
		xcol2->red = MAKE_4(red);
		xcol->green = MAKE_4(green);
		xcol2->green = MAKE_4(green);
		xcol->blue = MAKE_4(blue);
		xcol2->blue = MAKE_4(blue);

		xcol->flags = DoRed | DoGreen | DoBlue;
		xcol2->flags = DoRed | DoGreen | DoBlue;
	} else {
		g_palette_8to16[col_num] = MAKE_COL16(red, green, blue);
		g_palette_8to24[col_num] = MAKE_COL24(red, green, blue);
	}
}


extern int flash_state;

void
update_physical_colormap()
{
	int	palette;
	int	full;
	int	i;


	full = g_installed_full_superhires_colormap;

	if(!full) {
		palette = g_a2vid_palette << 4;
		for(i = 0; i < 16; i++) {
			convert_to_xcolor(&xcolor_a2vid_array[palette + i],
				&dummy_xcolor, palette + i, lores_colors[i], 1);
		}
	}

	if(full) {
		if(g_screen_depth != 8) {
			/* Must redraw all, for now */
			a2_screen_buffer_changed = -1;
			g_full_refresh_needed = -1;
			return;
		}
		/* Not really supported any more */
		XStoreColors(display, g_a2_colormap,
			&xcolor_superhires_array[0], Max_color_size);
	} else {
		XStoreColors(display, g_a2_colormap,
			&xcolor_a2vid_array[0], Max_color_size);
	}
}

void
show_xcolor_array()
{
	int i;

	for(i = 0; i < 256; i++) {
		printf("%02x: %08x, %08x\n", i,
			g_palette_8to16[i], g_palette_8to24[i]);
			
#if 0
		printf("%02x: %04x %04x %04x, %02x %x\n",
			i, xcolor_array[i].red, xcolor_array[i].green,
			xcolor_array[i].blue, (word32)xcolor_array[i].pixel,
			xcolor_array[i].flags);
#endif
	}
}


int
my_error_handler(Display *dp, XErrorEvent *ev)
{
	char msg[1024];
	XGetErrorText(dp, ev->error_code, msg, 1000);
	printf("X Error code %s\n", msg);
	fflush(stdout);

	return 0;
}

void
xdriver_end()
{

	printf("xdriver_end\n");
	if(display) {
		x_auto_repeat_on(1);
		XFlush(display);
	}
}

void
show_colormap(char *str, Colormap cmap, int index1, int index2, int index3)
{
	XColor	xcol;
	int	i;
	int	pix;

	printf("Show colormap: %08x = %s, cmap cells: %d,%d,%d\n",
			(int)cmap, str, index1, index2, index3);
	for(i = 0; i < index1 + index2 + index3; i++) {
		pix = i;
		if(i >= index1) {
			pix = (i-index1)*index1;
			if(i >= (index1 + index2)) {
				pix = (i - index1 - index2)*index2*index1;
			}
		}
		if(i == 0 && index1 < 250) {
			pix = 0x842;
		}
		xcol.pixel = pix;
		XQueryColor(display, cmap, &xcol);
		printf("Cell %03x: pix: %03x, R:%04x, G:%04x, B:%04x\n",
			i, (int)xcol.pixel, xcol.red, xcol.green, xcol.blue);
	}
}


void
dev_video_init()
{
	int	tmp_array[0x80];
	XGCValues new_gc;
	XSetWindowAttributes win_attr;
	XSizeHints my_winSizeHints;
	XClassHint my_winClassHint;
	XTextProperty my_winText;
	XVisualInfo *visualList;
	Visual	*vis;
	char	**font_ptr;
	char	cursor_data;
	word32	create_win_list;
	int	depth;
	int	len;
	int	cmap_alloc_amt;
	int	cnt;
	int	font_height;
	int	screen_num;
	char	*myTextString[1];
	int	red, green, blue;
	word32	lores_col;
	int	ret;
	int	i;
	int	keycode;

	printf("Preparing X Windows graphics system\n");

	g_num_a2_keycodes = 0;
	for(i = 0; i <= 0x7f; i++) {
		tmp_array[i] = 0;
	}
	for(i = 0; i < 0x7f; i++) {
		keycode = a2_key_to_xsym[i][0];
		if(keycode < 0) {
			g_num_a2_keycodes = i;
			break;
		} else if(keycode > 0x7f) {
			printf("a2_key_to_xsym[%d] = %02x!\n", i, keycode);
				exit(2);
		} else {
			if(tmp_array[keycode]) {
				printf("a2_key_to_x[%d] = %02x used by %d\n",
					i, keycode, tmp_array[keycode] - 1);
			}
			tmp_array[keycode] = i + 1;
		}
	}

#if 0
	printf("Setting _Xdebug = 1, makes X synchronous\n");
	_Xdebug = 1;
#endif

	display = XOpenDisplay(NULL);
	if(display == NULL) {
		fprintf(stderr, "Can't open display\n");
		exit(1);
	}

	vid_printf("Just opened display = %08x\n", (word32)display);
	fflush(stdout);

	screen_num = DefaultScreen(display);

	len = sizeof(g_depth_attempt_list)/sizeof(int);
	if(g_force_depth > 0) {
		/* Only use the requested user depth */
		len = 1;
		g_depth_attempt_list[0] = g_force_depth;
	}
	vis = 0;
	for(i = 0; i < len; i++) {
		depth = g_depth_attempt_list[i];

		vis = x_try_find_visual(display, depth, screen_num,
			&visualList);
		if(vis != 0) {
			break;
		}
	}
	if(vis == 0) {
		fprintf(stderr, "Couldn't find any visuals at any depth!\n");
		exit(2);
	}

	g_default_colormap = XDefaultColormap(display, screen_num);
	if(!g_default_colormap) {
		printf("g_default_colormap == 0!\n");
		exit(4);
	}

	g_a2_colormap = -1;
	cmap_alloc_amt = AllocNone;
	if(g_needs_cmap) {
		cmap_alloc_amt = AllocAll;
	}
	g_a2_colormap = XCreateColormap(display,
			RootWindow(display,screen_num), vis, cmap_alloc_amt);

	vid_printf("g_a2_colormap: %08x, main: %08x\n",
			(word32)g_a2_colormap, (word32)g_default_colormap);

	if(g_needs_cmap && g_a2_colormap == g_default_colormap) {
		printf("A2_colormap = default colormap!\n");
		exit(4);
	}

	/* and define cursor */
	cursor_data = 0;
	g_cursor_shape = XCreatePixmapFromBitmapData(display,
		RootWindow(display,screen_num), &cursor_data, 1, 1, 1, 0, 1);
	g_cursor_mask = XCreatePixmapFromBitmapData(display,
		RootWindow(display,screen_num), &cursor_data, 1, 1, 1, 0, 1);

	g_cursor = XCreatePixmapCursor(display, g_cursor_shape,
			g_cursor_mask, &g_xcolor_black, &g_xcolor_white, 0, 0);

	XFreePixmap(display, g_cursor_shape);
	XFreePixmap(display, g_cursor_mask);

	XFlush(display);

	win_attr.event_mask = X_A2_WIN_EVENT_LIST;
	win_attr.colormap = g_a2_colormap;
	win_attr.backing_store = WhenMapped;
	if(g_warp_pointer) {
		win_attr.cursor = g_cursor;
	} else {
		win_attr.cursor = None;
	}

	vid_printf("About to a2_win\n");
	fflush(stdout);

	create_win_list = CWEventMask | CWBackingStore | CWCursor;
	create_win_list |= CWColormap;
	a2_win = XCreateWindow(display, RootWindow(display, screen_num),
		0, 0, BASE_WINDOW_WIDTH, BASE_WINDOW_HEIGHT,
		0, g_screen_depth, InputOutput, vis,
		create_win_list, &win_attr);

	if(g_needs_cmap) {
	}
	XSetWindowColormap(display, a2_win, g_a2_colormap);

	XFlush(display);

/* Check for XShm */
#ifdef X_SHARED_MEM
	if(g_use_shmem) {
		ret = XShmQueryExtension(display);
		if(ret == 0) {
			printf("XShmQueryExt ret: %d\n", ret);
			printf("not using shared memory\n");
			g_use_shmem = 0;
		} else {
			printf("Will use shared memory for X\n");
		}
	}

	if(g_use_shmem) {
		g_use_shmem = get_shm(&ximage_text[0], display, &data_text[0],
			vis, &shm_text_seginfo[0], 0);
	}
	if(g_use_shmem) {
		g_use_shmem = get_shm(&ximage_text[1], display, &data_text[1],
			vis, &shm_text_seginfo[1], 0);
	}
	if(g_use_shmem) {
		g_use_shmem = get_shm(&ximage_hires[0], display, &data_hires[0],
			vis, &shm_hires_seginfo[0], 0);
	}
	if(g_use_shmem) {
		g_use_shmem = get_shm(&ximage_hires[1], display, &data_hires[1],
			vis, &shm_hires_seginfo[1], 0);
	}
	if(g_use_shmem) {
		g_use_shmem = get_shm(&ximage_superhires, display,
			&data_superhires, vis, &shm_superhires_seginfo, 0);
	}
	if(g_use_shmem) {
		g_use_shmem = get_shm(&ximage_border_special, display,
			&data_border_special,vis,&shm_border_special_seginfo,1);
	}
	if(g_use_shmem) {
		g_use_shmem = get_shm(&ximage_border_sides, display,
			&data_border_sides, vis, &shm_border_sides_seginfo, 2);
	}
	if(g_screen_depth != 8 && g_use_shmem) {
		/* allocate special buffers for this screen */
		g_use_shmem &= get_shm(&xint_border_sides, display,
			&dint_border_sides, vis,
			&shmint_border_sides_seginfo, 0x10 + 2);
		g_use_shmem &= get_shm(&xint_border_special, display,
			&dint_border_special, vis,
			&shmint_border_special_seginfo, 0x10 + 1);
		g_use_shmem &= get_shm(&xint_main_win, display,
			&dint_main_win, vis,
			&shmint_main_win_seginfo, 0x10);
	}
#endif
	if(!g_use_shmem) {
		if(g_screen_redraw_skip_amt < 0) {
			g_screen_redraw_skip_amt = 7;
		}
		printf("Not using shared memory, setting skip_amt = %d\n",
			g_screen_redraw_skip_amt);

		printf("Calling get_ximage!\n");

		ximage_text[0] = get_ximage(display, &data_text[0], vis, 0);
		ximage_text[1] = get_ximage(display, &data_text[1], vis, 0);
		ximage_hires[0] = get_ximage(display, &data_hires[0], vis, 0);
		ximage_hires[1] = get_ximage(display, &data_hires[1], vis, 0);
		ximage_superhires = get_ximage(display, &data_superhires,vis,0);
		ximage_border_special = get_ximage(display,
						&data_border_special, vis, 1);
		ximage_border_sides = get_ximage(display, &data_border_sides,
									vis, 2);

		xint_border_special = get_ximage(display,
					&dint_border_special, vis, 0x10 + 1);
		xint_border_sides = get_ximage(display,
					&dint_border_sides, vis, 0x10 + 2);
		xint_main_win = get_ximage(display,
					&dint_main_win, vis, 0x10);
	}

	vid_printf("data_text[0]: %08x, g_use_shmem: %d\n",
				(word32)data_text[0], g_use_shmem);

	/* Done with visualList now */
	XFree(visualList);

	for(i = 0; i < 256; i++) {
		xcolor_a2vid_array[i].pixel = i;
		xcolor_superhires_array[i].pixel = i;
		lores_col = lores_colors[i & 0xf];
		convert_to_xcolor(&xcolor_a2vid_array[i],
			&xcolor_superhires_array[i], i,
			lores_col, 1);
		red = (lores_col >> 8) & 0xf;
		green = (lores_col >> 4) & 0xf;
		blue = (lores_col) & 0xf;
		g_a2palette_8to16[i] = MAKE_COL16(red, green, blue);
		g_a2palette_8to24[i] = MAKE_COL24(red, green, blue);
	}

	if(g_needs_cmap) {
		XStoreColors(display, g_a2_colormap, &xcolor_a2vid_array[0],
			Max_color_size);
	}

	g_installed_full_superhires_colormap = !g_needs_cmap;
	
	myTextString[0] = "Sim65";

	XStringListToTextProperty(myTextString, 1, &my_winText);

	my_winSizeHints.flags = PSize | PMinSize | PMaxSize;
	my_winSizeHints.width = BASE_WINDOW_WIDTH;
	my_winSizeHints.height = BASE_WINDOW_HEIGHT;
	my_winSizeHints.min_width = BASE_WINDOW_WIDTH;
	my_winSizeHints.min_height = BASE_WINDOW_HEIGHT;
	my_winSizeHints.max_width = BASE_WINDOW_WIDTH;
	my_winSizeHints.max_height = BASE_WINDOW_HEIGHT;
	my_winClassHint.res_name = "KEGS";
	my_winClassHint.res_class = "KEGS";

	XSetWMProperties(display, a2_win, &my_winText, &my_winText, 0,
		0, &my_winSizeHints, 0, &my_winClassHint);
	XMapRaised(display, a2_win);

	XSync(display, False);

	a2_winGC = XCreateGC(display, a2_win, 0, (XGCValues *) 0);
	font_ptr = XListFonts(display, FONT_NAME_STATUS, 4, &cnt);

	vid_printf("act_cnt of fonts: %d\n", cnt);
	for(i = 0; i < cnt; i++) {
		vid_printf("Font %d: %s\n", i, font_ptr[i]);
	}
	fflush(stdout);
	text_FontSt = XLoadQueryFont(display, FONT_NAME_STATUS);
	vid_printf("font # returned: %08x\n", (word32)(text_FontSt->fid));
	font_height = text_FontSt->ascent + text_FontSt->descent;
	vid_printf("font_height: %d\n", font_height);

	vid_printf("widest width: %d\n", text_FontSt->max_bounds.width);

	new_gc.font = text_FontSt->fid;
	new_gc.fill_style = FillSolid;
	XChangeGC(display, a2_winGC, GCFillStyle | GCFont, &new_gc);

	/* XSync(display, False); */
#if 0
/* MkLinux for Powermac depth 15 has bugs--this was to try to debug them */
	if(g_screen_depth == 15) {
		/* draw phony screen */
		ptr16 = (word16 *)dint_main_win;
		for(i = 0; i < 320*400; i++) {
			ptr16[i] = 0;
		}
		for(i = 0; i < 400; i++) {
			for(j = 0; j < 640; j++) {
				sh = (j / 20) & 0x1f;
				val = sh;
				val = val;
				*ptr16++ = val;
			}
		}
		XPutImage(display, a2_win, a2_winGC, xint_main_win,
			0, 0,
			BASE_MARGIN_LEFT, BASE_MARGIN_TOP,
			640, 400);
		XFlush(display);
	}
#endif


	XFlush(display);
	fflush(stdout);
}

Visual *
x_try_find_visual(Display *display, int depth, int screen_num,
		XVisualInfo **visual_list_ptr)
{
	XVisualInfo *visualList;
	XVisualInfo vTemplate;
	int	visualsMatched;
	int	mdepth;
	int	needs_cmap;
	int	visual_chosen;
	int	match8, match24;
	int	i;

	vTemplate.screen = screen_num;
	vTemplate.depth = depth;

	visualList = XGetVisualInfo(display,
		(VisualScreenMask | VisualDepthMask),
		&vTemplate, &visualsMatched);

	vid_printf("visuals matched: %d\n", visualsMatched);
	if(visualsMatched == 0) {
		return (Visual *)0;
	}

	visual_chosen = -1;
	needs_cmap = 0;
	for(i = 0; i < visualsMatched; i++) {
		printf("Visual %d\n", i);
		printf("	id: %08x, screen: %d, depth: %d, class: %d\n",
			(word32)visualList[i].visualid,
			visualList[i].screen,
			visualList[i].depth,
			visualList[i].class);
		printf("	red: %08lx, green: %08lx, blue: %08lx\n",
			visualList[i].red_mask,
			visualList[i].green_mask,
			visualList[i].blue_mask);
		printf("	cmap size: %d, bits_per_rgb: %d\n",
			visualList[i].colormap_size,
			visualList[i].bits_per_rgb);
		match8 = (visualList[i].class == PseudoColor);
		match24 = (visualList[i].class == TrueColor);
		if((depth == 8) && match8) {
			visual_chosen = i;
			Max_color_size = visualList[i].colormap_size;
			needs_cmap = 1;
			break;
		}
		if((depth != 8) && match24) {
			visual_chosen = i;
			Max_color_size = -1;
			needs_cmap = 0;
			break;
		}
	}

	if(visual_chosen < 0) {
		printf("Couldn't find any good visuals at depth %d!\n",
			depth);
		return (Visual *)0;
	}

	printf("Chose visual: %d, max_colors: %d\n", visual_chosen,
		Max_color_size);

	g_screen_depth = depth;
	mdepth = depth;
	if(depth == 15) {
		mdepth = 16;
	}
	if(depth == 24) {
		mdepth = 32;
	}
	g_screen_mdepth = mdepth;
	g_needs_cmap = needs_cmap;
	*visual_list_ptr = visualList;

	return visualList[visual_chosen].visual;
}


#ifdef X_SHARED_MEM
int xshm_error = 0;

int
xhandle_shm_error(Display *display, XErrorEvent *event)
{
	xshm_error = 1;
	return 0;
}

int
get_shm(XImage **xim_in, Display *display, byte **databuf, Visual *visual,
		XShmSegmentInfo *seginfo, int extended_info)
{
	XImage *xim;
	int	(*old_x_handler)(Display *, XErrorEvent *);
	int	width;
	int	height;
	int	depth;

	width = A2_WINDOW_WIDTH;
	height = A2_WINDOW_HEIGHT;
	if((extended_info & 0xf) == 1) {
		/* border at top and bottom of screen */
		width = X_A2_WINDOW_WIDTH;
		height = X_A2_WINDOW_HEIGHT - A2_WINDOW_HEIGHT + 2*8;
	}
	if((extended_info & 0xf) == 2) {
		/* border at sides of screen */
		width = EFF_BORDER_WIDTH;
		height = A2_WINDOW_HEIGHT;
	}

	depth = 8;
	if(extended_info & 0x10) {
		depth = g_screen_depth;
	}
	xim = XShmCreateImage(display, visual, depth, ZPixmap,
		(char *)0, seginfo, width, height);
	if(extended_info & 0x10) {
		/* check mdepth! */
		if(xim->bits_per_pixel != g_screen_mdepth) {
			printf("shm_ximage bits_per_pix: %d != %d\n",
					xim->bits_per_pixel, g_screen_mdepth);
		}
	}

	vid_printf("xim: %08x\n", (word32)xim);
	*xim_in = xim;
	if(xim == 0) {
		return 0;
	}

	/* It worked, we got it */
	seginfo->shmid = shmget(IPC_PRIVATE, xim->bytes_per_line * xim->height,
		IPC_CREAT | 0777);
	vid_printf("seginfo->shmid = %d\n", seginfo->shmid);
	if(seginfo->shmid < 0) {
		XDestroyImage(xim);
		return 0;
	}

	/* Still working */
	seginfo->shmaddr = (char *)shmat(seginfo->shmid, 0, 0);
	vid_printf("seginfo->shmaddr: %08x\n", (word32)seginfo->shmaddr);
	if(seginfo->shmaddr == ((char *) -1)) {
		XDestroyImage(xim);
		return 0;
	}

	/* Still working */
	xim->data = seginfo->shmaddr;
	seginfo->readOnly = False;

	/* XShmAttach will trigger X error if server is remote, so catch it */
	xshm_error = 0;
	old_x_handler = XSetErrorHandler(xhandle_shm_error);

	XShmAttach(display, seginfo);
	XSync(display, False);


	vid_printf("about to RMID the shmid\n");
	shmctl(seginfo->shmid, IPC_RMID, 0);

	XFlush(display);
	XSetErrorHandler(old_x_handler);

	if(xshm_error) {
		XDestroyImage(xim);
		/* We could release the shared mem segment, but by doing the */
		/* RMID, it will go away when we die now, so just leave it */
		printf("Not using shared memory\n");
		return 0;
	}

	*databuf = (byte *)xim->data;
	vid_printf("Sharing memory. xim: %08x, xim->data: %08x\n",
		(word32)xim, (word32)xim->data);

	return 1;
}
#endif	/* X_SHARED_MEM */

XImage *
get_ximage(Display *display, byte **data_ptr, Visual *vis, int extended_info)
{
	XImage	*xim;
	byte	*ptr;
	int	width;
	int	height;
	int	depth;
	int	mdepth;

	width = A2_WINDOW_WIDTH;
	height = A2_WINDOW_HEIGHT;
	if((extended_info & 0xf) == 1) {
		/* border at top and bottom of screen */
		width = X_A2_WINDOW_WIDTH;
		height = X_A2_WINDOW_HEIGHT - A2_WINDOW_HEIGHT + 2*8;
	}
	if((extended_info & 0xf) == 2) {
		/* border at sides of screen */
		width = EFF_BORDER_WIDTH;
		height = A2_WINDOW_HEIGHT;
	}

	depth = 8;
	mdepth = 8;
	if(extended_info & 0x10) {
		depth = g_screen_depth;
		mdepth = g_screen_mdepth;
	}
	ptr = (byte *)malloc((width * height * mdepth) >> 3);

	vid_printf("ptr: %08x\n", (word32)ptr);

	if(ptr == 0) {
		printf("malloc for data failed, mdepth: %d\n", mdepth);
		exit(2);
	}

	*data_ptr = ptr;

	xim = XCreateImage(display, vis, depth, ZPixmap, 0,
		(char *)ptr, width, height, 8, 0);
	if(extended_info & 0x10) {
		/* check mdepth! */
		if(xim->bits_per_pixel != g_screen_mdepth) {
			printf("shm_ximage bits_per_pix: %d != %d\n",
					xim->bits_per_pixel, g_screen_mdepth);
		}
	}

	vid_printf("xim.data: %08x\n", (word32)xim->data);

	return xim;
}




double status1_time = 0.0;
double status2_time = 0.0;
double status3_time = 0.0;

#define MAX_STATUS_LINES	7
#define X_LINE_LENGTH		88


char	g_status_buf[MAX_STATUS_LINES][X_LINE_LENGTH + 1];

void
update_status_line(int line, const char *string)
{
	char	*buf;
	const char *ptr;
	int	i;

	if(line >= MAX_STATUS_LINES || line < 0) {
		printf("update_status_line: line: %d!\n", line);
		exit(1);
	}

	ptr = string;
	buf = &(g_status_buf[line][0]);
	for(i = 0; i < X_LINE_LENGTH; i++) {
		if(*ptr) {
			buf[i] = *ptr++;
		} else {
			buf[i] = ' ';
		}
	}

	buf[X_LINE_LENGTH] = 0;
}

void
redraw_status_lines()
{
	char	*buf;
	int	line;
	int	height;
	int	margin;
	word32	white, black;

	height = text_FontSt->ascent + text_FontSt->descent;
	margin = text_FontSt->ascent;

	white = (g_a2vid_palette << 4) + 0xf;
	black = (g_a2vid_palette << 4) + 0x0;
	if(g_screen_depth != 8) {
		white = (2 << (g_screen_depth - 1)) - 1;
		black = 0;
	}
	XSetForeground(display, a2_winGC, white);
	XSetBackground(display, a2_winGC, black);

	for(line = 0; line < MAX_STATUS_LINES; line++) {
		buf = &(g_status_buf[line][0]);
		XDrawImageString(display, a2_win, a2_winGC, 0,
			X_A2_WINDOW_HEIGHT + height*line + margin,
			buf, strlen(buf));
	}

	XFlush(display);
}



word32 g_cycs_in_xredraw = 0;
word32 g_refresh_bytes_xfer = 0;

void
x_refresh_ximage()
{
	register word32 start_time;
	register word32 end_time;
	int	start;
	word32	mask;
	int	line;
	int	left_pix, right_pix;
	int	left, right;
	XImage	*last_xim, *cur_xim;

	if(g_border_sides_refresh_needed) {
		g_border_sides_refresh_needed = 0;
		x_refresh_border_sides();
	}
	if(g_border_special_refresh_needed) {
		g_border_special_refresh_needed = 0;
		x_refresh_border_special();
	}

	if(a2_screen_buffer_changed == 0) {
		return;
	}

	GET_ITIMER(start_time);

	start = -1;
	mask = 1;
	last_xim = (XImage *)-1;

	left_pix = 640;
	right_pix = 0;

	for(line = 0; line < 25; line++) {
		if((g_full_refresh_needed & (1 << line)) != 0) {
			left = a2_line_full_left_edge[line];
			right = a2_line_full_right_edge[line];
		} else {
			left = a2_line_left_edge[line];
			right = a2_line_right_edge[line];
		}

		if(!(a2_screen_buffer_changed & mask)) {
			/* No need to update this line */
			/* Refresh previous chunks of lines, if any */
			if(start >= 0) {
				x_refresh_lines(last_xim, start, line,
					left_pix, right_pix);
				start = -1;
				left_pix = 640;
				right_pix = 0;
			}
		} else {
			/* Need to update this line */
			cur_xim = a2_line_xim[line];
			if(start < 0) {
				start = line;
				last_xim = cur_xim;
			}
			if(cur_xim != last_xim) {
				/* do the refresh */
				x_refresh_lines(last_xim, start, line,
					left_pix, right_pix);
				last_xim = cur_xim;
				start = line;
				left_pix = left;
				right_pix = right;
			}
			left_pix = MIN(left, left_pix);
			right_pix = MAX(right, right_pix);
		}
		mask = mask << 1;
	}

	if(start >= 0) {
		x_refresh_lines(last_xim, start, 25, left_pix, right_pix);
	}

	a2_screen_buffer_changed = 0;

	g_full_refresh_needed = 0;

	/* And redraw border rectangle? */

	XFlush(display);

	GET_ITIMER(end_time);

	g_cycs_in_xredraw += (end_time - start_time);
}

void
x_refresh_lines(XImage *xim, int start_line, int end_line, int left_pix,
		int right_pix)
{
	int	srcy;

	if(left_pix >= right_pix || left_pix < 0 || right_pix <= 0) {
		printf("x_refresh_lines: lines %d to %d, pix %d to %d\n",
			start_line, end_line, left_pix, right_pix);
		printf("a2_screen_buf_ch:%08x, g_full_refr:%08x\n",
			a2_screen_buffer_changed, g_full_refresh_needed);
		show_a2_line_stuff();
#ifdef HPUX
		U_STACK_TRACE();
#endif
		set_halt(1);
	}

	srcy = 16*start_line;

	if(xim == ximage_border_special) {
		/* fix up y pos in src */
		printf("x_refresh_lines called, ximage_border_special!!\n");
		srcy = 0;
	}

	if(g_screen_depth > 8 && g_screen_depth <= 16) {
		/* translate from 8-bit pseudo to correct visual */
		x_convert_8to16(xim, xint_main_win, left_pix, srcy,
			(right_pix - left_pix), 16*(end_line - start_line));
		xim = xint_main_win;
	}
	if(g_screen_depth == 24) {
		/* translate from 8-bit pseudo to correct visual */
		x_convert_8to24(xim, xint_main_win, left_pix, srcy,
			(right_pix - left_pix), 16*(end_line - start_line));
		xim = xint_main_win;
	}
	g_refresh_bytes_xfer += 16*(end_line - start_line) *
							(right_pix - left_pix);
#ifdef X_SHARED_MEM
	if(g_use_shmem) {
		XShmPutImage(display, a2_win, a2_winGC,
			xim, left_pix, srcy,
			BASE_MARGIN_LEFT + left_pix,
					BASE_MARGIN_TOP + 16*start_line,
			right_pix - left_pix, 16*(end_line - start_line),False);
	}
#endif
	if(!g_use_shmem) {
		XPutImage(display, a2_win, a2_winGC, xim,
			left_pix, srcy,
			BASE_MARGIN_LEFT + left_pix,
					BASE_MARGIN_TOP + 16*start_line,
			right_pix - left_pix, 16*(end_line - start_line));
	}
}

void
x_redraw_border_sides_lines(int end_x, int width, int start_line,
	int end_line)
{
	XImage	*xim;

	if(start_line < 0 || width < 0) {
		return;
	}

#if 0
	printf("redraw_border_sides lines:%d-%d from %d to %d\n",
		start_line, end_line, end_x - width, end_x);
#endif
	xim = ximage_border_sides;
	if(g_screen_depth > 8 && g_screen_depth <= 16) {
		/* translate from 8-bit pseudo to correct visual */
		x_convert_8to16(xim, xint_border_sides, 0, 16*start_line,
			width, 16*(end_line - start_line));
		xim = xint_border_sides;
	}
	if(g_screen_depth == 24) {
		/* translate from 8-bit pseudo to correct visual */
		x_convert_8to24(xim, xint_border_sides, 0, 16*start_line,
			width, 16*(end_line - start_line));
		xim = xint_border_sides;
	}
	g_refresh_bytes_xfer += 16 * (end_line - start_line) * width;

#ifdef X_SHARED_MEM
	if(g_use_shmem) {
		XShmPutImage(display, a2_win, a2_winGC, xim,
			0, 16*start_line,
			end_x - width, BASE_MARGIN_TOP + 16*start_line,
			width, 16*(end_line - start_line), False);
	}
#endif
	if(!g_use_shmem) {
		XPutImage(display, a2_win, a2_winGC, xim,
			0, 16*start_line,
			end_x - width, BASE_MARGIN_TOP + 16*start_line,
			width, 16*(end_line - start_line));
	}


}

void
x_refresh_border_sides()
{
	int	old_width;
	int	prev_line;
	int	width;
	int	mode;
	int	i;

#if 0
	printf("refresh border sides!\n");
#endif

	/* redraw left sides */
	x_redraw_border_sides_lines(BORDER_WIDTH, BORDER_WIDTH, 0, 25);

	/* right side--can be "jagged" */
	prev_line = -1;
	old_width = -1;
	for(i = 0; i < 25; i++) {
		mode = (a2_line_stat[i] >> 4) & 7;
		width = EFF_BORDER_WIDTH;
		if(mode == MODE_SUPER_HIRES) {
			width = BORDER_WIDTH;
		}
		if(width != old_width) {
			x_redraw_border_sides_lines(X_A2_WINDOW_WIDTH,
				old_width, prev_line, i);
			prev_line = i;
			old_width = width;
		}
	}

	x_redraw_border_sides_lines(X_A2_WINDOW_WIDTH, old_width, prev_line,25);

	XFlush(display);
}

void
x_refresh_border_special()
{
	XImage	*xim;
	int	width, height;

	width = X_A2_WINDOW_WIDTH;
	height = BASE_MARGIN_TOP;

	xim = ximage_border_special;
	if(g_screen_depth > 8 && g_screen_depth <= 16) {
		/* translate from 8-bit pseudo to correct visual */
		x_convert_8to16(xim, xint_border_special, 0, 0,
			width, BASE_MARGIN_BOTTOM);
		x_convert_8to16(xim, xint_border_special, 0, BASE_MARGIN_BOTTOM,
			width, BASE_MARGIN_TOP);
		xim = xint_border_special;
	}
	if(g_screen_depth == 24) {
		/* translate from 8-bit pseudo to correct visual */
		x_convert_8to24(xim, xint_border_special, 0, 0,
			width, BASE_MARGIN_BOTTOM);
		x_convert_8to24(xim, xint_border_special, 0, BASE_MARGIN_BOTTOM,
			width, BASE_MARGIN_TOP);
		xim = xint_border_special;
	}
	g_refresh_bytes_xfer += 16 * width *
				(BASE_MARGIN_TOP + BASE_MARGIN_BOTTOM);
#ifdef X_SHARED_MEM
	if(g_use_shmem) {
		XShmPutImage(display, a2_win, a2_winGC, xim,
			0, 0,
			0, BASE_MARGIN_TOP + A2_WINDOW_HEIGHT,
			width, BASE_MARGIN_BOTTOM, False);

		XShmPutImage(display, a2_win, a2_winGC, xim,
			0, BASE_MARGIN_BOTTOM,
			0, 0,
			width, BASE_MARGIN_TOP, False);
	}
#endif
	if(!g_use_shmem) {
		XPutImage(display, a2_win, a2_winGC, xim,
			0, 0,
			0, BASE_MARGIN_TOP + A2_WINDOW_HEIGHT,
			width, BASE_MARGIN_BOTTOM);
		XPutImage(display, a2_win, a2_winGC, xim,
			0, BASE_MARGIN_BOTTOM,
			0, 0,
			width, BASE_MARGIN_TOP);
	}
}

void
x_convert_8to16(XImage *xim, XImage *xout, int startx, int starty,
		int width, int height)
{
	byte	*indata, *inptr;
	word16	*outdata, *outptr;
	word32	*palptr;
	int	x, y;

	indata = (byte *)xim->data;
	outdata = (word16 *)xout->data;

	if(indata == data_superhires) {
		palptr = &(g_palette_8to16[0]);
	} else {
		palptr = &(g_a2palette_8to16[0]);
	}

	for(y = 0; y < height; y++) {
		if(indata == data_border_special) {
			inptr = &indata[(starty + y)*X_A2_WINDOW_WIDTH+startx];
			outptr = &outdata[(starty+ y)*X_A2_WINDOW_WIDTH+startx];
		} else if(indata == data_border_sides) {
			inptr = &indata[(starty + y)*EFF_BORDER_WIDTH + startx];
			outptr = &outdata[(starty + y)*EFF_BORDER_WIDTH+startx];
		} else {
			inptr = &indata[(starty + y)*A2_WINDOW_WIDTH + startx];
			outptr = &outdata[(starty + y)*A2_WINDOW_WIDTH +startx];
		}
		for(x = 0; x < width; x++) {
			*outptr++ = palptr[*inptr++];
		}
	}
}

void
x_convert_8to24(XImage *xim, XImage *xout, int startx, int starty,
		int width, int height)
{
	byte	*indata, *inptr;
	word32	*outdata, *outptr;
	word32	*palptr;
	int	x, y;

	indata = (byte *)xim->data;
	outdata = (word32 *)xout->data;

	if(indata == data_superhires) {
		palptr = &(g_palette_8to24[0]);
	} else {
		palptr = &(g_a2palette_8to24[0]);
	}

	for(y = 0; y < height; y++) {
		if(indata == data_border_special) {
			inptr = &indata[(starty + y)*X_A2_WINDOW_WIDTH+startx];
			outptr = &outdata[(starty+ y)*X_A2_WINDOW_WIDTH+startx];
		} else if(indata == data_border_sides) {
			inptr = &indata[(starty + y)*EFF_BORDER_WIDTH + startx];
			outptr = &outdata[(starty + y)*EFF_BORDER_WIDTH+startx];
		} else {
			inptr = &indata[(starty + y)*A2_WINDOW_WIDTH + startx];
			outptr = &outdata[(starty + y)*A2_WINDOW_WIDTH +startx];
		}
		for(x = 0; x < width; x++) {
			*outptr++ = palptr[*inptr++];
		}
	}
}




#if 0
void
redraw_border()
{

}
#endif


#define KEYBUFLEN	128

int g_num_check_input_calls = 0;
int g_check_input_flush_rate = 2;

void
check_input_events()
{
	XEvent	ev;
	int	len;
	int	motion;
	int	refresh_needed;

	g_num_check_input_calls--;
	if(g_num_check_input_calls < 0) {
		len = XPending(display);
		g_num_check_input_calls = g_check_input_flush_rate;
	} else {
		len = QLength(display);
	}

	motion = 0;
	refresh_needed = 0;
	while(len > 0) {
		XNextEvent(display, &ev);
		len--;
		if(len == 0) {
			/* Xaccel on linux only buffers one X event */
			/*  must look for more now */
			len = XPending(display);
		}
		switch(ev.type) {
		case FocusIn:
		case FocusOut:
			if(ev.xfocus.type == FocusOut) {
				/* Allow keyrepeat again! */
				vid_printf("Left window, auto repeat on\n");
				x_auto_repeat_on(0);
				g_has_focus = 0;
			} else if(ev.xfocus.type == FocusIn) {
				/* Allow keyrepeat again! */
				vid_printf("Enter window, auto repeat off\n");
				x_auto_repeat_off(0);
				g_has_focus = 1;
			}
			break;
		case EnterNotify:
		case LeaveNotify:
			/* These events are disabled now */
			printf("Enter/Leave event for winow %08x, sub: %08x\n",
				(word32)ev.xcrossing.window,
				(word32)ev.xcrossing.subwindow);
			printf("Enter/L mode: %08x, detail: %08x, type:%02x\n",
				ev.xcrossing.mode, ev.xcrossing.detail,
				ev.xcrossing.type);
			break;
		case ButtonPress:
			vid_printf("Got button press of button %d!\n",
				ev.xbutton.button);
			if(ev.xbutton.button == 1) {
				vid_printf("mouse button pressed\n");
				motion = update_mouse(ev.xbutton.x,
							ev.xbutton.y, 1, 1);
			} else if(ev.xbutton.button == 2) {
				g_limit_speed++;
				if(g_limit_speed > 2) {
					g_limit_speed = 0;
				}

				printf("Toggling g_limit_speed to %d\n",
					g_limit_speed);
				switch(g_limit_speed) {
				case 0:
					printf("...as fast as possible!\n");
					break;
				case 1:
					printf("...1.024MHz\n");
					break;
				case 2:
					printf("...2.5MHz\n");
					break;
				}
			} else {
				/* Re-enable kbd repeat for X */
				x_auto_repeat_on(0);
				set_halt(1);
				fflush(stdout);
			}
			break;
		case ButtonRelease:
			if(ev.xbutton.button == 1) {
				vid_printf("mouse button released\n");
				motion = update_mouse(ev.xbutton.x,
							ev.xbutton.y, 0, 1);
			}
			break;
		case Expose:
			refresh_needed = -1;
			break;
		case NoExpose:
			/* do nothing */
			break;
		case KeyPress:
		case KeyRelease:
			handle_keysym(&ev);
			break;
		case KeymapNotify:
			break;
		case ColormapNotify:
			vid_printf("ColormapNotify for %08x\n",
				(word32)(ev.xcolormap.window));
			vid_printf("colormap: %08x, new: %d, state: %d\n",
				(word32)ev.xcolormap.colormap,
				ev.xcolormap.new, ev.xcolormap.state);
			break;
		case MotionNotify:
			if(ev.xmotion.window != a2_win) {
				printf("Motion in window %08x unknown!\n",
					(word32)ev.xmotion.window);
			}
			motion = update_mouse(ev.xmotion.x, ev.xmotion.y, 0, 0);
			break;
		default:
			printf("X event 0x%08x is unknown!\n",
				ev.type);
			break;
		}
	}

	if(motion && g_warp_pointer) {
		XWarpPointer(display, None, a2_win, 0, 0, 0, 0,
			X_A2_WINDOW_WIDTH/2, X_A2_WINDOW_HEIGHT/2);
		update_mouse(-1,-1,-1,-1);
	}

	if(refresh_needed) {
		printf("Full refresh needed\n");
		a2_screen_buffer_changed = -1;
		g_full_refresh_needed = -1;

		g_border_sides_refresh_needed = 1;
		g_border_special_refresh_needed = 1;
		g_status_refresh_needed = 1;

		/* x_refresh_ximage(); */
		/* redraw_border(); */
	}

}

void
handle_keysym(XEvent *xev_in)
{
	KeySym	keysym;
	word32	state;
	int	keycode;
	int	a2code;
	int	type;
	int	is_up;

	keycode = xev_in->xkey.keycode;
	type = xev_in->xkey.type;

	keysym = XLookupKeysym(&(xev_in->xkey), 0);

	state = xev_in->xkey.state;

	vid_printf("keycode: %d, type: %d, state:%d, sym: %08x\n",
		keycode, type, state, (word32)keysym);

	x_update_modifier_state(state);

	is_up = 0;
	if(type == KeyRelease) {
		is_up = 1;
	}
	if(keysym == XK_F1) {
		/* Alias F1 to escape for OS/2 */
		keysym = XK_Escape;
	}

	if((keysym == XK_F7) && !is_up) {
		g_fast_disk_emul = !g_fast_disk_emul;
		printf("g_fast_disk_emul is now %d\n", g_fast_disk_emul);
	}

	if((keysym == XK_F9 || keysym == XK_F8) && !is_up) {
		/* warp pointer */
		g_warp_pointer = !g_warp_pointer;
		if(g_warp_pointer) {
			XDefineCursor(display, a2_win, g_cursor);
			printf("X Pointer grabbed\n");
		} else {
			XDefineCursor(display, a2_win, None);
			printf("X Pointer released\n");
		}
	}

	if(keysym == XK_F10 && !is_up) {
		change_a2vid_palette((g_a2vid_palette + 1) & 0xf);
	}

	if(keysym == XK_F11 && !is_up) {
		g_swap_paddles = !g_swap_paddles;
		printf("Swap paddles is now: %d\n", g_swap_paddles);
	}
	if(keysym == XK_F12 && !is_up) {
		g_invert_paddles = !g_invert_paddles;
		printf("Invert paddles is now: %d\n", g_invert_paddles);
	}

#if 0
	if(keysym == XK_Alt_L || keysym == XK_Meta_L) {
		g_alt_left_up = is_up;
	}

	if(keysym == XK_Alt_R || keysym == XK_Meta_R) {
		g_alt_right_up = is_up;
	}

	if(g_alt_left_up == 0 && g_alt_right_up == 0) {
		printf("Sending sound to file\n");
		g_send_sound_to_file = 1;
	} else {
		if(g_send_sound_to_file) {
			printf("Stopping sending sound to file\n");
			close_sound_file();
		}
		g_send_sound_to_file = 0;
	}
#endif

	/* first, do conversions */
	switch(keysym) {
	case XK_Alt_R:
	case XK_Meta_R:
	case XK_Cancel:
		keysym = XK_Print;		/* option */
		break;
	case XK_Alt_L:
	case XK_Meta_L:
	case XK_Menu:
		keysym = XK_Scroll_Lock;	/* cmd */
		break;
	case NoSymbol:
		switch(keycode) {
		/* 94-95 are for my PC101 kbd + windows keys on HPUX */
		case 0x0095:
			/* left windows key = option */
			keysym = XK_Print;
			break;
		case 0x0096:
		case 0x0094:
			/* right windows key = cmd */
			keysym = XK_Scroll_Lock;
			break;
		/* 0072 is for cra@WPI.EDU who says it's Break under XFree86 */
		case 0x0072:
			keysym = XK_Break;
			break;

		/* 0x0042, 0x0046, and 0x0048 are the windows keys according */
		/*  to Geoff Weiss on Solaris x86 */
		case 0x0042:
		case 0x0046:
			/* flying windows == open apple */
			keysym = XK_Scroll_Lock;
			break;
		case 0x0048:
			/* menu windows == option */
			keysym = XK_Print;
			break;
		}
	}

	a2code = x_keysym_to_a2code(keysym, is_up);
	if(a2code >= 0) {
		adb_physical_key_update(a2code, is_up);
	} else if(a2code != -2) {
		if((keysym >= XK_F7) && (keysym <= XK_F12)) {
			/* just get out quietly */
			return;
		}
		printf("Keysym: %04x of keycode: %02x unknown\n",
			(word32)keysym, keycode);
	}
}

int
x_keysym_to_a2code(int keysym, int is_up)
{
	int	i;

	if(keysym == 0) {
		return -1;
	}

	if((keysym == XK_Shift_L) || (keysym == XK_Shift_R)) {
		if(is_up) {
			g_x_shift_control_state &= ~ShiftMask;
		} else {
			g_x_shift_control_state |= ShiftMask;
		}
	}
	if(keysym == XK_Caps_Lock) {
		if(is_up) {
			g_x_shift_control_state &= ~LockMask;
		} else {
			g_x_shift_control_state |= LockMask;
		}
	}
	if((keysym == XK_Control_L) || (keysym == XK_Control_R)) {
		if(is_up) {
			g_x_shift_control_state &= ~ControlMask;
		} else {
			g_x_shift_control_state |= ControlMask;
		}
	}

	/* Look up Apple 2 keycode */
	for(i = g_num_a2_keycodes - 1; i >= 0; i--) {
		if((keysym == a2_key_to_xsym[i][1]) ||
					(keysym == a2_key_to_xsym[i][2])) {

			vid_printf("Found keysym:%04x = a[%d] = %04x or %04x\n",
				(int)keysym, i, a2_key_to_xsym[i][1],
				a2_key_to_xsym[i][2]);

			return a2_key_to_xsym[i][0];
		}
	}

	return -1;
}

void
x_update_modifier_state(int state)
{
	int	state_xor;
	int	is_up;

	state = state & (ControlMask | LockMask | ShiftMask);
	state_xor = g_x_shift_control_state ^ state;
	is_up = 0;
	if(state_xor & ControlMask) {
		is_up = ((state & ControlMask) == 0);
		adb_physical_key_update(0x36, is_up);
	}
	if(state_xor & LockMask) {
		is_up = ((state & LockMask) == 0);
		adb_physical_key_update(0x39, is_up);
	}
	if(state_xor & ShiftMask) {
		is_up = ((state & ShiftMask) == 0);
		adb_physical_key_update(0x38, is_up);
	}

	g_x_shift_control_state = state;
}

void
x_auto_repeat_on(int must)
{
	if((g_auto_repeat_on <= 0) || must) {
		g_auto_repeat_on = 1;
		XAutoRepeatOn(display);
		XFlush(display);
		adb_kbd_repeat_off();
	}
}

void
x_auto_repeat_off(int must)
{
	if((g_auto_repeat_on != 0) || must) {
		XAutoRepeatOff(display);
		XFlush(display);
		g_auto_repeat_on = 0;
		adb_kbd_repeat_off();
	}
}
