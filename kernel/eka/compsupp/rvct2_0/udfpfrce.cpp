// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of the License "ARM EABI LICENCE.txt"
// which accompanies this distribution, and is available
// in kernel/eka/compsupp.
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// This function is simple a way to for these EXPORT statements into
// the .in file. These symbols will therefore be referenced from
// the export table and so **forced** into the udfp.dll 
// 
//

__asm void __rt_exporter_dummy(void)
{
	AREA |.directive|, READONLY, NOALLOC

	PRESERVE8

	DCB "#<SYMEDIT>#\n"
	// From %ARMLIB%\armlib\f_a_p.l
	DCB "EXPORT _dneg\n"		// basic.o 
	DCB "EXPORT _fneg\n"		// basic.o 
	DCB "EXPORT _dabs\n"		// basic.o 
	DCB "EXPORT _fabs\n"		// basic.o 

	DCB "EXPORT _d2f\n"		// d2f.o 	

	DCB "EXPORT _dadd\n"		// daddsub.o
	DCB "EXPORT _drsb\n"		// daddsub.o
	DCB "EXPORT _dsub\n"		// daddsub.o

	DCB "EXPORT _deq\n"		// dcmp.o
	DCB "EXPORT _dneq\n"		// dcmp.o
	DCB "EXPORT _dgr\n"		// dcmp.o
	DCB "EXPORT _dgeq\n"		// dcmp.o
	DCB "EXPORT _dleq\n"		// dcmp.o
	DCB "EXPORT _dls\n"		// dcmp.o

	DCB "EXPORT __dcmp4\n"		// dcmp.o
	DCB "EXPORT _dcmp4\n"		// dcmp.o

	DCB "EXPORT _drdiv\n"		// ddiv.o
	DCB "EXPORT _ddiv\n"		// ddiv.o
//	DCB "EXPORT ddiv_mantissas\n"	// ddiv.o

	DCB "EXPORT _dcmpeq\n"		// deqf.o

	DCB "EXPORT _dfix\n"		// dfix.o
	DCB "EXPORT _dfix_r\n"		// dfix.o

	DCB "EXPORT _ll_sfrom_d\n"	// dfixll.o	
	DCB "EXPORT _ll_sfrom_d_r\n"	// dfixll.o

	DCB "EXPORT _dfixu\n"		// dfixu.o	
	DCB "EXPORT _dfixu_r\n"		// dfixu.o

	DCB "EXPORT _ll_ufrom_d\n"	// dfixull.o	
	DCB "EXPORT _ll_ufrom_d_r\n"	// dfixull.o

//	DCB "EXPORT _dflt_normalise\n"	// dflt.o
	DCB "EXPORT _dfltu\n"		// dflt.o
	DCB "EXPORT _dflt\n"		// dflt.o
	
	DCB "EXPORT _ll_uto_d\n"	// dfltll.o	
	DCB "EXPORT _ll_sto_d\n"	// dfltll.o

	DCB "EXPORT _dcmpge\n"		// dgeqf.o	

	DCB "EXPORT _dcmple\n"		// dleqf.o

	DCB "EXPORT _dmul\n"		// dmul.o
	
	DCB "EXPORT _drem\n"	 	// drem.o
	
	DCB "EXPORT _drnd\n"	 	// drnd.o
	
	DCB "EXPORT _dsqrt\n"	 	// dsqrt.o
	
	DCB "EXPORT _f2d\n"	 	// f2d.o
	
	DCB "EXPORT _fadd\n"	 	// faddsub.o
	DCB "EXPORT _frsb\n"	 	// faddsub.o
	DCB "EXPORT _fsub\n"	 	// faddsub.o
	
	DCB "EXPORT _feq\n"	 	// fcmp.o
	DCB "EXPORT _fneq\n"	 	// fcmp.o
	DCB "EXPORT _fgr\n"	 	// fcmp.o
	DCB "EXPORT _fgeq\n"	 	// fcmp.o
	DCB "EXPORT _fleq\n"	 	// fcmp.o
	DCB "EXPORT _fls\n"	 	// fcmp.o
	
	DCB "EXPORT _fcmp4\n"	 	// fcmp4.o
	
	DCB "EXPORT _frdiv\n"	 	// fdiv.o
	DCB "EXPORT _fdiv\n"	 	// fdiv.o
	
	DCB "EXPORT _fcmpeq\n"	 	// feqf.o
	
	DCB "EXPORT _ffix\n"	 	// ffix.o
	DCB "EXPORT _ffix_r\n"	 	// ffix.o
	
	DCB "EXPORT _ll_sfrom_f\n"	 // ffixll.o
	DCB "EXPORT _ll_sfrom_f_r\n"	 // ffixll.o
	
	DCB "EXPORT _ffixu\n"	 	// ffixu.o
	DCB "EXPORT _ffixu_r\n"	 	// ffixu.o
	
	DCB "EXPORT _ll_ufrom_f\n"	 // ffixull.o
	DCB "EXPORT _ll_ufrom_f_r\n"	 // ffixull.o
	
	DCB "EXPORT _ffltu\n"	 	// fflt.o
	DCB "EXPORT _fflt\n"	 	// fflt.o
	
	DCB "EXPORT _ll_uto_f\n"	 // ffltll.o
	DCB "EXPORT _ll_sto_f\n"	 // ffltll.o
	
	DCB "EXPORT _fcmpge\n"	 	// fgeqf.o
	
	DCB "EXPORT _fcmple\n"	 	// fleqf.o
	
	DCB "EXPORT _fmul\n"	 	// fmul.o
	
	DCB "EXPORT _frem\n"	 	// frem.o
	
	DCB "EXPORT _frnd\n"	 	// frnd.o
	
	DCB "EXPORT _fsqrt\n"	 	// fsqrt.o
		
}
