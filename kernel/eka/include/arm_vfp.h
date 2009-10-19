// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// e32\include\arm_vfp.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __ARM_VFP_H__
#define __ARM_VFP_H__
#include <cpudefs.h>
#if defined(__CPU_ARM) && defined(__CPU_HAS_VFP)

// MRCcccc Ppppp, iii, Rdddd, Cnnnn, Cmmmm, ttt
//	cccc 1110 iii1 nnnn dddd pppp ttt1 mmmm
// MCRcccc Ppppp, iii, Rdddd, Cnnnn, Cmmmm, ttt
//	cccc 1110 iii0 nnnn dddd pppp ttt1 mmmm
// CDPcccc pppp, iiii, Cdddd, Cnnnn, Cmmmm, ttt
//	cccc 1110 iiii nnnn dddd pppp ttt0 mmmm
// CDP2    pppp, iiii, Cdddd, Cnnnn, Cmmmm, ttt
//	1111 1110 iiii nnnn dddd pppp ttt0 mmmm
// LDCcccc
//	cccc 110P UNW1 nnnn dddd pppp oooo oooo
// STCcccc
//	cccc 110P UNW0 nnnn dddd pppp oooo oooo
#define _MRC(cc,p,i,r,c,c2,t)		asm("mrc"#cc" p"#p", "#i", r"#r", c"#c", c"#c2", "#t )
#define _MCR(cc,p,i,r,c,c2,t)		asm("mcr"#cc" p"#p", "#i", r"#r", c"#c", c"#c2", "#t )
#define _CDP(cc,p,i,d,n,m,t)		asm("cdp"#cc" p"#p", "#i", c"#d", c"#n", c"#m", "#t )

#define _MRC(cc,p,i,r,c,c2,t)		asm("mrc"#cc" p"#p", "#i", r"#r", c"#c", c"#c2", "#t )
#define _MCR(cc,p,i,r,c,c2,t)		asm("mcr"#cc" p"#p", "#i", r"#r", c"#c", c"#c2", "#t )
#define _CDPS(cc,p,i,d,n,m,t)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((p)<<8)|((i)<<20)|((t)<<5)|		\
																	(((d)>>1)<<12)|(((n)>>1)<<16)|((m)>>1)|		\
																	(((d)&1)<<22)|(((n)&1)<<7)|(((m)&1)<<5)|	\
																	0x0e000000									))


#define VFP_CPID_S				10		// coprocessor ID for single precision
#define VFP_CPID_D				11		// coprocessor ID for double precision

#define	VFP_XREG_FPSID			0
#define	VFP_XREG_FPSCR			1
#define	VFP_XREG_MVFR0			7
#define	VFP_XREG_FPEXC			8
#define	VFP_XREG_FPINST			9
#define	VFP_XREG_FPINST2		10

#define VFP_FPSID_IMP_SHIFT		24
#define VFP_FPSID_IMP_MASK		(255u<<VFP_FPSID_IMP_SHIFT)
#define	VFP_FPSID_SW			0x00800000		// software emulation
#define	VFP_FPSID_FMT_SHIFT		21
#define	VFP_FPSID_FMT_MASK		(3<<VFP_FPSID_FMT_SHIFT)
#define	VFP_FPSID_FMT1			0x00000000		// FLDMX/FSTMX format 1
#define	VFP_FPSID_FMT2			0x00200000		// FLDMX/FSTMX format 2
#define	VFP_FPSID_SNG			0x00100000		// single precision only
#define VFP_FPSID_ARCH_SHIFT	16
#define VFP_FPSID_ARCH_MASK		(15<<VFP_FPSID_ARCH_SHIFT)
#define VFP_FPSID_PART_SHIFT	8
#define VFP_FPSID_PART_MASK		(255<<VFP_FPSID_PART_SHIFT)
#define VFP_FPSID_VAR_SHIFT		4
#define VFP_FPSID_VAR_MASK		(15<<VFP_FPSID_VAR_SHIFT)
#define VFP_FPSID_REV_MASK		15


#define	VFP_FPSCR_N				0x80000000		// less than
#define	VFP_FPSCR_Z				0x40000000		// equal
#define	VFP_FPSCR_C				0x20000000		// equal greater or unordered
#define	VFP_FPSCR_V				0x10000000		// unordered
#define	VFP_FPSCR_DN			0x02000000		// enable default NAN mode
#define	VFP_FPSCR_FZ			0x01000000		// enable flush to zero mode
#define VFP_FPSCR_RMODE_SHIFT	22
#define VFP_FPSCR_RMODE_MASK	(3<<VFP_FPSCR_RMODE_SHIFT)
#define VFP_FPSCR_RMODE_NEAR	0x00000000		// round to nearest
#define VFP_FPSCR_RMODE_PLUS	0x00400000		// round up
#define VFP_FPSCR_RMODE_MINUS	0x00800000		// round down
#define VFP_FPSCR_RMODE_ZERO	0x00C00000		// round towards zero

#define VFP_FPSCR_STRIDE_SHIFT	20
#define VFP_FPSCR_STRIDE_MASK	(3<<VFP_FPSCR_STRIDE_SHIFT)
#define VFP_FPSCR_LEN_SHIFT		16
#define VFP_FPSCR_LEN_MASK		(7<<VFP_FPSCR_LEN_SHIFT)
#define	VFP_FPSCR_IDE			0x00008000		// enable input subnormal exception
#define	VFP_FPSCR_IXE			0x00001000		// enable inexact exception
#define	VFP_FPSCR_UFE			0x00000800		// enable underflow exception
#define	VFP_FPSCR_OFE			0x00000400		// enable overflow exception
#define	VFP_FPSCR_DZE			0x00000200		// enable division by zero exception
#define	VFP_FPSCR_IOE			0x00000100		// enable invalid operation exception
#define	VFP_FPSCR_IDC			0x00000080		// input subnormal cumulative flag
#define	VFP_FPSCR_IXC			0x00000010		// inexact cumulative flag
#define	VFP_FPSCR_UFC			0x00000008		// underflow cumulative flag
#define	VFP_FPSCR_OFC			0x00000004		// overflow cumulative flag
#define	VFP_FPSCR_DZC			0x00000002		// division by zero cumulative flag
#define	VFP_FPSCR_IOC			0x00000001		// invalid operation cumulative flag

#define VFP_FPSCR_RUNFAST		(VFP_FPSCR_DN|VFP_FPSCR_FZ)
#define VFP_FPSCR_IEEE_NO_EXC	0
#define VFP_FPSCR_EXCEPTIONS	(VFP_FPSCR_IDE|VFP_FPSCR_IXE|VFP_FPSCR_UFE|VFP_FPSCR_OFE|VFP_FPSCR_DZE|VFP_FPSCR_IOE)
#define VFP_FPSCR_MODE_MASK		(VFP_FPSCR_EXCEPTIONS|VFP_FPSCR_RUNFAST|VFP_FPSCR_RMODE_MASK)

#define VFP_FPEXC_EX			0x80000000		// exceptional state
#define VFP_FPEXC_EN			0x40000000		// enable VFP
#define VFP_FPEXC_FP2V			0x10000000		// FPINST2 register valid
#define VFP_FPEXC_VECITR_SHIFT	8
#define VFP_FPEXC_VECITR_MASK	(7<<VFP_FPEXC_VECITR_SHIFT)	// (remaining iterations - 1) mod 7
#define VFP_FPEXC_INV			0x00000080		// input exception flag (subnormal or NaN)
#define VFP_FPEXC_UFC			0x00000008		// underflow cumulative flag
#define VFP_FPEXC_OFC			0x00000004		// overflow cumulative flag
#define VFP_FPEXC_IOC			0x00000001		// invalid operation cumulative flag

#define VFP_FPEXC_INIT			(VFP_FPEXC_EN|VFP_FPEXC_VECITR_MASK)
#define VFP_FPEXC_THRD_INIT		(VFP_FPEXC_VECITR_MASK)

#define	VFP_MVFR0_ASIMD32		0x00000002		// Full 32 x 64-bit registers are supported for Advanced SIMD
#define	VFP_CPACR_ASEDIS		0x80000000		// Access to the NEON unit is disabled
#define	VFP_CPACR_D32DIS		0x40000000		// Access to the upper 16 64-bit registers is disabled

#define VFP_FMRX(cc,Rd,reg)			_MRC(cc,10,7,Rd,reg,0,0)
#define VFP_FMXR(cc,reg,Rd)			_MCR(cc,10,7,Rd,reg,0,0)

// VFPv3 adds D16-D31 extra double precision registers in previously UNDEF opcodes
#define _VFP_DN(Dn)	(((Dn)>=16) ? ((Dn)-16) : (Dn))
#define _VFP_U(Dn) 	(((Dn)>=16) ? 1 : 0)
#define _VFP_D(Dn)	(_VFP_U(Dn)<<22)
#define _VFP_N(Dn)	(_VFP_U(Dn)<<7)

#define VFP_FLDMIAX(cc,Rn,Dd,N)		asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N)+1)|_VFP_D(Dd)|0x0c900b00 )) )
#define VFP_FSTMIAX(cc,Rn,Dd,N)		asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N)+1)|_VFP_D(Dd)|0x0c800b00 )) )
#define VFP_FLDMIAXW(cc,Rn,Dd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N)+1)|_VFP_D(Dd)|0x0cb00b00 )) )
#define VFP_FSTMIAXW(cc,Rn,Dd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N)+1)|_VFP_D(Dd)|0x0ca00b00 )) )
#define VFP_FLDMDBXW(cc,Rn,Dd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N)+1)|_VFP_D(Dd)|0x0d300b00 )) )
#define VFP_FSTMDBXW(cc,Rn,Dd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N)+1)|_VFP_D(Dd)|0x0d200b00 )) )

#define VFP_FMDLR(cc,Dn,Rd)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rd)<<12)|(_VFP_DN(Dn)<<16)|_VFP_N(Dn)|0x0e000b10 )) )
#define VFP_FMDHR(cc,Dn,Rd)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rd)<<12)|(_VFP_DN(Dn)<<16)|_VFP_N(Dn)|0x0e200b10 )) )
#define VFP_FMRDL(cc,Rd,Dn)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rd)<<12)|(_VFP_DN(Dn)<<16)|_VFP_N(Dn)|0x0e100b10 )) )
#define VFP_FMRDH(cc,Rd,Dn)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rd)<<12)|(_VFP_DN(Dn)<<16)|_VFP_N(Dn)|0x0e300b10 )) )


#ifdef __CPU_ARM_HAS_MCRR
#define VFP_FMRRD(cc,Rd,Rn,Dm)		MRRCcc(cc,11,1,Rd,Rn,Dm)
#define VFP_FMDRR(cc,Dm,Rd,Rn)		MCRRcc(cc,11,1,Rd,Rn,Dm)
#endif

// MRCcccc Ppppp, iii, Rdddd, Cnnnn, Cmmmm, ttt
//	cccc 1110 iii1 nnnn dddd pppp ttt1 mmmm
// MCRcccc Ppppp, iii, Rdddd, Cnnnn, Cmmmm, ttt
//	cccc 1110 iii0 nnnn dddd pppp ttt1 mmmm
// CDPcccc pppp, iiii, Cdddd, Cnnnn, Cmmmm, ttt
//	cccc 1110 iiii nnnn dddd pppp ttt0 mmmm
#define VFP_FABSD(cc,Dd,Dm)			_CDP(cc,11,11,Dd,0,Dm,6)
#define VFP_FADDD(cc,Dd,Dn,Dm)		_CDP(cc,11,3,Dd,Dn,Dm,0)
#define VFP_FCMPD(cc,Dd,Dm)			_CDP(cc,11,11,Dd,4,Dm,2)
#define VFP_FCMPED(cc,Dd,Dm)		_CDP(cc,11,11,Dd,4,Dm,6)
#define VFP_FCMPEZD(cc,Dd)			_CDP(cc,11,11,Dd,5,0,6)
#define VFP_FCMPZD(cc,Dd)			_CDP(cc,11,11,Dd,5,0,2)
#define VFP_FCPYD(cc,Dd,Dm)			_CDP(cc,11,11,Dd,0,Dm,2)
#define VFP_FDIVD(cc,Dd,Dn,Dm)		_CDP(cc,11,8,Dd,Dn,Dm,0)
#define VFP_FMACD(cc,Dd,Dn,Dm)		_CDP(cc,11,0,Dd,Dn,Dm,0)
#define VFP_FMSCD(cc,Dd,Dn,Dm)		_CDP(cc,11,1,Dd,Dn,Dm,0)
#define VFP_FMULD(cc,Dd,Dn,Dm)		_CDP(cc,11,2,Dd,Dn,Dm,0)
#define VFP_FNEGD(cc,Dd,Dm)			_CDP(cc,11,11,Dd,1,Dm,2)
#define VFP_FNMACD(cc,Dd,Dn,Dm)		_CDP(cc,11,0,Dd,Dn,Dm,2)
#define VFP_FNMSCD(cc,Dd,Dn,Dm)		_CDP(cc,11,1,Dd,Dn,Dm,2)
#define VFP_FNMULD(cc,Dd,Dn,Dm)		_CDP(cc,11,2,Dd,Dn,Dm,2)
#define VFP_FSQRTD(cc,Dd,Dm)		_CDP(cc,11,11,Dd,1,Dm,6)
#define VFP_FSUBD(cc,Dd,Dn,Dm)		_CDP(cc,11,3,Dd,Dn,Dm,2)
#define VFP_FMSTAT(cc)				_MRC(cc,10,7,15,1,0,0)

#define _VFP_ADDR_U(off)	((off)>=0 ? 1 : 0)
#define _VFP_ADDR_O(off)	((off)>=0 ? (off) : -(off))
#define VFP_FLDD(cc,Dd,Rn,off)		asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|(_VFP_D(Dd))|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(_VFP_ADDR_U(off)<<23)|_VFP_ADDR_O(off)|0x0d100b00)))
#define VFP_FSTD(cc,Dd,Rn,off)		asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|(_VFP_D(Dd))|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(_VFP_ADDR_U(off)<<23)|_VFP_ADDR_O(off)|0x0d000b00)))
#define VFP_FLDMIAD(cc,Rn,Dd,N)		asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|(_VFP_D(Dd))|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N))|0x0c900b00 )) )
#define VFP_FSTMIAD(cc,Rn,Dd,N)		asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|(_VFP_D(Dd))|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N))|0x0c800b00 )) )
#define VFP_FLDMIADW(cc,Rn,Dd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|(_VFP_D(Dd))|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N))|0x0cb00b00 )) )
#define VFP_FSTMIADW(cc,Rn,Dd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|(_VFP_D(Dd))|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N))|0x0ca00b00 )) )
#define VFP_FLDMDBDW(cc,Rn,Dd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|(_VFP_D(Dd))|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N))|0x0d300b00 )) )
#define VFP_FSTMDBDW(cc,Rn,Dd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|(_VFP_D(Dd))|((Rn)<<16)|(_VFP_DN(Dd)<<12)|(2*(N))|0x0d200b00 )) )

#define VFP_FABSS(cc,Sd,Sm)			_CDPS(cc,10,11,Sd,0,Sm,6)
#define VFP_FADDS(cc,Sd,Sn,Sm)		_CDPS(cc,10,3,Sd,Sn,Sm,0)
#define VFP_FCMPS(cc,Sd,Sm)			_CDPS(cc,10,11,Sd,8,Sm,2)
#define VFP_FCMPES(cc,Sd,Sm)		_CDPS(cc,10,11,Sd,8,Sm,6)
#define VFP_FCMPEZS(cc,Sd)			_CDPS(cc,10,11,Sd,11,0,6)
#define VFP_FCMPZS(cc,Sd)			_CDPS(cc,10,11,Sd,10,0,2)
#define VFP_FCPYS(cc,Sd,Sm)			_CDPS(cc,10,11,Sd,0,Sm,2)
#define VFP_FDIVS(cc,Sd,Sn,Sm)		_CDPS(cc,10,8,Sd,Sn,Sm,0)
#define VFP_FMACS(cc,Sd,Sn,Sm)		_CDPS(cc,10,0,Sd,Sn,Sm,0)
#define VFP_FMSCS(cc,Sd,Sn,Sm)		_CDPS(cc,10,1,Sd,Sn,Sm,0)
#define VFP_FMULS(cc,Sd,Sn,Sm)		_CDPS(cc,10,2,Sd,Sn,Sm,0)
#define VFP_FNEGS(cc,Sd,Sm)			_CDPS(cc,10,11,Sd,2,Sm,2)
#define VFP_FNMACS(cc,Sd,Sn,Sm)		_CDPS(cc,10,0,Sd,Sn,Sm,2)
#define VFP_FNMSCS(cc,Sd,Sn,Sm)		_CDPS(cc,10,1,Sd,Sn,Sm,2)
#define VFP_FNMULS(cc,Sd,Sn,Sm)		_CDPS(cc,10,2,Sd,Sn,Sm,2)
#define VFP_FSQRTS(cc,Sd,Sm)		_CDPS(cc,10,11,Sd,3,Sm,6)
#define VFP_FSUBS(cc,Sd,Sn,Sm)		_CDPS(cc,10,3,Sd,Sn,Sm,2)

#define VFP_FLDS(cc,Sd,Rn,off)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|((Rn)<<16)|(_VFP_ADDR_U(off)<<23)|_VFP_ADDR_O(off)|0x0d100a00))
#define VFP_FSTS(cc,Sd,Rn,off)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|((Rn)<<16)|(_VFP_ADDR_U(off)<<23)|_VFP_ADDR_O(off)|0x0d000a00))
#define VFP_FLDMIAS(cc,Rn,Sd,N)		asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(N)|0x0c900a00 )) )
#define VFP_FSTMIAS(cc,Rn,Sd,N)		asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(N)|0x0c800a00 )) )
#define VFP_FLDMIASW(cc,Rn,Sd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(N)|0x0cb00a00 )) )
#define VFP_FSTMIASW(cc,Rn,Sd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(N)|0x0ca00a00 )) )
#define VFP_FLDMDBSW(cc,Rn,Sd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(N)|0x0d300a00 )) )
#define VFP_FSTMDBSW(cc,Rn,Sd,N)	asm(".word %a0" : : "i" ((TInt)( ((cc)<<28)|((Rn)<<16)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(N)|0x0d200a00 )) )

#define VFP_FMSR(cc,Sn,Rd)			asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|(((Sn)>>1)<<16)|(((Sn)&1)<<7)|((Rd)<<12)|0x0e000a10))
#define VFP_FMRS(cc,Rd,Sn)			asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|(((Sn)>>1)<<16)|(((Sn)&1)<<7)|((Rd)<<12)|0x0e100a10))

#define	VFP_FCVTDS(cc,Dd,Sm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((Sm)>>1)|(((Sm)&1)<<5)|((Dd)<<12)|0x0eb70ac0))
#define	VFP_FCVTSD(cc,Sd,Dm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(Dm)|0x0eb70bc0))

#define VFP_FSITOD(cc,Dd,Sm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((Sm)>>1)|(((Sm)&1)<<5)|((Dd)<<12)|0x0eb80bc0))
#define VFP_FSITOS(cc,Sd,Sm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((Sm)>>1)|(((Sm)&1)<<5)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|0x0eb80ac0))
#define VFP_FTOSID(cc,Sd,Dm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(Dm)|0x0ebd0b40))
#define VFP_FTOSIZD(cc,Sd,Dm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(Dm)|0x0ebd0bc0))
#define VFP_FTOSIS(cc,Sd,Sm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((Sm)>>1)|(((Sm)&1)<<5)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|0x0ebd0a40))
#define VFP_FTOSIZS(cc,Sd,Sm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((Sm)>>1)|(((Sm)&1)<<5)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|0x0ebd0ac0))
#define VFP_FUITOD(cc,Dd,Sm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((Sm)>>1)|(((Sm)&1)<<5)|((Dd)<<12)|0x0eb80b40))
#define VFP_FUITOS(cc,Sd,Sm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((Sm)>>1)|(((Sm)&1)<<5)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|0x0eb80a40))
#define VFP_FTOUID(cc,Sd,Dm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(Dm)|0x0ebc0b40))
#define VFP_FTOUIZD(cc,Sd,Dm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|(Dm)|0x0ebc0bc0))
#define VFP_FTOUIS(cc,Sd,Sm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((Sm)>>1)|(((Sm)&1)<<5)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|0x0ebc0a40))
#define VFP_FTOUIZS(cc,Sd,Sm)		asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((Sm)>>1)|(((Sm)&1)<<5)|(((Sd)>>1)<<12)|(((Sd)&1)<<22)|0x0ebc0ac0))

// VFPv3 
// conversion between floating point and fixed point
#define _VFP_VCVT_D(dp,d) ( (dp) ?  ( (((d)>>4)<<22)|(((d)&0xf)<<12) )  : ( (((d)&1)<<22)|(((d)>>1)<<12) ) )
#define _VFP_I_IMM4(sx,fbits) (((sx)==0 ? 16 : 32) - (fbits))
#define _VFP_VCVT_FBITS(sx,fbits) ( ((_VFP_I_IMM4(sx,fbits)&1) <<5) | (_VFP_I_IMM4(sx,fbits)>>1) )
#define VFP_VCT(cc,op,sf,U,sx,d,fbits) asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|((op)<<18)|((U)<<16)|((sf)<<8)|((sx)<<7)|_VFP_VCVT_D(sf,d)|_VFP_VCVT_FBITS(sx,fbits)|0x0eba0a40))
// from fixed to floating point 
// S32=>F64
#define VFP_VCT_F64_S32(cc,Dd,fbits) VFP_VCT((cc),0,1,0,1,(Dd),(fbits))
// S32=>F32
#define VFP_VCT_F32_S32(cc,Sd,fbits) VFP_VCT((cc),0,0,0,1,(Sd),(fbits))
// from floating point to fix
// F64=>S32 
#define VFP_VCT_S32_F64(cc,Dd,fbits) VFP_VCT((cc),1,1,0,1,(Dd),(fbits))
// F32=>S32
#define VFP_VCT_S32_F32(cc,Sd,fbits) VFP_VCT((cc),1,0,0,1,(Sd),(fbits))


// put immediate value to the register
// single_register=(sz==0) 
// imm (abcdefgh) 
/*
   bcd 000		001		010		011		100			101			110			111 
efgh                   
0000   2.0		4.0		8.0		16.0	0.125		0.25		0.5			1.0 
0001   2.125	4.25	8.5		17.0	0.1328125	0.265625	0.53125		1.0625 
0010   2.25		4.5		9.0		18.0	0.140625	0.28125		0.5625		1.125 
0011   2.375	4.75	9.5		19.0	0.1484375	0.296875	0.59375		1.1875 
0100   2.5		5.0		10.0	20.0	0.15625		0.3125		0.625		1.25 
0101   2.625	5.25	10.5	21.0	0.1640625	0.328125	0.65625		1.3125 
0110   2.75		5.5		11.0	22.0	0.171875	0.34375		0.6875		1.375 
0111   2.875	5.75	11.5	23.0	0.1796875	0.359375	0.71875		1.4375 
1000   3.0		6.0		12.0	24.0	0.1875		0.375		0.75		1.5 
1001   3.125	6.25	12.5	25.0	0.1953125	0.390625	0.78125		1.5625 
1010   3.25		6.5		13.0	26.0	0.203125	0.40625		0.8125		1.625 
1011   3.375	6.75	13.5	27.0	0.2109375	0.421875	0.84375		1.6875 
1100   3.5		7.0		14.0	28.0	0.21875		0.4375		0.875		1.75 
1101   3.625	7.25	14.5	29.0	0.2265625	0.453125	0.90625		1.8125 
1110   3.75		7.5		15.0	30.0	0.234375	0.46875		0.9375		1.875 
1111   3.875	7.75	15.5	31.0	0.2421875	0.484375	0.96875		1.9375 
*/
                                                                  
#define VFP_VMOV_IMM(cc,sz,d,imm)	asm(".word %a0" : : "i" ((TInt) ((cc)<<28)|(((imm)>>4)<<16)|((imm)&0xf)|(sz)<<8|((sz)?((((d)>>4)<<22)|(((d)&0xf)<<12)):((((d)&1)<<22)|(((d)>>1)<<12)))|0x0eb00a00))

#endif
#endif
