// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Internal class used to assemble debug functionality data block
// 

#ifndef T_DEBUG_FUNCTIONALITY_H
#define T_DEBUG_FUNCTIONALITY_H

/**
 * This class is used to represent and assemble the debug functionality
 * block
 */
class TDebugFunctionality
	{

	public:
		TUint32 GetDebugFunctionalityBufSize(void);
		TBool GetDebugFunctionality(TDes8& aDFBlock);
		TUint32 GetStopModeFunctionalityBufSize(void);
		TBool GetStopModeFunctionality(TDes8& aDFBlock);
		static TInt GetRegister(const Debug::TRegisterInfo aRegisterInfo, Debug::TTag& aTag);
		static TUint32 GetMemoryOperationMaxBlockSize();

	private:

		// Helper functions when assembling the buffer
		void AppendBlock(const Debug::TSubBlock& aDFSubBlock, TDes8& aDFBlock);
		TUint32 ComputeBlockSize(const Debug::TSubBlock& aDFSubBlock);
};

#endif	// T_DEBUG_FUNCTIONALITY_H
