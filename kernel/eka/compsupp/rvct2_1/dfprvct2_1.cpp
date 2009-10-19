// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This function is simple a way to get these EXPORT statements into
// the .in file. These symbols will therefore be referenced from
// the export table and so **forced** into the DLL
// 
//

__asm void __rt_exporter_dummy(void)
{
	AREA |.directive|, READONLY, NOALLOC

	PRESERVE8

	DCB "#<SYMEDIT>#\n"
	// From %ARMLIB%\armlib\f_a_p.l

	DCB "EXPORT _dfix_r\n"
	DCB "EXPORT _dfixu_r\n"
	DCB "EXPORT _ffix_r\n"
	DCB "EXPORT _ffixu_r\n"
	DCB "EXPORT _ll_sfrom_d_r\n"
	DCB "EXPORT _ll_ufrom_d_r\n"
	DCB "EXPORT _ll_sfrom_f_r\n"
	DCB "EXPORT _ll_ufrom_f_r\n"

	DCB "EXPORT _dcmpge\n"
	DCB "EXPORT _fcmpge\n"

	DCB "EXPORT _dneq\n"		// dcmp.o
	DCB "EXPORT _fneq\n"	 	// fcmp.o

	DCB "EXPORT _drem\n"	 	// drem.o
	DCB "EXPORT _drnd\n"	 	// drnd.o
	DCB "EXPORT _frem\n"	 	// frem.o
	DCB "EXPORT _frnd\n"	 	// frnd.o

	DCB "EXPORT _dabs\n"		// basic.o 
	DCB "EXPORT _fabs\n"		// basic.o 


	DCB "EXPORT __dcmp4\n"		// dcmp.o
	DCB "EXPORT _dcmp4\n"		// dcmp.o
	DCB "EXPORT __fcmp4\n"		// fcmp4.o
	DCB "EXPORT _fcmp4\n"	 	// fcmp4.o

	DCB "EXPORT _drdiv\n"		// ddiv.o
	DCB "EXPORT _frdiv\n"	 	// fdiv.o

	DCB "EXPORT _dfltu\n"		// dflt.o
	DCB "EXPORT _dflt\n"		// dflt.o
	DCB "EXPORT _ffltu\n"	 	// fflt.o
	DCB "EXPORT _fflt\n"	 	// fflt.o
	
	DCB "EXPORT _dsqrt\n"	 	// dsqrt.o
	DCB "EXPORT _fsqrt\n"	 	// fsqrt.o

}
