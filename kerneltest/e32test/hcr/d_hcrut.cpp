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
#include "hcr_hai.h"

#include "hcr_pil.h"

#include <drivers/hcr.h>

#define TEST_MEMGET(s, d, l)	kumemget(d, s, l)
#define TEST_MEMPUT(d, s, l)    kumemput(d, s, l)

#include "HcrImageData_102400.h"
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
	
	HCR::MVariant* GetVariant() {return iHcrInt->iVariant;};
	TInt SwitchRepository(const TText * aFileName, const HCR::HCRInternal::TReposId aId=HCR::HCRInternal::ECoreRepos);
	    
	TInt CheckIntegrity(void);
	TInt FindSetting(const TSettingId& aId, TSettingType aType, TSettingRef& aSetting);

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
    
    
TInt HCR::HCRInternalTestObserver::CheckIntegrity(void)
    {
    TInt retVal = iHcrInt->CheckIntegrity();
    return retVal;    
    }
    
TInt HCR::HCRInternalTestObserver::FindSetting(const TSettingId& aId, TSettingType aType, TSettingRef& aSetting)
    {
    TInt retVal = iHcrInt->FindSetting( aId, aType, aSetting);
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

TInt DHcrTestChannel::Request(TInt aReqNo, TAny* a1, TAny* /*a2*/ )
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
		TAny* args[3];
		TEST_MEMGET(a1, args, sizeof(args));

		HCR::TCategoryUid category = (HCR::TCategoryUid) args[0];
    
		HCR::TElementId key = (HCR::TElementId) args[1];

		TInt type = (TInt) args[2];

		const TText * fileInSysBinName = (const TText *)"filerepos.dat";
		TInt err = gObserver.SwitchRepository(fileInSysBinName, HCR::HCRInternal::ECoreRepos);
		if (err != KErrNone)
             HCR_TRACE_RETURN(err);

		// Negative tests on HCR::TRepositoryFile; aNum will be 0
		HCR::TRepository* repos = gObserver.GetCoreImgRepos();;
		__NK_ASSERT_DEBUG(repos != NULL);


		HCR::SSettingId* ids[1];// = new HCR::SSettingId*[1];

		TInt32* vals[1];
		TInt* errs[1];
		HCR::TSettingType* types[1];
		
		NKern::ThreadEnterCS();
		ids[0] = new HCR::SSettingId();
		vals[0] = new TInt32();
		errs[0] = new TInt();
		types[0] = new HCR::TSettingType();

		if(ids[0] == NULL || vals[0] == NULL || errs[0] == NULL || types[0] == NULL) 
			{
			delete ids[0];
			delete vals[0];
			delete errs[0];
			delete types[0];
			NKern::ThreadLeaveCS();
			HCR_TRACE_RETURN(KErrNoMemory);
			}

		ids[0]->iCat = category;
		ids[0]->iKey = key;

		// Negative tests on HCR::TRepositoryFile; aNum will be 0
		TInt r = repos->GetWordSettings(0, ids, vals, types, errs);
		// only expected errors are KErrNotFound or KErrNone
		// thest if there is other error; if yes fail the test
		if(r != KErrNotFound && r != KErrNone && r < KErrNone)
			{
			delete ids[0];
			delete vals[0];
			delete errs[0];
			delete types[0];
			NKern::ThreadLeaveCS();
			HCR_TRACE_RETURN(r);
			}

		// Negative testing on HCR::TRepositoryFile; try to get words for large value
		if(type > HCR::ETypeLinAddr)
			{
			r = repos->GetWordSettings(1, ids, vals, types, errs);
			if(r != KErrArgument && r != KErrNotFound && r < KErrNone)
				{
				delete ids[0];
				delete vals[0];
				delete errs[0];
				delete types[0];
				NKern::ThreadLeaveCS();
				HCR_TRACE_RETURN(r);
				}
			}

		HCR::TRepositoryCompiled* compiledRepos = reinterpret_cast<HCR::TRepositoryCompiled*>(gObserver.GetVariantImgRepos());
		__NK_ASSERT_DEBUG(compiledRepos != NULL);

		ids[0]->iCat = KHCRUID_TestCategory1;
		ids[0]->iKey = key;    
		
		// Negative tests on HCR::TRepositoryCompiled; aNum will be 0
		r = compiledRepos->GetWordSettings(0, ids, vals, types, errs);
		if(r != KErrNotFound && r != KErrNone && r < KErrNone)
			{
			delete ids[0];
			delete vals[0];
			delete errs[0];
			delete types[0];
			NKern::ThreadLeaveCS();
			HCR_TRACE_RETURN(r);
			}

		// Negative testing on HCR::TRepositoryFile; try to get words for large value
		if(type > HCR::ETypeLinAddr)
			{
			r = compiledRepos->GetWordSettings(1, ids, vals, types, errs);
			if(r != KErrArgument && r != KErrNotFound && r < KErrNone)
				{
				delete ids[0];
				delete vals[0];
				delete errs[0];
				delete types[0];
				NKern::ThreadLeaveCS();
				HCR_TRACE_RETURN(r);
				}
			}
		
		delete ids[0];
		delete vals[0];
		delete errs[0];
		delete types[0];
		NKern::ThreadLeaveCS();

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
		const TText * fileInSysBinName = (const TText *)"d_hcrsim_own.ldd";
		err = gObserver.SwitchRepository(fileInSysBinName, HCR::HCRInternal::ECoreRepos);
		if (err != KErrNone)
             HCR_TRACE_RETURN(err);


        // Switch iOverrideStore to a repositore store located in \sys\Data directory
        const TText * fileInSysDataName = (const TText *)"EMPTY.DAT";
        err = gObserver.SwitchRepository(fileInSysDataName, HCR::HCRInternal::EOverrideRepos);
		if (err != KErrNone)
             HCR_TRACE_RETURN(err);

        // Try to switch iCoreImgStore to a not existing one and check the SwitchRepository() 
        // keeps its original value.
        HCR::TRepository* oldRepos = gObserver.GetCoreImgRepos();
        HCR_TRACE1("--- value of iCoreImgStore:0x%08x before try to switch to a not exist", oldRepos);
        const TText * wrongFileName = (const TText *)"hcr.ldl";
		err = gObserver.SwitchRepository(wrongFileName, HCR::HCRInternal::ECoreRepos);
		if ( err != KErrNotFound)
             HCR_TRACE_RETURN(err);
             
        err = KErrNone;
        
        HCR::TRepository* newRepos = gObserver.GetCoreImgRepos();     
        HCR_TRACE1("--- value of iCoreImgStore:0x%08x after try to switch to a not exist", newRepos);             
        if ( oldRepos != newRepos )
            HCR_TRACE_RETURN(KErrGeneral);
            
        // Switch iOverrideStore to a new, existing repository, different the current and check the 
        // iOverrideStore value changed.
        oldRepos = gObserver.GetOverrideImgRepos();
        HCR_TRACE1("--- value of iOverrideStore:0x%08x before try to switch to existing one", oldRepos);
		err = gObserver.SwitchRepository(fileInSysBinName, HCR::HCRInternal::EOverrideRepos);
		if ( err != KErrNone)
             HCR_TRACE_RETURN(err);
        
        newRepos = gObserver.GetOverrideImgRepos();     
        HCR_TRACE1("--- value of iOverrideStore:0x%08x after try to switch to existing on", newRepos);             
        if ( oldRepos == newRepos )
            HCR_TRACE_RETURN(KErrGeneral);            
        
		return err;
		}
		
	case RHcrTest::ECtrlNegativeTestsLargeValues:
		{
		//Test that HCR::TRepositoryCompiled::GetLargeValue & HCR::TRepositoryFile::GetLargeValue return KErrArgument
		TAny* args[1];
		TEST_MEMGET(a1, args, sizeof(args));
		// Retrieve structures from client
		TInt expectedError = (TUint) args[0];

		const TText * fileInSysBinName = (const TText *)"filerepos.dat";
		TInt err = gObserver.SwitchRepository(fileInSysBinName, HCR::HCRInternal::ECoreRepos);
		if (err != KErrNone)
             HCR_TRACE_RETURN(err);

		// Do test for HCR::TRepositoryFile
		HCR::TRepository* repos = gObserver.GetCoreImgRepos();;
		__NK_ASSERT_DEBUG(repos != NULL);
		
		HCR::UValueLarge value;
		HCR::TSettingRef ref(0,0);
		HCR::TSettingId id(1,1); //word setting value in repository
		err = repos->FindSetting(id, ref);
		if(err == KErrNone)
		    {
            err = repos->GetLargeValue(ref, value);
            if(err != expectedError)
                {
                HCR_TRACE_RETURN(err);
                }
		    }
		
		//Do test for HCR::TRepositoryCompiled
		HCR::TRepositoryCompiled* compiledRepos = reinterpret_cast<HCR::TRepositoryCompiled*>(gObserver.GetVariantImgRepos());
		if (compiledRepos == 0) 
			{ 
		    HCR_TRACE_RETURN(KErrGeneral);
		    }
		    
		id = HCR::TSettingId(KHCRUID_TestCategory1,1);
		err = compiledRepos->FindSetting(id, ref);
		if(err == KErrNone)
			{
            err = compiledRepos->GetLargeValue(ref, value);
            if(err != expectedError)
				{
				HCR_TRACE_RETURN(err);
                }
			}
		
		return KErrNone;
		}


    case RHcrTest::ECtrlCheckOverrideReposIntegrity:
        {
        HCR::TRepository* overrideRepos = gObserver.GetOverrideImgRepos();  // Shadowed SMR/HCR
        TInt err = KErrNone;
        
        if( 0 != overrideRepos )
            {
            err = overrideRepos->CheckIntegrity();
            
            } 
        return err;
        }
        
    case RHcrTest::ECtrlCheckOverrideRepos102400Content:
        {
        HCR::TRepository* overrideRepos = gObserver.GetOverrideImgRepos();  // Shadowed SMR/HCR
        TInt err = KErrNone;
        
        if( 0 != overrideRepos )
            {
            for( TInt index = 0; index < itemsSize; ++index)
                {
                HCR::TSettingId id(items[index].iCategoryUID, items[index].iElementID);
                HCR_TRACE3("--- index:%5d, iCategoryUID:0x%08x, iElementID:0x%08x"
                            , index
                            , items[index].iCategoryUID
                            , items[index].iElementID
                            );
                HCR::TSettingRef val(overrideRepos, 0);
                HCR::TSettingType type = (HCR::TSettingType)items[index].iType;
    			TInt r = gObserver.FindSetting(id, type, val);
    			if( r != KErrNone)
    			    {
    			        err = KErrNotFound;
    			        break;
    			    }
    			HCR::UValueWord valueWord;
                r = overrideRepos->GetValue(val, valueWord);
                HCR_TRACE1("--- value:0x%08x", valueWord.iUInt32); 
                if( valueWord.iUInt32 != items[index].iValue)
                    {
                    err = KErrNotFound;
    			    break;    
                    }
                }
            }
        return err;
        }
        
    case RHcrTest::ECtrlSwitchFileRepository:
	    {
	    TInt r;
	    TAny* args[2];
	    TEST_MEMGET(a1, args, sizeof(args));
	    const TText* fileRepName = (TText*) args[0];
	    
	    r = gObserver.SwitchRepository(fileRepName, HCR::HCRInternal::ECoreRepos);
	    if (r != KErrNone)
	        {
	        HCR_TRACE_RETURN(r);
	        }
	    else
	        return r;
	    }
	    
	case RHcrTest::ECtrlCompiledFindSettingsInCategory:
	    {
	    TInt r = 0;
	    //Do test for HCR::TRepositoryCompiled
	    TAny* args[3];
	    

	    //It's a pre-condition to enter critical section before
	    //kernel memory allocation
	    NKern::ThreadEnterCS();
	    TInt32* pFirst = new TInt32;
	    TInt32* pLast = new TInt32;
	    //We've done with allocation, exit CS
	    NKern::ThreadLeaveCS();
	    
	    if(!pFirst || !pLast)
	        { 
	        HCR_TRACE_RETURN(KErrNoMemory);
	        }
	    
	    TEST_MEMGET(a1, args, sizeof(args));
	    HCR::TCategoryUid catUid = (HCR::TCategoryUid)args[0];
	    
	    
	    HCR::TRepositoryCompiled* compiledRepos = 
	    reinterpret_cast<HCR::TRepositoryCompiled*>(gObserver.GetVariantImgRepos());
	    if (compiledRepos == 0) 
	        { 
	        HCR_TRACE_RETURN(KErrGeneral);
	        }
	   
	    //This function return the result of operation r and first element and 
	    //last element in the category written back to the user side test code 
	    //variable referenced by pFirst and pLast pointers
	    r = compiledRepos->FindNumSettingsInCategory(catUid, 
	            *pFirst, *pLast);
	    
	    TEST_MEMPUT(args[1], pFirst, sizeof(TInt32));
	    TEST_MEMPUT(args[2], pLast, sizeof(TInt32));
	    
	    if(r < 0)
	        {HCR_TRACE_RETURN(r);}
	    else
	        return r;
	    }
        
	case RHcrTest::ECtrlFileFindSettingsInCategory:
	    {
	    TInt r;
	    TAny* args[3];
	    TEST_MEMGET(a1, args, sizeof(args));
	    HCR::TCategoryUid catUid = (HCR::TCategoryUid)args[0];

	    //It's a pre-condition to enter critical section before
	    //kernel memory allocation
	    NKern::ThreadEnterCS();
	    TInt32* pFirst = new TInt32;
	    TInt32* pLast = new TInt32;
	    //We've done with allocation, exit CS
	    NKern::ThreadLeaveCS();

	    if(!pFirst || !pLast)
	        { 
	        HCR_TRACE_RETURN(KErrNoMemory);
	        }


	    // Do test for HCR::TRepositoryFile
	    HCR::TRepository* repos = gObserver.GetCoreImgRepos();
	    __NK_ASSERT_DEBUG(repos != NULL);

	    //This function return the result of operation r and first element and 
	    //last element in the category written back to the user side test code 
	    //variable referenced by pFirst and pLast pointers
	    r = repos->FindNumSettingsInCategory(catUid, 
	            *pFirst, *pLast);

	    TEST_MEMPUT(args[1], pFirst, sizeof(TInt32));
	    TEST_MEMPUT(args[2], pLast, sizeof(TInt32));

	    if(r < 0)
	        {HCR_TRACE_RETURN(r);}
	    else
	        return r;
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

	// Try to initialise without a varian; KErrGeneral error should be returned
	new(&gTestHcrInt) HCR::HCRInternal(NULL);
	TInt err = gTestHcrInt.Initialise();
	if (err != KErrGeneral)
    	return 0;


    // Taken from HCR_PIL.CPP InitExtension() method
    
    HCR::MVariant* varPtr = CreateHCRVariant();
	if (varPtr==0)
    	return 0;

	new(&gTestHcrInt) HCR::HCRInternal(varPtr);
    	
	err = gTestHcrInt.Initialise();
	if (err != KErrNone)
    	return 0;

	new(&gObserver) HCR::HCRInternalTestObserver(&gTestHcrInt);
	               
	// ===== Above would be moved to DoRequest for test caes....
	               
   	return new DHcrTestFactory;
   	
	}


