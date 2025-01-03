/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Copyright (C) 2025 Thomas Eberhardt
 */

/*
 *	This module contains the SDL2 integration for the simulator.
 */

#ifdef WANT_SDL

#include <stdbool.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_main.h>

#include "simsdl.h"
#include "simmain.h"

#define MAX_WINDOWS 5

static int sim_thread_func(void *data);

typedef struct args {
	int argc;
	char **argv;
} args_t;

typedef struct window {
	bool in_use;
	bool is_new;
	bool quit;
	win_funcs_t *funcs;
} window_t;

static SDL_mutex *win_lock;
static window_t win[MAX_WINDOWS];
static bool sim_finished;	/* simulator thread finished flag */

int main(int argc, char *argv[])
{
	SDL_Event event;
	bool quit = false;
	SDL_Thread *sim_thread;
	int i, status;
	args_t args = {argc, argv};

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "Can't initialize SDL: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}
	if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) == 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "Can't initialize SDL_image: %s\n",
			     IMG_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	win_lock = SDL_CreateMutex();

	sim_finished = false;
	sim_thread = SDL_CreateThread(sim_thread_func, "Simulator", &args);
	if (sim_thread == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "Can't create simulator thread: %s\n",
			     SDL_GetError());
		IMG_Quit();
		SDL_Quit();
		return EXIT_FAILURE;
	}

	while (!quit) {
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT)
				quit = true;

			SDL_LockMutex(win_lock);
			for (i = 0; i < MAX_WINDOWS; i++)
				if (win[i].in_use)
					(*win[i].funcs->event)(&event);
			SDL_UnlockMutex(win_lock);
		}
		SDL_LockMutex(win_lock);
		for (i = 0; i < MAX_WINDOWS; i++)
			if (win[i].in_use) {
				if (win[i].quit) {
					(*win[i].funcs->close)();
					win[i].in_use = false;
				} else {
					if (win[i].is_new) {
						(*win[i].funcs->open)();
						win[i].is_new = false;
					}
					(*win[i].funcs->draw)();
				}
			}
		SDL_UnlockMutex(win_lock);
		if (sim_finished)
			quit = true;
	}

	SDL_WaitThread(sim_thread, &status);

	SDL_LockMutex(win_lock);
	for (i = 0; i < MAX_WINDOWS; i++)
		if (win[i].in_use)
			(*win[i].funcs->close)();
	SDL_UnlockMutex(win_lock);
	SDL_DestroyMutex(win_lock);

	IMG_Quit();
	SDL_Quit();

	return status;
}

/* this is called from the simulator thread */
int simsdl_create(win_funcs_t *funcs)
{
	int i;

	SDL_LockMutex(win_lock);
	for (i = 0; i < MAX_WINDOWS; i++)
		if (!win[i].in_use) {
			win[i].in_use = true;
			win[i].is_new = true;
			win[i].quit = false;
			win[i].funcs = funcs;
			break;
		}
	SDL_UnlockMutex(win_lock);

	if (i == MAX_WINDOWS) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "No more window slots left\n");
		i = -1;
	}

	return i;
}

/* this is called from the simulator thread */
void simsdl_destroy(int i)
{
	if (i >= 0 && i < MAX_WINDOWS) {
		SDL_LockMutex(win_lock);
		win[i].quit = true;
		SDL_UnlockMutex(win_lock);
	}
}

/* this thread function runs the simulator */
static int sim_thread_func(void *data)
{
	args_t *args = (args_t *) data;
	int status;

	status = sim_main(args->argc, args->argv);
	sim_finished = true;

	return status;
}

#endif /* WANT_SDL */
