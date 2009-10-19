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
// Hardware Configuration Respoitory Tests
//

#ifndef D_HCR_H
#define D_HCR_H

#include <e32cmn.h>
#include <e32ver.h>
#include <drivers/hcr.h>

#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif


/**
Interface to the fast-trace memory buffer.
*/
class RHcrTest : public RBusLogicalChannel
	{
public:
		
#ifndef __KERNEL_MODE__
	inline TInt Open()
		{
		return DoCreate(Name(),TVersion(0,1,1),KNullUnit,NULL,NULL,EOwnerThread);
		}

	inline TUint Test_SanityTestWordSettings()
		{
		return DoControl(ECtrlSanityTestWordSettings);
		}

	inline TUint Test_SanityTestLargeSettings()
		{
		return DoControl(ECtrlSanityTestLargeSettings);
		}

	inline TUint Test_ReleaseSDRs()
		{
		return DoControl(ECtrlFreePhyscialRam);
		}
		
	inline TUint Test_SwitchRepository()
		{
		return DoControl(ECtrlSwitchRepository);
		}
		

#endif

	inline static const TDesC& Name();

private:
	enum TControl
		{
		ECtrlUndefined = 0,
		
		ECtrlSanityTestWordSettings,
		ECtrlSanityTestLargeSettings,
		
		ECtrlGetWordSetting,
		ECtrlGetLargeSetting,
		ECtrlGetManyWordSettings,
		ECtrlGetManyLargeSettings,
		
		ECtrlSwitchRepository,
		
		ECtrlFreePhyscialRam
		};
		
	friend class DHcrTestChannel;
	friend class DHcrTestFactory;
	};

inline const TDesC& RHcrTest::Name()
	{
	_LIT(KTestDriver,"d_hcr");
	return KTestDriver;
	}



#endif // D_HCR_H
