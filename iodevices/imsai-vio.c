/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Common I/O devices used by various simulated machines
 *
 * Copyright (C) 2017-2019 by Udo Munk
 * Copyright (C) 2018 David McNaughton
 * Copyright (C) 2025 by Thomas Eberhardt
 *
 * Emulation of an IMSAI VIO S100 board
 *
 * History:
 * 10-JAN-2017 80x24 display output tested and working
 * 11-JAN-2017 implemented keyboard input for the X11 key events
 * 12-JAN-2017 all resolutions in all video modes tested and working
 * 04-FEB-2017 added function to terminate thread and close window
 * 21-FEB-2017 added scanlines to monitor
 * 20-APR-2018 avoid thread deadlock on Windows/Cygwin
 * 07-JUL-2018 optimization
 * 12-JUL-2018 use logging
 * 14-JUL-2018 integrate webfrontend
 * 05-NOV-2019 use correct memory access function
 * 03-JAN-2025 use SDL2 instead of X11
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "sim.h"
#include "simdefs.h"
#include "simglb.h"
#include "simmem.h"
#include "simport.h"
#include "simsdl.h"

#ifdef HAS_NETSERVER
#include <pthread.h>
#include "netsrv.h"
#include "log.h"
static const char *TAG = "VIO";
#endif

#include "imsai-vio-charset.h"
#include "imsai-vio.h"

#define XOFF 10				/* use some offset inside the window */
#define YOFF 15				/* for the drawing area */

/* SDL stuff */
       int slf = 1;			/* scanlines factor, default no lines */
static int xsize, ysize;		/* window size */
static int xscale, yscale;
static int sx, sy;
static SDL_Window *window;
static SDL_Renderer *renderer;
static int vio_win_id = -1;
       uint8_t bg_color[3] = {48, 48, 48};	/* default background color */
       uint8_t fg_color[3] = {255, 255, 255};	/* default foreground color */

/* VIO stuff */
static int state;			/* state on/off for refresh thread */
static int mode;		/* video mode written to command port memory */
static int modebuf;			/* and double buffer for it */
static int vmode, res, inv;		/* video mode, resolution & inverse */
int imsai_kbd_status, imsai_kbd_data;	/* keyboard status & data */

#ifdef HAS_NETSERVER
static pthread_t thread;
#endif

/* create the SDL window for VIO display */
static void open_window(void)
{
	xsize = 560 + (XOFF * 2);
	ysize = (240 * slf) + (YOFF * 2);

	window = SDL_CreateWindow("IMSAI VIO",
				  SDL_WINDOWPOS_UNDEFINED,
				  SDL_WINDOWPOS_UNDEFINED,
				  xsize, ysize, 0);
	renderer = SDL_CreateRenderer(window, -1, (SDL_RENDERER_ACCELERATED |
						   SDL_RENDERER_PRESENTVSYNC));

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

/* close the SDL window for VIO display */
static void close_window(void)
{
	SDL_DestroyRenderer(renderer);
	renderer = NULL;
	SDL_DestroyWindow(window);
	window = NULL;
}

/* shutdown VIO thread and window */
void imsai_vio_off(void)
{
	state = 0;		/* tell web refresh thread to stop */

#ifdef HAS_NETSERVER
	if (!n_flag) {
#endif
		if (vio_win_id >= 0) {
			simsdl_destroy(vio_win_id);
			vio_win_id = -1;
		}
#ifdef HAS_NETSERVER
	} else {
		sleep_for_ms(50);	/* and wait a bit for thread to stop */
		if (thread != 0) {
			pthread_cancel(thread);
			pthread_join(thread, NULL);
			thread = 0;
		}
	}
#endif
}

/* display characters 80-FF from bits 0-6, bit 7 = inverse video */
static void dc1(BYTE c)
{
	register int x, y;
	int cinv = (c & 128) ? 1 : 0;

	for (x = 0; x < 7; x++) {
		for (y = 0; y < 10; y++) {
			if (charset[(c << 1) & 0xff][y][x] == 1) {
				if ((cinv ^ inv) == 0)
					SDL_SetRenderDrawColor(renderer,
							       fg_color[0],
							       fg_color[1],
							       fg_color[2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       bg_color[0],
							       bg_color[1],
							       bg_color[2],
							       SDL_ALPHA_OPAQUE);
			} else {
				if ((cinv ^ inv) == 0)
					SDL_SetRenderDrawColor(renderer,
							       bg_color[0],
							       bg_color[1],
							       bg_color[2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       fg_color[0],
							       fg_color[1],
							       fg_color[2],
							       SDL_ALPHA_OPAQUE);
			}
			SDL_RenderDrawPoint(renderer,
					    sx + (x * xscale),
					    sy + (y * yscale * slf));
			if (res & 1)
				SDL_RenderDrawPoint(renderer,
						    sx + (x * xscale) + 1,
						    sy + (y * yscale * slf));
			if (res & 2)
				SDL_RenderDrawPoint(renderer,
						    sx + (x * xscale),
						    sy + (y * yscale * slf) + (1 * slf));
			if ((res & 3) == 3)
				SDL_RenderDrawPoint(renderer,
						    sx + (x * xscale) + 1,
						    sy + (y * yscale * slf) + (1 * slf));
		}
	}
}

/* display characters 00-7F from bits 0-6, bit 7 = inverse video */
static void dc2(BYTE c)
{
	register int x, y;
	int cinv = (c & 128) ? 1 : 0;

	for (x = 0; x < 7; x++) {
		for (y = 0; y < 10; y++) {
			if (charset[c & 0x7f][y][x] == 1) {
				if ((cinv ^ inv) == 0)
					SDL_SetRenderDrawColor(renderer,
							       fg_color[0],
							       fg_color[1],
							       fg_color[2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       bg_color[0],
							       bg_color[1],
							       bg_color[2],
							       SDL_ALPHA_OPAQUE);
			} else {
				if ((cinv ^ inv) == 0)
					SDL_SetRenderDrawColor(renderer,
							       bg_color[0],
							       bg_color[1],
							       bg_color[2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       fg_color[0],
							       fg_color[1],
							       fg_color[2],
							       SDL_ALPHA_OPAQUE);
			}
			SDL_RenderDrawPoint(renderer,
					    sx + (x * xscale),
					    sy + (y * yscale * slf));
			if (res & 1)
				SDL_RenderDrawPoint(renderer,
						    sx + (x * xscale) + 1,
						    sy + (y * yscale * slf));
			if (res & 2)
				SDL_RenderDrawPoint(renderer,
						    sx + (x * xscale),
						    sy + (y * yscale * slf) + (1 * slf));
			if ((res & 3) == 3)
				SDL_RenderDrawPoint(renderer,
						    sx + (x * xscale) + 1,
						    sy + (y * yscale * slf) + (1 * slf));
		}
	}
}

/* display characters 00-FF from bits 0-7, inverse video from command word */
static void dc3(BYTE c)
{
	register int x, y;

	for (x = 0; x < 7; x++) {
		for (y = 0; y < 10; y++) {
			if (charset[c][y][x] == 1) {
				if (inv == 0)
					SDL_SetRenderDrawColor(renderer,
							       fg_color[0],
							       fg_color[1],
							       fg_color[2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       bg_color[0],
							       bg_color[1],
							       bg_color[2],
							       SDL_ALPHA_OPAQUE);
			} else {
				if (inv == 0)
					SDL_SetRenderDrawColor(renderer,
							       bg_color[0],
							       bg_color[1],
							       bg_color[2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       fg_color[0],
							       fg_color[1],
							       fg_color[2],
							       SDL_ALPHA_OPAQUE);
			}
			SDL_RenderDrawPoint(renderer,
					    sx + (x * xscale),
					    sy + (y * yscale * slf));
			if (res & 1)
				SDL_RenderDrawPoint(renderer,
						    sx + (x * xscale) + 1,
						    sy + (y * yscale * slf));
			if (res & 2)
				SDL_RenderDrawPoint(renderer,
						    sx + (x * xscale),
						    sy + (y * yscale * slf) + (1 * slf));
			if ((res & 3) == 3)
				SDL_RenderDrawPoint(renderer,
						    sx + (x * xscale) + 1,
						    sy + (y * yscale * slf) + (1 * slf));
		}
	}
}

/*
 * Process a SDL event, we are only interested in keyboard input.
 * Note that I'm using the event queue as typeahead buffer, saves to
 * implement one self.
 */
static void event_handler(SDL_Event *event)
{
	/* if the last character wasn't processed already do nothing */
	/* keep event in queue until the CPU emulation got current one */
	if (imsai_kbd_status != 0)
		return;

	/* if there is a keyboard event get it and convert with keymap */
	if (event && event->type == SDL_KEYDOWN) {
		if (event->key.windowID == SDL_GetWindowID(window)) {
			imsai_kbd_data = event->key.keysym.sym & 0x7F; /* WRONG!!! */
			imsai_kbd_status = 2;
		}
	}
#ifdef HAS_NETSERVER
	if (n_flag) {
		int res = net_device_get(DEV_VIO);
		if (res >= 0) {
			imsai_kbd_data =  res;
			imsai_kbd_status = 2;
		}
	}
#endif
}

/* refresh the display buffer dependent on video mode */
static void refresh(void)
{
	static int cols, rows;
	static BYTE c;
	register int x, y;

	sx = XOFF;
	sy = YOFF;

	mode = getmem(0xf7ff);
	if (mode != modebuf) {
		modebuf = mode;

		vmode = (mode >> 2) & 3;
		res = mode & 3;
		inv = (mode & 16) ? 1 : 0;

		if (res & 1) {
			cols = 40;
			xscale = 2;
		} else {
			cols = 80;
			xscale = 1;
		}

		if (res & 2) {
			rows = 12;
			yscale = 2;
		} else {
			rows = 24;
			yscale = 1;
		}
	}

	switch (vmode) {
	case 0:	/* Video mode 0: video off, screen blanked */
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		break;

	case 1: /* Video mode 1: display character codes 80-FF */
		for (y = 0; y < rows; y++) {
			sx = XOFF;
			for (x = 0; x < cols; x++) {
				c = getmem(0xf000 + (y * cols) + x);
				dc1(c);
				sx += (res & 1) ? 14 : 7;
			}
			sy += (res & 2) ? 20 * slf : 10 * slf;
		}
		break;

	case 2:	/* Video mode 2: display character codes 00-7F */
		for (y = 0; y < rows; y++) {
			sx = XOFF;
			for (x = 0; x < cols; x++) {
				c = getmem(0xf000 + (y * cols) + x);
				dc2(c);
				sx += (res & 1) ? 14 : 7;
			}
			sy += (res & 2) ? 20 * slf : 10 * slf;
		}
		break;

	case 3:	/* Video mode 3: display character codes 00-FF */
		for (y = 0; y < rows; y++) {
			sx = XOFF;
			for (x = 0; x < cols; x++) {
				c = getmem(0xf000 + (y * cols) + x);
				dc3(c);
				sx += (res & 1) ? 14 : 7;
			}
			sy += (res & 2) ? 20 * slf : 10 * slf;
		}
		break;
	}
}

#ifdef HAS_NETSERVER
static uint8_t dblbuf[2048];

static struct {
	uint16_t addr;
	union {
		uint16_t len;
		uint16_t mode;
	};
	uint8_t buf[2048];
} msg;

static void ws_refresh(void)
{
	static int cols, rows;

	mode = getmem(0xf7ff);
	if (mode != modebuf) {
		modebuf = mode;
		memset(dblbuf, 0, 2048);

		res = mode & 3;

		if (res & 1) {
			cols = 40;
		} else {
			cols = 80;
		}

		if (res & 2) {
			rows = 12;
		} else {
			rows = 24;
		}

		msg.mode = mode;
		msg.addr = 0xf7ff;
		net_device_send(DEV_VIO, (char *) &msg, 4);
		LOGD(__func__, "MODE change");
	}

	event_handler(NULL);

	int len = rows * cols;
	int addr;
	int i, n, x;
	bool cont;
	uint8_t val;

	for (i = 0; i < len; i++) {
		addr = i;
		n = 0;
		cont = true;
		while (cont && (i < len)) {
			val = getmem(0xf000 + i);
			while ((val != dblbuf[i]) && (i < len)) {
				dblbuf[i++] = val;
				msg.buf[n++] = val;
				cont = false;
				val = getmem(0xf000 + i);
			}
			if (cont)
				break;
			x = 0;
#define LOOKAHEAD 4
			/* look-ahead up to 4 bytes for next change */
			while ((x < LOOKAHEAD) && !cont && (i < len)) {
				val = getmem(0xf000 + i++);
				msg.buf[n++] = val;
				val = getmem(0xf000 + i);
				if ((i < len) && (val != dblbuf[i])) {
					cont = true;
				}
				x++;
			}
			if (!cont) {
				n -= x;
			}
		}
		if (n) {
			msg.addr = 0xf000 + addr;
			msg.len = n;
			net_device_send(DEV_VIO, (char *) &msg, msg.len + 4);
			LOGD(__func__, "BUF update FROM %04X TO %04X",
			     msg.addr, msg.addr + msg.len);
		}
	}
}

/* thread for updating the web server */
static void *ws_update(void *arg)
{
	uint64_t t1, t2;
	int tdiff;

	UNUSED(arg);

	t1 = get_clock_us();

	while (state) {
		ws_refresh();

		/* sleep rest to 33ms so that we get 30 fps */
		t2 = get_clock_us();
		tdiff = t2 - t1;
		if ((tdiff > 0) && (tdiff < 33000))
			sleep_for_ms(33 - (tdiff / 1000));

		t1 = get_clock_us();
	}

	pthread_exit(NULL);
}
#endif

/* function for updating the display */
static void update_display(void)
{
	/* update display window */
	refresh();
	SDL_RenderPresent(renderer);
}

static win_funcs_t vio_funcs = {
	open_window,
	close_window,
	event_handler,
	update_display
};

/* create the SDL window and start display refresh thread */
void imsai_vio_init(void)
{
#ifdef HAS_NETSERVER
	if (!n_flag)
#endif
		if (vio_win_id < 0)
			vio_win_id = simsdl_create(&vio_funcs);

	state = 1;
	modebuf = -1;
	putmem(0xf7ff, 0x00);

#ifdef HAS_NETSERVER
	if (n_flag) {
		if (pthread_create(&thread, NULL, ws_update, (void *) NULL)) {
			LOGE(TAG, "can't create thread");
			exit(EXIT_FAILURE);
		}
	}
#endif
}
