// lp_window.c	lightpanel window functions

/* Copyright (c) 2007-2008, John Kichury
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>

#include "lpanel.h"
#include "lp_materials.h"
#include "lp_font.h"

#define UNUSED(x) (void) (x)

// OpenGL Light source

static GLfloat light_pos0[] = { 0., 0.5, 1., 0.};
// static GLfloat light_pos1[] = { 0.,-0.5,-1.,0.};
static GLfloat light_diffuse[] = { 1., 1., 1., 1.};
static GLfloat light_specular[] = { 1., 1., 1., 1.};

static GLfloat mtl_amb[] = { 0.2, 0.2, 0.2, 1.0 };
static GLfloat mtl_dif[] = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat mtl_spec[] = { 0.0, 0.0, 0.0, 1.0 };
static GLfloat mtl_shine[] = { 0.0 };
static GLfloat mtl_emission[] = { 0.0, 0.0, 0.0, 1.0 };

static int mousex, mousey, omx, omy, lmouse;

void Lpanel_procEvent(Lpanel_t *p, SDL_Event *event)
{
	switch (event->type) {
	case SDL_WINDOWEVENT:
		if (event->window.windowID != SDL_GetWindowID(p->window))
			break;

		Lpanel_resizeWindow(p);
		break;

	case SDL_QUIT:
		// call user quit callback if exists
		if (p->quit_callbackfunc)
			(*p->quit_callbackfunc)();
		break;

	case SDL_MOUSEBUTTONDOWN:
		if (event->button.windowID != SDL_GetWindowID(p->window))
			break;

		if (!Lpanel_pick(p, event->button.button - 1, 1, event->button.x, event->button.y)) {
			if (event->button.button == SDL_BUTTON_LEFT) {	// left mousebutton ?
				lmouse = 1;
				mousex = event->button.x;
				mousey = event->button.y;
			}
		}
		break;

	case SDL_MOUSEBUTTONUP:
		if (event->button.windowID != SDL_GetWindowID(p->window))
			break;

		if (!Lpanel_pick(p, event->button.button - 1, 0, event->button.x, event->button.y)) {
			if (event->button.button == SDL_BUTTON_LEFT)
				lmouse = 0;
		}
		break;

	case SDL_KEYUP:
		if (event->key.windowID != SDL_GetWindowID(p->window))
			break;

		switch (event->key.keysym.sym) {
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			p->shift_key_pressed = false;
			break;

		}
		break;

	case SDL_KEYDOWN:
		if (event->key.windowID != SDL_GetWindowID(p->window))
			break;

		switch (event->key.keysym.sym) {
		case SDLK_ESCAPE:
			// exit(EXIT_SUCCESS);
			break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			p->shift_key_pressed = true;
			break;

		case SDLK_c:
			p->do_cursor = !p->do_cursor;
			break;

		case SDLK_d:
			p->view.pan[1] -= 0.1;
			p->view.redo_projections = true;
			break;

		case SDLK_s:
			p->do_stats = !p->do_stats;
			break;

		case SDLK_l:
			p->view.rot[1] += -1.;
			p->view.redo_projections = true;
			break;

		case SDLK_r:
			p->view.rot[1] -= -1.;
			p->view.redo_projections = true;
			break;

		case SDLK_u:
			p->view.pan[1] += 0.1;
			p->view.redo_projections = true;
			break;

		case SDLK_v:
			if (p->view.projection == LP_ORTHO)
				p->view.projection = LP_PERSPECTIVE;
			else
				p->view.projection = LP_ORTHO;

			p->view.redo_projections = true;
			break;

		case SDLK_z:
			if (p->shift_key_pressed) {
				p->view.pan[2] += .1;
				p->view.redo_projections = true;
			} else {
				p->view.pan[2] -= .1;
				p->view.redo_projections = true;
			}
			break;

		case SDLK_UP:
			if (p->do_cursor)
				Lpanel_inc_cursor(p, 0., p->cursor_inc);
			else {
				if (p->shift_key_pressed)
					p->view.pan[1] += -0.1;
				else
					p->view.rot[0] += -1.;

				p->view.redo_projections = true;
			}
			break;

		case SDLK_DOWN:
			if (p->do_cursor)
				Lpanel_inc_cursor(p, 0., -p->cursor_inc);
			else {
				if (p->shift_key_pressed)
					p->view.pan[1] += 0.1;
				else
					p->view.rot[0] += 1.;

				p->view.redo_projections = true;
			}
			break;

		case SDLK_RIGHT:
			if (p->do_cursor)
				Lpanel_inc_cursor(p, p->cursor_inc, 0.);
			else {
				if (p->shift_key_pressed)
					p->view.pan[0] += -.1;
				else
					p->view.rot[1] += 1.;

				p->view.redo_projections = true;
			}
			break;

		case SDLK_LEFT:
			if (p->do_cursor)
				Lpanel_inc_cursor(p, -p->cursor_inc, 0.);
			else {
				if (p->shift_key_pressed)
					p->view.pan[0] += .1;
				else
					p->view.rot[1] += -1.;

				p->view.redo_projections = true;
			}
			break;

		case SDLK_1:
		case SDLK_KP_1:
			break;

		default:
			break;
		}
		/* fallthrough */

	case SDL_MOUSEMOTION:
		if (event->motion.windowID != SDL_GetWindowID(p->window))
			break;

		if (lmouse) {
			omx = mousex;
			omy = mousey;

			if (p->shift_key_pressed) {
				p->view.pan[0] += ((float) event->motion.x - (float) omx) * .02;
				p->view.pan[1] -= ((float) event->motion.y - (float) omy) * .02;
			} else {
				p->view.rot[1] += ((float) event->motion.x - (float) omx) * .2;
				p->view.rot[0] += ((float) event->motion.y - (float) omy) * .2;
			}

			mousex = event->motion.x;
			mousey = event->motion.y;
			p->view.redo_projections = true;
		}
		break;
	default:
		break;
	}
} // end Lpanel_procEvent()

int Lpanel_openWindow(Lpanel_t *p, const char *title)
{
	float geom_aspect = (p->bbox.xyz_max[0] - p->bbox.xyz_min[0]) /
			    (p->bbox.xyz_max[1] - p->bbox.xyz_min[1]);
	p->window_ysize = (int) ((float) p->window_xsize / geom_aspect);
	p->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				     p->window_xsize, p->window_ysize,
				     SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (p->window == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "Can't create window: %s\n", SDL_GetError());
		return 0;
	}

	return 1;
}

void Lpanel_destroyWindow(Lpanel_t *p)
{
	SDL_DestroyWindow(p->window);
	p->cx = NULL;
	p->window = NULL;
}

void Lpanel_setModelview(Lpanel_t *p, int dopick)
{
	float x, y;

	if (!dopick)
		glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	switch (p->view.projection) {
	case LP_ORTHO:
		glTranslatef(0., 0., -10.);	// so objects at z=0 don't get clipped
		break;

	case LP_PERSPECTIVE:
		x = -((p->bbox.xyz_min[0] + p->bbox.xyz_max[0]) * 0.5 - p->view.pan[0]);
		y = -((p->bbox.xyz_min[1] + p->bbox.xyz_max[1]) * 0.5 - p->view.pan[1]);

		glTranslatef(x, y, p->view.pan[2]);

		glTranslatef(p->bbox.center[0], p->bbox.center[1], p->bbox.center[2]);
		glRotatef(p->view.rot[2], 0., 0., 1.);
		glRotatef(p->view.rot[0], 1., 0., 0.);
		glRotatef(p->view.rot[1], 0., 1., 0.);
		glTranslatef(-p->bbox.center[0], -p->bbox.center[1], -p->bbox.center[2]);
		break;
	}
}

void Lpanel_setProjection(Lpanel_t *p, int dopick)
{
	switch (p->view.projection) {
	case LP_ORTHO:
		if (!dopick) {
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
		}

		glOrtho(p->bbox.xyz_min[0], p->bbox.xyz_max[0],
			p->bbox.xyz_min[1], p->bbox.xyz_max[1],
			.1, 1000.);

		if (!dopick)
			glMatrixMode(GL_MODELVIEW);
		break;

	case LP_PERSPECTIVE:
		if (!dopick) {
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
		}

		// gluPerspective(p->view.fovy, p->view.aspect, p->view.znear, p->view.zfar);
		double deltaz = p->view.zfar - p->view.znear;
		double cotangent = 1 / tan(p->view.fovy / 2 * M_PI / 180);
		GLdouble m[16] = {
			cotangent / p->view.aspect, 0, 0, 0,
			0, cotangent, 0, 0,
			0, 0, -(p->view.zfar + p->view.znear) / deltaz, -1,
			0, 0, -2 * p->view.znear *p->view.zfar / deltaz, 0
		};
		glMultMatrixd(m);

		if (!dopick)
			glMatrixMode(GL_MODELVIEW);
		break;
	}
}

void Lpanel_resizeWindow(Lpanel_t *p)
{
	int width, height;

	SDL_GL_GetDrawableSize(p->window, &width, &height);

	p->window_xsize = width;
	p->window_ysize = height;
	p->view.aspect = (double) width / (double) height;
	glViewport(0, 0, width, height);
	glGetIntegerv(GL_VIEWPORT, p->viewport);

	Lpanel_setProjection(p, 0);

	Lpanel_setModelview(p, 0);
}

void Lpanel_doPickProjection(Lpanel_t *p)
{
	// glOrtho(p->bbox.xyz_min[0], p->bbox.xyz_max[0],
	// 	p->bbox.xyz_min[1], p->bbox.xyz_max[1],
	// 	.1, 1000.);
	Lpanel_setProjection(p, 1);
}

void Lpanel_doPickModelview(Lpanel_t *p)
{
	Lpanel_setModelview(p, 1);
	// glTranslatef(0., 0., -10.);	// so objects at z=0 don't get clipped
}

void Lpanel_initGraphics(Lpanel_t *p)
{
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 8);
	p->cx = SDL_GL_CreateContext(p->window);
	if (p->cx == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
			     "Can't create OpenGL context: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	/* First try adaptive vsync, than vsync */
	if (SDL_GL_SetSwapInterval(-1) < 0)
		SDL_GL_SetSwapInterval(1);

	Lpanel_resizeWindow(p);

	// initialize materials

	lp_init_materials_dlist();

	// define lights in case we use them

	glLoadIdentity();
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_specular);

	// glLightfv(GL_LIGHT1, GL_POSITION, light_pos1);
	// glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
	// glLightfv(GL_LIGHT1, GL_DIFFUSE, light_specular);

	glEnable(GL_LIGHT0);
	if (p->view.do_depthtest)
		glEnable(GL_DEPTH_TEST);
	glPolygonOffset(0., -10.);
	// glEnable(GL_LIGHT1);

	glDisable(GL_LIGHTING);
	// glEnable(GL_LIGHTING);
	glClearColor(0., 0., 0., 1.);

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mtl_amb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mtl_dif);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mtl_spec);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mtl_shine);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mtl_emission);

	glEnable(GL_NORMALIZE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SDL_GL_SwapWindow(p->window);

	// download any textures that may have been read in

	lpTextures_downloadTextures(&p->textures);

	if (p->envmap_detected) {
		glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	}

	p->cursor[0] = (p->bbox.xyz_max[0] + p->bbox.xyz_min[0]) * .5;
	p->cursor[1] = (p->bbox.xyz_max[1] + p->bbox.xyz_min[1]) * .5;
	makeRasterFont();
	Lpanel_make_cursor_text(p);
}

void Lpanel_exitGraphics(Lpanel_t *p)
{
	glFlush();
	glFinish();

	SDL_GL_DeleteContext(p->cx);
}

void Lpanel_draw_stats(Lpanel_t *p)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0., p->window_xsize, 0., p->window_ysize, .1, 1000.);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(0., 0., -10.);
	glDisable(GL_DEPTH_TEST);

	glColor3f(1., 1., 0.);
	snprintf(p->perf_txt, sizeof(p->perf_txt), "fps:%d sps:%d",
		 p->frames_per_second, p->samples_per_second);
	printStringAt(p->perf_txt, p->bbox.xyz_min[0] + .2, p->bbox.xyz_min[1] + .2);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

void Lpanel_draw_cursor(Lpanel_t *p)
{
	float size = 0.1;

	glColor3f(1., 1., 0.);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
	glVertex3f(p->cursor[0] - size, p->cursor[1] - size, p->cursor[2]);
	glVertex3f(p->cursor[0] + size, p->cursor[1] + size, p->cursor[2]);

	glVertex3f(p->cursor[0] - size, p->cursor[1] + size, p->cursor[2]);
	glVertex3f(p->cursor[0] + size, p->cursor[1] - size, p->cursor[2]);

	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0., p->window_xsize, 0., p->window_ysize, .1, 1000.);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslatef(0., 0., -10.);

	printStringAt(p->cursor_txt, p->cursor_textpos[0], p->cursor_textpos[1]);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
}

void Lpanel_inc_cursor(Lpanel_t *p, float x, float y)
{
	if (p->shift_key_pressed) {
		x *= 10.;
		y *= 10.;
	}
	p->cursor[0] += x;
	p->cursor[1] += y;
	Lpanel_make_cursor_text(p);
}

void Lpanel_make_cursor_text(Lpanel_t *p)
{
	p->cursor_textpos[0] = (p->bbox.xyz_max[0] + p->bbox.xyz_min[0]) * .5;
	p->cursor_textpos[1] = p->bbox.xyz_min[1] + .1;
	snprintf(p->cursor_txt, sizeof(p->cursor_txt), "cursor position=%7.3f,%7.3f",
		 p->cursor[0], p->cursor[1]);
}
