// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// some utility classes for writing data to buffer
// 
//

/**
 @file
 @internalTechnology
*/
#ifndef __CRASH_LOG_WALKER_H_INCLUDED__
#define __CRASH_LOG_WALKER_H_INCLUDED__

#include <e32cmn.h> 

#include <scmdatatypes.h>

namespace Debug
	{
	
	/**
	 * This class provides functionality to walk through a crash log in a data buffer
	 * ensuring it is valid and getting the information we require back
	 */
	class TCrashLogWalker
		{
	public:
		TCrashLogWalker(TDesC8& aBuffer);
		
		TInt ReadLogHeader(const TInt aStartPoint);		
		TInt GetCrashSize() const;
		TInt GetCrashId() const;
		const TRmdArmExcInfo& GetCrashContext() const;		
		const TCrashInfoHeader& GetCrashHeader() const;
		const TCrashOffsetsHeader& GetOffsetsHeader() const;
		
#ifndef __KERNEL_MODE__		
		MByteStreamSerializable*  GetNextDataTypeL(TInt& aPos, SCMStructId& aId, TInt& aBufferSize);
		TRawData* GetRawDataTypeL(TInt& aPos, TInt& aBufferSize, TDes8& aRawBuf, TInt aStartRawPosition = 0);
#endif
		
		void UpdateBuffer(TDesC8& aBuffer);
		
	private:
		
		void HelpAssignRegisterToContext(const TRegisterValue& aRegVal);
		
	private:
		TDesC8& iBuffer;					//buffer containing data for the log we are walking - not all of it, just the bit of interest
		TCrashInfoHeader iCrashHeader;		//Stores the header of the log we are walking
		TCrashOffsetsHeader iOffsets;		//Stores the offsets header of the log we are walking
		TRmdArmExcInfo iContext;			//Stores the register context of the log we are walking
		
		TByteStreamReader iReader;
		
	private:
		TInt VerifyHeader();
		
		};
	
	}

#endif // __CRASH_LOG_WALKER_H_INCLUDED__
