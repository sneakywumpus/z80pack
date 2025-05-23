/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Copyright (C) 1987-2021 by Udo Munk
 * Copyright (C) 2022-2024 by Thomas Eberhardt
 */

/*
 *	Like the function "cpu_z80()" this one emulates multi byte opcodes
 *	starting with 0xed
 */

#include "sim.h"
#include "simdefs.h"
#include "simglb.h"
#include "simcore.h"
#include "simmem.h"
#include "simz80-ed.h"

#ifdef FRONTPANEL
#include "frontpanel.h"
#endif

#if !defined(EXCLUDE_Z80) && !defined(ALT_Z80)

static int trap_ed(void);
static int op_im0(void), op_im1(void), op_im2(void);
static int op_reti(void), op_retn(void);
static int op_neg(void);
static int op_inaic(void), op_inbic(void), op_incic(void);
static int op_indic(void), op_ineic(void);
static int op_inhic(void), op_inlic(void);
static int op_outca(void), op_outcb(void), op_outcc(void);
static int op_outcd(void), op_outce(void);
static int op_outch(void), op_outcl(void);
static int op_ini(void), op_inir(void), op_ind(void), op_indr(void);
static int op_outi(void), op_otir(void), op_outd(void), op_otdr(void);
static int op_ldai(void), op_ldar(void), op_ldia(void), op_ldra(void);
static int op_ldbcinn(void), op_lddeinn(void);
static int op_ldhlinn(void), op_ldspinn(void);
static int op_ldinbc(void), op_ldinde(void), op_ldinhl(void), op_ldinsp(void);
static int op_adchb(void), op_adchd(void), op_adchh(void), op_adchs(void);
static int op_sbchb(void), op_sbchd(void), op_sbchh(void), op_sbchs(void);
static int op_ldi(void), op_ldir(void), op_ldd(void), op_lddr(void);
static int op_cpi(void), op_cpir(void), op_cpdop(void), op_cpdr(void);
static int op_oprld(void), op_oprrd(void);

#ifdef UNDOC_INST
static int op_undoc_outc0(void), op_undoc_infic(void);
#endif

int op_ed_handle(void)
{

#ifdef UNDOC_INST
#define UNDOC(f) f
#else
#define UNDOC(f) trap_ed
#endif

	static int (*op_ed[256])(void) = {
		trap_ed,			/* 0x00 */
		trap_ed,			/* 0x01 */
		trap_ed,			/* 0x02 */
		trap_ed,			/* 0x03 */
		trap_ed,			/* 0x04 */
		trap_ed,			/* 0x05 */
		trap_ed,			/* 0x06 */
		trap_ed,			/* 0x07 */
		trap_ed,			/* 0x08 */
		trap_ed,			/* 0x09 */
		trap_ed,			/* 0x0a */
		trap_ed,			/* 0x0b */
		trap_ed,			/* 0x0c */
		trap_ed,			/* 0x0d */
		trap_ed,			/* 0x0e */
		trap_ed,			/* 0x0f */
		trap_ed,			/* 0x10 */
		trap_ed,			/* 0x11 */
		trap_ed,			/* 0x12 */
		trap_ed,			/* 0x13 */
		trap_ed,			/* 0x14 */
		trap_ed,			/* 0x15 */
		trap_ed,			/* 0x16 */
		trap_ed,			/* 0x17 */
		trap_ed,			/* 0x18 */
		trap_ed,			/* 0x19 */
		trap_ed,			/* 0x1a */
		trap_ed,			/* 0x1b */
		trap_ed,			/* 0x1c */
		trap_ed,			/* 0x1d */
		trap_ed,			/* 0x1e */
		trap_ed,			/* 0x1f */
		trap_ed,			/* 0x20 */
		trap_ed,			/* 0x21 */
		trap_ed,			/* 0x22 */
		trap_ed,			/* 0x23 */
		trap_ed,			/* 0x24 */
		trap_ed,			/* 0x25 */
		trap_ed,			/* 0x26 */
		trap_ed,			/* 0x27 */
		trap_ed,			/* 0x28 */
		trap_ed,			/* 0x29 */
		trap_ed,			/* 0x2a */
		trap_ed,			/* 0x2b */
		trap_ed,			/* 0x2c */
		trap_ed,			/* 0x2d */
		trap_ed,			/* 0x2e */
		trap_ed,			/* 0x2f */
		trap_ed,			/* 0x30 */
		trap_ed,			/* 0x31 */
		trap_ed,			/* 0x32 */
		trap_ed,			/* 0x33 */
		trap_ed,			/* 0x34 */
		trap_ed,			/* 0x35 */
		trap_ed,			/* 0x36 */
		trap_ed,			/* 0x37 */
		trap_ed,			/* 0x38 */
		trap_ed,			/* 0x39 */
		trap_ed,			/* 0x3a */
		trap_ed,			/* 0x3b */
		trap_ed,			/* 0x3c */
		trap_ed,			/* 0x3d */
		trap_ed,			/* 0x3e */
		trap_ed,			/* 0x3f */
		op_inbic,			/* 0x40 */
		op_outcb,			/* 0x41 */
		op_sbchb,			/* 0x42 */
		op_ldinbc,			/* 0x43 */
		op_neg,				/* 0x44 */
		op_retn,			/* 0x45 */
		op_im0,				/* 0x46 */
		op_ldia,			/* 0x47 */
		op_incic,			/* 0x48 */
		op_outcc,			/* 0x49 */
		op_adchb,			/* 0x4a */
		op_ldbcinn,			/* 0x4b */
		trap_ed,			/* 0x4c */
		op_reti,			/* 0x4d */
		trap_ed,			/* 0x4e */
		op_ldra,			/* 0x4f */
		op_indic,			/* 0x50 */
		op_outcd,			/* 0x51 */
		op_sbchd,			/* 0x52 */
		op_ldinde,			/* 0x53 */
		trap_ed,			/* 0x54 */
		trap_ed,			/* 0x55 */
		op_im1,				/* 0x56 */
		op_ldai,			/* 0x57 */
		op_ineic,			/* 0x58 */
		op_outce,			/* 0x59 */
		op_adchd,			/* 0x5a */
		op_lddeinn,			/* 0x5b */
		trap_ed,			/* 0x5c */
		trap_ed,			/* 0x5d */
		op_im2,				/* 0x5e */
		op_ldar,			/* 0x5f */
		op_inhic,			/* 0x60 */
		op_outch,			/* 0x61 */
		op_sbchh,			/* 0x62 */
		op_ldinhl,			/* 0x63 */
		trap_ed,			/* 0x64 */
		trap_ed,			/* 0x65 */
		trap_ed,			/* 0x66 */
		op_oprrd,			/* 0x67 */
		op_inlic,			/* 0x68 */
		op_outcl,			/* 0x69 */
		op_adchh,			/* 0x6a */
		op_ldhlinn,			/* 0x6b */
		trap_ed,			/* 0x6c */
		trap_ed,			/* 0x6d */
		trap_ed,			/* 0x6e */
		op_oprld,			/* 0x6f */
		UNDOC(op_undoc_infic),		/* 0x70 */
		UNDOC(op_undoc_outc0),		/* 0x71 */
		op_sbchs,			/* 0x72 */
		op_ldinsp,			/* 0x73 */
		trap_ed,			/* 0x74 */
		trap_ed,			/* 0x75 */
		trap_ed,			/* 0x76 */
		trap_ed,			/* 0x77 */
		op_inaic,			/* 0x78 */
		op_outca,			/* 0x79 */
		op_adchs,			/* 0x7a */
		op_ldspinn,			/* 0x7b */
		trap_ed,			/* 0x7c */
		trap_ed,			/* 0x7d */
		trap_ed,			/* 0x7e */
		trap_ed,			/* 0x7f */
		trap_ed,			/* 0x80 */
		trap_ed,			/* 0x81 */
		trap_ed,			/* 0x82 */
		trap_ed,			/* 0x83 */
		trap_ed,			/* 0x84 */
		trap_ed,			/* 0x85 */
		trap_ed,			/* 0x86 */
		trap_ed,			/* 0x87 */
		trap_ed,			/* 0x88 */
		trap_ed,			/* 0x89 */
		trap_ed,			/* 0x8a */
		trap_ed,			/* 0x8b */
		trap_ed,			/* 0x8c */
		trap_ed,			/* 0x8d */
		trap_ed,			/* 0x8e */
		trap_ed,			/* 0x8f */
		trap_ed,			/* 0x90 */
		trap_ed,			/* 0x91 */
		trap_ed,			/* 0x92 */
		trap_ed,			/* 0x93 */
		trap_ed,			/* 0x94 */
		trap_ed,			/* 0x95 */
		trap_ed,			/* 0x96 */
		trap_ed,			/* 0x97 */
		trap_ed,			/* 0x98 */
		trap_ed,			/* 0x99 */
		trap_ed,			/* 0x9a */
		trap_ed,			/* 0x9b */
		trap_ed,			/* 0x9c */
		trap_ed,			/* 0x9d */
		trap_ed,			/* 0x9e */
		trap_ed,			/* 0x9f */
		op_ldi,				/* 0xa0 */
		op_cpi,				/* 0xa1 */
		op_ini,				/* 0xa2 */
		op_outi,			/* 0xa3 */
		trap_ed,			/* 0xa4 */
		trap_ed,			/* 0xa5 */
		trap_ed,			/* 0xa6 */
		trap_ed,			/* 0xa7 */
		op_ldd,				/* 0xa8 */
		op_cpdop,			/* 0xa9 */
		op_ind,				/* 0xaa */
		op_outd,			/* 0xab */
		trap_ed,			/* 0xac */
		trap_ed,			/* 0xad */
		trap_ed,			/* 0xae */
		trap_ed,			/* 0xaf */
		op_ldir,			/* 0xb0 */
		op_cpir,			/* 0xb1 */
		op_inir,			/* 0xb2 */
		op_otir,			/* 0xb3 */
		trap_ed,			/* 0xb4 */
		trap_ed,			/* 0xb5 */
		trap_ed,			/* 0xb6 */
		trap_ed,			/* 0xb7 */
		op_lddr,			/* 0xb8 */
		op_cpdr,			/* 0xb9 */
		op_indr,			/* 0xba */
		op_otdr,			/* 0xbb */
		trap_ed,			/* 0xbc */
		trap_ed,			/* 0xbd */
		trap_ed,			/* 0xbe */
		trap_ed,			/* 0xbf */
		trap_ed,			/* 0xc0 */
		trap_ed,			/* 0xc1 */
		trap_ed,			/* 0xc2 */
		trap_ed,			/* 0xc3 */
		trap_ed,			/* 0xc4 */
		trap_ed,			/* 0xc5 */
		trap_ed,			/* 0xc6 */
		trap_ed,			/* 0xc7 */
		trap_ed,			/* 0xc8 */
		trap_ed,			/* 0xc9 */
		trap_ed,			/* 0xca */
		trap_ed,			/* 0xcb */
		trap_ed,			/* 0xcc */
		trap_ed,			/* 0xcd */
		trap_ed,			/* 0xce */
		trap_ed,			/* 0xcf */
		trap_ed,			/* 0xd0 */
		trap_ed,			/* 0xd1 */
		trap_ed,			/* 0xd2 */
		trap_ed,			/* 0xd3 */
		trap_ed,			/* 0xd4 */
		trap_ed,			/* 0xd5 */
		trap_ed,			/* 0xd6 */
		trap_ed,			/* 0xd7 */
		trap_ed,			/* 0xd8 */
		trap_ed,			/* 0xd9 */
		trap_ed,			/* 0xda */
		trap_ed,			/* 0xdb */
		trap_ed,			/* 0xdc */
		trap_ed,			/* 0xdd */
		trap_ed,			/* 0xde */
		trap_ed,			/* 0xdf */
		trap_ed,			/* 0xe0 */
		trap_ed,			/* 0xe1 */
		trap_ed,			/* 0xe2 */
		trap_ed,			/* 0xe3 */
		trap_ed,			/* 0xe4 */
		trap_ed,			/* 0xe5 */
		trap_ed,			/* 0xe6 */
		trap_ed,			/* 0xe7 */
		trap_ed,			/* 0xe8 */
		trap_ed,			/* 0xe9 */
		trap_ed,			/* 0xea */
		trap_ed,			/* 0xeb */
		trap_ed,			/* 0xec */
		trap_ed,			/* 0xed */
		trap_ed,			/* 0xee */
		trap_ed,			/* 0xef */
		trap_ed,			/* 0xf0 */
		trap_ed,			/* 0xf1 */
		trap_ed,			/* 0xf2 */
		trap_ed,			/* 0xf3 */
		trap_ed,			/* 0xf4 */
		trap_ed,			/* 0xf5 */
		trap_ed,			/* 0xf6 */
		trap_ed,			/* 0xf7 */
		trap_ed,			/* 0xf8 */
		trap_ed,			/* 0xf9 */
		trap_ed,			/* 0xfa */
		trap_ed,			/* 0xfb */
		trap_ed,			/* 0xfc */
		trap_ed,			/* 0xfd */
		trap_ed,			/* 0xfe */
		trap_ed				/* 0xff */
	};

#undef UNDOC

	register int t;

#ifdef BUS_8080
	/* M1 opcode fetch */
	cpu_bus = CPU_WO | CPU_M1 | CPU_MEMR;
	m1_step = true;
#endif
#ifdef FRONTPANEL
	if (F_flag) {
		/* update frontpanel */
		fp_clock++;
		fp_sampleLightGroup(0, 0);
	}
#endif

	R++;				/* increment refresh register */

	t = (*op_ed[memrdr(PC++)])();	/* execute next opcode */

	return t;
}

/*
 *	This function traps undocumented opcodes following the
 *	initial 0xed of a multi byte opcode.
 */
static int trap_ed(void)
{
	cpu_error = OPTRAP2;
	cpu_state = ST_STOPPED;
	return 0;
}

static int op_im0(void)			/* IM 0 */
{
	int_mode = 0;
	return 8;
}

static int op_im1(void)			/* IM 1 */
{
	int_mode = 1;
	return 8;
}

static int op_im2(void)			/* IM 2 */
{
	int_mode = 2;
	return 8;
}

static int op_reti(void)		/* RETI */
{
	register WORD i;

	i = memrdr(SP++);
	i += memrdr(SP++) << 8;
	PC = i;
	return 14;
}

static int op_retn(void)		/* RETN */
{
	register WORD i;

	i = memrdr(SP++);
	i += memrdr(SP++) << 8;
	PC = i;
	if (IFF & 2)
		IFF |= 1;
	return 14;
}

static int op_neg(void)			/* NEG */
{
	(A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
	(A == 0x80) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(0 - ((SBYTE) A & 0xf) < 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
	A = 0 - A;
	F |= N_FLAG;
	(A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	return 8;
}

static int op_inaic(void)		/* IN A,(C) */
{
	A = io_in(C, B);
	F &= ~(N_FLAG | H_FLAG);
	(A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	(parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	return 12;
}

static int op_inbic(void)		/* IN B,(C) */
{
	B = io_in(C, B);
	F &= ~(N_FLAG | H_FLAG);
	(B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(B & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	(parity[B]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	return 12;
}

static int op_incic(void)		/* IN C,(C) */
{
	C = io_in(C, B);
	F &= ~(N_FLAG | H_FLAG);
	(C) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(C & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	(parity[C]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	return 12;
}

static int op_indic(void)		/* IN D,(C) */
{
	D = io_in(C, B);
	F &= ~(N_FLAG | H_FLAG);
	(D) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(D & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	(parity[D]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	return 12;
}

static int op_ineic(void)		/* IN E,(C) */
{
	E = io_in(C, B);
	F &= ~(N_FLAG | H_FLAG);
	(E) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(E & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	(parity[E]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	return 12;
}

static int op_inhic(void)		/* IN H,(C) */
{
	H = io_in(C, B);
	F &= ~(N_FLAG | H_FLAG);
	(H) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	(parity[H]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	return 12;
}

static int op_inlic(void)		/* IN L,(C) */
{
	L = io_in(C, B);
	F &= ~(N_FLAG | H_FLAG);
	(L) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(L & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	(parity[L]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	return 12;
}

static int op_outca(void)		/* OUT (C),A */
{
	io_out(C, B, A);
	return 12;
}

static int op_outcb(void)		/* OUT (C),B */
{
	io_out(C, B, B);
	return 12;
}

static int op_outcc(void)		/* OUT (C),C */
{
	io_out(C, B, C);
	return 12;
}

static int op_outcd(void)		/* OUT (C),D */
{
	io_out(C, B, D);
	return 12;
}

static int op_outce(void)		/* OUT (C),E */
{
	io_out(C, B, E);
	return 12;
}

static int op_outch(void)		/* OUT (C),H */
{
	io_out(C, B, H);
	return 12;
}

static int op_outcl(void)		/* OUT (C),L */
{
	io_out(C, B, L);
	return 12;
}

static int op_ini(void)			/* INI */
{
	BYTE data;
	WORD k;

	data = io_in(C, B);
	B--;
	memwrt((H << 8) + L, data);
	L++;
	if (!L)
		H++;
#if 0
	F |= N_FLAG; /* As documented in the "Z80 CPU User Manual" */
#else
	/* S,H,P,N,C flags according to "The Undocumented Z80 Documented" */
	k = (WORD) ((C + 1) & 0xff) + (WORD) data;
	(k > 255) ? (F |= (H_FLAG | C_FLAG)) : (F &= ~(H_FLAG | C_FLAG));
	(parity[(k & 0x07) ^ B]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	(data & 128) ? (F |= N_FLAG) : (F &= ~N_FLAG);
	(B & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
#endif
	(B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	return 16;
}

#ifdef FAST_BLOCK
static int op_inir(void)		/* INIR */
{
	WORD addr;
	BYTE data;
	WORD k;
	register int t = -21;

	addr = (H << 8) + L;
	R -= 2;
	do {
		data = io_in(C, B);
		B--;
		memwrt(addr++, data);
		t += 21;
		R += 2;
	} while (B);
	H = addr >> 8;
	L = addr;
#if 0
	F |= N_FLAG; /* As documented in the "Z80 CPU User Manual" */
#else
	/* S,H,P,N,C flags according to "The Undocumented Z80 Documented" */
	k = (WORD) ((C + 1) & 0xff) + (WORD) data;
	(k > 255) ? (F |= (H_FLAG | C_FLAG)) : (F &= ~(H_FLAG | C_FLAG));
	(parity[k & 0x07]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	(data & 128) ? (F |= N_FLAG) : (F &= ~N_FLAG);
	F &= ~S_FLAG;
#endif
	F |= Z_FLAG;
	return t + 16;
}
#else /* !FAST_BLOCK */
static int op_inir(void)		/* INIR */
{
	op_ini();
	if (!(F & Z_FLAG)) {
		PC -= 2;
		return 21;
	}
	return 16;
}
#endif /* !FAST_BLOCK */

static int op_ind(void)			/* IND */
{
	BYTE data;
	WORD k;

	data = io_in(C, B);
	B--;
	memwrt((H << 8) + L, data);
	L--;
	if (L == 0xff)
		H--;
#if 0
	F |= N_FLAG; /* As documented in the "Z80 CPU User Manual" */
#else
	/* S,H,P,N,C flags according to "The Undocumented Z80 Documented" */
	k = (WORD) ((C - 1) & 0xff) + (WORD) data;
	(k > 255) ? (F |= (H_FLAG | C_FLAG)) : (F &= ~(H_FLAG | C_FLAG));
	(parity[(k & 0x07) ^ B]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	(data & 128) ? (F |= N_FLAG) : (F &= ~N_FLAG);
	(B & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
#endif
	(B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	return 16;
}

#ifdef FAST_BLOCK
static int op_indr(void)		/* INDR */
{
	WORD addr;
	BYTE data;
	WORD k;
	register int t = -21;

	addr = (H << 8) + L;
	R -= 2;
	do {
		data = io_in(C, B);
		memwrt(addr--, data);
		B--;
		t += 21;
		R += 2;
	} while (B);
	H = addr >> 8;
	L = addr;
#if 0
	F |= N_FLAG; /* As documented in the "Z80 CPU User Manual" */
#else
	/* S,H,P,N,C flags according to "The Undocumented Z80 Documented" */
	k = (WORD) ((C - 1) & 0xff) + (WORD) data;
	(k > 255) ? (F |= (H_FLAG | C_FLAG)) : (F &= ~(H_FLAG | C_FLAG));
	(parity[k & 0x07]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	(data & 128) ? (F |= N_FLAG) : (F &= ~N_FLAG);
	F &= ~S_FLAG;
#endif
	F |= Z_FLAG;
	return t + 16;
}
#else /* !FAST_BLOCK */
static int op_indr(void)		/* INDR */
{
	op_ind();
	if (!(F & Z_FLAG)) {
		PC -= 2;
		return 21;
	}
	return 16;
}
#endif /* !FAST_BLOCK */

static int op_outi(void)		/* OUTI */
{
	BYTE data;
	WORD k;

	data = memrdr((H << 8) + L);
	B--;
	io_out(C, B, data);
	L++;
	if (!L)
		H++;
#if 0
	F |= N_FLAG; /* As documented in the "Z80 CPU User Manual" */
#else
	/* S,H,P,N,C flags according to "The Undocumented Z80 Documented" */
	k = (WORD) L + (WORD) data;
	(k > 255) ? (F |= (H_FLAG | C_FLAG)) : (F &= ~(H_FLAG | C_FLAG));
	(parity[(k & 0x07) ^ B]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	(data & 128) ? (F |= N_FLAG) : (F &= ~N_FLAG);
	(B & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
#endif
	(B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	return 16;
}

#ifdef FAST_BLOCK
static int op_otir(void)		/* OTIR */
{
	WORD addr;
	BYTE data;
	WORD k;
	register int t = -21;

	addr = (H << 8) + L;
	R -= 2;
	do {
		B--;
		data = memrdr(addr++);
		io_out(C, B, data);
		t += 21;
		R += 2;
	} while (B);
	H = addr >> 8;
	L = addr;
#if 0
	F |= N_FLAG; /* As documented in the "Z80 CPU User Manual" */
#else
	/* S,H,P,N,C flags according to "The Undocumented Z80 Documented" */
	k = (WORD) L + (WORD) data;
	(k > 255) ? (F |= (H_FLAG | C_FLAG)) : (F &= ~(H_FLAG | C_FLAG));
	(parity[k & 0x07]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	(data & 128) ? (F |= N_FLAG) : (F &= ~N_FLAG);
	F &= ~S_FLAG;
#endif
	F |= Z_FLAG;
	return t + 16;
}
#else /* !FAST_BLOCK */
static int op_otir(void)		/* OTIR */
{
	op_outi();
	if (!(F & Z_FLAG)) {
		PC -= 2;
		return 21;
	}
	return 16;
}
#endif /* !FAST_BLOCK */

static int op_outd(void)		/* OUTD */
{
	BYTE data;
	WORD k;

	data = memrdr((H << 8) + L);
	B--;
	io_out(C, B, data);
	L--;
	if (L == 0xff)
		H--;
#if 0
	F |= N_FLAG; /* As documented in the "Z80 CPU User Manual" */
#else
	/* S,H,P,N,C flags according to "The Undocumented Z80 Documented" */
	k = (WORD) L + (WORD) data;
	(k > 255) ? (F |= (H_FLAG | C_FLAG)) : (F &= ~(H_FLAG | C_FLAG));
	(parity[(k & 0x07) ^ B]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	(data & 128) ? (F |= N_FLAG) : (F &= ~N_FLAG);
	(B & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
#endif
	(B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	return 16;
}

#ifdef FAST_BLOCK
static int op_otdr(void)		/* OTDR */
{
	WORD addr;
	BYTE data;
	WORD k;
	register int t = -21;

	addr = (H << 8) + L;
	R -= 2;
	do {
		B--;
		data = memrdr(addr--);
		io_out(C, B, data);
		t += 21;
		R += 2;
	} while (B);
	H = addr >> 8;
	L = addr;
#if 0
	F |= N_FLAG; /* As documented in the "Z80 CPU User Manual" */
#else
	/* S,H,P,N,C flags according to "The Undocumented Z80 Documented" */
	k = (WORD) L + (WORD) data;
	(k > 255) ? (F |= (H_FLAG | C_FLAG)) : (F &= ~(H_FLAG | C_FLAG));
	(parity[k & 0x07]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	(data & 128) ? (F |= N_FLAG) : (F &= ~N_FLAG);
	F &= ~S_FLAG;
#endif
	F |= Z_FLAG;
	return t + 16;
}
#else /* !FAST_BLOCK */
static int op_otdr(void)		/* OTDR */
{
	op_outd();
	if (!(F & Z_FLAG)) {
		PC -= 2;
		return 21;
	}
	return 16;
}
#endif /* !FAST_BLOCK */

static int op_ldai(void)		/* LD A,I */
{
	A = I;
	F &= ~(N_FLAG | H_FLAG);
	(IFF & 2) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	return 9;
}

static int op_ldar(void)		/* LD A,R */
{
	A = (R_ & 0x80) | (R & 0x7f);
	F &= ~(N_FLAG | H_FLAG);
	(IFF & 2) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	return 9;
}

static int op_ldia(void)		/* LD I,A */
{
	I = A;
	return 9;
}

static int op_ldra(void)		/* LD R,A */
{
	R_ = R = A;
	return 9;
}

static int op_ldbcinn(void)		/* LD BC,(nn) */
{
	register WORD i;

	i = memrdr(PC++);
	i += memrdr(PC++) << 8;
	C = memrdr(i++);
	B = memrdr(i);
	return 20;
}

static int op_lddeinn(void)		/* LD DE,(nn) */
{
	register WORD i;

	i = memrdr(PC++);
	i += memrdr(PC++) << 8;
	E = memrdr(i++);
	D = memrdr(i);
	return 20;
}

static int op_ldhlinn(void)		/* LD HL,(nn) */
{
	register WORD i;

	i = memrdr(PC++);
	i += memrdr(PC++) << 8;
	L = memrdr(i++);
	H = memrdr(i);
	return 20;
}

static int op_ldspinn(void)		/* LD SP,(nn) */
{
	register WORD i;

	i = memrdr(PC++);
	i += memrdr(PC++) << 8;
	SP = memrdr(i++);
	SP += memrdr(i) << 8;
	return 20;
}

static int op_ldinbc(void)		/* LD (nn),BC */
{
	register WORD i;

	i = memrdr(PC++);
	i += memrdr(PC++) << 8;
	memwrt(i++, C);
	memwrt(i, B);
	return 20;
}

static int op_ldinde(void)		/* LD (nn),DE */
{
	register WORD i;

	i = memrdr(PC++);
	i += memrdr(PC++) << 8;
	memwrt(i++, E);
	memwrt(i, D);
	return 20;
}

static int op_ldinhl(void)		/* LD (nn),HL */
{
	register WORD i;

	i = memrdr(PC++);
	i += memrdr(PC++) << 8;
	memwrt(i++, L);
	memwrt(i, H);
	return 20;
}

static int op_ldinsp(void)		/* LD (nn),SP */
{
	register WORD addr;
	WORD i;

	addr = memrdr(PC++);
	addr += memrdr(PC++) << 8;
	i = SP;
	memwrt(addr++, i);
	memwrt(addr, i >> 8);
	return 20;
}

static int op_adchb(void)		/* ADC HL,BC */
{
	int carry, i;
	WORD hl, bc;
	SWORD shl, sbc;

	hl = (H << 8) + L;
	bc = (B << 8) + C;
	shl = hl;
	sbc = bc;
	carry = (F & C_FLAG) ? 1 : 0;
	(((hl & 0x0fff) + (bc & 0x0fff) + carry) > 0x0fff) ? (F |= H_FLAG)
							   : (F &= ~H_FLAG);
	i = shl + sbc + carry;
	((i > 32767) || (i < -32768)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(hl + bc + carry > 0xffff) ? (F |= C_FLAG) : (F &= ~C_FLAG);
	i &= 0xffff;
	(i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(i & 0x8000) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	H = i >> 8;
	L = i;
	F &= ~N_FLAG;
	return 15;
}

static int op_adchd(void)		/* ADC HL,DE */
{
	int carry, i;
	WORD hl, de;
	SWORD shl, sde;

	hl = (H << 8) + L;
	de = (D << 8) + E;
	shl = hl;
	sde = de;
	carry = (F & C_FLAG) ? 1 : 0;
	(((hl & 0x0fff) + (de & 0x0fff) + carry) > 0x0fff) ? (F |= H_FLAG)
							   : (F &= ~H_FLAG);
	i = shl + sde + carry;
	((i > 32767) || (i < -32768)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(hl + de + carry > 0xffff) ? (F |= C_FLAG) : (F &= ~C_FLAG);
	i &= 0xffff;
	(i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(i & 0x8000) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	H = i >> 8;
	L = i;
	F &= ~N_FLAG;
	return 15;
}

static int op_adchh(void)		/* ADC HL,HL */
{
	int carry, i;
	WORD hl;
	SWORD shl;

	hl = (H << 8) + L;
	shl = hl;
	carry = (F & C_FLAG) ? 1 : 0;
	(((hl & 0x0fff) + (hl & 0x0fff) + carry) > 0x0fff) ? (F |= H_FLAG)
							   : (F &= ~H_FLAG);
	i = shl + shl + carry;
	((i > 32767) || (i < -32768)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(hl + hl + carry > 0xffff) ? (F |= C_FLAG) : (F &= ~C_FLAG);
	i &= 0xffff;
	(i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(i & 0x8000) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	H = i >> 8;
	L = i;
	F &= ~N_FLAG;
	return 15;
}

static int op_adchs(void)		/* ADC HL,SP */
{
	int carry, i;
	WORD hl, sp;
	SWORD shl, ssp;

	hl = (H << 8) + L;
	sp = SP;
	shl = hl;
	ssp = sp;
	carry = (F & C_FLAG) ? 1 : 0;
	(((hl & 0x0fff) + (sp & 0x0fff) + carry) > 0x0fff) ? (F |= H_FLAG)
							   : (F &= ~H_FLAG);
	i = shl + ssp + carry;
	((i > 32767) || (i < -32768)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(hl + sp + carry > 0xffff) ? (F |= C_FLAG) : (F &= ~C_FLAG);
	i &= 0xffff;
	(i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(i & 0x8000) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	H = i >> 8;
	L = i;
	F &= ~N_FLAG;
	return 15;
}

static int op_sbchb(void)		/* SBC HL,BC */
{
	int carry, i;
	WORD hl, bc;
	SWORD shl, sbc;

	hl = (H << 8) + L;
	bc = (B << 8) + C;
	shl = hl;
	sbc = bc;
	carry = (F & C_FLAG) ? 1 : 0;
	(((bc & 0x0fff) + carry) > (hl & 0x0fff)) ? (F |= H_FLAG)
						  : (F &= ~H_FLAG);
	i = shl - sbc - carry;
	((i > 32767) || (i < -32768)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(bc + carry > hl) ? (F |= C_FLAG) : (F &= ~C_FLAG);
	i &= 0xffff;
	(i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(i & 0x8000) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	H = i >> 8;
	L = i;
	F |= N_FLAG;
	return 15;
}

static int op_sbchd(void)		/* SBC HL,DE */
{
	int carry, i;
	WORD hl, de;
	SWORD shl, sde;

	hl = (H << 8) + L;
	de = (D << 8) + E;
	shl = hl;
	sde = de;
	carry = (F & C_FLAG) ? 1 : 0;
	(((de & 0x0fff) + carry) > (hl & 0x0fff)) ? (F |= H_FLAG)
						  : (F &= ~H_FLAG);
	i = shl - sde - carry;
	((i > 32767) || (i < -32768)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(de + carry > hl) ? (F |= C_FLAG) : (F &= ~C_FLAG);
	i &= 0xffff;
	(i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(i & 0x8000) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	H = i >> 8;
	L = i;
	F |= N_FLAG;
	return 15;
}

static int op_sbchh(void)		/* SBC HL,HL */
{
	int carry, i;
	WORD hl;
	SWORD shl;

	hl = (H << 8) + L;
	shl = hl;
	carry = (F & C_FLAG) ? 1 : 0;
	(((hl & 0x0fff) + carry) > (hl & 0x0fff)) ? (F |= H_FLAG)
						  : (F &= ~H_FLAG);
	i = shl - shl - carry;
	((i > 32767) || (i < -32768)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(hl + carry > hl) ? (F |= C_FLAG) : (F &= ~C_FLAG);
	i &= 0xffff;
	(i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(i & 0x8000) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	H = i >> 8;
	L = i;
	F |= N_FLAG;
	return 15;
}

static int op_sbchs(void)		/* SBC HL,SP */
{
	int carry, i;
	WORD hl, sp;
	SWORD shl, ssp;

	hl = (H << 8) + L;
	sp = SP;
	shl = hl;
	ssp = sp;
	carry = (F & C_FLAG) ? 1 : 0;
	(((sp & 0x0fff) + carry) > (hl & 0x0fff)) ? (F |= H_FLAG)
						  : (F &= ~H_FLAG);
	i = shl - ssp - carry;
	((i > 32767) || (i < -32768)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(sp + carry > hl) ? (F |= C_FLAG) : (F &= ~C_FLAG);
	i &= 0xffff;
	(i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(i & 0x8000) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	H = i >> 8;
	L = i;
	F |= N_FLAG;
	return 15;
}

static int op_ldi(void)			/* LDI */
{
	memwrt((D << 8) + E, memrdr((H << 8) + L));
	E++;
	if (!E)
		D++;
	L++;
	if (!L)
		H++;
	C--;
	if (C == 0xff)
		B--;
	(B | C) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	F &= ~(N_FLAG | H_FLAG);
	return 16;
}

#ifdef FAST_BLOCK
static int op_ldir(void)		/* LDIR */
{
	register int t = -21;
	register WORD i;
	register WORD s, d;

	i = (B << 8) + C;
	d = (D << 8) + E;
	s = (H << 8) + L;
	R -= 2;
	do {
		memwrt(d++, memrdr(s++));
		t += 21;
		R += 2;
	} while (--i);
	B = C = 0;
	D = d >> 8;
	E = d;
	H = s >> 8;
	L = s;
	F &= ~(N_FLAG | P_FLAG | H_FLAG);
	return t + 16;
}
#else /* !FAST_BLOCK */
static int op_ldir(void)		/* LDIR */
{
	op_ldi();
	if (F & P_FLAG) {
		PC -= 2;
		return 21;
	}
	return 16;
}
#endif /* !FAST_BLOCK */

static int op_ldd(void)			/* LDD */
{
	memwrt((D << 8) + E, memrdr((H << 8) + L));
	E--;
	if (E == 0xff)
		D--;
	L--;
	if (L == 0xff)
		H--;
	C--;
	if (C == 0xff)
		B--;
	(B | C) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	F &= ~(N_FLAG | H_FLAG);
	return 16;
}

#ifdef FAST_BLOCK
static int op_lddr(void)		/* LDDR */
{
	register int t = -21;
	register WORD i;
	register WORD s, d;

	i = (B << 8) + C;
	d = (D << 8) + E;
	s = (H << 8) + L;
	R -= 2;
	do {
		memwrt(d--, memrdr(s--));
		t += 21;
		R += 2;
	} while (--i);
	B = C = 0;
	D = d >> 8;
	E = d;
	H = s >> 8;
	L = s;
	F &= ~(N_FLAG | P_FLAG | H_FLAG);
	return t + 16;
}
#else /* !FAST_BLOCK */
static int op_lddr(void)		/* LDDR */
{
	op_ldd();
	if (F & P_FLAG) {
		PC -= 2;
		return 21;
	}
	return 16;
}
#endif /* !FAST_BLOCK */

static int op_cpi(void)			/* CPI */
{
	register BYTE i;

	i = memrdr(((H << 8) + L));
	((i & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
	i = A - i;
	L++;
	if (!L)
		H++;
	C--;
	if (C == 0xff)
		B--;
	F |= N_FLAG;
	(B | C) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	return 16;
}

#ifdef FAST_BLOCK
static int op_cpir(void)		/* CPIR */
{
	register int t = -21;
	register WORD s;
	register BYTE d;
	register WORD i;
	register BYTE tmp;

	i = (B << 8) + C;
	s = (H << 8) + L;
	R -= 2;
	do {
		tmp = memrdr(s++);
		((tmp & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
		d = A - tmp;
		t += 21;
		R += 2;
	} while (--i && d);
	F |= N_FLAG;
	B = i >> 8;
	C = i;
	H = s >> 8;
	L = s;
	(i) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(d) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(d & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	return t + 16;
}
#else /* !FAST_BLOCK */
static int op_cpir(void)		/* CPIR */
{
	op_cpi();
	if ((F & (P_FLAG | Z_FLAG)) == P_FLAG) {
		PC -= 2;
		return 21;
	}
	return 16;
}
#endif /* !FAST_BLOCK */

static int op_cpdop(void)		/* CPD */
{
	register BYTE i;

	i = memrdr(((H << 8) + L));
	((i & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
	i = A - i;
	L--;
	if (L == 0xff)
		H--;
	C--;
	if (C == 0xff)
		B--;
	F |= N_FLAG;
	(B | C) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	return 16;
}

#ifdef FAST_BLOCK
static int op_cpdr(void)		/* CPDR */
{
	register int t = -21;
	register WORD s;
	register BYTE d;
	register WORD i;
	register BYTE tmp;

	i = (B << 8) + C;
	s = (H << 8) + L;
	R -= 2;
	do {
		tmp = memrdr(s--);
		((tmp & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
		d = A - tmp;
		t += 21;
		R += 2;
	} while (--i && d);
	F |= N_FLAG;
	B = i >> 8;
	C = i;
	H = s >> 8;
	L = s;
	(i) ? (F |= P_FLAG) : (F &= ~P_FLAG);
	(d) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(d & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	return t + 16;
}
#else /* !FAST_BLOCK */
static int op_cpdr(void)		/* CPDR */
{
	op_cpdop();
	if ((F & (P_FLAG | Z_FLAG)) == P_FLAG) {
		PC -= 2;
		return 21;
	}
	return 16;
}
#endif /* !FAST_BLOCK */

static int op_oprld(void)		/* RLD (HL) */
{
	register BYTE i, j;

	i = memrdr((H << 8) + L);
	j = A & 0x0f;
	A = (A & 0xf0) | (i >> 4);
	i = (i << 4) | j;
	memwrt((H << 8) + L, i);
	F &= ~(H_FLAG | N_FLAG);
	(A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	(parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	return 18;
}

static int op_oprrd(void)		/* RRD (HL) */
{
	register BYTE i, j;

	i = memrdr((H << 8) + L);
	j = A & 0x0f;
	A = (A & 0xf0) | (i & 0x0f);
	i = (i >> 4) | (j << 4);
	memwrt((H << 8) + L, i);
	F &= ~(H_FLAG | N_FLAG);
	(A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	(parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	return 18;
}

/**********************************************************************/
/**********************************************************************/
/*********       UNDOCUMENTED Z80 INSTRUCTIONS, BEWARE!      **********/
/**********************************************************************/
/**********************************************************************/

#ifdef UNDOC_INST

static int op_undoc_outc0(void)		/* OUT (C),0 */
{
	if (u_flag)
		return trap_ed();

	io_out(C, B, 0); /* NMOS, CMOS outputs 0xff */
	return 12;
}

static int op_undoc_infic(void)		/* IN F,(C) */
{
	BYTE tmp;

	if (u_flag)
		return trap_ed();

	tmp = io_in(C, B);
	F &= ~(N_FLAG | H_FLAG);
	(tmp) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
	(tmp & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
	(parity[tmp]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
	return 12;
}

#endif /* UNDOC_INST */

#endif /* !EXCLUDE_Z80 && !ALT_Z80 */
