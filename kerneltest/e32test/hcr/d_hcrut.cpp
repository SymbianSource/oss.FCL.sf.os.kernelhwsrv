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
// Prototype HCR test driver library
//


// -- INCLUDES ----------------------------------------------------------------

#include "hcr_uids.h"
#include "hcr_debug.h"

#include <kernel/kern_priv.h>
#include <platform.h>
#include <u32hal.h>
#include "d_hcrut.h"


#include "hcr_pil.h"

#include <drivers/hcr.h>




// -- CLASSES -----------------------------------------------------------------


class DHcrTestFactory : public DLogicalDevice
	{
public:
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};


class DHcrTestChannel : public DLogicalChannelBase
	{
public:
	DHcrTestChannel();
	virtual ~DHcrTestChannel();
	
	//	Inherited from DObject
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	
	// Inherited from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
		
public:
	static void TestTrace(DHcrTestChannel* aSelf);
	
private:
	DThread* iClient;
    
	};
	
namespace HCR {
class HCRInternalTestObserver 
	{
public:
	HCRInternalTestObserver() : iHcrInt(0) { return; } ;
	HCRInternalTestObserver(HCR::HCRInternal* aSubject) :
		iHcrInt(aSubject) { return; } ;
	~HCRInternalTestObserver() { return; } ; 

	TInt PrintAttirbutes(); 
	
	TInt PrintState();
	
	HCR::TRepository* GetVariantImgRepos();
	HCR::TRepository* GetCoreImgRepos();
	HCR::TRepository* GetOverrideImgRepos();
	
	TInt SwitchRepository(const TText * aFileName, const HCR::HCRInternal::TReposId aId=HCR::HCRInternal::ECoreRepos);

public:

	HCR::HCRInternal* iHcrInt;
	};
}

TInt Testfunc1(TSuperPage* aSuperPagePtr);


TInt HCR::HCRInternalTestObserver::PrintAttirbutes() 
	{
	HCR_TRACE1("HCRInternalTestObserver initialised, iVariant=0x%0x\n", iHcrInt->iVariant);
	return KErrNone;
	}
	
TInt HCR::HCRInternalTestObserver::PrintState() 
	{
	HCR_TRACE2("iVariant     =0x%08X, iVariantStore =0x%08X, \n", iHcrInt->iVariant, iHcrInt->iVariantStore);
	HCR_TRACE2("iCoreImgStore=0x%08X, iOverrideStore=0x%08X, \n", iHcrInt->iCoreImgStore, iHcrInt->iOverrideStore);
	return KErrNone;
	}	


HCR::TRepository* HCR::HCRInternalTestObserver::GetVariantImgRepos()
	{
	return iHcrInt->iVariantStore;
	}

HCR::TRepository* HCR::HCRInternalTestObserver::GetCoreImgRepos()
	{
	return iHcrInt->iCoreImgStore;
	}
HCR::TRepository* HCR::HCRInternalTestObserver::GetOverrideImgRepos()
	{
	return iHcrInt->iOverrideStore;
	}
TInt HCR::HCRInternalTestObserver::SwitchRepository(const TText * aFileName, const HCR::HCRInternal::TReposId aId)
    {
    NKern::ThreadEnterCS();
    TInt retVal = iHcrInt->SwitchRepository(aFileName, aId);
    NKern::ThreadLeaveCS();
    return retVal;
    }
// -- GLOBALS -----------------------------------------------------------------
//


static HCR::HCRInternal gTestHcrInt;
static HCR::HCRInternalTestObserver gObserver;

// -- METHODS -----------------------------------------------------------------
//
// DHcrTestFactory
//

TInt DHcrTestFactory::Install()
	{
    HCR_FUNC("DHcrTestFactory::Install");
	return SetName(&RHcrTest::Name());
	}

void DHcrTestFactory::GetCaps(TDes8& aDes) const
	{
    HCR_FUNC("DHcrTestFactory::GetCaps");
  	Kern::InfoCopy(aDes,0,0);
	}

TInt DHcrTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
    HCR_FUNC("DHcrTestFactory::Create");
   
   	aChannel=new DHcrTestChannel();
	if(!aChannel)
		return KErrNoMemory;
	return KErrNone;
	}


// -- METHODS -----------------------------------------------------------------
//
// DHcrTestChannel
//

DHcrTestChannel::DHcrTestChannel()
	{
    HCR_FUNC("DHcrTestChannel");
   	}

DHcrTestChannel::~DHcrTestChannel()
	{
    HCR_FUNC("~DHcrTestChannel");
	}

TInt DHcrTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
    HCR_FUNC("DHcrTestChannel::DoCreate");
   	
    iClient = &Kern::CurrentThread();
	return KErrNone;
	}

TInt DHcrTestChannel::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
    HCR_FUNC("DHcrTestChannel::RequestUserHandle");
    
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
	}

TInt DHcrTestChannel::Request(TInt aReqNo, TAny*, TAny*)
	{
    HCR_FUNC("DHcrTestChannel::Request");



	switch(aReqNo)
		{
		
	case RHcrTest::ECtrlSanityTestWordSettings:
		{
 		return KErrNone;
		}

	case RHcrTest::ECtrlSanityTestLargeSettings:
		{
		return KErrNone;
		}

	case RHcrTest::ECtrlGetWordSetting:
		{
		return KErrNone;
		}

	case RHcrTest::ECtrlGetLargeSetting:
		{
		return KErrNone;
		}

	case RHcrTest::ECtrlGetManyWordSettings:
		{
		return KErrNone;
		}

	case RHcrTest::ECtrlGetManyLargeSettings:
		{
		return KErrNone;
		}

	case RHcrTest::ECtrlSwitchRepository:
		{
		TInt err = KErrNone;

        // Clear and reset iCoreImgStore
	    HCR_TRACE1("--- value of iCoreImgStore:0x%08x before clear", gObserver.GetCoreImgRepos());
	    err = gObserver.SwitchRepository(NULL, HCR::HCRInternal::ECoreRepos);

    	HCR_TRACE1("--- value of iCoreImgStore:0x%08x after clear",  gObserver.GetCoreImgRepos());
    	if( err != KErrNone )
    		{
    		return err;
    		}
    	
    	// Clear and reset iOverrideStore	
        HCR_TRACE1("--- value of iOverrideStore:0x%08x before clear", gObserver.GetOverrideImgRepos());
	    err = gObserver.SwitchRepository(NULL, HCR::HCRInternal::EOverrideRepos);

    	HCR_TRACE1("--- value of iOverrideStore:0x%08x after clear",  gObserver.GetOverrideImgRepos());
    	if( err != KErrNone )
    		{
    		return err;
    		}
    	
	    // Switch iCoreImgStore to a repositore store located in \sys\bin directory
		const TText * fileInSysBinName = (const TText *)"t_hcr.exe";
		err = gObserver.SwitchRepository(fileInSysBinName, HCR::HCRInternal::ECoreRepos);
		if (err != KErrNone)
             HCR_LOG_RETURN(err);


        // Switch iOverrideStore to a repositore store located in \sys\Data directory
        const TText * fileInSysDataName = (const TText *)"EMPTY.DAT";
        err = gObserver.SwitchRepository(fileInSysDataName, HCR::HCRInternal::EOverrideRepos);
		if (err != KErrNone)
             HCR_LOG_RETURN(err);

        // Try to switch iCoreImgStore to a not existing one and check the SwitchRepository() 
        // keeps its original value.
        HCR::TRepository* oldRepos = gObserver.GetCoreImgRepos();
        HCR_TRACE1("--- value of iCoreImgStore:0x%08x before try to switch to a not exist", oldRepos);
        const TText * wrongFileName = (const TText *)"hcr.ldl";
		err = gObserver.SwitchRepository(wrongFileName, HCR::HCRInternal::ECoreRepos);
		if ( err != KErrNotFound)
             HCR_LOG_RETURN(err);
             
        err = KErrNone;
        
        HCR::TRepository* newRepos = gObserver.GetCoreImgRepos();     
        HCR_TRACE1("--- value of iCoreImgStore:0x%08x after try to switch to a not exist", newRepos);             
        if ( oldRepos != newRepos )
            HCR_LOG_RETURN(KErrGeneral);
            
        // Switch iOverrideStore to a new, existing repository, different the current and check the 
        // iOverrideStore value changed.
        oldRepos = gObserver.GetOverrideImgRepos();
        HCR_TRACE1("--- value of iOverrideStore:0x%08x before try to switch to existing one", oldRepos);
		err = gObserver.SwitchRepository(fileInSysBinName, HCR::HCRInternal::EOverrideRepos);
		if ( err != KErrNone)
             HCR_LOG_RETURN(err);
        
        newRepos = gObserver.GetOverrideImgRepos();     
        HCR_TRACE1("--- value of iOverrideStore:0x%08x after try to switch to existing on", newRepos);             
        if ( oldRepos == newRepos )
            HCR_LOG_RETURN(KErrGeneral);            
        
		return err;
		}
		
	case RHcrTest::ECtrlFreePhyscialRam:
        {
		return KErrNone;
        }

	default:
		break;
		}
		
	return KErrNotSupported;
	}


// -- GLOBALS -----------------------------------------------------------------


DECLARE_STANDARD_LDD()
	{
    HCR_FUNC("D_HCR_DECLARE_STANDARD_LDD");
    
    // Taken from HCR_PIL.CPP InitExtension() method
    
    HCR::MVariant* varPtr = CreateHCRVariant();
	if (varPtr==0)
    	return (0) ; //HCR_LOG_RETURN(0);
    	
	new(&gTestHcrInt) HCR::HCRInternal(varPtr);

	TInt err = gTestHcrInt.Initialise();
	if (err != KErrNone)
    	return (0) ; //HCR_LOG_RETURN(0);

	new(&gObserver) HCR::HCRInternalTestObserver(&gTestHcrInt);
	               
	// ===== Above would be moved to DoRequest for test caes....
	               
   	return new DHcrTestFactory;
   	
	}


