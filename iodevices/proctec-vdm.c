/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Common I/O devices used by various simulated machines
 *
 * Copyright (C) 2017-2019 by Udo Munk
 * Copyright (C) 2025 by Thomas Eberhardt
 *
 * Emulation of a Processor Technology VDM-1 S100 board
 *
 * History:
 * 28-FEB-2017 first version, all software tested with working
 * 21-JUN-2017 don't use dma_read(), switches Tarbell ROM off
 * 20-APR-2018 avoid thread deadlock on Windows/Cygwin
 * 15-JUL-2018 use logging
 * 04-NOV-2019 eliminate usage of mem_base()
 * 03-JAN-2025 use SDL2 instead of X11
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "sim.h"
#include "simdefs.h"
#include "simglb.h"
#include "simmem.h"
#include "simport.h"
#include "simsdl.h"

#include "proctec-vdm-charset.h"
#include "proctec-vdm.h"

#if 0
#include "log.h"
static const char *TAG = "VDM";
#endif

#define XOFF 10				/* use some offset inside the window */
#define YOFF 15				/* for the drawing area */

/* SDL stuff */
       int slf = 1;			/* scanlines factor, default no lines */
static int xsize, ysize;		/* window size */
static int sx, sy;
static SDL_Window *window;
static SDL_Renderer *renderer;
static int proctec_win_id = -1;
       uint8_t bg_color[3] = {48, 48, 48};	/* default background color */
       uint8_t fg_color[3] = {255, 255, 255};	/* default foreground color */

/* VDM stuff */
static int state;			/* state on/off for refresh thread */
static int mode;			/* video mode from I/O port */
int proctec_kbd_status = 1;		/* keyboard status */
int proctec_kbd_data = -1;		/* keyboard data */
static int first;			/* first displayed screen position */
static int beg;				/* beginning display line address */

/* create the SDL window for VDM display */
static void open_window(void)
{
	xsize = 576 + (XOFF * 2);
	ysize = (208 * slf) + (YOFF * 2);

	window = SDL_CreateWindow("Processor Technology VDM-1",
				  SDL_WINDOWPOS_UNDEFINED,
				  SDL_WINDOWPOS_UNDEFINED,
				  xsize, ysize, 0);
	renderer = SDL_CreateRenderer(window, -1, (SDL_RENDERER_ACCELERATED |
						   SDL_RENDERER_PRESENTVSYNC));

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
}

/* close the SDL window for VDM display */
static void close_window(void)
{
	SDL_DestroyRenderer(renderer);
	renderer = NULL;
	SDL_DestroyWindow(window);
	window = NULL;
}

/* shutdown VDM window */
void proctec_vdm_off(void)
{
	state = 0;		/* tell refresh thread to stop */

	if (proctec_win_id >= 0) {
		simsdl_destroy(proctec_win_id);
		proctec_win_id = -1;
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
	if (proctec_kbd_status == 0)
		return;

	/* if there is a keyboard event get it and convert to ASCII */
	if (event->type == SDL_KEYDOWN) {
		if (event->key.windowID == SDL_GetWindowID(window)) {
			proctec_kbd_data = event->key.keysym.sym & 0x7F; /* WRONG!!! */
			proctec_kbd_status = 0;
		}
	}
}

/* display characters, bit 7 = inverse video */
static void dc(BYTE c)
{
	register int x, y;
	int inv = (c & 128) ? 1 : 0;

	for (x = 0; x < 9; x++) {
		for (y = 0; y < 13; y++) {
			if (charset[c & 0x7f][y][x] == 1) {
				if (!inv)
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
				if (!inv)
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
			SDL_RenderDrawPoint(renderer, sx + x, sy + (y * slf));
		}
	}
}

/* refresh the display buffer */
static void refresh(void)
{
	register int x, y;
	static int addr;
	static BYTE c;

	sy = YOFF;
	addr = 0xcc00 + beg * 64;

	for (y = 0; y < 16; y++) {
		sx = XOFF;
		for (x = 0; x < 64; x++) {
			if (y >= first) {
				c = getmem(addr + x);
				dc(c);
			} else
				dc(' ');
			sx += 9;
		}
		sy += 13 * slf;
		addr += 64;
		if (addr >= 0xd000)
			addr = 0xcc00;
	}
}

/* function for updating the display */
static void update_display(void)
{
	if (state) {
		/* update display window */
		refresh();
		SDL_RenderPresent(renderer);
	}
}

static win_funcs_t proctec_funcs = {
	open_window,
	close_window,
	event_handler,
	update_display
};

/* I/O port for the VDM */
void proctec_vdm_out(BYTE data)
{
	mode = data;
	first = (data & 0xf0) >> 4;
	beg = data & 0x0f;

	if (proctec_win_id < 0)
		proctec_win_id = simsdl_create(&proctec_funcs);

	state = 1;
}
