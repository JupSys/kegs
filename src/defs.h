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

#ifdef INCLUDE_RCSID_S
	.data
	.export rcsdif_defs_h,data
rcsdif_defs_h
	.stringz "@(#)$Header: defs.h,v 1.19 99/09/13 21:39:42 kentd Exp $"
	.code
#endif

#include "defcomm.h"

link		.reg	%r2
acc		.reg	%r3
xreg		.reg	%r4
yreg		.reg	%r5
stack		.reg	%r6
dbank		.reg	%r7
direct		.reg	%r8
neg		.reg	%r9
zero		.reg	%r10
psr		.reg	%r11
kpc		.reg	%r12
const_fd	.reg	%r13
instr		.reg	%r14
#if 0
cycles		.reg	%r13
kbank		.reg	%r14
#endif

page_info_ptr	.reg	%r15
inst_tab_ptr	.reg	%r16
fcycles_stop_ptr .reg	%r17
addr_latch	.reg	%r18

scratch1	.reg	%r19
scratch2	.reg	%r20
scratch3	.reg	%r21
scratch4	.reg	%r22
;instr		.reg	%r23		; arg3

fcycles		.reg	%fr12
fr_plus_1	.reg	%fr13
fr_plus_2	.reg	%fr14
fr_plus_3	.reg	%fr15
fr_plus_x_m1	.reg	%fr16
fcycles_stop	.reg	%fr17

ftmp1		.reg	%fr4
ftmp2		.reg	%fr5
fscr1		.reg	%fr6

#define LDC(val,reg) ldil L%val,reg ! ldo R%val(reg),reg


