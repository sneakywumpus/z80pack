/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Copyright (C) 1987-2024 by Udo Munk
 */

#ifndef SIMIO_INC
#define SIMIO_INC

#include "sim.h"
#include "simdefs.h"

#define IO_DATA_UNUSED	0xff	/* data returned on unused ports */

extern BYTE (*const port_in[256])(void);
extern void (*const port_out[256])(BYTE data);

extern void init_io(void);
extern void exit_io(void);

#endif /* !SIMIO_INC */
