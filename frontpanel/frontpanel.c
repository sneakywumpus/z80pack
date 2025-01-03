// frontpanel.c	lightpanel main interface

/* Copyright (c) 2007-2015, John Kichury
   C and SDL2 conversion is Copyright (c) 2024-2025, Thomas Eberhardt

   This software is freely distributable free of charge and without license fees with the
   following conditions:

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   JOHN KICHURY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   The above copyright notice must be included in any copies of this software.

*/

/* Fixed portability problems, March 2014, Udo Munk */

#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "frontpanel.h"
#include "lpanel.h"
#include "lp_utils.h"

#define UNUSED(x) (void) (x)

static SDL_mutex *data_lock;
static SDL_mutex *data_sample_lock;

static int samplecount = 0;

Lpanel_t *panel;
Parser_t parser;

// bind functions

int fp_bindLight64(const char *name, uint64_t *bits, int bitnum)
{
	// SDL_Log("fp_bindLight64: name=%s *bits=%lx bitnum=%d\n", name, *bits, bitnum);
	Lpanel_bindLight64(panel, name, bits, bitnum);
	return 1;
}

int fp_bindLight32(const char *name, uint32_t *bits, int bitnum)
{
	Lpanel_bindLight32(panel, name, bits, bitnum);
	return 1;
}

int fp_bindLight16(const char *name, uint16_t *bits, int bitnum)
{
	Lpanel_bindLight16(panel, name, bits, bitnum);
	return 1;
}

int fp_bindLightfv(const char *name, float *bits)
{
	Lpanel_bindLightfv(panel, name, bits);
	return 1;
}

int fp_bindLight8(const char *name, uint8_t *bits, int bitnum)
{
	Lpanel_bindLight8(panel, name, bits, bitnum);
	return 1;
}

int fp_bindLight8invert(const char *name, uint8_t *bits, int bitnum, uint8_t mask)
{
	Lpanel_bindLight8invert(panel, name, bits, bitnum, mask);
	return 1;
}

int fp_bindLight16invert(const char *name, uint16_t *bits, int bitnum, uint16_t mask)
{
	Lpanel_bindLight16invert(panel, name, bits, bitnum, mask);
	return 1;
}

int fp_bindLight32invert(const char *name, uint32_t *bits, int bitnum, uint32_t mask)
{
	Lpanel_bindLight32invert(panel, name, bits, bitnum, mask);
	return 1;
}

int fp_bindLight64invert(const char *name, uint64_t *bits, int bitnum, uint64_t mask)
{
	Lpanel_bindLight64invert(panel, name, bits, bitnum, mask);
	return 1;
}

int fp_smoothLight(const char *name, int nframes)
{
	Lpanel_smoothLight(panel, name, nframes);
	return 1;
}

void fp_bindRunFlag(uint8_t *addr)
{
	Lpanel_bindRunFlag(panel, addr);
}

void fp_bindSimclock(uint64_t *addr)
{
	Lpanel_bindSimclock(panel, addr);
}

int fp_bindSwitch8(const char *name, uint8_t *loc_down, uint8_t *loc_up, int bitnum)
{
	Lpanel_bindSwitch8(panel, name, loc_down, loc_up, bitnum);
	return 1;
}

int fp_bindSwitch16(const char *name, uint16_t *loc_down, uint16_t *loc_up, int bitnum)
{
	Lpanel_bindSwitch16(panel, name, loc_down, loc_up, bitnum);
	return 1;
}

int fp_bindSwitch32(const char *name, uint32_t *loc_down, uint32_t *loc_up, int bitnum)
{
	Lpanel_bindSwitch32(panel, name, loc_down, loc_up, bitnum);
	return 1;
}

int fp_bindSwitch64(const char *name, uint64_t *loc_down, uint64_t *loc_up, int bitnum)
{
	Lpanel_bindSwitch64(panel, name, loc_down, loc_up, bitnum);
	return 1;
}

void fp_ignoreBindErrors(int n)
{
	Lpanel_ignoreBindErrors(panel, n != 0);
}

void fp_framerate(float v)
{
	Lpanel_framerate_set(panel, v);
	framerate_set(v);
}

int fp_init(const char *cfg_fname)
{
	return fp_init2(NULL, cfg_fname, 800);
}

int fp_init2(const char *cfg_root_path, const char *cfg_fname, int size)
{
	printf("FrontPanel Simulator v2.1 Copyright (C) 2007-2015 by John Kichury\n");
	printf("C and SDL2 conversion Copyright (C) 2024-2025 by Thomas Eberhardt\n");

	// allocate & initialize panel and parser instance
	panel = Lpanel_new();
	Parser_init(&parser);

	if (cfg_root_path)
		Lpanel_setConfigRootPath(panel, cfg_root_path);

	// set a default framerate
	fp_framerate(30.);

	// set initial window size
	panel->window_xsize = size;

	if (!Lpanel_readConfig(panel, cfg_fname)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "fp_init: error initializing the panel\n");
		return 0;
	}

	return 1;
}

void fp_openWindow(void)
{
	data_lock = SDL_CreateMutex();
	data_sample_lock = SDL_CreateMutex();

	Lpanel_openWindow(panel, "FrontPanel");
	Lpanel_initGraphics(panel);
}

void fp_procEvent(SDL_Event *event)
{
	Lpanel_procEvent(panel, event);
}

void fp_draw(void)
{
	// lock
	SDL_LockMutex(data_lock);
	SDL_LockMutex(data_sample_lock);

	SDL_GL_MakeCurrent(panel->window, panel->cx);

	// draw
	Lpanel_draw(panel);

	// unlock
	SDL_UnlockMutex(data_sample_lock);
	SDL_UnlockMutex(data_lock);

	glFinish();
	SDL_GL_SwapWindow(panel->window);
}

void fp_sampleData(void)
{
	// mutex lock
	SDL_LockMutex(data_sample_lock);

	Lpanel_sampleData(panel);

	// mutex unlock
	SDL_UnlockMutex(data_sample_lock);

	samplecount++;
}

void fp_sampleLightGroup(int groupnum, int clockwarp)
{
	Lpanel_sampleLightGroup(panel, groupnum, clockwarp);
	samplecount++;
}

void fp_sampleSwitches(void)
{
	Lpanel_sampleSwitches(panel);
}

// callback functions

int fp_addSwitchCallback(const char *name, void (*cbfunc)(int state, int val),
			 int userval)
{
	Lpanel_addSwitchCallback(panel, name, cbfunc, userval);
	return 1;
}

void fp_addQuitCallback(void (*cbfunc)(void))
{
	Lpanel_addQuitCallback(panel, cbfunc);
}

void fp_quit(void)
{
	Lpanel_exitGraphics(panel);
	Lpanel_destroyWindow(panel);

	SDL_DestroyMutex(data_sample_lock);
	SDL_DestroyMutex(data_lock);

	// finalize & deallocate panel and parser instance
	Parser_fini(&parser);
	Lpanel_delete(panel);
}

int fp_test(int n)
{
	return Lpanel_test(panel, n);
}
