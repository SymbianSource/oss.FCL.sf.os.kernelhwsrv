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

#ifndef D_HCRUT_H
#define D_HCRUT_H

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
		
	inline TUint Test_SwitchRepository()
		{
		return DoControl(ECtrlSwitchRepository);
		}
	
	inline TUint Test_FindCompiledSettingsInCategory(HCR::TCategoryUid aCatUid,
	        TInt32* aFirst, TInt32* aLast)
	    {
	    TAny* args[3];
	    args[0] = (TAny*) aCatUid;
	    args[1] = (TAny*) aFirst;
	    args[2] = (TAny*) aLast;

	    return DoControl(ECtrlCompiledFindSettingsInCategory, (TAny*)args);
	    }
	
	inline TUint Test_FindFileSettingsInCategory(HCR::TCategoryUid aCatUid,
	        TInt32* aFirst, TInt32* aLast)
	    {
	    TAny* args[3];
	    args[0] = (TAny*) aCatUid;
	    args[1] = (TAny*) aFirst;
	    args[2] = (TAny*) aLast;
	    
	    return DoControl(ECtrlFileFindSettingsInCategory, (TAny*)args);
	    }
	
	inline TUint Test_SwitchFileRepository(const TText* aRepName)
	    {
	    TAny* args[1];
	    args[0] = (TAny*) aRepName;
	    return DoControl(ECtrlSwitchFileRepository, (TAny*)args);
	    }
		

	inline TUint Test_CheckIntegrity()
		{
		return DoControl(ECtrlCheckOverrideReposIntegrity);
		}
		
	inline TUint Test_CheckContent()
		{
		return DoControl(ECtrlCheckOverrideRepos102400Content);
		}

	inline TUint Test_NegativeTestsLargeValues(TInt& aExpectedError)
  		{
  		TAny* args[1];
  		args[0] = (TAny*) aExpectedError;
  		return DoControl(ECtrlNegativeTestsLargeValues, (TAny*) args);
  		}
  	inline TUint Test_TRepositoryGetWordValue(HCR::TCategoryUid& aCategory, HCR::TElementId& aKey,TInt& type)
  		{
  		TAny* args[3];
  		args[0] = (TAny*) aCategory;
  		args[1] = (TAny*) aKey;
  		args[2] = (TAny*) type;
  		return DoControl(ECtrlGetWordSetting, (TAny*) args);
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

		ECtrlNegativeTestsLargeValues, 
		
		ECtrlFreePhyscialRam,
        ECtrlCheckOverrideReposIntegrity,
		ECtrlCheckOverrideRepos102400Content,

		ECtrlSwitchFileRepository,
		ECtrlFileFindSettingsInCategory,
		ECtrlCompiledFindSettingsInCategory
		};
		
	friend class DHcrTestChannel;
	friend class DHcrTestFactory;
	};

inline const TDesC& RHcrTest::Name()
	{
	_LIT(KTestDriver,"d_hcrut");
	return KTestDriver;
	}

#endif // D_HCRUT_H
