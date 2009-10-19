// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Bootstrap Shadow Memory Region Test Driver User I/F
//

#ifndef D_SMR_H
#define D_SMR_H

#include <e32cmn.h>
#include <e32ver.h>

#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif


/**
Interface to the fast-trace memory buffer.
*/
class RSMRTest : public RBusLogicalChannel
	{
public:
		
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{
		return DoCreate(Name(),TVersion(0,1,1),KNullUnit,NULL,NULL,EOwnerThread);
		}
		
	/* SMR Feature Testing	
	 */	
	 
	inline TUint Test_CheckSMRIBPtr(TBool aEnforce)
		{
		return DoControl(ECtrlCheckSMRIBPtr, (TAny*)aEnforce);
		}
	inline TUint Test_PrintSMRIB(TBool aEnforce)
		{
		return DoControl(ECtrlPrintSMRIB, (TAny*)aEnforce);
		}
	inline TUint Test_AccessAllSMRs(TBool aEnforce)
		{
		return DoControl(ECtrlAccessAllSMRs, (TAny*)aEnforce);
		}
	inline TUint Test_FreeHalfSMR1PhysicalRam(TBool aEnforce)
		{
		return DoControl(ECtrlFreeHalfSMR1PhysicalRam, (TAny*)aEnforce);
		}
	inline TUint Test_FreeAllSMR2PhysicalRam(TBool aEnforce)
		{
		return DoControl(ECtrlFreeAllSMR2PhysicalRam, (TAny*)aEnforce);
		}

#endif

	inline static const TDesC& Name();

private:
	enum TControl
		{
		ECtrlUndefined = 0,
		
		ECtrlCheckSMRIBPtr,
		ECtrlPrintSMRIB,
		ECtrlAccessAllSMRs,
		ECtrlFreeHalfSMR1PhysicalRam,
		ECtrlFreeAllSMR2PhysicalRam
		
		};
		
	friend class DSMRTestChannel;
	friend class DSMRTestFactory;
	};

inline const TDesC& RSMRTest::Name()
	{
	_LIT(KTestDriver,"d_smr");
	return KTestDriver;
	}



#endif // D_SMR_H
