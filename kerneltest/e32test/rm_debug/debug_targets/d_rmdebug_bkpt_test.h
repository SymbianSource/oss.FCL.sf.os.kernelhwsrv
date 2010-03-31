// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
//

// definitions of functions in d_rmdebug_bkpt_test.s

#ifndef D_RMDEBUG_BKPT_TESTS_H
#define D_RMDEBUG_BKPT_TESTS_H

extern "C"
{
	// Breakpoints in loop test
	unsigned int RMDebug_Bkpt_Test_Entry(void);
	unsigned int RMDebug_Bkpt_Test_Loop_Break_1(void);
	unsigned int RMDebug_Bkpt_Test_Loop_Break_2(void);
}
#endif // D_RMDEBUG_BKPT_TESTS_H
