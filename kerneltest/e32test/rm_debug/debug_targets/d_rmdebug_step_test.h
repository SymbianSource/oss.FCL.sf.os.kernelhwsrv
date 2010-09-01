// Copyright (c) 2007-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// definitions of functions in d_rmdebug_step_tests.s
//

#ifndef D_RMDEBUG_STEP_TESTS_H
#define D_RMDEBUG_STEP_TESTS_H

extern "C"
{
	// ARM tests
	unsigned int RMDebug_StepTest_Non_PC_Modifying(void);
	unsigned int RMDebug_StepTest_Non_PC_Modifying_OK(void);
	
	unsigned int RMDebug_StepTest_Branch(void);
	unsigned int RMDebug_StepTest_Branch_1(void);

	unsigned int RMDebug_StepTest_Branch_And_Link(void);
	unsigned int RMDebug_StepTest_Branch_And_Link_1(void);
	unsigned int RMDebug_StepTest_Branch_And_Link_2(void);
	
	unsigned int RMDebug_StepTest_MOV_PC(void);
	unsigned int RMDebug_StepTest_MOV_PC_1(void);
	unsigned int RMDebug_StepTest_MOV_PC_2(void);
	
	unsigned int RMDebug_StepTest_LDR_PC(void);
	unsigned int RMDebug_StepTest_LDR_PC_1(void);

	// Thumb tests
	unsigned int RMDebug_StepTest_Thumb_Non_PC_Modifying(void);
	unsigned int RMDebug_StepTest_Thumb_Non_PC_Modifying_1(void);
	unsigned int RMDebug_StepTest_Thumb_Non_PC_Modifying_2(void);

	unsigned int RMDebug_StepTest_Thumb_Branch(void);
	unsigned int RMDebug_StepTest_Thumb_Branch_1(void);
	unsigned int RMDebug_StepTest_Thumb_Branch_2(void);
	
	unsigned int RMDebug_StepTest_Thumb_Branch_And_Link(void);
	unsigned int RMDebug_StepTest_Thumb_Branch_And_Link_1(void);
	unsigned int RMDebug_StepTest_Thumb_Branch_And_Link_2(void);
	unsigned int RMDebug_StepTest_Thumb_Branch_And_Link_3(void);

	unsigned int RMDebug_StepTest_Thumb_Back_Branch_And_Link(void);
	unsigned int RMDebug_StepTest_Thumb_Back_Branch_And_Link_1(void);
	unsigned int RMDebug_StepTest_Thumb_Back_Branch_And_Link_2(void);
	unsigned int RMDebug_StepTest_Thumb_Back_Branch_And_Link_3(void);

	unsigned int RMDebug_StepTest_Thumb_AddPC(void);
	unsigned int RMDebug_StepTest_Thumb_AddPC_1(void);
	unsigned int RMDebug_StepTest_Thumb_AddPC_2(void);
	unsigned int RMDebug_StepTest_Thumb_AddPC_3(void);

	// ARM<->Thumb interworking tests
	unsigned int RMDebug_StepTest_Interwork(void);
	unsigned int RMDebug_StepTest_Interwork_1(void);
	unsigned int RMDebug_StepTest_Interwork_2(void);
	unsigned int RMDebug_StepTest_Interwork_3(void);

	// Stepping performance test
	unsigned int RMDebug_StepTest_Count(void);
	unsigned int RMDebug_StepTest_Count_1(void);
	unsigned int RMDebug_StepTest_Count_2(void);

	// Multiple step test
	unsigned int RMDebug_StepTest_ARM_Step_Multiple(void);
	unsigned int RMDebug_StepTest_ARM_Step_Multiple_1(void);

}
#endif // D_RMDEBUG_STEP_TESTS_H
