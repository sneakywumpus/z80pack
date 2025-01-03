/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Common I/O devices used by various simulated machines
 *
 * Copyright (C) 2015-2019 by Udo Munk
 * Copyright (C) 2018 David McNaughton
 * Copyright (C) 2025 by Thomas Eberhardt
 *
 * Emulation of a Cromemco DAZZLER S100 board
 *
 * History:
 * 24-APR-2015 first version
 * 25-APR-2015 fixed a few things, good enough for a BETA release now
 * 27-APR-2015 fixed logic bugs with on/off state and thread handling
 * 08-MAY-2015 fixed Xlib multithreading problems
 * 26-AUG-2015 implemented double buffering to prevent flicker
 * 27-AUG-2015 more bug fixes
 * 15-NOV-2016 fixed logic bug, display wasn't always clear after
 *	       the device is switched off
 * 06-DEC-2016 added bus request for the DMA
 * 16-DEC-2016 use DMA function for memory access
 * 26-JAN-2017 optimization
 * 15-JUL-2018 use logging
 * 19-JUL-2018 integrate webfrontend
 * 04-NOV-2019 remove fake DMA bus request
 * 03-JAN-2025 use SDL2 instead of X11
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <SDL.h>

#include "sim.h"

#ifdef HAS_DAZZLER

#include "simdefs.h"
#include "simglb.h"
#include "simcfg.h"
#include "simmem.h"
#include "simport.h"
#include "simsdl.h"

#ifdef HAS_NETSERVER
#include <pthread.h>
#include "netsrv.h"
/* #define LOG_LOCAL_LEVEL LOG_DEBUG */
#include "log.h"
static const char *TAG = "DAZZLER";
#endif

#include "cromemco-dazzler.h"

/* SDL stuff */
#define WSIZE 512
static int size = WSIZE;
static SDL_Window *window;
static SDL_Renderer *renderer;
static int dazzler_win_id = -1;
static uint8_t colors[16][3] = {
	{ 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00 },
	{ 0x00, 0x80, 0x00 },
	{ 0x80, 0x80, 0x00 },
	{ 0x00, 0x00, 0x80 },
	{ 0x80, 0x00, 0x80 },
	{ 0x00, 0x80, 0x80 },
	{ 0x80, 0x80, 0x80 },
	{ 0x00, 0x00, 0x00 },
	{ 0xFF, 0x00, 0x00 },
	{ 0x00, 0xFF, 0x00 },
	{ 0xFF, 0xFF, 0x00 },
	{ 0x00, 0x00, 0xFF },
	{ 0xFF, 0x00, 0xFF },
	{ 0x00, 0xFF, 0xFF },
	{ 0xFF, 0xFF, 0xFF }
};
static uint8_t grays[16][3] = {
	{ 0x00, 0x00, 0x00 },
	{ 0x11, 0x11, 0x11 },
	{ 0x22, 0x22, 0x22 },
	{ 0x33, 0x33, 0x33 },
	{ 0x44, 0x44, 0x44 },
	{ 0x55, 0x55, 0x55 },
	{ 0x66, 0x66, 0x66 },
	{ 0x77, 0x77, 0x77 },
	{ 0x88, 0x88, 0x88 },
	{ 0x99, 0x99, 0x99 },
	{ 0xAA, 0xAA, 0xAA },
	{ 0xBB, 0xBB, 0xBB },
	{ 0xCC, 0xCC, 0xCC },
	{ 0xDD, 0xDD, 0xDD },
	{ 0xEE, 0xEE, 0xEE },
	{ 0xFF, 0xFF, 0xFF }
};

/* DAZZLER stuff */
static int state;
static WORD dma_addr;
static BYTE flags = 64;
static BYTE format;

#ifdef HAS_NETSERVER
static void ws_clear(void);
static BYTE formatBuf = 0;
static pthread_t thread;
#endif

/* create the SDL window for DAZZLER display */
static void open_window(void)
{
	window = SDL_CreateWindow("Cromemco DAzzLER",
				  SDL_WINDOWPOS_UNDEFINED,
				  SDL_WINDOWPOS_UNDEFINED,
				  size, size, 0);
	renderer = SDL_CreateRenderer(window, -1, (SDL_RENDERER_ACCELERATED |
						   SDL_RENDERER_PRESENTVSYNC));
}

/* close the SDL window for DAZZLER display */
static void close_window(void)
{
	SDL_DestroyRenderer(renderer);
	renderer = NULL;
	SDL_DestroyWindow(window);
	window = NULL;
}

/* switch DAZZLER off from front panel */
void cromemco_dazzler_off(void)
{
	state = 0;

#ifdef HAS_NETSERVER
	if (!n_flag) {
#endif
		if (dazzler_win_id >= 0) {
			simsdl_destroy(dazzler_win_id);
			dazzler_win_id = -1;
		}
#ifdef HAS_NETSERVER
	} else {
		sleep_for_ms(50);
		if (thread != 0) {
			pthread_cancel(thread);
			pthread_join(thread, NULL);
			thread = 0;
		}
		ws_clear();
	}
#endif
}

/* process SDL event */
static void proc_event(SDL_Event *event)
{
	UNUSED(event);
}

/* draw pixels for one frame in hires */
static void draw_hires(void)
{
	int psize, x, y, i;
	WORD addr = dma_addr;
	SDL_Rect r;

	/* set color or grayscale from lower nibble in graphics format */
	i = format & 0x0f;
	if (format & 16)
		SDL_SetRenderDrawColor(renderer,
				       colors[i][0], colors[i][1], colors[i][2],
				       SDL_ALPHA_OPAQUE);
	else
		SDL_SetRenderDrawColor(renderer,
				       grays[i][0], grays[i][1], grays[i][2],
				       SDL_ALPHA_OPAQUE);
	if (format & 32) {	/* 2048 bytes memory */
		psize = size / 128;	/* size of one pixel for 128x128 */
		r.w = r.h = psize;
		for (y = 0; y < 64; y += 2) {
			for (x = 0; x < 64;) {
				i = dma_read(addr);
				if (i & 1) {
					r.x = x * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 2) {
					r.x = (x + 1) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 4) {
					r.x = x * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 8) {
					r.x = (x + 1) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 16) {
					r.x = (x + 2) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 32) {
					r.x = (x + 3) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 64) {
					r.x = (x + 2) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 128) {
					r.x = (x + 3) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				x += 4;
				addr++;
			}
		}
		for (y = 0; y < 64; y += 2) {
			for (x = 64; x < 128;) {
				i = dma_read(addr);
				if (i & 1) {
					r.x = x * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 2) {
					r.x = (x + 1) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 4) {
					r.x = x * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 8) {
					r.x = (x + 1) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 16) {
					r.x = (x + 2) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 32) {
					r.x = (x + 3) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 64) {
					r.x = (x + 2) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 128) {
					r.x = (x + 3) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				x += 4;
				addr++;
			}
		}
		for (y = 64; y < 128; y += 2) {
			for (x = 0; x < 64;) {
				i = dma_read(addr);
				if (i & 1) {
					r.x = x * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 2) {
					r.x = (x + 1) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 4) {
					r.x = x * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 8) {
					r.x = (x + 1) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 16) {
					r.x = (x + 2) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 32) {
					r.x = (x + 3) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 64) {
					r.x = (x + 2) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 128) {
					r.x = (x + 3) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				x += 4;
				addr++;
			}
		}
		for (y = 64; y < 128; y += 2) {
			for (x = 64; x < 128;) {
				i = dma_read(addr);
				if (i & 1) {
					r.x = x * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 2) {
					r.x = (x + 1) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 4) {
					r.x = x * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 8) {
					r.x = (x + 1) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 16) {
					r.x = (x + 2) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 32) {
					r.x = (x + 3) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 64) {
					r.x = (x + 2) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 128) {
					r.x = (x + 3) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				x += 4;
				addr++;
			}
		}
	} else {		/* 512 bytes memory */
		psize = size / 64;	/* size of one pixel for 64x64 */
		r.w = r.h = psize;
		for (y = 0; y < 64; y += 2) {
			for (x = 0; x < 64;) {
				i = dma_read(addr);
				if (i & 1) {
					r.x = x * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 2) {
					r.x = (x + 1) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 4) {
					r.x = x * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 8) {
					r.x = (x + 1) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 16) {
					r.x = (x + 2) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 32) {
					r.x = (x + 3) * psize;
					r.y = y * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 64) {
					r.x = (x + 2) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				if (i & 128) {
					r.x = (x + 3) * psize;
					r.y = (y + 1) * psize;
					SDL_RenderFillRect(renderer, &r);
				}
				x += 4;
				addr++;
			}
		}
	}
}

/* draw pixels for one frame in lowres */
static void draw_lowres(void)
{
	int psize, x, y, i;
	WORD addr = dma_addr;
	SDL_Rect r;

	/* get size of DMA memory and draw the pixels */
	if (format & 32) {	/* 2048 bytes memory */
		psize = size / 64;	/* size of one pixel for 64x64 */
		r.w = r.h = psize;
		for (y = 0; y < 32; y++) {
			for (x = 0; x < 32;) {
				i = dma_read(addr) & 0x0f;
				if (format & 16)
					SDL_SetRenderDrawColor(renderer,
							       colors[i][0],
							       colors[i][1],
							       colors[i][2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       grays[i][0],
							       grays[i][1],
							       grays[i][2],
							       SDL_ALPHA_OPAQUE);
				r.x = x * psize;
				r.y = y * psize;
				SDL_RenderFillRect(renderer, &r);
				x++;
				i = (dma_read(addr) & 0xf0) >> 4;
				if (format & 16)
					SDL_SetRenderDrawColor(renderer,
							       colors[i][0],
							       colors[i][1],
							       colors[i][2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       grays[i][0],
							       grays[i][1],
							       grays[i][2],
							       SDL_ALPHA_OPAQUE);
				r.x = x * psize;
				r.y = y * psize;
				SDL_RenderFillRect(renderer, &r);
				x++;
				addr++;
			}
		}
		for (y = 0; y < 32; y++) {
			for (x = 32; x < 64;) {
				i = dma_read(addr) & 0x0f;
				if (format & 16)
					SDL_SetRenderDrawColor(renderer,
							       colors[i][0],
							       colors[i][1],
							       colors[i][2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       grays[i][0],
							       grays[i][1],
							       grays[i][2],
							       SDL_ALPHA_OPAQUE);
				r.x = x * psize;
				r.y = y * psize;
				SDL_RenderFillRect(renderer, &r);
				x++;
				i = (dma_read(addr) & 0xf0) >> 4;
				if (format & 16)
					SDL_SetRenderDrawColor(renderer,
							       colors[i][0],
							       colors[i][1],
							       colors[i][2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       grays[i][0],
							       grays[i][1],
							       grays[i][2],
							       SDL_ALPHA_OPAQUE);
				r.x = x * psize;
				r.y = y * psize;
				SDL_RenderFillRect(renderer, &r);
				x++;
				addr++;
			}
		}
		for (y = 32; y < 64; y++) {
			for (x = 0; x < 32;) {
				i = dma_read(addr) & 0x0f;
				if (format & 16)
					SDL_SetRenderDrawColor(renderer,
							       colors[i][0],
							       colors[i][1],
							       colors[i][2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       grays[i][0],
							       grays[i][1],
							       grays[i][2],
							       SDL_ALPHA_OPAQUE);
				r.x = x * psize;
				r.y = y * psize;
				SDL_RenderFillRect(renderer, &r);
				x++;
				i = (dma_read(addr) & 0xf0) >> 4;
				if (format & 16)
					SDL_SetRenderDrawColor(renderer,
							       colors[i][0],
							       colors[i][1],
							       colors[i][2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       grays[i][0],
							       grays[i][1],
							       grays[i][2],
							       SDL_ALPHA_OPAQUE);
				r.x = x * psize;
				r.y = y * psize;
				SDL_RenderFillRect(renderer, &r);
				x++;
				addr++;
			}
		}
		for (y = 32; y < 64; y++) {
			for (x = 32; x < 64;) {
				i = dma_read(addr) & 0x0f;
				if (format & 16)
					SDL_SetRenderDrawColor(renderer,
							       colors[i][0],
							       colors[i][1],
							       colors[i][2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       grays[i][0],
							       grays[i][1],
							       grays[i][2],
							       SDL_ALPHA_OPAQUE);
				r.x = x * psize;
				r.y = y * psize;
				SDL_RenderFillRect(renderer, &r);
				x++;
				i = (dma_read(addr) & 0xf0) >> 4;
				if (format & 16)
					SDL_SetRenderDrawColor(renderer,
							       colors[i][0],
							       colors[i][1],
							       colors[i][2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       grays[i][0],
							       grays[i][1],
							       grays[i][2],
							       SDL_ALPHA_OPAQUE);
				r.x = x * psize;
				r.y = y * psize;
				SDL_RenderFillRect(renderer, &r);
				x++;
				addr++;
			}
		}
	} else {		/* 512 bytes memory */
		psize = size / 32;	/* size of one pixel for 32x32 */
		r.w = r.h = psize;
		for (y = 0; y < 32; y++) {
			for (x = 0; x < 32;) {
				i = dma_read(addr) & 0x0f;
				if (format & 16)
					SDL_SetRenderDrawColor(renderer,
							       colors[i][0],
							       colors[i][1],
							       colors[i][2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       grays[i][0],
							       grays[i][1],
							       grays[i][2],
							       SDL_ALPHA_OPAQUE);
				r.x = x * psize;
				r.y = y * psize;
				SDL_RenderFillRect(renderer, &r);
				x++;
				i = (dma_read(addr) & 0xf0) >> 4;
				if (format & 16)
					SDL_SetRenderDrawColor(renderer,
							       colors[i][0],
							       colors[i][1],
							       colors[i][2],
							       SDL_ALPHA_OPAQUE);
				else
					SDL_SetRenderDrawColor(renderer,
							       grays[i][0],
							       grays[i][1],
							       grays[i][2],
							       SDL_ALPHA_OPAQUE);
				r.x = x * psize;
				r.y = y * psize;
				SDL_RenderFillRect(renderer, &r);
				x++;
				addr++;
			}
		}
	}
}

#ifdef HAS_NETSERVER
static uint8_t dblbuf[2048];

static struct {
	uint16_t format;
	uint16_t addr;
	uint16_t len;
	uint8_t buf[2048];
} msg;

static void ws_clear(void)
{
	memset(dblbuf, 0, 2048);

	msg.format = 0;
	msg.addr = 0xFFFF;
	msg.len = 0;
	net_device_send(DEV_DZLR, (char *) &msg, msg.len + 6);
	LOGD(TAG, "Clear the screen.");
}

static void ws_refresh(void)
{

	int len = (format & 32) ? 2048 : 512;
	int addr;
	int i, n, x, la_count;
	bool cont;
	uint8_t val;

	for (i = 0; i < len; i++) {
		addr = i;
		n = 0;
		la_count = 0;
		cont = true;
		while (cont && (i < len)) {
			val = dma_read(dma_addr + i);
			while ((val != dblbuf[i]) && (i < len)) {
				dblbuf[i++] = val;
				msg.buf[n++] = val;
				cont = false;
				val = dma_read(dma_addr + i);
			}
			if (cont)
				break;
			x = 0;
#define LOOKAHEAD 6
			/* look-ahead up to n bytes for next change */
			while ((x < LOOKAHEAD) && !cont && (i < len)) {
				val = dma_read(dma_addr + i++);
				msg.buf[n++] = val;
				la_count++;
				val = dma_read(dma_addr + i);
				if ((i < len) && (val != dblbuf[i])) {
					cont = true;
				}
				x++;
			}
			if (!cont) {
				n -= x;
				la_count -= x;
			}
		}
		if (n || (format != formatBuf)) {
			formatBuf = format;
			msg.format = format;
			msg.addr = addr;
			msg.len = n;
			net_device_send(DEV_DZLR, (char *) &msg, msg.len + 6);
			LOGD(TAG, "BUF update 0x%04X-0x%04X "
			     "len: %d format: 0x%02X l/a: %d",
			     msg.addr, msg.addr + msg.len,
			     msg.len, msg.format, la_count);
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

	while (1) {	/* do forever or until canceled */

		/* draw one frame dependent on graphics format */
		if (state == 1) {	/* draw frame if on */
			if (net_device_alive(DEV_DZLR)) {
				ws_refresh();
			} else {
				if (msg.format) {
					memset(dblbuf, 0, 2048);
					msg.format = 0;
				}
			}
		}

		/* frame done, set frame flag for 4ms */
		flags = 0;
		sleep_for_ms(4);
		flags = 64;

		/* sleep rest to 33ms so that we get 30 fps */
		t2 = get_clock_us();
		tdiff = t2 - t1;
		if ((tdiff > 0) && (tdiff < 33000))
			sleep_for_ms(33 - (tdiff / 1000));

		t1 = get_clock_us();
	}

	/* just in case it ever gets here */
	pthread_exit(NULL);
}
#endif /* HAS_NETSERVER */

/* function for updating the display */
static void update_display(void)
{
	/* draw one frame dependent on graphics format */
	if (state == 1) {	/* draw frame if on */
		SDL_SetRenderDrawColor(renderer,
				       colors[0][0],
				       colors[0][1],
				       colors[0][2],
				       SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		if (format & 64)
			draw_hires();
		else
			draw_lowres();
		SDL_RenderPresent(renderer);

		/* frame done, set frame flag for 4ms */
		flags = 0;
		sleep_for_ms(4);
		flags = 64;
	} else {
		SDL_SetRenderDrawColor(renderer,
				       colors[0][0],
				       colors[0][1],
				       colors[0][2],
				       SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
	}
}

static win_funcs_t dazzler_funcs = {
	open_window,
	close_window,
	proc_event,
	update_display
};

void cromemco_dazzler_ctl_out(BYTE data)
{
	/* get DMA address for display memory */
	dma_addr = (data & 0x7f) << 9;

	/* switch DAZZLER on/off */
	if (data & 128) {
#ifdef HAS_NETSERVER
		if (!n_flag) {
#endif
			if (dazzler_win_id < 0)
				dazzler_win_id = simsdl_create(&dazzler_funcs);
			state = 1;
#ifdef HAS_NETSERVER
		} else {
			if (state == 0)
				ws_clear();
			state = 1;
			if (thread == 0) {
				if (pthread_create(&thread, NULL, ws_update,
						   NULL)) {
					LOGE(TAG, "can't create thread");
					exit(EXIT_FAILURE);
				}
			}
		}
#endif
	} else {
		if (state == 1) {
			state = 0;
#ifdef HAS_NETSERVER
			if (n_flag) {
				sleep_for_ms(50);
				ws_clear();
			}
#endif
		}
	}
}

BYTE cromemco_dazzler_flags_in(void)
{
#ifdef HAS_NETSERVER
	if (!n_flag) {
#endif
		if (dazzler_win_id >= 0)
			return flags;
		else
			return (BYTE) 0xff;
#ifdef HAS_NETSERVER
	} else {
		if (thread != 0)
			return flags;
		else
			return (BYTE) 0xff;
	}
#endif
}

void cromemco_dazzler_format_out(BYTE data)
{
	format = data;
}

#endif /* HAS_DAZZLER */
