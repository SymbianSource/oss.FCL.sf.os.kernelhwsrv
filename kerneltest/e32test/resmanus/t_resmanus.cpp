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

#include <e32test.h>
#include <e32def.h>
#include <e32def_private.h>

#include <d32resmanus.h>

#ifdef PRM_ENABLE_EXTENDED_VERSION
#define LDD_NAME _L("resourcecontrollerextended.ldd")
#else
#define LDD_NAME _L("resourcecontrol.ldd")
#endif

#ifdef PRM_ENABLE_EXTENDED_VERSION
#define PDD_NAME _L("resourcecontrollerextended.pdd")
#else
#define PDD_NAME _L("resourcecontroller.pdd")
#endif

//#define PIRATE_THREAD_TESTS

_LIT(testableResourceName,"SymbianSimulResource");

#ifdef RESMANUS_KERN
_LIT(testName,"t_resmanuskern");
#else
_LIT(testName,"t_resmanus");
#endif

TBuf<16> gTestName(testName);

GLDEF_D RBusDevResManUs gChannel;

TUint8 KNoOfGetStateRequests = 5;
TUint8 KNoOfSetStateRequests = 4;
TUint8 KNoOfNotifyRequests = 7;
#define MAX_NUM_REQUESTS 8		// Must be (at least) one greater than the largest of 
								// KNoOfGetStateRequests, KNoOfSetStateRequests, KNoOfNotifyRequests
#ifdef _DEBUG
TUint gLongLatencyResource;
TBool gHaveAsyncRes = EFalse;
TInt gAsyncResStateDelta = 0;

TUint gSharedResource;
TBool gHaveSharedRes = EFalse;
TInt gSharedResStateDelta = 0;
#else
// The UREL version of the driver will not implement the arrays and functionality required
// to determine the async and shared resources to use at runtime. The values provided here
// have been pre-determined to operate successfully with the simulated PSL.
//
TUint gLongLatencyResource = 6;
TBool gHaveAsyncRes = ETrue;
TInt gAsyncResStateDelta = -1;

TUint gSharedResource = 12;
TBool gHaveSharedRes = ETrue;
TInt gSharedResStateDelta = -1;
#endif

TBool gUseCached = EFalse;

class RTestSafe: public RTest
	{
public:
	RTestSafe(const TDesC &aTitle) :
		RTest(aTitle), iCleanUpLevelMask (0), iFailHdnFunc(NULL)
		{
		}
	RTestSafe(const TDesC &aTitle, void(*func)(RTestSafe &aTest)) :
		RTest(aTitle), iFailHdnFunc(func)
		{
		}

	// new version of operator(int), which calls our cleanup handler if check has failed
	void operator()(TInt aResult)
		{
		if(!aResult && iFailHdnFunc)
			iFailHdnFunc(*this);
		RTest::operator ()(aResult);
		}

	void operator()(TInt aResult, TInt aLineNum)
		{
		if(!aResult && iFailHdnFunc)
			iFailHdnFunc(*this);
		RTest::operator ()(aResult, aLineNum);
		}

	void operator()(TInt aResult, TInt aLineNum, const TText* aFileName)
		{
		if(!aResult && iFailHdnFunc)
			iFailHdnFunc(*this);
		RTest::operator ()(aResult, aLineNum, aFileName);
		}

	// new version of End, which calls handler before exit..
	void End()
		{
		if(iFailHdnFunc)
			iFailHdnFunc(*this);
		RTest::End();
		}

	void SetCleanupFlag(TUint aFlag)
		{
		iCleanUpLevelMask |= 1 << aFlag;
		}

	TBool CleanupNeeded(TUint aFlag)
		{
		return (iCleanUpLevelMask & (1 << aFlag)) >> aFlag;
		}

	TUint iCleanUpLevelMask;
	void (*iFailHdnFunc)(RTestSafe &aTest);
	};

// cleanup handler
enum TCleanupLevels
	{
	EPddLoaded = 0,
	ELddLoaded,
	EChannelOpened
	};


void TestCleanup(RTestSafe &aTest)
	{
	// cleanup for all 3 levels..
	if(aTest.CleanupNeeded(EChannelOpened))
		{
		gChannel.Close();
		}

	if(aTest.CleanupNeeded(ELddLoaded))
		{
		User::FreeLogicalDevice(KLddRootName);
		}

	if(aTest.CleanupNeeded(EPddLoaded))
		{
		User::FreePhysicalDevice(PDD_NAME);
		}
	}

// global gTest object..
RTestSafe gTest(testName, &TestCleanup);

LOCAL_C TInt CheckCaps()
	{
	TInt r = KErrNone;
	RDevice d;
	TPckgBuf<TCapsDevResManUs> caps;
	r = d.Open(KLddRootName);
	if(r == KErrNone)
		{
		d.GetCaps(caps);
		d.Close();

		TVersion ver = caps().version;
		if(ver.iMajor != 1 || ver.iMinor != 0 || ver.iBuild != KE32BuildVersionNumber)
			{
			gTest.Printf(_L("Capabilities returned wrong version"));
			gTest.Printf(_L("Expected(1, 0, %d), got (%d , %d, %d)"),
			                KE32BuildVersionNumber, ver.iMajor, ver.iMinor, ver.iBuild);
			r = KErrGeneral;
			}
		}
	return r;
	}

LOCAL_C TInt OpenChannel(TDesC16& aName, RBusDevResManUs& aChannel)
	{
	TInt r = KErrNone;
	// API accepts 8-bit descriptors, only - so convert name accordingly
	TBuf8<MAX_RESOURCE_NAME_LENGTH+1>EightBitName;
	EightBitName.Copy(aName);
    r=(aChannel.Open(EightBitName));
    if (r!=KErrNone)
		gTest.Printf(_L("OpenChannel: Handle for channel %S error code =0x%x\n"),&aName,r);
	else
		gTest.Printf(_L("OpenChannel: Handle for channel %S =0x%x\n"),&aName,aChannel.Handle());
	return r;
	}

LOCAL_C TInt HelperResources()
//
// Helper method to support OpenAndRegisterChannel
// Invokes GetNoOfResources, GetAllResourcesInfo and GetResourceIdByName
//
	{
	TInt r = KErrNone;

	__KHEAP_MARK;

	// Check what resources are available
    gTest.Printf(_L("**Test GetNoOfResources\n"));
	TUint numResources=0;
	if((r=gChannel.GetNoOfResources(numResources))!=KErrNone)
		{
		gTest.Printf(_L("GetNoOfResources for test channel returned %d\n"),r);
		return r;
		}
	gTest.Printf(_L("Number of resources = %d (=0x%x)\n"),numResources,numResources);

	// Read the resource information
    gTest.Printf(_L("**Test GetAllResourcesInfo\n"));

	// To support the GetAllResourcesInfo testing, instantiate TResourceInfoBuf objects
	// and reference via an RSimplePointerArray
	TUint bufSize = numResources;
	RSimplePointerArray<TResourceInfoBuf> infoPtrs(bufSize);
	for(TUint i=0;i<bufSize;i++)
		{
		TResourceInfoBuf *info = new TResourceInfoBuf();
		if((r=infoPtrs.Insert(info, i))!=KErrNone)
			{
			gTest.Printf(_L("GetAllResourcesInfo infoPtrs.Insert at index %d returned %d\n"),i,r);
			}
		}
	TUint updateNumResources=numResources;
	if((r=gChannel.GetAllResourcesInfo(&infoPtrs,updateNumResources))!=KErrNone)
		{
		gTest.Printf(_L("GetAllResourcesInfo for channel returned %d\n"),r);
		return r;
		}
	gTest.Printf(_L("Updated number of resources = %d\n"),updateNumResources);

#ifdef _DEBUG
	// Print resource names
	{
	TBuf16<MAX_RESOURCE_NAME_LENGTH+1>name;
	for(TUint i=0; i<updateNumResources; i++)
		{
		TResourceInfoBuf* currRes = infoPtrs[i];
		name.Copy((*currRes)().iName);
		name.PtrZ();
		gTest.Printf(_L("Resource %d name = %S \n"),i,&name);
		};
	}
#endif

	// Select a resource to use, then pass its name to GetResourceIdByName
	// to check that the corresponding resource ID is acquired.
	TResourceInfo currRes = (*infoPtrs[0])(); 
	if(updateNumResources>1)
		currRes=(*infoPtrs[1])();// First resource may be a dummy
	TUint resourceId;
	gTest.Printf(_L("Invoking GetResourceIdByName for last resource name extracted \n"));
	if((r=gChannel.GetResourceIdByName(currRes.iName, resourceId))!=KErrNone)
		{
		gTest.Printf(_L("GetResourceIdByName for channel returned %d \n"),r);
		return r;
		}
	gTest.Printf(_L("GetResourceIdByName gave ID = %d\n"),resourceId);

	infoPtrs.Close();

	__KHEAP_MARKEND;

	return r;
	}

LOCAL_C TInt CheckForSimulatedResources()
//
// Get the name of the first resource - if it does not match the expected name
// then the testing must not proceed, so return the relevant error code
//
	{
    TInt r = KErrNone;
	TResourceInfoBuf buffer;
	if((r=gChannel.GetResourceInfo(1, &buffer))!=KErrNone)	// first resource ID = 1
		{
		gTest.Printf(_L("CheckForSimulatedResources, candidate get resource info returned %d\n"),r);
		return r;
		}
	// Check the name of the resource
	TBuf16<MAX_RESOURCE_NAME_LENGTH+1>name;
	name.Copy(buffer().iName);
	name.PtrZ();
	if((r=name.Compare(testableResourceName))!=KErrNone)
		{
		gTest.Printf(_L("Resource name = %S, require %S \n"),&name,&testableResourceName);
		r=KErrNotSupported;
		}
	return r;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_RESMANUS-0607
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1398
//! @SYMTestCaseDesc    This test case tests channel opening and initialisation APIs.
//! @SYMTestActions     0) Call Open API with a valid name.
//! 
//!						1) Call Open API for two more channels 
//!						(to demonstrate multiple clients (channels) can be supported concurrently).
//! 
//!						2) Call Open with an oversized name. 
//! 
//!						3) Call GetNoOfResources API
//! 
//!						4) Call GetAllResourcesInfo API
//! 
//!						5) Call GetResourceIdByName API for last of resource names gathered
//!						in previous step
//! 
//!						6) Call Initialise API on the channel originally created, with non-zero arguments.
//!
//! @SYMTestExpectedResults 0) API should return with KErrNone, exits otherwise.
//!						1) API should return with KErrNone, exits otherwise.
//!						2) API should return with KErrBadName, exits otherwise.
//!						3) API should return with KErrNone, exits otherwise.
//!						4) API should return with KErrNone, exits otherwise.
//!						5) API should return with KErrNone, exits otherwise.
//!						6) API should return with KErrNone, exits otherwise.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt OpenAndRegisterChannel()
//
// Test Open and Initialise functionality
// Also invokes Resource inspection methods via HelperResource
//
    {
    TInt r;

	if((r=OpenChannel(gTestName, gChannel))!=KErrNone)
		return r;

	// Check that the simulated resources required to support this testing are present
	// If not, close the channel and return the propagated error code.
	if((r=CheckForSimulatedResources())!=KErrNone)
		{
		gChannel.Close();
		return r;
		}

	__KHEAP_MARK; // gTestName will remain open (allocated) when heap is checked

	// Open a couple of additional channels to prove that more than one
	// can exist
	_LIT(tempStr1,"temp1");
	TBufC<16> tempName1(tempStr1);
	_LIT(tempStr2,"temp2");
	TBufC<16> tempName2(tempStr2);

	RBusDevResManUs channelTwo;
	RBusDevResManUs channelThree;

	if((r=OpenChannel(tempName1, channelTwo))!=KErrNone)
		return r;
	if((r=OpenChannel(tempName2, channelThree))!=KErrNone)
		return r;

	// The following test requires manual enabling in resource controller of macro
	// DEBUG_VERSION - this is not done by default, so test is deactivated here.
	//
#if 0
	// Test rejection if try a name already in use
	//
	// (For UREL builds, duplicate names are allowed; but
	//  for UDEB builds, they are monitored and rejected)
	//
	RBusDevResManUs channelSameName;
	r=OpenChannel(tempName1, channelSameName);
	channelSameName.Close(); // Channel not used after here
    if (r==KErrNone)
		{
		gTest.Printf(_L("Error: Handle for re-used name channel =0x%x\n"),channelSameName.Handle());
		return KErrGeneral;
		}
	else if(r!=KErrCouldNotConnect)
		{
		gTest.Printf(_L("Error: re-used name gave unexpected error code =0x%x\n"),r);
		return r;
		}
	else // if(r==KErrCouldNotConnect)
		{
		gTest.Printf(_L("Re-used channel name rejected with correct error code\n"));
		}
#endif // if 0

	// Test oversized name rejection
	_LIT(longStr,"1abcdefghijklmnopqrstuvwxyz2abcdefghijklmnopqrstuvwxyz3abcdefghijklmnopqrstuvwxyz4abcdefghijklmnopqrstuvwxyz5abcdefghijklmnopqrstuvwxyz6abcdefghijklmnopqrstuvwxyz7abcdefghijklmnopqrstuvwxyz8abcdefghijklmnopqrstuvwxyz9abcdefghijklmnopqrstuvwxyz10abcdefghijklmnopqrstuvwxyz");
	TBufC<271> longName(longStr);

	// API accepts 8-bit descriptors for names, only
	TBuf8<271>longName_8Bit;
	longName_8Bit.Copy(longName);

	RBusDevResManUs channelLong;
    r=(channelLong.Open(longName_8Bit));
	channelLong.Close(); // Channel not used after here
    if (r==KErrNone)
		{
		gTest.Printf(_L("Error: Handle for oversize name channel =0x%x\n"),channelLong.Handle());
		return KErrGeneral;
		}
	else if(r!=KErrBadName)
		{
		gTest.Printf(_L("Error: oversized name gave unexpected error code =0x%x\n"),r);
		return r;
		}
	else // if(r==KErrBadName)
		{
		gTest.Printf(_L("Oversized name for channel rejected with correct error code\n"));
		}

	// Invokes GetNoOfResources, GetAllResourcesInfo and GetResourceIdByName
	if((r=HelperResources())!=KErrNone)
		return r;

	// Close the temporary channels
	// Do this before  channel registration to enable valid check of kernel heap
	channelTwo.Close();
	channelThree.Close();

	__KHEAP_MARKEND;

	// Channel registration
    gTest.Printf(_L("Invoking Initialise with values 0x%x, 0x%x, 0x%x\n"),KNoOfGetStateRequests,KNoOfSetStateRequests,KNoOfNotifyRequests);
    if ((r=gChannel.Initialise(KNoOfGetStateRequests,KNoOfSetStateRequests,KNoOfNotifyRequests))!=KErrNone)
		{
		gTest.Printf(_L("Initialise for channel returned %d\n"),r);
		return r;
		}

    return KErrNone;
    }


LOCAL_C TInt HelperClients()
//
// Helper method to support TestGetClientGetResourceInfo
// Invokes GetNoOfClients and GetNamesAllClients
//
	{
	__KHEAP_MARK;

	TInt r = KErrNone;
	TUint numClients = 0;
	TUint numAllClients = 0;
	//
	// GetNoOfClients - with aIncludeKern=EFalse
	//
	if((r=gChannel.GetNoOfClients(numClients, EFalse)) != KErrNone)
		{
		gTest.Printf(_L("GetNoOfClients (aIncludeKern==EFalse) returned %d\n"),r);
		return r;
		}
	gTest.Printf(_L("GetNoOfClients (aIncludeKern==EFalse) gave 0x%x clients\n"),numClients);

	//
	// GetNoOfClients - with aIncludeKern=ETrue
	//
	r=gChannel.GetNoOfClients(numAllClients, ETrue);
#ifdef RESMANUS_KERN
	if(r==KErrNone)
		gTest.Printf(_L("GetNoOfClients (aIncludeKern==ETrue) returned KErrNone\n"));
#else
	if(r==KErrPermissionDenied)
		gTest.Printf(_L("GetNoOfClients (aIncludeKern==ETrue) returned KErrPermissionDenied\n"));
#endif
	else
		{
		gTest.Printf(_L("GetNoOfClients (aIncludeKern==ETrue) returned %d"),r);
		return KErrGeneral;
		}

	// To support the GetNamesAllClients testing, instantiate TClientName objects
	// and reference via an RSimplePointerArray
	TUint bufSize = (numAllClients>numClients)?numAllClients:numClients;
	RSimplePointerArray<TClientName> infoPtrs(bufSize);
	for(TUint i=0;i<bufSize;i++)
		{
		TClientName *info = new TClientName();
		if((r=infoPtrs.Insert(info, i))!=KErrNone)
			{
			gTest.Printf(_L("GetNamesAllClients infoPtrs.Insert at index %d returned %d\n"),i,r);
			}
		}

	//
	// GetNamesAllClients - with aIncludeKern=EFalse
	//
	if((r=gChannel.GetNamesAllClients(&infoPtrs, numClients, EFalse)) != KErrNone)
		{
		gTest.Printf(_L("GetNamesAllClients (aIncludeKern==EFalse) returned %d\n"),r);
		return r;
		}
#ifdef _DEBUG
	else
		{
		gTest.Printf(_L("GetNamesAllClients (aIncludeKern==EFalse) returned KErrNone, names follow\n"));
		for(TUint i=0;i<numClients;i++)
			{
			TClientName *currName = infoPtrs[i];
			TBuf16<sizeof(TClientName)> name;
			name.Copy(*currName);
			gTest.Printf(_L("Client name %d = %S\n"),i,&name);
			}
		}
#endif

	//
	// GetNamesAllClients - with aIncludeKern=ETrue
	//
#ifdef RESMANUS_KERN
	if((r=gChannel.GetNamesAllClients(&infoPtrs, numAllClients, ETrue)) != KErrNone)
		{
		gTest.Printf(_L("GetNamesAllClients (aIncludeKern==ETrue) returned %d\n"),r);
		return r;
		}
#ifdef _DEBUG
	else
		{
		gTest.Printf(_L("GetNamesAllClients (aIncludeKern==ETrue) returned KErrNone, names follow\n"));
		for(TUint i=0;i<numAllClients;i++)
			{
			TClientName *currName = infoPtrs[i];
			TBuf16<sizeof(TClientName)> name;
			name.Copy(*currName);
			gTest.Printf(_L("Client name %d = %S\n"),i,&name);
			}
		}
#endif
#else
	if((r=gChannel.GetNamesAllClients(&infoPtrs, numClients, ETrue)) == KErrPermissionDenied)
		{
		gTest.Printf(_L("GetNamesAllClients (aIncludeKern==ETrue) returned KErrPermissionDenied\n"));
		r=KErrNone; // Ensure misleading status is not returned
		}
	else
		{
		gTest.Printf(_L("GetNamesAllClients (aIncludeKern==ETrue) returned %d"),r);
		return r;
		}
#endif

	infoPtrs.Close();
	__KHEAP_MARKEND;

	return r;
	}

LOCAL_C TInt HelperClientsUsingResource(TUint aResourceId)
//
// Helper method to support TestGetClientGetResourceInfo
// Invokes GetNumClientsUsingResource and GetInfoOnClientsUsingResource
//
	{
	__KHEAP_MARK;

	TInt r = KErrNone;
	//
	// GetNumClientsUsingResource - with aIncludeKern=ETrue
	//
	TUint resourceAllClients = 0;
	if((r=gChannel.GetNumClientsUsingResource(aResourceId, resourceAllClients, ETrue)) == KErrPermissionDenied)
		{
		gTest.Printf(_L("GetNumClientsUsingResource (aIncludeKern==ETrue) returned KErrPermissionDenied\n"));
#ifdef RESMANUS_KERN
		return r;
		}
	else
		{
		if(r!=KErrNone)
			{
			gTest.Printf(_L("GetNumClientsUsingResource (aIncludeKern==ETrue) returned %d\n"),r);
			return r;
			}
		else
			gTest.Printf(_L("GetNumClientsUsingResource (aIncludeKern==ETrue) reported 0x%x clients\n"),resourceAllClients);
		}
#else
		}
	else
		{
		gTest.Printf(_L("GetNumClientsUsingResource (aIncludeKern==ETrue) returned %d\n"),r);
		return r;
		}
#endif
	//
	// GetNumClientsUsingResource - with aIncludeKern=EFalse
	//
	TUint resourceClients = 0;
	if((r=gChannel.GetNumClientsUsingResource(aResourceId, resourceClients, EFalse)) != KErrNone)
		{
		// If there are no clients that have requested a level then the Resource Controller will
		// the client ID as a bad argument
		if(!((resourceClients==0)&&(r==KErrArgument)))
			{
			gTest.Printf(_L("GetNumClientsUsingResource (aIncludeKern==EFalse) returned %d\n"),r);
			return r;
			}
		else
			r=KErrNone;	// Ensure expected error is not misinterpeted
		}
	else
		gTest.Printf(_L("GetNumClientsUsingResource (aIncludeKern==EFalse) reported 0x%x clients\n"),resourceClients);
		
	// To support the GetInfoOnClientsUsingResource testing, instantiate TClientInfoBuf objects
	// and reference via an RSimplePointerArray
	TUint bufSize = (resourceAllClients>resourceClients)?resourceAllClients:resourceClients;
	if(bufSize>0)
		{
		RSimplePointerArray<TClientInfoBuf> infoPtrs(bufSize);
		for(TUint i=0;i<bufSize;i++)
			{
			TClientInfoBuf *info = new TClientInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("GetInfoOnClientsUsingResource infoPtrs.Insert at index %d returned %d\n"),i,r);
				}
			}

		//
		// GetInfoOnClientsUsingResource - with aIncludeKern=EFalse
		//
		if((r=gChannel.GetInfoOnClientsUsingResource(aResourceId, resourceClients, &infoPtrs, EFalse)) != KErrNone)
			{
			// If there are no clients that have requested a level then the resource will not
			// have been found
			if(!((resourceClients==0)&&(r==KErrNotFound)))
				{
				gTest.Printf(_L("GetInfoOnClientsUsingResource (aIncludeKern==EFalse) returned %d\n"),r);
				return r;
				}
			}
#ifdef _DEBUG
		else
			{
			gTest.Printf(_L("GetInfoOnClientsUsingResource (aIncludeKern==EFalse) returned KErrNone, info follows\n"));
			for(TUint i=0;i<resourceClients;i++)
				{
				TClientInfoBuf* currInfoBuf = infoPtrs[i];
				TClientInfo currInfo=(*currInfoBuf)();
				TBuf16<sizeof(TClientName)> name;
				name.Copy(currInfo.iName);
				gTest.Printf(_L("Client name %d = %S, ID=0x%x\n"),i,&name,currInfo.iId);
				}
			}
#endif
		//
		// GetInfoOnClientsUsingResource - with aIncludeKern=ETrue
		//
		r=gChannel.GetInfoOnClientsUsingResource(aResourceId, resourceAllClients, &infoPtrs, ETrue);
			{
#ifdef RESMANUS_KERN
			if(r != KErrNone)
				{
				// If there are no clients that have requested a level then the Resource Controller
				// will report a request for information on 0 clients as a bad argument
				if(!((resourceClients==0)&&(r==KErrArgument)))
					{
					gTest.Printf(_L("GetInfoOnClientsUsingResource (aIncludeKern==ETrue) returned %d\n"),r);
					return r;
					}
				else
					r=KErrNone; // Ensure misleading result is not returned
				}
#ifdef _DEBUG
			else
				{
				gTest.Printf(_L("GetInfoOnClientsUsingResource (aIncludeKern==ETrue) returned KErrNone, info follows\n"));
				for(TUint i=0;i<resourceClients;i++)
					{
					TClientInfoBuf* currInfoBuf = infoPtrs[i];
					TClientInfo currInfo=(*currInfoBuf)();
					TBuf16<sizeof(TClientName)> name;
					name.Copy(currInfo.iName);
					gTest.Printf(_L("Client name %d = %S, ID=0x%x\n"),i,&name,currInfo.iId);
					}
				}
#endif
#else
			if(r == KErrNone)
				{
				gTest.Printf(_L("GetInfoOnClientsUsingResource (aIncludeKern==ETrue) returned KErrNone"));
				return KErrGeneral;
				}
			else if(r==KErrPermissionDenied)
				{
				gTest.Printf(_L("GetInfoOnClientsUsingResource (aIncludeKern==ETrue) returned KErrPermissionDenied\n"));
				r=KErrNone; // Ensure that misleading result is not propagated
				}
			else
				{
				gTest.Printf(_L("GetInfoOnClientsUsingResource (aIncludeKern==ETrue) returned %d\n"),r);
				// If there are no clients that have requested a level then the Resource Controller
				// will report a request for information on 0 clients as a bad argument
				if(!((resourceClients==0)&&(r==KErrArgument)))
					return r;
				}
#endif
			}
		}

	__KHEAP_MARKEND;

	return r;
	}



LOCAL_C TInt HelperResourcesInUseByClient()
//
// Helper method to supportTestGetClientGetResourceInfo
// Invokes GetNumResourcesInUseByClient and GetInfoOnResourcesInUseByClient
//
	{
	__KHEAP_MARK;

	TInt r = KErrNone;
	//
	// GetNumResourcesInUseByClient
	//
	// API accepts 8-bit descriptors, only - so convert name accordingly
	TBuf8<MAX_RESOURCE_NAME_LENGTH+1>name8Bit;
	name8Bit.Copy(gTestName);
	TClientName* clientName = (TClientName*)&name8Bit;
#if _DEBUG
	TBuf <MAX_CLIENT_NAME_LENGTH> clientName16Bit;
	clientName16Bit.Copy(*clientName);
	clientName16Bit.SetLength(clientName->Length());
	gTest.Printf(_L("Invoking GetNumResourcesInUseByClient with %S (expect KErrPermissionDenied if no levels requested yet)\n"),&clientName16Bit);
#endif
	TUint numResourcesForClient;
	if((r=gChannel.GetNumResourcesInUseByClient(*clientName, numResourcesForClient)) != KErrNone)
		{
		gTest.Printf(_L("GetNumResourcesInUseByClient returned %d\n"),r);
		return r;
		}
	gTest.Printf(_L("GetNumResourcesInUseByClient gave number of resources = %d\n"),numResourcesForClient);
	//
	// In addition, check response when the name of an unknown client is passed
	//
	// Negative test - ensure that an unknown client name fails
	_LIT(dumName,"DoesNotExist");
	TBuf<16> dumNameBuf(dumName);
	TBuf8<MAX_RESOURCE_NAME_LENGTH+1>dumName8Bit;
	dumName8Bit.Copy(dumNameBuf);
	TClientName* dumClientName = (TClientName*)&dumName8Bit;
#if _DEBUG
	gTest.Printf(_L("Invoking GetNumResourcesInUseByClient with %S\n"),&dumNameBuf);
#endif
	TUint numResForDumClient;
	if((r=gChannel.GetNumResourcesInUseByClient(*dumClientName, numResForDumClient)) != KErrNotFound)
		{
		gTest.Printf(_L("GetNumResourcesInUseByClient returned %d\n"),r);
		if(r==KErrNone)
			r=KErrGeneral;
		return r;
		}
	gTest.Printf(_L("GetNumResourcesInUseByClient returned %d\n"),r);
	r=KErrNone;	// Ensure misleading error code is not propagated

	//
	// GetInfoOnResourcesInUseByClient
	//
	// If the (TUint) number of resources in use by the client is zero skip the attempt to read the resource information
	TUint updatedNumResourcesForClient = numResourcesForClient;
	if(numResourcesForClient!=0)
		{
		TUint bufSize = numResourcesForClient;
		RSimplePointerArray<TResourceInfoBuf> infoPtrs(bufSize);
		for(TUint i=0;i<bufSize;i++)
			{
			TResourceInfoBuf *info = new TResourceInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("GetInfoOnResourcesInUseByClient infoPtrs.Insert at index %d returned %d\n"),i,r);
				}
			}

		if((r=gChannel.GetInfoOnResourcesInUseByClient(*clientName, updatedNumResourcesForClient, &infoPtrs)) != KErrNone)
			{
			gTest.Printf(_L("GetInfoOnResourcesInUseByClient returned %d\n"),r);
			// If there are no clients that have requested a level then the Resource Controller
			// will report a request for information on 0 clients as a bad argument
			if(!((updatedNumResourcesForClient==0)&&(r==KErrArgument)))
				return r;
			else
				r=KErrNone; // Ensure misleading value is not returned by this function
			}
		else
			{
			gTest.Printf(_L("GetInfoOnResourcesInUseByClient gave updated number of resources %d\n"),updatedNumResourcesForClient);
#ifdef _DEBUG
			// Print resource names
			TBuf16<MAX_RESOURCE_NAME_LENGTH+1>name;
			for(TUint i=0; i<updatedNumResourcesForClient; i++)
				{
				TResourceInfoBuf* currRes = infoPtrs[i];
				name.Copy((*currRes)().iName);
				name.PtrZ();
				gTest.Printf(_L("Resource %d name = %S \n"),i,&name);
				};
#endif
			}
		//
		// In addition, check response when the name of an unknown client is passed
		//
		// Negative test - ensure that an unknown client name fails
		r=gChannel.GetInfoOnResourcesInUseByClient(*dumClientName, updatedNumResourcesForClient, &infoPtrs);
		gTest.Printf(_L("GetInfoOnResourcesInUseByClient for dummy client returned %d\n"),r);
		if(r==KErrNone)
			return KErrGeneral;
		else if(r!=KErrNotFound)
			return r;
		// Ensure that misleading information is not returned to the calling function
		r=KErrNone;

		infoPtrs.Close();
		}
	
	__KHEAP_MARKEND;

	return r;
	}


LOCAL_C TInt HelperGetClientResourceInfo()
//
// Test methods to access information about clients and resources
//
	{
	__KHEAP_MARK;

	TInt r = KErrNone;
	// Invokes GetNoOfClients and GetNamesAllClients
	if((r=HelperClients())!=KErrNone)
		return r;

	// Invokes GetNumClientsUsingResource and GetInfoOnClientsUsingResource
	//
	// First invoke on the Async resource
	TUint resourceId = 1;	// Arbitrary
	if(gHaveAsyncRes)
		{
		resourceId = gLongLatencyResource;
		gTest.Printf(_L("Invoking HelperClientsUsinResource for Async resource ID %d\n"),resourceId);
		}
	else
		{
		gTest.Printf(_L("Invoking HelperClientsUsinResource for default resource ID %d (Async resource not yet accessed)\n"),resourceId);
		}
	if((r=HelperClientsUsingResource(resourceId))!=KErrNone)
		return r;
	//
	// Second invoke on the Shared resource - skip if not available
	if(gHaveSharedRes)
		{
		resourceId = gSharedResource;
		gTest.Printf(_L("Invoking HelperClientsUsinResource for Shared resource ID %d\n"),resourceId);
		if((r=HelperClientsUsingResource(resourceId))!=KErrNone)
			return r;
		}

	// Invokes GetNumResourcesInUseByClient and GetInfoOnResourcesInUseByClient
	if((r=HelperResourcesInUseByClient())!=KErrNone)
		return r;

	__KHEAP_MARKEND;

	return r;
	}

#ifdef _DEBUG
LOCAL_C TInt SetAsyncResource()
//
// Support function for tests of asynchronous API methods
//
	{
	if(!gHaveAsyncRes)
		{
		gTest.Printf(_L("SetAsyncResource, Find Async resource to use\n"));
		TRequestStatus status;
		TBool cached = gUseCached;
		TInt readValue = 0;
		TInt levelOwnerId = 0;
		TUint numPotentialResources;
		TUint index=0;
		TInt r=gChannel.GetNumCandidateAsyncResources(numPotentialResources);
		if(r!=KErrNone)
			{
			gTest.Printf(_L("SetAsyncResource, GetNumCandidateAsyncResources returned %d\n"),r);
			return r;
			}
		gTest.Printf(_L("SetAsyncResource, GetNumCandidateAsyncResources found %d resources\n"),numPotentialResources);
		while((numPotentialResources>0) && !gHaveAsyncRes)
			{
			TUint tryResourceId=0;
			r=gChannel.GetCandidateAsyncResourceId(index,tryResourceId);
			if(r!=KErrNone)
				{
				gTest.Printf(_L("SetAsyncResource, GetCandidateAsyncResourceId returned %d\n"),r);
				break;
				}
			gTest.Printf(_L("SetAsyncResource, GetNumCandidateAsyncResources index %d, resource ID %d\n"),index,tryResourceId);
			// For the candidate resource to be usable, we need its current state
			// to be sufficiently less the maximum for positive sense (or sufficiently
			// more than the greater than the minimum for negative sense - but the current 
			// version of the code only considers positive sense).
			gChannel.GetResourceState(status,tryResourceId,cached,&readValue,&levelOwnerId);
			User::WaitForRequest(status);
			if(status.Int() != KErrNone)
				{
				gTest.Printf(_L("SetAsyncResource, candidate get state returned %d\n"),r);
				return r;
				}
			gTest.Printf(_L("SetAsyncResource, candidate get state gave %d, levelOwnerId = %d\n"),readValue,levelOwnerId);
			TResourceInfoBuf buffer;
			if((r=gChannel.GetResourceInfo(tryResourceId, &buffer))!=KErrNone)
				{
				gTest.Printf(_L("SetAsyncResource, candidate get resource info returned %d\n"),r);
				return r;
				}
			// Print resource info
			TBuf16<MAX_RESOURCE_NAME_LENGTH+1>name;
			TResourceInfo* infoPtr = &(buffer());
			name.Copy(infoPtr->iName);
			gTest.Printf(_L("SetAsyncResource: Resource name = %S \n"),&name);
			gTest.Printf(_L("SetAsyncResource: Resource Class =%d\n"),infoPtr->iClass);
			gTest.Printf(_L("SetAsyncResource: Resource Type =%d\n"), infoPtr->iType);
			gTest.Printf(_L("SetAsyncResource: Resource Usage =%d\n"), infoPtr->iUsage);
			gTest.Printf(_L("SetAsyncResource: Resource Sense =%d\n"), infoPtr->iSense);
			gTest.Printf(_L("SetAsyncResource: Resource MinLevel =%d\n"),infoPtr->iMinLevel);
			gTest.Printf(_L("SetAsyncResource: Resource MaxLevel =%d\n"),infoPtr->iMaxLevel);

			if((infoPtr->iMaxLevel - readValue) > LEVEL_GAP_REQUIRED_FOR_ASYNC_TESTING)
				{
				gLongLatencyResource = tryResourceId;
				gAsyncResStateDelta = 1;  // Will change resource level in positive direction
				gHaveAsyncRes = ETrue;
				}
			else if((readValue - infoPtr->iMinLevel) > LEVEL_GAP_REQUIRED_FOR_ASYNC_TESTING)
				{
				gLongLatencyResource = tryResourceId;
				gAsyncResStateDelta = -1;  // Will change resource level in negative direction
				gHaveAsyncRes = ETrue;
				}
			else
				{
				++index;
				--numPotentialResources;
				}
			};
		}
	if(!gHaveAsyncRes)
		{
	    gTest.Printf(_L("**Test SetAsyncResource - don't have suitable resource ... exiting\n"));
		return KErrNotReady;
		}

	return KErrNone;
	}

LOCAL_C TInt SetSharedResource()
//
// Support function for tests of shareable resources
//
	{
	__KHEAP_MARK;

	if(!gHaveSharedRes)
		{
		TRequestStatus status;
		TBool cached = gUseCached;
		TInt readValue = 0;
		TUint numPotentialResources;
		TUint index=0;
		TInt r=gChannel.GetNumCandidateSharedResources(numPotentialResources);
		if(r!=KErrNone)
			{
			gTest.Printf(_L("SetSharedResource, GetNumCandidateSharedResources returned %d\n"),r);
			return r;
			}
		gTest.Printf(_L("SetSharedResource, GetNumCandidateSharedResources found %d resources\n"),numPotentialResources);
		while((numPotentialResources>0) && !gHaveSharedRes)
			{
			TUint tryResourceId=0;
			r=gChannel.GetCandidateSharedResourceId(index,tryResourceId);
			if(r!=KErrNone)
				{
				gTest.Printf(_L("SetSharedResource, GetCandidateSharedResourceId returned %d\n"),r);
				break;
				}
			gTest.Printf(_L("SetSharedResource, GetNumCandidateSharedResources index %d, resource ID %d\n"),index,tryResourceId);
			// To support the tests, the selected shareable resource must not be the same
			// resource as that selected for asynchronous testing
			if(gHaveAsyncRes)
				if(tryResourceId==gLongLatencyResource)
					{
					gTest.Printf(_L("SetSharedResource - skipping candidate resource %d - already used for async testing\n"),tryResourceId);
					continue;
					}
			// For the candidate resource to be usable, we need its current state
			// to be sufficiently less the maximum for positive sense (or sufficiently
			// more than the greater than the minimum for negative sense - but the current 
			// version of the code only considers positive sense).
			TInt levelOwnerId = 0;
			gChannel.GetResourceState(status,tryResourceId,cached,&readValue,&levelOwnerId);
			User::WaitForRequest(status);
			if(status.Int() != KErrNone)
				{
				gTest.Printf(_L("SetSharedResource, candidate get state returned %d\n"),r);
				return r;
				}
			gTest.Printf(_L("SetSharedResource, candidate get state gave %d, levelOwnerId = %d\n"),readValue,levelOwnerId);
			TResourceInfoBuf buffer;
			if((r=gChannel.GetResourceInfo(tryResourceId, &buffer))!=KErrNone)
				{
				gTest.Printf(_L("SetSharedResource, candidate get resource info returned %d\n"),r);
				return r;
				}
			// Print resource info
			TBuf16<MAX_RESOURCE_NAME_LENGTH+1>name;
			TResourceInfo* infoPtr = &buffer();
			name.Copy(infoPtr->iName);
			gTest.Printf(_L("SetSharedResource: Resource name = %S \n"),&name);
			gTest.Printf(_L("SetSharedResource: Resource Class =%d\n"),infoPtr->iClass);
			gTest.Printf(_L("SetSharedResource: Resource Type =%d\n"), infoPtr->iType);
			gTest.Printf(_L("SetSharedResource: Resource Usage =%d\n"), infoPtr->iUsage);
			gTest.Printf(_L("SetSharedResource: Resource Sense =%d\n"), infoPtr->iSense);
			gTest.Printf(_L("SetSharedResource: Resource MinLevel =%d\n"),infoPtr->iMinLevel);
			gTest.Printf(_L("SetSharedResource: Resource MaxLevel =%d\n"),infoPtr->iMaxLevel);

			if((infoPtr->iMaxLevel - readValue) > LEVEL_GAP_REQUIRED_FOR_ASYNC_TESTING)
				{
				gSharedResource = tryResourceId;
				gSharedResStateDelta = 1;  // Will change resource level in positive direction
				gHaveSharedRes = ETrue;
				}
			else if((readValue - infoPtr->iMinLevel) > LEVEL_GAP_REQUIRED_FOR_ASYNC_TESTING)
				{
				gSharedResource = tryResourceId;
				gSharedResStateDelta = -1;  // Will change resource level in negative direction
				gHaveSharedRes = ETrue;
				}
			else
				{
				++index;
				--numPotentialResources;
				}
			};
		}
	if(!gHaveSharedRes)
		{
	    gTest.Printf(_L("**Test SetSharedResource - don't have suitable resource ... exiting\n"));
		return KErrNotReady;
		}

	__KHEAP_MARKEND;
	return KErrNone;
	}

#endif


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_RESMANUS-0609
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1398
//! @SYMTestCaseDesc    This test case tests APIs for retrieving information about 
//!						(1) clients of the channel and 
//!						(2) power resources. 
//!						Since the client lacks the ReadDeviceData PlatSec capability it will not
//!						be permitted to access information about kernel-side clients
//! 
//!						The tests are invoked a number of times:
//!						-	first, to examine the starting state
//!						-	then, to examine the effect of adding a new client (channel)
//!						-	then, the examine the effect of adding a new client that requests a
//|							level on a resource
//!						-	then, to test the effect of the original client requesting a level 
//!							on a resource
//! 
//! @SYMTestActions     0) Call GetNoOfClients API with default aIncludeKern=EFalse.
//! 
//!						1) Call GetNoOfClients API with aIncludeKern=ETrue.
//!
//!						2) Call GetNamesAllClients API with default aIncludeKern=EFalse.
//!
//!						3) Call GetNamesAllClients API with aIncludeKern=ETrue.
//!
//!						4) Call GetNumClientsUsingResource API with aIncludeKern=ETrue.
//!
//!						5) Call GetNumClientsUsingResource API with default aIncludeKern=EFalse.
//!
//!						6) Call GetInfoOnClientsUsingResource API with default aIncludeKern=EFalse.
//!
//!						7) Call GetInfoOnClientsUsingResource API with aIncludeKern=ETrue.
//!
//!						8) GetNumResourcesInUseByClient for the original client
//!
//!						9) GetNumResourcesInUseByClient for a non-existent client
//!
//!						10) GetInfoOnResourcesInUseByClient for the original client
//!
//!						11) GetInfoOnResourcesInUseByClient for a non-existent client
//!
//! @SYMTestExpectedResults 0) API should return with KErrNone, exits otherwise.
//!
//!						1) If client exhibits PlatSec capability ReadDeviceData, API should return with KErrNone, exits otherwise.
//!						   If client lacks PlatSec capability ReadDeviceData, API should return with KErrPermissionDenied, exits otherwise.
//!
//!						2) API should return with KErrNone, exits otherwise.
//!
//!						3) If client exhibits PlatSec capability ReadDeviceData, API should return with KErrNone, exits otherwise
//!						   If client lacks PlatSec capability ReadDeviceData, API should return with KErrPermissionDenied, exits otherwise
//!
//!						4) If client exhibits PlatSec capability ReadDeviceData, API should return with KErrNone, exits otherwise.
//!						   If client lacks PlatSec capability ReadDeviceData, API should return with KErrPermissionDenied, exits otherwise.
//!
//!						5) API should return with KErrNone, exits otherwise
//!
//!						6) API should return with KErrNone, exits otherwise.
//!
//!						7) If client exhibits PlatSec capability ReadDeviceData, API should return with KErrNone, exits otherwise
//!						   If client lacks PlatSec capability ReadDeviceData, API should return with KErrPermissionDenied, exits otherwise
//!
//!						8) API should return with KErrNone, exits otherwise.
//!
//!						9) API should return with KErrNotFound, exits otherwise.
//!
//!						10) API should return with KErrNone, exits otherwise.
//!
//!						11) API should return with KErrNotFound, exits otherwise.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt TestGetClientGetResourceInfo()
//
// Test methods to access information about clients and resources
//
	{
	TInt r = KErrNone;

	// Open a couple of additional channels  
	_LIT(tempStr1,"ExtraChan1");
	TBufC<16> tempName1(tempStr1);
	_LIT(tempStr2,"ExtraChan2");
	TBufC<16> tempName2(tempStr2);

	RBusDevResManUs channelTwo;
	RBusDevResManUs channelThree;

	if((r=OpenChannel(tempName1, channelTwo))!=KErrNone)
		return r;
	if((r=OpenChannel(tempName2, channelThree))!=KErrNone)
		return r;

	// Test the tracking of the client and resource info
	//

	// First invocation to establish starting state
#ifdef _DEBUG
	gTest.Printf(_L("TestGetClientGetResourceInfo: First HelperGetClientResourceInfo call (starting state):\n"));
#endif
	if((r=HelperGetClientResourceInfo())!=KErrNone)
		return r;

	// Second invocation - examine effect of adding a client
	_LIT(tempStr3,"ExtraChan3");
	TBufC<16> tempName3(tempStr3);
	RBusDevResManUs channelFour;
	if((r=OpenChannel(tempName3, channelFour))!=KErrNone)
		return r;
#ifdef _DEBUG
	gTest.Printf(_L("TestGetClientGetResourceInfo: Second HelperGetClientResourceInfo call (added client ExtraChan3):\n"));
#endif

	if((r=HelperGetClientResourceInfo())!=KErrNone)
		return r;

	// Third invocation  - examine effect of new client requesting a level for a resource
	// (This relies on getting and setting the state of gSharedResource - so skip the 
	// test if this has not yet been identified
	//
	TUint startingLevel = 0;
#ifdef _DEBUG
	if((r=SetSharedResource())!=KErrNone)
		return r;
#endif
	if(!gHaveSharedRes)
		{
		gTest.Printf(_L("TestGetClientGetResourceInfo: no suitable shareable resource, so skipping third call:\n"));
		}
	else
		{
		// Channel registration
		gTest.Printf(_L("Initialise for temporary channel with arguments 1,1,0\n"));
		if ((r=channelFour.Initialise(1,1,0))!=KErrNone)  // Just need 1 get and 1 set state
			{
			gTest.Printf(_L("Initialise for channel returned %d\n"),r);
			return r;
			}
		// Get initial state
		TRequestStatus status;
		TBool cached = gUseCached;
		TInt readValue;
		TInt levelOwnerId = 0;
		channelFour.GetResourceState(status,gSharedResource,cached,&readValue,&levelOwnerId);
		User::WaitForRequest(status);
		r=status.Int();
		if(r != KErrNone)
			{
			gTest.Printf(_L("TestGetClientGetResourceInfo, first get state for shareable returned %d\n"),r);
			return r;
			}
		startingLevel = (TUint)readValue;
		// Write updated state
		TUint newLevel = (TUint)(readValue + gSharedResStateDelta);
		gTest.Printf(_L("TestGetClientGetResourceInfo: levelOwnerId = %d\n"), levelOwnerId);
		gTest.Printf(_L("TestGetClientGetResourceInfo: shareable resource startingLevel=0x%x, writing 0x%x\n"), startingLevel, newLevel);
		channelFour.ChangeResourceState(status,gSharedResource,newLevel);
		User::WaitForRequest(status);
		r=status.Int();
		if(r != KErrNone)
			{
			gTest.Printf(_L("TestGetClientGetResourceInfo, first change state for shareable resource returned %d\n"),r);
			return r;
			}
#ifdef _DEBUG
		gTest.Printf(_L("TestGetClientGetResourceInfo: third HelperGetClientResourceInfo call (new client set level on shared resource):\n"));
#endif
		if((r=HelperGetClientResourceInfo())!=KErrNone)
			return r;
		}


	// Fourth invocation - examine effect of oryginal client requesting a level for 
	// the Shared resource
	if(gHaveSharedRes)
		{
		TRequestStatus status;
		TBool cached = gUseCached;
		TInt readValue;
		TInt levelOwnerId;
		gChannel.GetResourceState(status,gSharedResource,cached,&readValue,&levelOwnerId);
		User::WaitForRequest(status);
		r=status.Int();
		if(r != KErrNone)
			{
			gTest.Printf(_L("TestGetClientGetResourceInfo, gChannel get state on Shareable resource returned %d\n"),r);
			return r;
			}
		gTest.Printf(_L("TestGetClientGetResourceInfo, GetResourceState levelOwnerId =  %d\n"),levelOwnerId);		// Request a level on the resource
		gChannel.ChangeResourceState(status,gSharedResource,(readValue+gSharedResStateDelta));
		User::WaitForRequest(status);
		if(status.Int() != KErrNone)
			{
			gTest.Printf(_L("TestGetClientGetResourceInfo, gChannel change state on Shareable returned %d\n"),r);
			return r;
			}
#ifdef _DEBUG
	gTest.Printf(_L("TestGetClientGetResourceInfo: fourth HelperGetClientResourceInfo call (gChannel set level on Shareable resource):\n"));
#endif
		if((r=HelperGetClientResourceInfo())!=KErrNone)
		return r;
		}

	// Return the resource to the state it was on function entry
	if(gHaveSharedRes)
		{
		TRequestStatus status;
		gTest.Printf(_L("TestGetClientGetResourceInfo: returning sharable resource to startingLevel=0x%x\n"), startingLevel);
		gChannel.ChangeResourceState(status,gSharedResource,startingLevel);
		User::WaitForRequest(status);
		r=status.Int();
		if(r != KErrNone)
			{
			gTest.Printf(_L("TestGetClientGetResourceInfo, attempt to reset shareable resource state returned %d\n"),r);
			return r;
			}
		}

	// Close the temporary channels
	channelTwo.Close();
	channelThree.Close();
	channelFour.Close();

	return r;
	}



//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_RESMANUS-0610
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1398
//! @SYMTestCaseDesc    This test case tests APIs for getting and setting the state of resources; 
//!						it also tests APIs to cancel such requests.
//! 
//! @SYMTestActions     0) Call API to get the initial state of a selected resource.
//! 
//!						1) Call API to modify the state of the resource.
//! 
//!						2) Call API to get the new state of the resource and check it exhibits
//!						the expected value.
//!
//!						3) Call API to modify the state of the resource by setting its level to zero.
//!
//!						4) Call API to get the state of the resource and check it exhibits the expected zero value.
//! 
//!						5) Call API to return the resource state to its original value.
//! 
//!						6) Call API to get the state of a long latency resource then call API
//!						with operation-type qualifier cancel the request.
//! 
//!						7) Call API to modify the state of the long latency resource then call API
//!						with operation-type qualifier to cancel the request.
//! 
//!						8) Call API to get the state of a long latency resource and wait for it 
//!						to complete. Then call API with operation-type qualifier to cancel the request.
//! 
//!						9) Call API to modify the state of the long latency resource and wait for
//!						it to complete. Then call API with operation-type qualifier to cancel the request.
//! 
//!						10) Call API to get the state of a long latency resource then call API
//!						without operation-type qualifier to cancel the request.
//! 
//!						11) Call API to modify the state of the long latency resource then call API
//!						without operation-type qualifier to cancel the request.
//! 
//!						12) Call API to get the state of a long latency resource and wait for it 
//!						to complete. Then call API without operation-type qualifier to cancel the request.
//! 
//!						13) Call API to modify the state of the long latency resource and wait for
//!						it to complete. Then call API without operation-type qualifier to cancel the request.
//! 
//!						14) Call API to get the state of a long latency resource 'n' times. Then call API with 
//!						resource qualifier to cancel the requests.
//! 
//!						15) Call API to modify the state of a long latency resource 'm' times. Then call API with 
//!						resource qualifier to cancel the requests.
//! 
//!						16) Call API to get the state of a long latency resource 'n' times and wait for them to complete.
//!						Then call API with resource qualifier to cancel the requests.
//! 
//!						17) Call API to modify the state of a long latency resource 'm' times and wait for them to complete.
//!						Then call API with resource qualifier to cancel the requests.
//! 
//!						18) Call API to get the state of a long latency resource 'n' times, call API to modify the state of
//!						a long latency resource 'm' times. Call the API to cancel the get operations with resource qualifier.
//!						Wait for the operations to complete. Check the state of the associated TRequestStatus objects.
//! 
//!						19) Call API to get the state of a long latency resource 'n' times, call API to modify the state of
//!						a long latency resource 'm' times. Call the API to cancel the modify operations with resource qualifier.
//!						Wait for the get operations to complete. Check the state of the associated TRequestStatus objects.
//! 
//!						20) Call API to get the state of a long latency resource 'n' times, call API to modify the state of
//!						a long latency resource 'm' times. Wait for the get operations to complete. Call the API to cancel the get
//!						operations with resource qualifier. Check the state of the associated TRequestStatus objects.
//! 
//!						21) Call API to get the state of a long latency resource 'n' times, call API to modify the state of
//!						a long latency resource 'm' times. Wait for the modify operations to complete. Call the API to cancel the modify
//!						operations with resource qualifier. Check the state of the associated TRequestStatus objects.
//! 
//!						22) Call API to get the state of a long latency resource 'n' times, call API to modify the state of
//!						a long latency resource 'm' times.
//!						Then call API with operation-type qualifier to cancel the even-numbered get request(s).
//!						Then call API without operation-type qualifier to cancel the even-numbered modify request(s).
//!						Check the state of the associated TRequestStatus objects.
//! 
//! @SYMTestExpectedResults 0) The associated TRequestStatus object should indicate KErrNone, exits otherwise.
//! 
//!						1) The associated TRequestStatus object should indicate KErrNone, exits otherwise.
//! 
//!						2) The associated TRequestStatus object should indicate KErrNone, exits otherwise.
//!						Exit if the value read back is not as expected.
//! 
//!						3) The associated TRequestStatus object should indicate KErrNone, exits otherwise.
//! 
//!						4) The associated TRequestStatus object should indicate KErrNone, exits otherwise.
//!						Exit if the value read back is not as expected.
//! 
//!						5) The associated TRequestStatus object should indicate KErrNone, exits otherwise.
//! 
//!						6) The associated TRequestStatus object should indicate KErrCancel if the cancel 
//!						request was accepted, exits otherwise.
//!						The associated TRequestStatus object should indicate KErrNone if the cancel request
//!						was not accepted, exits otherwise.
//! 
//!						7) The associated TRequestStatus object should indicate KErrCancel if the cancel 
//!						request was accepted, exits otherwise.
//!						The associated TRequestStatus object should indicate KErrNone if the cancel request
//!						was not accepted, exits otherwise.
//! 
//!						8) The TRequestStatus object associated with the get operation should indicate 
//!						KErrNone - exits otherwise. The TRequestStatus object associated with the cancel
//!						operation should indicate KErrNone - exits otherwise.
//! 
//!						9) The TRequestStatus object associated with the get operation should indicate
//!						KErrNone - exits otherwise. The TRequestStatus object associated with the cancel
//!						operation should indicate KErrNone - exits otherwise.
//! 
//!						10) The associated TRequestStatus object should indicate KErrCancel, exits otherwise.
//! 
//!						11) The associated TRequestStatus object should indicate KErrCancel, exits otherwise.
//! 
//!						12) The TRequestStatus object associated with the get operation should indicate 
//!						KErrNone - exits otherwise. The TRequestStatus object associated with the cancel
//!						operation should indicate KErrNone - exits otherwise.
//! 
//!						13) The TRequestStatus object associated with the get operation should indicate
//!						KErrNone - exits otherwise. The TRequestStatus object associated with the cancel
//!						operation should indicate KErrNone - exits otherwise.
//!
//!						14) The TRequestStatus objects should all exibit KErrCancel - exits otherwise.
//! 
//!						15) The TRequestStatus objects should all exibit KErrCancel - exits otherwise.
//! 
//!						16) The TRequestStatus objects associated with the get operations should all exibit KErrNone - exits otherwise.
//!						The TRequestStatus objects associated with the cancel operations should all exibit KErrNone - exits otherwise
//! 
//!						17) The TRequestStatus objects associated with the modify operations should all exibit KErrNone - exits otherwise.
//!						The TRequestStatus objects associated with the cancel operations should all exibit KErrNone - exits otherwise
//! 
//!						18) The TRequestStatus objects associated with the get operations should all exibit KErrCancel - exits otherwise.
//!						The TRequestStatus objects associated with the modify operations should all exibit KErrNone - exits otherwise
//! 
//!						19) The TRequestStatus objects associated with the get operations should all exibit KErrNone - exits otherwise.
//!						The TRequestStatus objects associated with the modify operations should all exibit KErrCancel - exits otherwise
//! 
//!						20) The TRequestStatus objects associated with the get and modify operations should all exibit KErrNone - exits otherwise.
//! 
//!						21) The TRequestStatus objects associated with the get and modify operations should all exibit KErrNone - exits otherwise.
//!
//!						22) The TRequestStatus objects associated with the even-numbered request should exhibit KErrCancel.
//!						The TRequestStatus objects associated with the odd-numbered request should exhibit KErrNone.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt TestGetSetResourceStateOps()
//
// Test resource state access methods
//
	{
	TInt r = KErrNone;

	TRequestStatus status;
	TRequestStatus status2;
	TBool cached = gUseCached;
	TInt readValue = 0;
	TInt readValue2 = 0;
	TInt levelOwnerId = 0;
	TInt testNo = 0;

#ifdef _DEBUG
	// Ensure we have a resource we can use
	if((r=SetAsyncResource())!=KErrNone)
		return r;
#endif

	// 0) Call API to get the initial state of a selected resource.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	gChannel.GetResourceState(status,gLongLatencyResource,cached,&readValue,&levelOwnerId);
	User::WaitForRequest(status);
	if(status.Int() != KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, get state status = %d\n"),r);
		return r;
		}
	TUint startingLevel = (TUint)readValue;
	gTest.Printf(_L("TestGetSetResourceStateOps: initial level read =0x%x, levelOwnerId = %d\n"),readValue,levelOwnerId);

	// 1) Call API to modify the state of the resource.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	TUint newLevel = (TUint)(readValue + gAsyncResStateDelta);
	gTest.Printf(_L("TestGetSetResourceStateOps: writing 0x%x\n"), newLevel);
	gChannel.ChangeResourceState(status,gLongLatencyResource,newLevel);
	User::WaitForRequest(status);
	if(status.Int() != KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, first change state returned %d\n"),r);
		return r;
		}

	// 2) Call API to get the new state of the resource and check it exhibits the expected value.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	gChannel.GetResourceState(status,gLongLatencyResource,cached,&readValue,&levelOwnerId);
	User::WaitForRequest(status);
	if(status.Int() != KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, get state status = %d\n"),r);
		return r;
		}
	gTest.Printf(_L("TestGetSetResourceStateOps: level read back =0x%x, levelOwnerId=%d\n"),readValue,levelOwnerId);
	gTest(newLevel==(TUint)readValue);

	// 3) Call API to modify the state of the resource by setting its level to zero
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	gChannel.ChangeResourceState(status,gLongLatencyResource,0);
	User::WaitForRequest(status);
	if(status != KErrNone)
		{
		gTest.Printf(_L("ChangeResourceState to level 0 returned %d\n"),r);
		return r;
		}

	// 4) Call API to get the state of the resource and check it exhibits the expected zero value.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	gChannel.GetResourceState(status,gLongLatencyResource,cached,&readValue,&levelOwnerId);
	User::WaitForRequest(status);
	if(status != KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, get state status = %d\n"),r);
		return r;
		}
	gTest.Printf(_L("TestGetSetResourceStateOps: level read back =0x%x, levelOwnerId=%d\n"),readValue,levelOwnerId);
	gTest(readValue==0);

	// 5) Call API to return the resource state to its original value.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	gTest.Printf(_L("TestGetSetResourceStateOps: write original level 0x%x\n"), startingLevel);
	gChannel.ChangeResourceState(status,gLongLatencyResource,startingLevel);
	User::WaitForRequest(status);
	if(status.Int() != KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, change state status = %d\n"),r);
		return r;
		}
	gChannel.GetResourceState(status,gLongLatencyResource,cached,&readValue,&levelOwnerId);
	User::WaitForRequest(status);
	if(status.Int() != KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, get state status = %d\n"),r);
		return r;
		}
	gTest.Printf(_L("TestGetSetResourceStateOps: check original level read back =0x%x, levelOwnerId=%d\n"),readValue,levelOwnerId);

	// 6) Call API to get the state of a long latency resource then call API with operation-type qualifier cancel the request.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	gChannel.GetResourceState(status,gLongLatencyResource,cached,&readValue,&levelOwnerId);
	r=gChannel.CancelGetResourceState(status);
	if(r!=KErrInUse)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, CancelGetResourceState returned %d\n"),r);
		}
	if(r!=KErrCompletion) // If request had not completed before cancellation request
		{
		User::WaitForRequest(status);
		if(r==KErrNone)	// Cancel expected to proceed as requested
			{
			if(status.Int() != KErrCancel)
				{
				gTest.Printf(_L("TestGetSetResourceStateOps, expected KErrCancel but cancelled get state status = %d\n"),r);
				return r;
				}
			}
		else if(r==KErrInUse)	// Cancel failed since request was being processed - so expect successful completion
			{
			if(status.Int() != KErrNone)
				{
				gTest.Printf(_L("TestGetSetResourceStateOps, expected KErrNone but cancelled get state status = %d\n"),r);
				return r;
				}
			}
		else if(status.Int() != KErrCancel)	// Just report the error code and return
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, cancelled get state status = %d\n"),r);
			return r;
			}

		}

	// 7) Call API to modify the state of the long latency resource then call API with operation-type qualifier to cancel the request.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	newLevel = (TUint)(readValue + gAsyncResStateDelta);
	gChannel.ChangeResourceState(status,gLongLatencyResource,newLevel);
	r=gChannel.CancelChangeResourceState(status);
	if(r!=KErrInUse)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, CancelChangeResourceState returned %d\n"),r);
		}
	if(r!=KErrCompletion) // If request had not completed before cancellation request
		{
		User::WaitForRequest(status);
		if(r==KErrNone)	// Cancel expected to proceed as requested
			{
			if(status.Int() != KErrCancel)
				{
				gTest.Printf(_L("TestGetSetResourceStateOps, expected KErrCancel but cancelled get state status = %d\n"),r);
				return r;
				}
			}
		else if(r==KErrInUse)	// Cancel failed since request was being processed - so expect successful completion
			{
			if(status.Int() != KErrNone)
				{
				gTest.Printf(_L("TestGetSetResourceStateOps, expected KErrNone but cancelled get state status = %d\n"),r);
				return r;
				}
			}
		else if(status.Int() != KErrCancel)	// Just report the error code and return
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, cancelled get state status = %d\n"),r);
			return r;
			}
		}


	// 8) Call API to get the state of a long latency resource and wait for it to complete.
	//    Then call API with operation-type qualifier to cancel the request.	
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	gChannel.GetResourceState(status,gLongLatencyResource,cached,&readValue,&levelOwnerId);
	User::WaitForRequest(status);
	if(status.Int() != KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, get state status = %d\n"),r);
		return r;
		}
	r=gChannel.CancelGetResourceState(status);
	if(r!=KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, CancelGetResourceState returned %d\n"),r);
		}
	if(status.Int() != KErrNone)	// TRequestStatus should be unchanged
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, completed-then-cancelled get state status = %d\n"),r);
		return r;
		}

	// 9) Call API to modify the state of the long latency resource and wait for it to complete. 
	//    Then call API with operation-type qualifier to cancel the request.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	gChannel.ChangeResourceState(status,gLongLatencyResource,(readValue + gAsyncResStateDelta));
	User::WaitForRequest(status);
	if(status.Int() != KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, change state status = %d\n"),r);
		return r;
		}
	r=gChannel.CancelChangeResourceState(status);
	if(r!=KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, CancelChangeResourceState returned %d\n"),r);
		}
	if(status.Int() != KErrNone)	// TRequestStatus should be unchanged
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, completed-then-cancelled change state status = %d\n"),r);
		return r;
		}

	// 10) Call API to get the state of a long latency resource then call API without operation-type qualifier to cancel the request.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	
	// NOTE: Cancel operation can only remove request which is still inside the resource controller
	// message queue. If the queue is empty, the resource controller may process the request very quickly
	// after it is sent. It may cause the test fail. To solve this, two get long latency resource state 
	// requests are submitted. So that the second one must be inside the resource controller. 
	// And we will always test the second request
	
	gChannel.GetResourceState(status,gLongLatencyResource,cached,&readValue,&levelOwnerId);
	gChannel.GetResourceState(status2,gLongLatencyResource,cached,&readValue2,&levelOwnerId);
	gChannel.CancelAsyncOperation(&status);
	gChannel.CancelAsyncOperation(&status2);
	User::WaitForRequest(status);
	User::WaitForRequest(status2);
	if(status2.Int() != KErrCancel)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, cancelled get state status = %d\n"),r);
		return r;
		}

	// 11) Call API to modify the state of the long latency resource then call API without operation-type qualifier to cancel the request.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	
	// NOTE: Cancel operation can only remove request which is still inside the resource controller
	// message queue. If the queue is empty, the resource controller may process the request very quickly
	// after it is sent. It may cause the test fail. To solve this, two get long latency resource state 
	// requests are submitted. So that the second one must be inside the resource controller. 
	// And we will always test the second request
		
	newLevel = (TUint)(readValue + gAsyncResStateDelta);
	gChannel.ChangeResourceState(status,gLongLatencyResource,newLevel);
	gChannel.ChangeResourceState(status2,gLongLatencyResource,newLevel);
	gChannel.CancelAsyncOperation(&status);
	gChannel.CancelAsyncOperation(&status2);
	User::WaitForRequest(status);
	User::WaitForRequest(status2);
	if(status2.Int() != KErrCancel)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, cancelled change state status = %d\n"),r);
		return r;
		}

	// 12) Call API to get the state of a long latency resource and wait for it to complete.
	//     Then call API without operation-type qualifier to cancel the request.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	gChannel.GetResourceState(status,gLongLatencyResource,cached,&readValue,&levelOwnerId);
	User::WaitForRequest(status);
	if(status.Int() != KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, get state status = %d\n"),r);
		return r;
		}
	gChannel.CancelAsyncOperation(&status);
	if(status.Int() != KErrNone)	// TRequestStatus should be unchanged
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, completed-then-cancelled get state status = %d\n"),r);
		return r;
		}

	// 13) Call API to modify the state of the long latency resource and wait for it to complete. 
	//     Then call API without operation-type qualifier to cancel the request.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	gChannel.ChangeResourceState(status,gLongLatencyResource,(readValue + gAsyncResStateDelta));
	User::WaitForRequest(status);
	if(status.Int() != KErrNone)
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, change state status = %d\n"),r);
		return r;
		}
	gChannel.CancelAsyncOperation(&status);
	if(status.Int() != KErrNone)	// TRequestStatus should be unchanged
		{
		gTest.Printf(_L("TestGetSetResourceStateOps, completed-then-cancelled change state status = %d\n"),r);
		return r;
		}

	// 'n' and 'm' values and support for cancellation of multiple requests
	const TInt KLoopVarN = 2;
	const TInt KLoopVarM = 3;
	TRequestStatus getReqStatus[KLoopVarN];
	TRequestStatus setReqStatus[KLoopVarM];
	TInt i=0;

	// 14) Call API to get the state of a long latency resource 'n' times. 
	//     Then call API with resource qualifier to cancel the requests.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	for(i=0;i<KLoopVarN;i++)
		{
		gChannel.GetResourceState(getReqStatus[i],gLongLatencyResource,cached,&readValue,&levelOwnerId);
		}
	gChannel.CancelGetResourceStateRequests(gLongLatencyResource);
	for(i=0;i<KLoopVarN;i++)
		{
		User::WaitForRequest(getReqStatus[i]);

	// NOTE: Cancel operation can only remove request which is still inside the resource controller
	// message queue. If the queue is empty, the resource controller may process the request very quickly
	// after it is sent. It may cause the test fail. To solve this, we skip the test for request 0.
			
		if(i>0 && ((r=getReqStatus[i].Int()) != KErrCancel))
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, cancelled get state status[%d] = %d\n"),i,r);
			return r;
			}
		}

	// 15) Call API to modify the state of a long latency resource 'm' times. 
	//     Then call API with resource qualifier to cancel the requests.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	for(i=0;i<KLoopVarM;i++)
		{
		gChannel.ChangeResourceState(setReqStatus[i],gLongLatencyResource,(readValue + gAsyncResStateDelta));
		}
	gChannel.CancelChangeResourceStateRequests(gLongLatencyResource);
	for(i=0;i<KLoopVarM;i++)
		{
		User::WaitForRequest(setReqStatus[i]);
		
	// NOTE: Cancel operation can only remove request which is still inside the resource controller
	// message queue. If the queue is empty, the resource controller may process the request very quickly 
	// after it is sent. It may cause the test fail. To solve this, we skip the test for request 0.
		
		if(i>0 && ((r=setReqStatus[i].Int()) != KErrCancel))
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, cancelled change state status[%d] = %d\n"),i,r);
			return r;
			}
		}

	// 16) Call API to get the state of a long latency resource 'n' times and wait for them to complete.
	//     Then call API with resource qualifier to cancel the requests.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	for(i=0;i<KLoopVarN;i++)
		{
		gChannel.GetResourceState(getReqStatus[i],gLongLatencyResource,cached,&readValue,&levelOwnerId);
		}
	for(i=0;i<KLoopVarN;i++)
		{
		User::WaitForRequest(getReqStatus[i]);
		if((r=getReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, before cancel get state status[%d] = %d\n"),i,r);
			return r;
			}
		}
	gChannel.CancelGetResourceStateRequests(gLongLatencyResource);
	for(i=0;i<KLoopVarN;i++)
		{
		if((r=getReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, after cancel get state status[%d] = %d\n"),i,r);
			return r;
			}
		}

	// 17) Call API to modify the state of a long latency resource 'm' times and wait for them to complete.
	//     Then call API with resource qualifier to cancel the requests.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	for(i=0;i<KLoopVarM;i++)
		{
		gChannel.ChangeResourceState(setReqStatus[i],gLongLatencyResource,(readValue + gAsyncResStateDelta));
		}
	for(i=0;i<KLoopVarM;i++)
		{
		User::WaitForRequest(setReqStatus[i]);
		if((r=setReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, before cancel change state status[%d] = %d\n"),i,r);
			return r;
			}
		}
	gChannel.CancelChangeResourceStateRequests(gLongLatencyResource);
	for(i=0;i<KLoopVarM;i++)
		{
		if((r=setReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, after cancel change state status[%d] = %d\n"),i,r);
			return r;
			}
		}

	// 18) Call API to get the state of a long latency resource 'n' times, call API to modify the state of
	//     a long latency resource 'm' times. 
	//     Call the API to cancel the get operations with resource qualifier.
	//     Wait for the operations to complete. Check the state of the associated TRequestStatus objects.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	for(i=0;i<KLoopVarN;i++)
		{
		gChannel.GetResourceState(getReqStatus[i],gLongLatencyResource,cached,&readValue,&levelOwnerId);
		}
	for(i=0;i<KLoopVarM;i++)
		{
		gChannel.ChangeResourceState(setReqStatus[i],gLongLatencyResource,(readValue + gAsyncResStateDelta));
		}
	gChannel.CancelGetResourceStateRequests(gLongLatencyResource);
	for(i=0;i<KLoopVarN;i++)
		{
		User::WaitForRequest(getReqStatus[i]);

	// NOTE: Cancel operation can only remove request which is still inside the resource controller
	// message queue. If the queue is empty, the resource controller may process the request very quickly 
	// after it is sent. It may cause the test fail. To solve this, we skip the test for request 0.
		
		if(i>0 && ((r=getReqStatus[i].Int()) != KErrCancel))
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, cancelled get state status[%d] = %d\n"),i,r);
			return r;
			}
		}
	for(i=0;i<KLoopVarM;i++)
		{
		User::WaitForRequest(setReqStatus[i]);
		if((r=setReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, completed  change state status[%d] = %d\n"),i,r);
			return r;
			}
		}

	// 19) Call API to get the state of a long latency resource 'n' times, call API to modify the state of
	//     a long latency resource 'm' times. 
	//     Call the API to cancel the modify operations with resource qualifier.
	//     Wait for the get operations to complete. Check the state of the associated TRequestStatus objects.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	for(i=0;i<KLoopVarN;i++)
		{
		gChannel.GetResourceState(getReqStatus[i],gLongLatencyResource,cached,&readValue,&levelOwnerId);
		}
	for(i=0;i<KLoopVarM;i++)
		{
		gChannel.ChangeResourceState(setReqStatus[i],gLongLatencyResource,(readValue + gAsyncResStateDelta));
		}
	gChannel.CancelChangeResourceStateRequests(gLongLatencyResource);
	for(i=0;i<KLoopVarN;i++)
		{
		User::WaitForRequest(getReqStatus[i]);
		if((r=getReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, completed get state status[%d] = %d\n"),i,r);
			return r;
			}
		}
	for(i=0;i<KLoopVarM;i++)
		{
		User::WaitForRequest(setReqStatus[i]);
		if((r=setReqStatus[i].Int()) != KErrCancel)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, cancelled change state status[%d] = %d\n"),i,r);
			return r;
			}
		}

	// 20) Call API to get the state of a long latency resource 'n' times, call API to modify the state of
	//     a long latency resource 'm' times. Wait for the get operations to complete. 
	//     Call the API to cancel the get operations with resource qualifier. Check the state of the associated TRequestStatus objects.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	for(i=0;i<KLoopVarN;i++)
		{
		gChannel.GetResourceState(getReqStatus[i],gLongLatencyResource,cached,&readValue,&levelOwnerId);
		}
	TInt flipper = -1;
	for(i=0;i<KLoopVarM;i++)
		{
		gChannel.ChangeResourceState(setReqStatus[i],gLongLatencyResource,(readValue + (flipper*gAsyncResStateDelta)));
		flipper*=-1;
		}
	for(i=0;i<KLoopVarN;i++)
		{
		User::WaitForRequest(getReqStatus[i]);
		if((r=getReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, completed get state status[%d] = %d\n"),i,r);
			return r;
			}
		}
	gChannel.CancelGetResourceStateRequests(gLongLatencyResource);
	for(i=0;i<KLoopVarM;i++)
		{
		User::WaitForRequest(setReqStatus[i]);
		if((r=setReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, completed change state status[%d]= %d\n"),i,r);
			return r;
			}
		}
	for(i=0;i<KLoopVarN;i++)
		{
		if((r=getReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, completed-then-cancelled get state status[%d]= %d\n"),i,r);
			return r;
			}
		}

	// 21) Call API to get the state of a long latency resource 'n' times, call API to modify the state of
	//     a long latency resource 'm' times. Wait for the modify operations to complete. Call the API to cancel the modify
	//     operations with resource qualifier. Check the state of the associated TRequestStatus objects.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	for(i=0;i<KLoopVarN;i++)
		{
		gChannel.GetResourceState(getReqStatus[i],gLongLatencyResource,cached,&readValue,&levelOwnerId);
		}
	for(i=0;i<KLoopVarM;i++)
		{
		gChannel.ChangeResourceState(setReqStatus[i],gLongLatencyResource,(readValue + gAsyncResStateDelta));
		}
	for(i=0;i<KLoopVarM;i++)
		{
		User::WaitForRequest(setReqStatus[i]);
		if((r=setReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, completed change state status[%d] = %d\n"),i,r);
			return r;
			}
		}
	gChannel.CancelChangeResourceStateRequests(gLongLatencyResource);
	for(i=0;i<KLoopVarN;i++)
		{
		User::WaitForRequest(getReqStatus[i]);
		if((r=getReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, completed get state status[%d] = %d\n"),i,r);
			return r;
			}
		}
	for(i=0;i<KLoopVarM;i++)
		{
		if((r=setReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, cancelled change state status[%d] = %d\n"),i,r);
			return r;
			}
		}

	// 22) Call API to get the state of a long latency resource 'n' times, call API to modify the state of a long latency resource 'm' times.
	//     Then call API without operation-type qualifier to cancel the even-numbered modify request(s).
	//     Then call API with operation-type qualifier to cancel the even-numbered get request(s).
	//     Check the state of the associated TRequestStatus objects.
	gTest.Printf(_L("TestGetSetResourceStateOps, starting test %d\n"),testNo++);
	for(i=0;i<KLoopVarN;i++)
		{
		gChannel.GetResourceState(getReqStatus[i],gLongLatencyResource,cached,&readValue,&levelOwnerId);
		}
	for(i=0;i<KLoopVarM;i++)
		{
		gChannel.ChangeResourceState(setReqStatus[i],gLongLatencyResource,(readValue + gAsyncResStateDelta));
		}
	for(i=0;i<KLoopVarM;i+=2)
		{
		gChannel.CancelAsyncOperation(&(setReqStatus[i]));
		}
	for(i=0;i<KLoopVarN;i+=2)
		{
		r=gChannel.CancelGetResourceState(getReqStatus[i]);
		if(r!=KErrNone)
			{

	// NOTE: Cancel operation can only remove request which is still inside the resource controller
	// message queue. If the queue is empty, the resource controller may process the request very quickly 
	// after it is sent. It may cause the test fail. To solve this, we skip the test for request 0.
			
			if(i!=0)
				{
				gTest.Printf(_L("TestGetSetResourceStateOps, CancelGetResourceState for index %d returned %d\n"),i,r);
				return r;
				}
			}
		}
	for(i=0;i<KLoopVarM;i++)
		{
		User::WaitForRequest(setReqStatus[i]);
		if((r=setReqStatus[i].Int()) != KErrCancel)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, cancelled change state status[%d] = %d\n"),i,r);
			return r;
			}
		if(++i >= KLoopVarM)
			break;
		User::WaitForRequest(setReqStatus[i]);
		if((r=setReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, completed change state status[%d] = %d\n"),i,r);
			return r;
			}
		}
	for(i=0;i<KLoopVarN;i++)
		{
		User::WaitForRequest(getReqStatus[i]);

	// NOTE: Cancel operation can only remove request which is still inside the resource controller
	// message queue. If the queue is empty, the resource controller may process the request very quickly 
	// after it is sent. It may cause the test fail. To solve this, we skip the test for request 0.
		
		if(i>0 && ((r=getReqStatus[i].Int()) != KErrCancel))
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, cancelled get state status[%d] = %d\n"),i,r);
			return r;
			}
		if(++i >= KLoopVarN)
			break;
		User::WaitForRequest(getReqStatus[i]);
		if((r=getReqStatus[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateOps, completed get state status[%d] = %d\n"),i,r);
			return r;
			}
		}	
	return KErrNone;
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_RESMANUS-0611
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1398
//! @SYMTestCaseDesc    This test case tests APIs for regulating getting and setting the state of resources 
//! 
//! @SYMTestActions     0) Issue the maximum number (requested in the call to the Initialise API)
//!						of requests to get the current state of a resource. Then issue one further request.
//!
//!						1) Issue the maximum number (requested in the call to the Initialise API) of 
//!						requests to set the current state of a resource. Then issue one further request.
//!
//! @SYMTestExpectedResults 0) Test that the TRequestStatus object associated with the last request
//!							exhibits status code KErrUnderflow - exit otherwise. Test that the 
//!							TRequestStatus objects associated with the preceding requests exhibit 
//!							status code KErrNone - exit otherwise.
//!
//!							1) Test that the TRequestStatus object associated with the last request
//!							exhibits status code KErrUnderflow - exit otherwise. Test that the 
//!							TRequestStatus objects associated with the preceding requests exhibit 
//!							status code KErrNone - exit otherwise.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt TestGetSetResourceStateQuota()
//
// Test resource state access quota management
//
	{
	TUint resourceId;

	// To perform the quota test we need a long latency resource
	// If one has not been defined alert the user and return
	if(!gHaveAsyncRes)
		{
		gTest.Printf(_L("TestGetSetResourceStateQuota: don't have suitable asynchronous resource ... exiting\n"));
		return KErrNone;
		}
	else
		resourceId = gLongLatencyResource;

	TInt r = KErrNone;
	TBool lastErr = KErrNone;

	TInt i = 0;
	TRequestStatus status[MAX_NUM_REQUESTS];
	TInt state[MAX_NUM_REQUESTS];
	TBool cached = gUseCached;
	TInt levelOwnerId = 0;

	//
	//	Test GetResourceState - check client can not exceed quota of requests
	//
    gTest.Printf(_L("**Test GetResourceState (quota management)\n"));

	// KNoOfGetStateRequests Get state requests (of the same resource, ID=1) to consume the client quota
	for(i=0; i<KNoOfGetStateRequests; i++)
		gChannel.GetResourceState(status[i],resourceId,cached,&(state[i]),&levelOwnerId);

	// Addition Get state request to exceed the quota - the provided TRequestStatus
	// object should indicate KErrUnderflow
	gChannel.GetResourceState(status[KNoOfGetStateRequests],resourceId,cached,&(state[KNoOfGetStateRequests]),&levelOwnerId);
	User::WaitForRequest(status[KNoOfGetStateRequests]);
	if(status[KNoOfGetStateRequests].Int() != KErrUnderflow)
		{
		gTest.Printf(_L("TestGetSetResourceStateQuota, extra get state returned %d\n"),r);
		return r;
		}

	// Need to check the TRequestStatus objects
	for(i=0; i<KNoOfGetStateRequests; i++)
		{
		User::WaitForRequest(status[i]);
		if((r=status[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateQuota, get state %d returned %d\n"),i, r);
			lastErr = r;
			}
		}
	if(lastErr != KErrNone)
		return lastErr;

	//
	//	Test ChangeResourceState - check client can not exceed quota of requests
	//
    gTest.Printf(_L("**Test ChangeResourceState (quota management)\n"));

	// KNoOfSetStateRequests Set state requests (of the same resource, ID=1) to consume the client quota
	TInt newState = (state[1])+1;
	for(i=0; i<KNoOfSetStateRequests; i++)
		{
		gChannel.ChangeResourceState(status[i],resourceId,newState);
		}

	// Addition Set state request to exceed the quota - the provided TRequestStatus
	// object should indicate KErrUnderflow
	gChannel.ChangeResourceState(status[KNoOfSetStateRequests],resourceId,newState);
	User::WaitForRequest(status[KNoOfSetStateRequests]);
	if(status[KNoOfSetStateRequests].Int() != KErrUnderflow)
		{
		gTest.Printf(_L("TestGetSetResourceStateQuota, extra set state returned %d\n"),r);
		return r;
		}

	// Need to check the TRequestStatus objects
	for(i=0; i<KNoOfSetStateRequests; i++)
		{
		User::WaitForRequest(status[i]);
		if((r=status[i].Int()) != KErrNone)
			{
			gTest.Printf(_L("TestGetSetResourceStateQuota, set state %d returned %d\n"),i, r);
			lastErr = r;
			}
		}
	if(lastErr != KErrNone)
		return lastErr;

	return r;
	}


LOCAL_C TInt TriggerNotification(TRequestStatus& aStatus, TUint aResourceId, TBool aCached, TInt aDelta)
//
// Support the notification tests - cause a notification for the specified resource
//
	{
	TInt r = KErrNone;
	TInt readValue = 0;
	TInt levelOwnerId = 0;
	// Get initial state
	gChannel.GetResourceState(aStatus,aResourceId,aCached,&readValue,&levelOwnerId);
	User::WaitForRequest(aStatus);
	if(aStatus.Int() != KErrNone)
		{
		r=aStatus.Int();
		gTest.Printf(_L("TriggerNotification, get state returned %d\n"),r);
		return r;
		}
	TUint startingLevel = (TUint)readValue;

	// Write updated state
	TUint newLevel = (TUint)(readValue + aDelta);
	gTest.Printf(_L("TriggerNotification: startingLevel=0x%x, writing 0x%x\n"), startingLevel, newLevel);
	gChannel.ChangeResourceState(aStatus,aResourceId,newLevel);
	User::WaitForRequest(aStatus);
	if(aStatus.Int() != KErrNone)
		{
		r=aStatus.Int();
		gTest.Printf(_L("TriggerNotification, change state returned %d\n"),r);
		return r;
		}

	return r;
	}


LOCAL_C TInt CalcNotifyDirAndThr(TRequestStatus& aStatus, TUint aResourceId, TBool aCached, TInt& aThreshold, TBool& aDirection)
//
// Support the notification tests - determine an appropriate threshold and direction to request
//
	{
	__KHEAP_MARK;

	// Need to know current state
	TInt r = KErrNone;
	TInt readValue = 0;
	TInt levelOwnerId = 0;
		{
		gChannel.GetResourceState(aStatus,aResourceId,aCached,&readValue,&levelOwnerId);
		User::WaitForRequest(aStatus);
		if(aStatus.Int() != KErrNone)
			{
			r=aStatus.Int();
			gTest.Printf(_L("TestNotificationOps, pre-qualified notify get state returned %d\n"),r);
			return r;
			}
		}
	aThreshold = readValue + gAsyncResStateDelta;
	aDirection=(gAsyncResStateDelta>0)?ETrue:EFalse;

	__KHEAP_MARKEND;

	return r;
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_RESMANUS-0612
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1398
//! @SYMTestCaseDesc    This test case tests APIs for requesting both qualified and unqualified
//!						notifications; it also tests APIs to cancel such requests. 
//! 
//! @SYMTestActions     0) Call API to request notification of an unqualified change in resource
//!						state for a selected resource. Trigger a state change.
//!
//!						1) Call API to request notification of a qualified change in resource
//!						state for a selected resource. Trigger a state change.
//!	
//!						2) Call API to request notification of an unqualified change in resource
//!						state for a selected resource. Then call API to cancel all notifications for the resource.
//!
//!						3) Call API to request notification of a qualified change in resource 
//!						state for a selected resource. Then call API to cancel all notifications for the resource.
//!
//!						4) Call API to request notification of an unqualified change in resource 
//!						state for a selected resource. Trigger a state change. Then call API to cancel all
//!						notifications for the resource.
//!
//!						5) Call API to request notification of a qualified change in resource 
//!						state for a selected resource. Trigger a state change. Then call API to cancel all
//!						notifications for the resource.
//!
//!						6) Call API to request notification of an unqualified change in resource
//!						state for a selected resource. Then call API to cancel the notification request
//!
//!						7) Call API to request notification of a qualified change in resource 
//!						state for a selected resource. Then call API to cancel the notification request
//!
//!						8) Call API to request notification of an unqualified change in resource 
//!						state for a selected resource. Trigger a state change. Then call API to cancel the
//!						notification request
//!
//!						9) Call API to request notification of a qualified change in resource 
//!						state for a selected resource. Trigger a state change. Then call API to cancel the
//!						notification request
//!
//!						10) Call API to request notification of an unqualified change in resource
//!						state for a selected resource. Then call API to cancel generic async request
//!
//!						11) Call API to request notification of a qualified change in resource 
//!						state for a selected resource. Then call API to cancel generic async request
//!
//!						12) Call API to request notification of an unqualified change in resource 
//!						state for a selected resource. Trigger a state change. Then call API to cancel
//!						generic async request
//!
//!						13) Call API to request notification of a qualified change in resource 
//!						state for a selected resource. Trigger a state change. Then call API to cancel
//!						generic async request
//!
//! @SYMTestExpectedResults 0) The associated TRequestStatus object should indicate KErrNone 
//!						- exits otherwise.
//!
//!						1) The associated TRequestStatus object should indicate KErrNone 
//!						- exits otherwise.
//!
//!						2) The associated TRequestStatus object should indicate KErrCancel 
//!						- exits otherwise. 
//!
//!						3) The associated TRequestStatus object should indicate KErrCancel 
//!						- exits otherwise. 
//!
//!						4) The TRequestStatus object should indicate state KRequestPending until
//!						the state change is triggered, upon which it should exhibit state 
//!						KErrNone. After the cancellation it should still exhibit state KErrNone.
//!						Exit for any deviation from this behaviour.
//!
//!						5) The TRequestStatus object should indicate state KRequestPending until
//!						the state change is triggered, upon which it should exhibit state 
//!						KErrNone. After the cancellation it should still exhibit state KErrNone.
//!						Exit for any deviation from this behaviour.
//!
//!						6) The associated TRequestStatus object should indicate KErrCancel 
//!						- exits otherwise. 
//!
//!						7) The associated TRequestStatus object should indicate KErrCancel 
//!						- exits otherwise. 
//!
//!						8) The TRequestStatus object should indicate state KRequestPending until
//!						the state change is triggered, upon which it should exhibit state 
//!						KErrNone. After the cancellation it should still exhibit state KErrNone.
//!						Exit for any deviation from this behaviour.
//!
//!						9) The TRequestStatus object should indicate state KRequestPending until
//!						the state change is triggered, upon which it should exhibit state 
//!						KErrNone. After the cancellation it should still exhibit state KErrNone.
//!						Exit for any deviation from this behaviour.
//!
//!						10) The associated TRequestStatus object should indicate KErrCancel 
//!						- exits otherwise. 
//!
//!						11) The associated TRequestStatus object should indicate KErrCancel 
//!						- exits otherwise. 
//!
//!						12) The TRequestStatus object should indicate state KRequestPending until
//!						the state change is triggered, upon which it should exhibit state 
//!						KErrNone. After the cancellation it should still exhibit state KErrNone.
//!						Exit for any deviation from this behaviour.
//!
//!						13) The TRequestStatus object should indicate state KRequestPending until
//!						the state change is triggered, upon which it should exhibit state 
//!						KErrNone. After the cancellation it should still exhibit state KErrNone.
//!						Exit for any deviation from this behaviour.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt TestNotificationOps()
//
// Test notification methods
//
	{
	__KHEAP_MARK;

	TInt r = KErrNone;
	TRequestStatus status;
	TUint resourceId;
	TInt stateDelta;
	if(!gHaveAsyncRes)
		{
		resourceId = 2; // Arbitrary
		stateDelta = 1; // Arbitrary
		}
	else
		{
		resourceId = gLongLatencyResource;
		stateDelta = gAsyncResStateDelta;
		}
	TInt testIndex = 0;

	// 0) Call API to request notification of an unqualified change in resource
	//    state for a selected resource. Trigger a state change.
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	gChannel.RequestNotification(status, resourceId);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	TBool cached = gUseCached;
	TRequestStatus triggerStatus;
	if((r=TriggerNotification(triggerStatus, resourceId, cached, stateDelta))!=KErrNone)
		return r;
	User::WaitForRequest(status);
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: after TriggerNotification status = %d\n"),status.Int());
		return r;
		}

	// 1) Call API to request notification of a qualified change in resource
	//    state for a selected resource. Trigger a state change.
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	TInt dumThreshold=0;
	TBool dumDirection=EFalse;
	if((r=CalcNotifyDirAndThr(status, resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
		return r;
	gChannel.RequestNotification(status, resourceId, dumThreshold, dumDirection);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	// Pass gAsyncResStateDelta as this represents the delta value
	if((r=TriggerNotification(triggerStatus, resourceId, cached, gAsyncResStateDelta))!=KErrNone)
		return r;
	User::WaitForRequest(status);
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: after TriggerNotification status = %d\n"),status.Int());
		return r;
		}

	// 2) Call API to request notification of an unqualified change in resource
	//    state for a selected resource. Then call API to cancel all notifications for the resource.
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	gChannel.RequestNotification(status, resourceId);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	r=gChannel.CancelNotificationRequests(resourceId);
	if(r!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps, CancelNotificationRequests returned %d\n"),r);
		return r;
		}
	User::WaitForRequest(status);
	if(status.Int()!=KErrCancel)
		{
		gTest.Printf(_L("TestNotificationOps: after cancel basic request status = %d\n"),status.Int());
		return r;
		}

	// 3) Call API to request notification of a qualified change in resource 
	//    state for a selected resource. Then call API to cancel all notifications for the resource.
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	dumThreshold = 0;			// Arbitrary
	dumDirection = ETrue;		// Arbitrary
	if((r=CalcNotifyDirAndThr(status, resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
		return r;
	gChannel.RequestNotification(status, resourceId, dumThreshold, dumDirection);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	r=gChannel.CancelNotificationRequests(resourceId);
	if(r!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps, CancelNotificationRequests (qualified) returned %d\n"),r);
		return r;
		}
	User::WaitForRequest(status);
	if(status.Int()!=KErrCancel)
		{
		gTest.Printf(_L("TestNotificationOps: after qualified request cancel status = %d\n"),status.Int());
		return r;
		}

	// 4) Call API to request notification of an unqualified change in resource 
	//    state for a selected resource. Trigger a state change. Then call API to cancel all
	//    notifications for the resource.
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	gChannel.RequestNotification(status, resourceId);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	// Pass gAsyncResStateDelta as this represents the delta value
	if((r=TriggerNotification(triggerStatus, resourceId, cached, gAsyncResStateDelta))!=KErrNone)
		return r;
	User::WaitForRequest(status);
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: trigger basic request before cancel, status = %d\n"),status.Int());
		return r;
		}
	r=gChannel.CancelNotificationRequests(resourceId);
	if(r!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps, CancelNotificationRequests returned %d\n"),r);
		return r;
		}
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: after cancel (completed) basic request status = %d\n"),status.Int());
		return r;
		}

	// 5) Call API to request notification of a qualified change in resource 
	//    state for a selected resource. Trigger a state change. Then call API to cancel all
	//    notifications for the resource.
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	if((r=CalcNotifyDirAndThr(status, resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
		return r;
	gChannel.RequestNotification(status, resourceId, dumThreshold, dumDirection);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	// Pass gAsyncResStateDelta as this represents the delta value
	if((r=TriggerNotification(triggerStatus, resourceId, cached, gAsyncResStateDelta))!=KErrNone)
		return r;
	User::WaitForRequest(status);
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: trigger qualified request before cancel, status = %d\n"),status.Int());
		return r;
		}
	r=gChannel.CancelNotificationRequests(resourceId);
	if(r!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps, CancelNotificationRequests (qualified) returned %d\n"),r);
		return r;
		}
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: after cancel (completed) qualified request status = %d\n"),status.Int());
		return r;
		}

	// 6) Call API to request notification of an unqualified change in resource
	//    state for a selected resource. Then call API to cancel the notification request
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	gChannel.RequestNotification(status, resourceId);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	r=gChannel.CancelRequestNotification(status);
	if(r!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps, CancelNotificationRequests returned %d\n"),r);
		return r;
		}
	User::WaitForRequest(status);
	if(status.Int()!=KErrCancel)
		{
		gTest.Printf(_L("TestNotificationOps: after cancel basic request status = %d\n"),status.Int());
		return r;
		}

	// 7) Call API to request notification of a qualified change in resource 
	//    state for a selected resource. Then call API to cancel the notification request
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	dumThreshold = 0;			// Arbitrary
	dumDirection = ETrue;		// Arbitrary
	if((r=CalcNotifyDirAndThr(status, resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
		return r;
	gChannel.RequestNotification(status, resourceId, dumThreshold, dumDirection);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	r=gChannel.CancelRequestNotification(status);
	if(r!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps, CancelNotificationRequests (qualified) returned %d\n"),r);
		return r;
		}
	User::WaitForRequest(status);
	if(status.Int()!=KErrCancel)
		{
		gTest.Printf(_L("TestNotificationOps: after qualified request cancel status = %d\n"),status.Int());
		return r;
		}

	// 8) Call API to request notification of an unqualified change in resource 
	//    state for a selected resource. Trigger a state change. Then call API to cancel the
	//    notification request
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	gChannel.RequestNotification(status, resourceId);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	// Pass gAsyncResStateDelta as this represents the delta value
	if((r=TriggerNotification(triggerStatus, resourceId, cached, gAsyncResStateDelta))!=KErrNone)
		return r;
	User::WaitForRequest(status);
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: trigger basic request before cancel, status = %d\n"),status.Int());
		return r;
		}
	r=gChannel.CancelRequestNotification(status);
	if(r!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps, CancelNotificationRequests returned %d\n"),r);
		return r;
		}
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: after cancel (completed) basic request status = %d\n"),status.Int());
		return r;
		}

	// 9) Call API to request notification of a qualified change in resource 
	//    state for a selected resource. Trigger a state change. Then call API to cancel the
	//    notification request
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	if((r=CalcNotifyDirAndThr(status, resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
		return r;
	gChannel.RequestNotification(status, resourceId, dumThreshold, dumDirection);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	// Pass gAsyncResStateDelta as this represents the delta value
	if((r=TriggerNotification(triggerStatus, resourceId, cached, gAsyncResStateDelta))!=KErrNone)
		return r;
	User::WaitForRequest(status);
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: trigger qualified request before cancel, status = %d\n"),status.Int());
		return r;
		}
	r=gChannel.CancelRequestNotification(status);
	if(r!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps, CancelNotificationRequests (qualified) returned %d\n"),r);
		return r;
		}
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: after cancel (completed) qualified request status = %d\n"),status.Int());
		return r;
		}


	// 10) Call API to request notification of an unqualified change in resource
	//     state for a selected resource. Then call API to cancel generic async request
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	gChannel.RequestNotification(status, resourceId);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	gChannel.CancelAsyncOperation(&status);
	User::WaitForRequest(status);
	if(status.Int()!=KErrCancel)
		{
		gTest.Printf(_L("TestNotificationOps: after cancel basic request status = %d\n"),status.Int());
		return r;
		}

	// 11) Call API to request notification of a qualified change in resource 
	//     state for a selected resource. Then call API to cancel generic async request
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	dumThreshold = 0;			// Arbitrary
	dumDirection = ETrue;		// Arbitrary
	if((r=CalcNotifyDirAndThr(status, resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
		return r;
	gChannel.RequestNotification(status, resourceId, dumThreshold, dumDirection);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	gChannel.CancelAsyncOperation(&status);
	User::WaitForRequest(status);
	if(status.Int()!=KErrCancel)
		{
		gTest.Printf(_L("TestNotificationOps: after qualified request cancel status = %d\n"),status.Int());
		return r;
		}

	// 12) Call API to request notification of an unqualified change in resource 
	//     state for a selected resource. Trigger a state change. Then call API to cancel
	//     generic async request
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	gChannel.RequestNotification(status, resourceId);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	// Pass gAsyncResStateDelta as this represents the delta value
	if((r=TriggerNotification(triggerStatus, resourceId, cached, gAsyncResStateDelta))!=KErrNone)
		return r;
	User::WaitForRequest(status);
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: trigger basic request before cancel, status = %d\n"),status.Int());
		return r;
		}
	gChannel.CancelAsyncOperation(&status);
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: after cancel (completed) basic request status = %d\n"),status.Int());
		return r;
		}

	// 13) Call API to request notification of a qualified change in resource 
	//     state for a selected resource. Trigger a state change. Then call API to cancel
	//     generic async request
	gTest.Printf(_L("TestNotificationOps: starting test %d\n"),testIndex++);
	if((r=CalcNotifyDirAndThr(status, resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
		return r;
	gChannel.RequestNotification(status, resourceId, dumThreshold, dumDirection);
	if(status.Int() != KRequestPending)
		{
		gTest.Printf(_L("TestNotificationOps: status=%d\n"),status.Int());
		return KErrGeneral;
		}
	// Pass gAsyncResStateDelta as this represents the delta value
	if((r=TriggerNotification(triggerStatus, resourceId, cached, gAsyncResStateDelta))!=KErrNone)
		return r;
	User::WaitForRequest(status);
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: trigger qualified request before cancel, status = %d\n"),status.Int());
		return r;
		}
	gChannel.CancelAsyncOperation(&status);
	if(status.Int()!=KErrNone)
		{
		gTest.Printf(_L("TestNotificationOps: after cancel (completed) qualified request status = %d\n"),status.Int());
		return r;
		}

	__KHEAP_MARKEND;
	
	return r;
	}

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_RESMANUS-0613
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1398
//! @SYMTestCaseDesc    This test case tests quota management for notification operations.
//! 
//! @SYMTestActions     0) Issue the maximum number (requested in the call to the Initialise API)
//!						of requests for notification of unqualified changes to the state of a 
//!						resource. Then issue one further request.
//!
//!						1) Issue the maximum number (requested in the call to the Initialise API)
//!						of requests for notification of qualified changes to the state of a 
//!						resource. Then issue one further request.
//!
//!						2) Issue the maximum number (requested in the call to the Initialise API)
//!						of requests for notification changes to the state of a resource, where 
//!						every odd request is for an unqualified change and every even request is
//!						for a qualified change. Then issue one further request.
//!
//! @SYMTestExpectedResults 0) Test that the TRequestStatus object associated with the last 
//!							request exhibits status code KErrUnderflow - exit otherwise. Test
//!							that the TRequestStatus objects associated with the preceding requests
//!							exhibit status code KErrNone - exit otherwise.
//!
//!							1) Test that the TRequestStatus object associated with the last request
//!							exhibits status code KErrUnderflow - exit otherwise. Test that the 
//!							TRequestStatus objects associated with the preceding requests exhibit
//!							status code KErrNone - exit otherwise.
//!
//!							2) Test that the TRequestStatus object associated with the last request
//!							exhibits status code KErrUnderflow - exit otherwise. Test that the 
//!							TRequestStatus objects associated with the preceding requests exhibit
//!							status code KErrNone - exit otherwise.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt TestNotificationQuota()
//
// Test notification request quota management
//
	{
	__KHEAP_MARK;

	TInt r = KErrNone;
	TRequestStatus status[MAX_NUM_REQUESTS];
	TInt loopVar=0;
	TBool cached = gUseCached;
	TRequestStatus triggerStatus;
	TUint resourceId = gLongLatencyResource;

	// Test quota for basic operation
	//
	// Loop to perform maximum number of requests, check TRequestStatus objects are pending
	for(loopVar=0; loopVar<KNoOfNotifyRequests; loopVar++)
		{
		gChannel.RequestNotification(status[loopVar], resourceId);
		if(status[loopVar].Int() != KRequestPending)
			{
			gTest.Printf(_L("TestNotificationQuota: status not KRequestPending, =%d\n"),
													status[loopVar].Int());
			return KErrGeneral;
			}
		}
	// Issue one more request, check that TRequestStatus object exhibits state KErrUnderflow
	gChannel.RequestNotification(status[KNoOfNotifyRequests], resourceId);
	if(status[KNoOfNotifyRequests].Int() != KErrUnderflow)
		{
		gTest.Printf(_L("TestNotificationQuota: status[%d] not KErrUnderflow, =%d\n"),
													KNoOfNotifyRequests,status[loopVar].Int());
		return KErrGeneral;
		}
	// Loop to trigger previously-issued notifications, check TRequestStatus objects
	for(loopVar=0; loopVar<KNoOfNotifyRequests; loopVar++)
		{
		// Pass gAsyncResStateDelta as this represents the delta value
		if((r=TriggerNotification(triggerStatus, resourceId, cached, gAsyncResStateDelta))!=KErrNone)
			return r;
		User::WaitForRequest(status[loopVar]);
		if(status[loopVar].Int()!=KErrNone)
			{
			gTest.Printf(_L("TestNotificationQuota: trigger basic request status[%d] = %d\n"),
																loopVar,status[loopVar].Int());
			return r;
			}
		}


	// Test quota for qualified operation
	//
	TInt dumThreshold = -25;	// Arbitrary
	TBool dumDirection = ETrue;	// Arbitrary
	//
	// Loop to perform maximum number of requests, check TRequestStatus objects are pending
	for(loopVar=0; loopVar<KNoOfNotifyRequests; loopVar++)
		{
	if((r=CalcNotifyDirAndThr(status[loopVar], resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
		return r;
		gChannel.RequestNotification(status[loopVar], resourceId, dumThreshold, dumDirection);
		if(status[loopVar].Int() != KRequestPending)
			{
			gTest.Printf(_L("TestNotificationQuota: status[%d] not KRequestPending, =%d\n"),
																loopVar,status[loopVar].Int());
			return KErrGeneral;
			}
		}
	// Issue one more request, check that TRequestStatus object exhibits state KErrUnderflow
	if((r=CalcNotifyDirAndThr(status[KNoOfNotifyRequests], resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
		return r;
	gChannel.RequestNotification(status[KNoOfNotifyRequests], resourceId, dumThreshold, dumDirection);
	if(status[KNoOfNotifyRequests].Int() != KErrUnderflow)
		{
		gTest.Printf(_L("TestNotificationQuota: status[%d] not KErrUnderflow, =%d\n"),
													KNoOfNotifyRequests,status[loopVar].Int());
		return KErrGeneral;
		}
	// Loop to trigger previously-issued notifications, check TRequestStatus objects
	for(loopVar=0; loopVar<KNoOfNotifyRequests; loopVar++)
		{
		// Pass gAsyncResStateDelta as this represents the delta value
		if((r=TriggerNotification(triggerStatus, resourceId, cached, gAsyncResStateDelta))!=KErrNone)
			return r;
		User::WaitForRequest(status[loopVar]);
		if(status[loopVar].Int()!=KErrNone)
			{
			gTest.Printf(_L("TestNotificationQuota: trigger qualified request status[%d] = %d\n"),
																loopVar,status[loopVar].Int());
			return r;
			}
		}

	// Text quota with mixture of basic and qualified requests
	//
	TBool qualified = ETrue;
	// Issue requests and check TRequestStatus objects are pending
	for(loopVar=0; loopVar<KNoOfNotifyRequests; loopVar++)
		{
		if(qualified)
			{
			if((r=CalcNotifyDirAndThr(status[loopVar], resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
				return r;
			gChannel.RequestNotification(status[loopVar], resourceId, dumThreshold, dumDirection);
			}
		else
			{
			gChannel.RequestNotification(status[loopVar], resourceId);
			}
		qualified=!qualified;
		if(status[loopVar].Int() != KRequestPending)
			{
			gTest.Printf(_L("TestNotificationQuota: mixed loop status[%d] not KRequestPending, =%d\n"),
																loopVar,status[loopVar].Int());
			return KErrGeneral;
			}
		}
	// Issue one more request, check that TRequestStatus object exhibits state KErrUnderflow
	if(qualified)
		{
		if((r=CalcNotifyDirAndThr(status[KNoOfNotifyRequests], resourceId, cached, dumThreshold, dumDirection))!=KErrNone)
			return r;
		gChannel.RequestNotification(status[KNoOfNotifyRequests], resourceId, dumThreshold, dumDirection);
		}
	else
		{
		gChannel.RequestNotification(status[KNoOfNotifyRequests], resourceId);
		}
	if(status[KNoOfNotifyRequests].Int() != KErrUnderflow)
		{
		gTest.Printf(_L("TestNotificationQuota: mixed loop status[%d] not KErrUnderflow, =%d\n"),
													KNoOfNotifyRequests,status[loopVar].Int());
		return KErrGeneral;
		}
	// Loop to trigger previously-issued notifications, check TRequestStatus objects
	for(loopVar=0; loopVar<KNoOfNotifyRequests; loopVar++)
		{
		// Pass gAsyncResStateDelta as this represents the delta value
		if((r=TriggerNotification(triggerStatus, resourceId, cached, gAsyncResStateDelta))!=KErrNone)
			return r;
		User::WaitForRequest(status[loopVar]);
		if(status[loopVar].Int()!=KErrNone)
			{
			gTest.Printf(_L("TestNotificationQuota: trigger mixed request status[%d] = %d\n"),
																loopVar,status[loopVar].Int());
			return r;
			}
		}

	__KHEAP_MARKEND;

	return r;
	}

#ifdef PIRATE_THREAD_TESTS

RThread PirateThread;
TRequestStatus PirateStatus;
const TInt KHeapSize=0x4000;
const TInt KStackSize=0x4000;

_LIT(KPirateThreadName,"Pirate");

TInt PirateThreadFn(TAny* /*aSrcThread*/)
	{
#if 0
	TInt r=KErrNone;
	RBusDevResManUs pirateChannel = gChannel;
	RThread& thread = *((RThread*)aSrcThread);

/* 1 - pirate with current thread - Panics kernel with KErrBadHandle */
	if((r=pirateChannel.Duplicate(RThread(),EOwnerProcess))!=KErrAccessDenied)
		{
		gTest.Printf(_L("TestThreadExclusiveAccess: pirateChannel.Duplicate(RThread(),EOwnerProcess) returned %d\n"),r);
		return KErrGeneral;
		}

/* 2 - pirate with parent thread - Panics kernel with KErrBadHandle */
	pirateChannel = gChannel;
	if((r=pirateChannel.Duplicate(thread,EOwnerThread))!=KErrAccessDenied)
		{
		gTest.Printf(_L("TestThreadExclusiveAccess: pirateChannel.Duplicate(thread,EOwnerThread) returned %d\n"),r);
		return KErrGeneral;
		}

/* 3 - gChannel with current thread - Panics kernel with KErrBadHandle */
	if((r=gChannel.Duplicate(RThread(),EOwnerThread))!=KErrAccessDenied)
		{
		gTest.Printf(_L("TestThreadExclusiveAccess: gChannel.Duplicate(RThread(),EOwnerThread) returned %d\n"),r);
		return KErrGeneral;
		}

/* 4 - gChannel with parent thread - Panics kernel with KErrBadHandle */
	if((r=gChannel.Duplicate(thread,EOwnerThread))!=KErrAccessDenied)
		{
		gTest.Printf(_L("TestThreadExclusiveAccess: gChannel.Duplicate(thread,EOwnerThread)returned %d\n"),r);
		return KErrGeneral;
		}
#endif

//	pirateChannel.Close();
	return KErrNone;
	}

TInt StartPirate(RThread& aSrcThread)
	{
	TAny* srcThread =(TAny*)(&aSrcThread);
	TInt r=PirateThread.Create(KPirateThreadName,PirateThreadFn,KStackSize,KHeapSize,KHeapSize,srcThread,EOwnerThread);
	if (r!=KErrNone)
		return r;
	PirateThread.Logon(PirateStatus);
	PirateThread.Resume();
	return KErrNone;
	}

TInt WaitForPirateThread()
	{
	User::WaitForRequest(PirateStatus);
	TInt exitType=PirateThread.ExitType();
	TInt exitReason=PirateThread.ExitReason();
	TBuf<16> exitCat=PirateThread.ExitCategory();
	if((exitType!= EExitKill)||(exitReason!=KErrNone))
		{
		gTest.Printf(_L("Pirate thread error: %d\n"),PirateStatus.Int());
		gTest.Printf(_L("Thread exit reason: %d,%d,%S\n"),exitType,exitReason,&exitCat);
		gTest(0);		
		}
	PirateThread.Close();
	return KErrNone;
	}

#endif

//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_RESMANUS-0608
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1398
//! @SYMTestCaseDesc    This test case tests that channels can not be shared between threads.
//! @SYMTestActions     0) Attempt to Duplicate the channel handle with EOwnerProcess as the owner type.
//!
//!						1) Attempt to Duplicate the channel handle with EOwnerThread
//!
//! @SYMTestExpectedResults 0) API should return with KErrAccessDenied, exits otherwise.
//!						1) API should return with KErrNone, exits otherwise.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt TestThreadExclusiveAccess()
//
// Test mechanism to prevent other threads accessing a channel
//
	{
	__KHEAP_MARK;

	TInt r;
	RBusDevResManUs pirateChannel = gChannel;
	if((r=pirateChannel.Duplicate(RThread(),EOwnerProcess))!=KErrAccessDenied)
		{
		gTest.Printf(_L("TestThreadExclusiveAccess: Duplicate with EOwnerProcess returned %d\n"),r);
		if(r==KErrNone)
			r=KErrGeneral;
		return r; // return error which is neither KErrNone nor KErrAccessDenied
		}
	pirateChannel = gChannel;
	if((r=pirateChannel.Duplicate(RThread(),EOwnerThread))!=KErrNone)
		{
		gTest.Printf(_L("TestThreadExclusiveAccess: Duplicate with EOwnerThread returned %d\n"),r);
		return r;
		}
#ifdef PIRATE_THREAD_TESTS
	RThread& threadRef = RThread();
	if((r=StartPirate(threadRef))!=KErrNone)
		{
		gTest.Printf(_L("TestThreadExclusiveAccess: StartPirate returned %d\n"),r);
		return KErrGeneral;
		}
	if((r=WaitForPirateThread())!=KErrNone)
		{
		gTest.Printf(_L("TestThreadExclusiveAccess: WaitForPirateThread returned %d\n"),r);
		return KErrGeneral;
		}
#endif
	pirateChannel.Close();

	__KHEAP_MARKEND;

	return KErrNone;
	}

RThread Thrd2;
TRequestStatus Thrd2Status;
const TInt KHeapSize=0x4000;
const TInt KStackSize=0x4000;

_LIT(KThrd2Name,"Thread2");


TInt Thread2Fn(TAny* /* */)
//
// Test that more than one thread can be supported
//
	{
	TInt r;

	// Open a channel
	//
	_LIT(secThrdStr,"Thrd2Channel");
	TBufC<16> secThrdName(secThrdStr);
	// API accepts 8-bit descriptors, only - so convert name accordingly
	TBuf8<MAX_RESOURCE_NAME_LENGTH+1>EightBitName;
	EightBitName.Copy(secThrdName);
	RBusDevResManUs secThrdChannel;
    if((r=secThrdChannel.Open(EightBitName))!=KErrNone)
		return r;

	// Read the resource information
	//
	TUint numResources=0;
	if((r=secThrdChannel.GetNoOfResources(numResources))!=KErrNone)
		return r;

	// To support the GetAllResourcesInfo testing, instantiate TResourceInfoBuf objects
	// and reference via an RSimplePointerArray
	TUint bufSize = numResources;
	RSimplePointerArray<TResourceInfoBuf> resPtrs(bufSize);
	for(TUint i=0;i<bufSize;i++)
		{
		TResourceInfoBuf *info = new TResourceInfoBuf();
		if((r=resPtrs.Insert(info, i))!=KErrNone)
			return r;
		}

	TUint updateNumResources=numResources;
	if((r=secThrdChannel.GetAllResourcesInfo(&resPtrs,updateNumResources))!=KErrNone)
		return r;
	resPtrs.Close();


	// Read current client information
	//
	TUint numClients=0;
	TUint numAllClients=0;
	//
	// GetNoOfClients - with default aIncludeKern=EFalse
	//
	if((r=secThrdChannel.GetNoOfClients(numClients,EFalse)) != KErrNone)
		return r;
	//
	// GetNoOfClients - with aIncludeKern=ETrue
	//
	r=secThrdChannel.GetNoOfClients(numAllClients, ETrue);
#ifdef RESMANUS_KERN
	if(r!=KErrNone)
#else
	if(r!=KErrPermissionDenied)
#endif
		return KErrGeneral;
	//
	// Need a buffer big enough to contain numClients instances of TClientName
	// To support the GetNamesAllClients testing, instantiate TClientName objects
	// and reference via an RSimplePointerArray
	TUint resBufSize = (numAllClients>numClients)?numAllClients:numClients;
	RSimplePointerArray<TClientName> infoPtrs(resBufSize);
	for(TUint j=0;j<resBufSize;j++)
		{
		TClientName *info = new TClientName();
		if((r=infoPtrs.Insert(info, j))!=KErrNone)
			return r;
		}
	//
	// GetNamesAllClients - with aIncludeKern=EFalse
	//
	if((r=secThrdChannel.GetNamesAllClients(&infoPtrs, numClients, EFalse)) != KErrNone)
		return r;
	//
	// GetNamesAllClients - with aIncludeKern=ETrue
	//
#ifdef RESMANUS_KERN
	if((r=secThrdChannel.GetNamesAllClients(&infoPtrs, numAllClients, ETrue)) != KErrNone)
		return r;
#else
	if((r=secThrdChannel.GetNamesAllClients(&infoPtrs, numClients, ETrue)) != KErrPermissionDenied)
		{
		if(r==KErrNone)
			r=KErrGeneral;
		return r; // return error which is neither KErrPermissionDenied nor KErrGeneral
		}
	else
		r=KErrNone; // Ensure misleading result is not propagated
#endif
	infoPtrs.Close();

	// If we don't have a shared resource identified, skip the remaining tests
	if(gHaveSharedRes)
		{
		// Register with the Resource Controller
		//
		if ((r=secThrdChannel.Initialise(1,1,0))!=KErrNone)  // Just need 1 get and 1 set state
			return r;
		// Set a level on the shared resource
		//
		// Get initial state
		TRequestStatus status;
		TBool cached = gUseCached;
		TInt readValue;
		TInt levelOwnerId;
		secThrdChannel.GetResourceState(status,gSharedResource,cached,&readValue,&levelOwnerId);
		User::WaitForRequest(status);
		r=status.Int();
		if(r != KErrNone)
			return r;
		// Write updated state
		TUint newLevel = (TUint)(readValue + gSharedResStateDelta);
		secThrdChannel.ChangeResourceState(status,gSharedResource,newLevel);
		User::WaitForRequest(status);
		r=status.Int();
		if(r != KErrNone)
			return r;
		
		// Read current client information
		//
		numClients=0;
		numAllClients=0;
		//
		// GetNoOfClients - with default aIncludeKern=EFalse
		//
		if((r=secThrdChannel.GetNoOfClients(numClients,EFalse)) != KErrNone)
			return r;
		//
		// GetNoOfClients - with aIncludeKern=ETrue
		//
#ifdef RESMANUS_KERN
		if((r=secThrdChannel.GetNoOfClients(numAllClients, ETrue))!=KErrNone)
#else
		if((r=secThrdChannel.GetNoOfClients(numAllClients, ETrue))!=KErrPermissionDenied)
#endif
			return KErrGeneral;
		//
		// Need a buffer big enough to contain numClients instances of TClientName
		// To support the GetNamesAllClients testing, instantiate TClientName objects
		// and reference via an RSimplePointerArray
		TUint bufSize = (numAllClients>numClients)?numAllClients:numClients;
		RSimplePointerArray<TClientName> infoPtrs(bufSize);
		for(TUint i=0;i<bufSize;i++)
			{
			TClientName *info = new TClientName();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				return r;
			}
		//
		// GetNamesAllClients - with aIncludeKern=EFalse
		//
		if((r=secThrdChannel.GetNamesAllClients(&infoPtrs, numClients, EFalse)) != KErrNone)
			return r;
		//
		// GetNamesAllClients - with aIncludeKern=ETrue
		//
#ifdef RESMANUS_KERN
		if((r=secThrdChannel.GetNamesAllClients(&infoPtrs, numAllClients, ETrue)) != KErrNone)
			return r;
#else
		if((r=secThrdChannel.GetNamesAllClients(&infoPtrs, numClients, ETrue)) != KErrPermissionDenied)
			{
			if(r==KErrNone)
				r=KErrGeneral;
			return r; // return error which is neither KErrPermissionDenied nor KErrGeneral
			}
		else
			r=KErrNone; // Ensure misleading result is not propagated
#endif
		infoPtrs.Close();

		// Read the resource information
		//
		numResources=0;
		if((r=secThrdChannel.GetNoOfResources(numResources))!=KErrNone)
			return r;

		//
		// Need a buffer big enough to contain numResources instances of TResourceInfoBuf
		// To support the GetAllResourcesInfo testing, instantiate TResourceInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TResourceInfoBuf> resPtrs(numResources);
		for(TUint j=0;j<numResources;j++)
			{
			TResourceInfoBuf *info = new TResourceInfoBuf();
			if((r=resPtrs.Insert(info, j))!=KErrNone)
				return r;
			}

		if((r=secThrdChannel.GetAllResourcesInfo(&resPtrs,numResources))!=KErrNone)
			return r;
		resPtrs.Close();
		}

	// Close the channel
	secThrdChannel.Close();
	return r;
	}

TInt StartThread2()
	{
	TInt r=Thrd2.Create(KThrd2Name,Thread2Fn,KStackSize,KHeapSize,KHeapSize,NULL,EOwnerThread);
	if (r!=KErrNone)
		return r;
	Thrd2.Logon(Thrd2Status);
	Thrd2.Resume();
	return KErrNone;
	}

TInt WaitForThread2()
	{
	User::WaitForRequest(Thrd2Status);
	TInt exitType=Thrd2.ExitType();
	TInt exitReason=Thrd2.ExitReason();
	TBuf<16> exitCat=Thrd2.ExitCategory();
	if((exitType!= EExitKill)||(exitReason!=KErrNone))
		{
		gTest.Printf(_L("Thread2 error: %d\n"),Thrd2Status.Int());
		gTest.Printf(_L("Thread exit reason: %d,%d,%S\n"),exitType,exitReason,&exitCat);
		gTest(0);		
		}
	Thrd2.Close();
	return KErrNone;
	}


//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_RESMANUS-0614
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1398
//! @SYMTestCaseDesc    This test case tests that an additional thread can open a channel and
//!						exercise selected APIs.
//! 
//! @SYMTestActions     0) Call API to open a channel.
//! 
//!						1) Call GetNoOfResources API.
//! 
//!						2) Call GetAllResourcesInfo API.
//! 
//!						3) Call GetNoOfClients API with default aIncludeKern=EFalse.
//! 
//!						4) Call GetNoOfClients API with aIncludeKern=ETrue
//! 
//!						5) Call GetNamesAllClientsAPI with default aIncludeKern=EFalse.
//! 
//!						6) Call GetNamesAllClientsAPI with aIncludeKern=ETrue
//! 
//!						7) Call Initialise API.
//! 
//!						8) Call GetResourceState API for selected resource. 
//! 
//!						9) Call GetResourceState API for selected resource.
//! 
//!						10) Call GetNoOfClients API with default aIncludeKern=EFalse.
//! 
//!						11) Call GetNoOfClients API with aIncludeKern=ETrue
//! 
//!						12) Call GetNamesAllClientsAPI with default aIncludeKern=EFalse.
//! 
//!						13) Call GetNamesAllClientsAPI with aIncludeKern=ETrue
//! 
//!						14) Call GetNoOfResources API.
//! 
//!						15) Call GetAllResourcesInfo API.
//!
//! @SYMTestExpectedResults 0) Test that the channel was opened - exit otherwise.
//! 
//!							1) Test that the API call returned KErrNone - exit otherwise.
//! 
//!							2) Test that the API call returned KErrNone - exit otherwise.
//! 
//!							3) Test that the API call returned KErrNone - exit otherwise.
//! 
//!							4) If client exhibits PlatSec capability ReadDeviceData, Test that the API call returned KErrNone - exit otherwise
//!							   If client lacks PlatSec capability ReadDeviceData, Test that the API call returned KErrPermissionDenied - exit otherwise
//! 
//!							5) Test that the API call returned KErrNone - exit otherwise.
//! 
//!							6) If client exhibits PlatSec capability ReadDeviceData, Test that the API call returned KErrNone - exit otherwise
//!							   If client lacks PlatSec capability ReadDeviceData, Test that the API call returned KErrPermissionDenied - exit otherwise
//! 
//!							7) Test that the API call returned KErrNone - exit otherwise
//! 
//!							8) Test that the associated TRequestStatus object exhibited state KErrNone - exit otherwise
//! 
//!							9) Test that the associated TRequestStatus object exhibited state KErrNone - exit otherwise
//! 
//!							10) Test that the API call returned KErrNone - exit otherwise.
//! 
//!							11) If client exhibits PlatSec capability ReadDeviceData, Test that the API call returned KErrNone - exit otherwise
//!							    If client lacks PlatSec capability ReadDeviceData, Test that the API call returned KErrPermissionDenied - exit otherwise
//! 
//!							12) Test that the API call returned KErrNone - exit otherwise.
//! 
//!							13) If client exhibits PlatSec capability ReadDeviceData, Test that the API call returned KErrNone - exit otherwise
//!							    If client lacks PlatSec capability ReadDeviceData, Test that the API call returned KErrPermissionDenied - exit otherwise
//! 
//!							14) Test that the API call returned KErrNone - exit otherwise.
//! 
//!							15) Test that the API call returned KErrNone - exit otherwise.
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt TestAdditionalThread()
//
// Test that more than one thread can be supported
//
	{
	TInt r;
	if((r=StartThread2())!=KErrNone)
		{
		gTest.Printf(_L("TestAdditionalThread: StartThread2 returned %d\n"),r);
		return KErrGeneral;
		}
	if((r=WaitForThread2())!=KErrNone)
		{
		gTest.Printf(_L("TestAdditionalThread: WaitForThread2 returned %d\n"),r);
		return KErrGeneral;
		}

	return r;
	}


LOCAL_C TInt LocateResourceWithDependencies(TUint &aNumResources, TUint &aResId, TUint &aNumDependents)
//
// Support function for tests of dependencies
//
	{
	__KHEAP_MARK;

	TInt r = KErrNone;
	if((r=gChannel.GetNoOfResources(aNumResources))!=KErrNone)
		{
		gTest.Printf(_L("GetNoOfResources for test channel returned %d\n"),r);
		return r;
		}
	TUint bufSize = aNumResources;
	RSimplePointerArray<TResourceInfoBuf> infoPtrs(bufSize);
	for(TUint i=0;i<bufSize;i++)
		{
		TResourceInfoBuf *info = new TResourceInfoBuf();
		if((r=infoPtrs.Insert(info, i))!=KErrNone)
			{
			gTest.Printf(_L("GetAllResourcesInfo infoPtrs.Insert at index %d returned %d\n"),i,r);
			}
		}
	TUint updateNumResources=aNumResources;
	if((r=gChannel.GetAllResourcesInfo(&infoPtrs,updateNumResources))!=KErrNone)
		{
		gTest.Printf(_L("GetAllResourcesInfo for channel returned %d\n"),r);
		return r;
		}
	for(TUint resNo=0;resNo<aNumResources;resNo++)
		{
		TResourceInfoBuf *info = infoPtrs[resNo];
		TUint resId = ((*info)()).iId;
		TUint numDependents = 0;
		r=gChannel.GetNumDependentsForResource(resId,&numDependents, EFalse); // EFalse - don't load the dependency info
		if((r!=KErrNone)&&(r!=KErrNotFound)&&(r!=KErrNotSupported))
			{
			gTest.Printf(_L("TestTransientHandling: GetNumDependentsForResource returned %d\n"),r);
			return r;
			}
		if(numDependents>0)
			{
			aResId = resId;
			aNumDependents = numDependents;
			break;
			}
		}

	infoPtrs.Close();

	__KHEAP_MARKEND;
	
	return r;
	}



//----------------------------------------------------------------------------------------------
//! @SYMTestCaseID      KBASE-T_RESMANUS-0615
//! @SYMTestType        UT
//! @SYMPREQ            PREQ1398
//! @SYMTestCaseDesc    This test case tests the different options available for handling transient data.
//! @SYMTestActions     0) Locate a resource that has at least one dependency.
//! 
//!						1) Read the number of dependencies without reading the dependency information. 
//!						   Call to read the dependency information (without re-loading).
//! 
//!						2) Read the number of dependencies (with default option of reading the dependency information). 
//!						   Then read the dependency information (without re-loading).
//!
//!						3) Repeat the read the number of dependencies without reading the dependency information
//!						   Then read the dependency information (without re-loading).
//!						   Then read the dependency information (with re-loading).
//!
//!						4) Attempt to read the dependency information (without re-loading) with a buffer of insufficient size 
//! 
//!						5) Read the number of (all) resources without reading the resource information. 
//!						   Call to read (all) the resource information (without re-loading).
//!
//!						6) Read the number of (all) resources (with default option of reading the resource information). 
//!						   Then read (all) the resource information (without re-loading).
//!
//!						7) Repeat the read the number of (all) resources without reading the resources information
//!						   Then read (all) the resources information (without re-loading).
//!						   Then read (all) the resources information (with re-loading).
//!
//!						8) Attempt to read (all) the resource information (without re-loading) with a buffer of insufficient size 
//!
//!						9) Read the number of user-side clients using a resource without reading the resource information. 
//!						   Call to read the information for user-side clients using a resource (without re-loading).
//!
//!						10) Read the number of user-side clients (with loading of the client information)
//!						   Call to read the client information (without re-loading) should return be successful
//!						   Call to read the information for a resourceID with zero clients (without re-loading)
//!						   Call to read the information for the first resourceID
//!
//!						11) Repeat the read of the number of user-side clients (without re-loading)
//!						   Call to read the client information (without re-loading)
//!						   Call to read the client information (with re-loading)
//!
//!						12) Open a second channel on the User-Side API, and call initialise support for resource state access
//!						   Get the current state of a specific resource, then change the state
//!						   Call to read the number of user-side clients for the resource (with loading of the client information)
//!						   but with a buffer of insufficient size.
//!
//!						13) Read the number of resources in use by a specified client (without loading)
//!						   Call to read the information on the resources in use by the client (without loading)
//!
//!						14) Open a third channel on the User-Side API, do not call initialise.
//!						   Read the number of resources in use by a specified client (with loading)
//!						   Call to read the information for resources in use by the third channel (without re-loading)
//!						   Call to read the information for resources in use by the second channel (without re-loading)
//!
//!						15) Read the number of resources in use by a specified client (without loading)
//!						   Call to read the information on the resources in use by the client (without loading)
//!						   Call to read the information on the resources in use by the client (with loading)
//!
//!						16) Read the number of resources in use by a specified client (with loading)
//!						   Call to read the information on the resources in use by the client (without loading) 
//!						   with a buffer of insufficient size.
//!
//! @SYMTestExpectedResults 0) If a suitable resource is found, the number of dependencies is reported - exit otherwise.
//!
//!						1) Test that the read of the number of dependencies returns KErrNone - exit otherwise.
//!						   Test that the read of the dependency information returned KErrNotReady - exit otherwise.
//!
//!						2) Test that the read of the number of dependencies returns KErrNone - exit otherwise.
//!						   Test that the read of the dependency information returned KErrNone - exit otherwise.
//!
//!						3) Test that the read of the number of dependencies returns KErrNone - exit otherwise.
//!						   Test that the read of the dependency information (without re-loading) returned KErrNotReady - exit otherwise.
//!						   Test that the read of the dependency information (with re-loading) returned KErrNone - exit otherwise.
//!
//!						4) Test that the read of the dependency information returns KErrArgument - exit otherwise 
//!
//!						5) Test that the read of the number of resources returns KErrNone - exit otherwise.
//!						   Test that the read of the resource information returned KErrNotReady - exit otherwise.
//!
//!						6) Test that the read of the number of resources returns KErrNone - exit otherwise.
//!						   Test that the read of the resource information returned KErrNone - exit otherwise.
//!
//!						7) Test that the read of the number of resources returns KErrNone - exit otherwise.
//!						   Test that the read of the resource information (without re-loading) returned KErrNotReady - exit otherwise.
//!						   Test that the read of the resources information (with re-loading) returned KErrNone - exit otherwise.
//!
//!						8) Test that the read of the resource information returns KErrArgument - exit otherwise 
//!
//!						9) Test that the read of the number of clients returns KErrNone - exit otherwise.
//!						   Test that the read of the client information returned KErrNotReady - exit otherwise.
//!
//!						10) Test that the read of the number of clients returns KErrNone - exit otherwise.
//!						  Test that the read of the client information returned KErrNone - exit otherwise.
//!						  Test that the read of the client information returned KErrNone and numClients==0 - exit otherwise.
//!						  Test that the read of the client information returned KErrNotReady - exit otherwise.
//!
//!						11) Test that the read of the number of clients returns KErrNone - exit otherwise.
//!						  Test that the read of the client information returned KErrNotReady - exit otherwise.
//!						  Test that the read of the client information returned KErrNone - exit otherwise.
//!
//!						12) Test that the opening and initialisation of the channel was successful - exit otherwise.
//!						  Test that the get and change of the resource state completed successfully - exit otherwise
//!						  Test that the read of the client information returned KErrArgument - exit otherwise.
//!
//!						13) Test that the read of the number of resources returns KErrNone - exit otherwise.
//!						  Test that the read of the resource information returned KErrNotReady - exit otherwise.
//!
//!						14) Test that the opening of the channel was successful - exit otherwise.
//!						   Test that the read of the resource information returned KErrNone - exit otherwise.
//!						   Test that the read of the client information returned KErrNone, and numResources = 0 - exit otherwise.
//!						   Test that the read of the client information returned KErrNotReady - exit otherwise.
//!
//!						15) Test that the read of the number of resources returns KErrNone - exit otherwise.
//!						   Test that the read of the resource information returned KErrNotReady - exit otherwise.
//!						   Test that the read of the resource information returned KErrNone - exit otherwise.
//!
//!						16) Test that the read of the number of resources returns KErrNone - exit otherwise.
//!						   Test that the read of the resource information returned KErrArgument - exit otherwise..
//!
//! @SYMTestPriority        High
//! @SYMTestStatus          Implemented
//----------------------------------------------------------------------------------------------
LOCAL_C TInt TestTransientHandling()
//
// Test APIs with options for handling transient data (dependencies, resources and clients)
//
	{
	TInt r = KErrNone;
	TUint testNo = 1;

	TUint numDependents = 0; 
	TUint resNo = 0;

	//
	//					Dependency data tests
	//

	gTest.Printf(_L("TestTransientHandling: dependency data tests ...\n"));

	// 0) Find a resource that has dependents - if none is located skip the remaining dependancy tests
	// (this reads the number of dependencies, without loading the dependency data)
	//
	TUint numResources=0;
	if((r=LocateResourceWithDependencies(numResources, resNo, numDependents))!=KErrNone)
		{
		gTest.Printf(_L("TestTransientHandling: no resource with dependencies found ... skipping tests\n"));
		return KErrNone;
		}
	if(numDependents==0)
		{
		gTest.Printf(_L("TestTransientHandling: no resource with dependencies found ... skipping tests\n"));
		}
	else
		{
		// 1) Read the number of dependencies without reading the dependency information
		// Subsequent call to read the dependency information (without re-loading) should return error KErrNotReady
		gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		if((r=gChannel.GetNumDependentsForResource(resNo,&numDependents, EFalse))!=KErrNone)
			{
			gTest.Printf(_L("TestTransientHandling: GetNumDependentsForResource returned %d\n"),r);
			return r;
			}
		gTest.Printf(_L("TestTransientHandling: GetNumDependentsForResource reported %d dependents for resource %d\n"),numDependents,resNo);
		if(numDependents > 0)
			{
			RBuf8 buffer;
			if((buffer.Create(numDependents*sizeof(SResourceDependencyInfo)))!=KErrNone)
				return KErrGeneral;
			buffer.SetLength(0);
			if((r=gChannel.GetDependentsIdForResource(resNo, buffer, &numDependents))!=KErrNotReady)
				{
				gTest.Printf(_L("TestTransientHandling: GetDependentsIdForResource returned %d\n"),r);
				return KErrGeneral;
				}
			buffer.Close();
			}

		// 2) Read the number of dependencies (and, by default, read the dependency information)
		// Subsequent call to read the dependency information (without re-loading) should return be successful
		gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		if((r=gChannel.GetNumDependentsForResource(resNo,&numDependents))!=KErrNone)
			{
			gTest.Printf(_L("TestTransientHandling: GetNumDependentsForResource returned %d\n"),r);
			return r;
			}
		gTest.Printf(_L("TestTransientHandling: GetNumDependentsForResource reported %d dependents for resource %d\n"),numDependents,resNo);
		if(numDependents > 0)
			{
			RBuf8 buffer;
			if((buffer.Create(numDependents*sizeof(SResourceDependencyInfo)))!=KErrNone)
				return KErrGeneral;
			buffer.SetLength(0);
			if((r=gChannel.GetDependentsIdForResource(resNo, buffer, &numDependents))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling: GetDependentsIdForResource returned %d\n"),r);
				return r;
				}
			SResourceDependencyInfo* tempPtr = (SResourceDependencyInfo*)(buffer.Ptr());
			gTest.Printf(_L("TestTransientHandling: numDependents = %d\n"),numDependents);
			for(TUint i=0; i<numDependents; i++,tempPtr++)
				{
				gTest.Printf(_L("TestTransientHandling: info.iResourceId = %d\n"),tempPtr->iResourceId);
				gTest.Printf(_L("TestTransientHandling: info.iDependencyPriority= %d\n"),tempPtr->iDependencyPriority);
				}
			buffer.Close();
			}

		// 3) Repeat the read the number of dependencies without reading the dependency information
		// Subsequent call to read the dependency information (without re-loading) should return error KErrNotReady
		// Then call to read the dependency information (with re-loading) should be successful
		gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		if((r=gChannel.GetNumDependentsForResource(resNo,&numDependents, EFalse))!=KErrNone)
			{
			gTest.Printf(_L("TestTransientHandling: GetNumDependentsForResource returned %d\n"),r);
			return r;
			}
		gTest.Printf(_L("TestTransientHandling: GetNumDependentsForResource reported %d dependents for resource %d\n"),numDependents,resNo);
		if(numDependents > 0)
			{
			RBuf8 buffer;
			if((buffer.Create(numDependents*sizeof(SResourceDependencyInfo)))!=KErrNone)
				return KErrGeneral;
			buffer.SetLength(0);

			if((r=gChannel.GetDependentsIdForResource(resNo, buffer, &numDependents))!=KErrNotReady)
				{
				gTest.Printf(_L("TestTransientHandling: GetDependentsIdForResource returned %d\n"),r);
				return KErrGeneral;
				}
			if((r=gChannel.GetDependentsIdForResource(resNo, buffer, &numDependents, ETrue))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling: GetDependentsIdForResource returned %d\n"),r);
				return KErrGeneral;
				}
			SResourceDependencyInfo* tempPtr = (SResourceDependencyInfo*)(buffer.Ptr());
			for(TUint i=0; i<numDependents; i++,tempPtr++)
				{
				gTest.Printf(_L("TestTransientHandling: info.iResourceId = %d"),tempPtr->iResourceId);
				gTest.Printf(_L("TestTransientHandling: info.iDependencyPriority= %d"),tempPtr->iDependencyPriority);
				}
			buffer.Close();
			}

		// 4) Attempt to read the dependency information (without re-loading) with a buffer of insufficient size should
		// return error KErrArgument
		gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		if(numDependents > 0)
			{
			RBuf8 buffer;
			if((buffer.Create((numDependents-1)*sizeof(SResourceDependencyInfo)))!=KErrNone)
				return KErrGeneral;
			buffer.SetLength(0);
			if((r=gChannel.GetDependentsIdForResource(resNo, buffer, &numDependents))!=KErrArgument)
				{
				gTest.Printf(_L("TestTransientHandling: GetDependentsIdForResource returned %d\n"),r);
				return KErrGeneral;
				}
			// Ensure misleading result is not returned
			r=KErrNone;
			buffer.Close();
			}
		}
	

	//
	//					All resource data tests
	//

	gTest.Printf(_L("TestTransientHandling: All resource data tests ...\n"));
	testNo=1;

	// 5) Attempt to read the resource information without having previously loaded it.
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numResources = 0;
		r = gChannel.GetNoOfResources(numResources, EFalse); // EFalse - don't load the resource info
		if(r!=KErrNone)
			{
			gTest.Printf(_L("GetNoOfResources returned %d\n"),r);
			return r;
			}
		// To support the GetAllResourcesInfo testing, instantiate TResourceInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TResourceInfoBuf> infoPtrs(numResources);
		for(TUint i=0;i<numResources;i++)
			{
			TResourceInfoBuf *info = new TResourceInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetAllResourcesInfo infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetAllResourcesInfo(&infoPtrs, numResources))!=KErrNotReady)
			{
			gTest.Printf(_L("TestTransientHandling: GetAllResourcesInfo returned %d\n"),r);
			return KErrGeneral;
			}
		else
			r=KErrNone;	// Ensure misleading result is not propagated
		infoPtrs.Close();
		}

	// 6) Read the number of resources (and, by default, read the resource information)
	// Subsequent call to read the resource information (without re-loading) should return be successful
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numResources = 0;
		if((r=gChannel.GetNoOfResources(numResources))!=KErrNone)
			{
			gTest.Printf(_L("GetNoOfResources returned %d\n"),r);
			return r;
			}
		// To support the GetAllResourcesInfo testing, instantiate TResourceInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TResourceInfoBuf> infoPtrs(numResources);
		for(TUint i=0;i<numResources;i++)
			{
			TResourceInfoBuf *info = new TResourceInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetAllResourcesInfo infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetAllResourcesInfo(&infoPtrs, numResources))!=KErrNone)
			{
			gTest.Printf(_L("TestTransientHandling: GetAllResourcesInfo returned %d\n"),r);
			return r;
			}
		infoPtrs.Close();
		}

	// 7) Repeat the read the number of resources without reading the resource information
	// Subsequent call to read the resource information (without re-loading) should return error KErrNotReady
	// Then call to read the resource information (with re-loading) should be successful
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numResources = 0;
		if((r=gChannel.GetNoOfResources(numResources, EFalse))!=KErrNone)	// EFalse - don't load the resource info
			{
			gTest.Printf(_L("GetNoOfResources returned %d\n"),r);
			return r;
			}
		// To support the GetAllResourcesInfo testing, instantiate TResourceInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TResourceInfoBuf> infoPtrs(numResources);
		for(TUint i=0;i<numResources;i++)
			{
			TResourceInfoBuf *info = new TResourceInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetAllResourcesInfo infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetAllResourcesInfo(&infoPtrs, numResources))!=KErrNotReady)
			{
			gTest.Printf(_L("TestTransientHandling: GetAllResourcesInfo returned %d\n"),r);
			return KErrGeneral;
			}
		if((r=gChannel.GetAllResourcesInfo(&infoPtrs, numResources,ETrue))!=KErrNone)
			{
			gTest.Printf(_L("TestTransientHandling: GetAllResourcesInfo returned %d\n"),r);
			return r;
			}
		else
		infoPtrs.Close();
		}


	// 8) Attempt to read the resource information (without re-loading) with a buffer of insufficient size should
	// return error KErrArgument
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numResources = 0;
		if((r=gChannel.GetNoOfResources(numResources, EFalse))!=KErrNone)	// EFalse - don't load the resource info
			{
			gTest.Printf(_L("GetNoOfResources returned %d\n"),r);
			return r;
			}
		// To support the GetAllResourcesInfo testing, instantiate TResourceInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TResourceInfoBuf> infoPtrs(numResources - 1);
		for(TUint i=0;i<(numResources-1);i++)
			{
			TResourceInfoBuf *info = new TResourceInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetAllResourcesInfo infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetAllResourcesInfo(&infoPtrs, numResources))!=KErrArgument)
			{
			gTest.Printf(_L("TestTransientHandling: GetAllResourcesInfo returned %d\n"),r);
			return KErrGeneral;
			}
		// Ensure misleading result is not returned
		r=KErrNone;
		infoPtrs.Close();
		}

	//
	//					Specific resource data tests
	//

	gTest.Printf(_L("TestTransientHandling: Resource-specific data tests ...\n"));
	testNo=1;

	// 9) Attempt to read the resource information without having previously loaded it.
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numClients = 0;
		if((r=gChannel.GetNumClientsUsingResource(gLongLatencyResource, numClients, EFalse, EFalse))!=KErrNone) // user-side clients, don't load the info
			{
			gTest.Printf(_L("GetNumClientsUsingResource returned %d\n"),r);
			return r;
			}
		// To support the GetInfoOnClientsUsingResource testing, instantiate TClientInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TClientInfoBuf> infoPtrs(numClients);
		for(TUint i=0;i<numClients;i++)
			{
			TClientInfoBuf *info = new TClientInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetInfoOnClientsUsingResource infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetInfoOnClientsUsingResource(gLongLatencyResource, numClients, &infoPtrs, EFalse))!=KErrNotReady)
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnClientsUsingResource returned %d\n"),r);
			return KErrGeneral;
			}
		else
			r=KErrNone;	// Ensure misleading result is not propagated
		infoPtrs.Close();
		}

	// 10) Read the number of clients (and, by default, read the client information)
	// Subsequent call to read the client information (without re-loading) should return be successful
	// Call to read the information for a resourceID with zero clients (without re-loading) should return KErrNone, numClients==0
	// Call to read the information for a resourceID with one or more clients (without re-loading) should return KErrNotReady
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numClients = 0;
		if((r=gChannel.GetNumClientsUsingResource(gLongLatencyResource, numClients, EFalse))!=KErrNone) // user-side clients, load the info
			{
			gTest.Printf(_L("GetNumClientsUsingResource returned %d\n"),r);
			return r;
			}
		// To support the GetInfoOnClientsUsingResource testing, instantiate TClientInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TClientInfoBuf> infoPtrs(numClients);
		for(TUint i=0;i<numClients;i++)
			{
			TClientInfoBuf *info = new TClientInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetInfoOnClientsUsingResource infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetInfoOnClientsUsingResource(gLongLatencyResource, numClients, &infoPtrs, EFalse))!=KErrNone)
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnClientsUsingResource for gLongLatencyResource returned %d\n"),r);
			return r;
			}
		if(((r=gChannel.GetInfoOnClientsUsingResource((gLongLatencyResource+1), numClients, &infoPtrs, EFalse))!=KErrNone) || (numClients!=0))
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnClientsUsingResource for (gLongLatencyResource+1) returned %d\n"),r);
			if(numClients!=0)
				gTest.Printf(_L("TestTransientHandling: GetInfoOnClientsUsingResource for (gLongLatencyResource+1), %d clients\n"),numClients);
			return KErrGeneral;
			}
		if((r=gChannel.GetInfoOnClientsUsingResource(gSharedResource, numClients, &infoPtrs, EFalse))!=KErrNotReady)
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnClientsUsingResource (for gSharedResource) returned %d\n"),r);
			return KErrGeneral;
			}
		infoPtrs.Close();
		}


	// 11) Repeat the read the number of clients without reading the client information
	// Subsequent call to read the client information (without re-loading) should return error KErrNotReady
	// Then call to read the client information (with re-loading) should be successful
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numClients = 0;
		if((r=gChannel.GetNumClientsUsingResource(gLongLatencyResource, numClients, EFalse, EFalse))!=KErrNone) // user-side clients, don't load the info
			{
			gTest.Printf(_L("GetNumClientsUsingResource returned %d\n"),r);
			return r;
			}
		// To support the GetInfoOnClientsUsingResource testing, instantiate TClientInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TClientInfoBuf> infoPtrs(numClients);
		for(TUint i=0;i<numClients;i++)
			{
			TClientInfoBuf *info = new TClientInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetInfoOnClientsUsingResource infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetInfoOnClientsUsingResource(gLongLatencyResource, numClients, &infoPtrs, EFalse))!=KErrNotReady)
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnClientsUsingResource for gLongLatencyResource returned %d\n"),r);
			return KErrGeneral;
			}
		if((r=gChannel.GetInfoOnClientsUsingResource(gLongLatencyResource, numClients, &infoPtrs, EFalse, ETrue))!=KErrNone)
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnClientsUsingResource for gLongLatencyResource returned %d\n"),r);
			return r;
			}

		infoPtrs.Close();
		}

	// 12) To support the following test (and specific resource data tests, below) need a second channel to be using the resource
	_LIT(tempStr1,"ExtraChan1");
	TBufC<16> tempName1(tempStr1);
	RBusDevResManUs channelTwo;
	if((r=OpenChannel(tempName1, channelTwo))!=KErrNone)
		{
		gTest.Printf(_L("Failed to open channelTwo, %d\n"),r);
		channelTwo.Close();
		return r;
		}
	if ((r=channelTwo.Initialise(1,1,1))!=KErrNone)
		{
		gTest.Printf(_L("Failed to Initialise channelTwo, %d\n"),r);
		channelTwo.Close();
		return r;
		}
	// Attempt to change the resource level
	// Get initial state
	TRequestStatus status;
	TBool cached = gUseCached;
	TInt readValue;
	TInt levelOwnerId = 0;
	channelTwo.GetResourceState(status,gSharedResource,cached,&readValue,&levelOwnerId);
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("GetResourceState for gSharedResource completed with = 0x%x\n"),r);
		return r;
		}
	// Write updated state
	TUint newLevel = (TUint)(readValue + gSharedResStateDelta);
	channelTwo.ChangeResourceState(status,gSharedResource,newLevel);
	User::WaitForRequest(status);
	r=status.Int();
	if(r != KErrNone)
		{
		gTest.Printf(_L("ChangeResourceState forgSharedResource completed with %d\n"),r);
		return r;
		}

	// Attempt to read the client information (without re-loading) with a buffer of insufficient size should
	// return error KErrArgument
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numClients = 0;
		if((r=gChannel.GetNumClientsUsingResource(gSharedResource, numClients, EFalse))!=KErrNone) // user-side clients, load the info
			{
			gTest.Printf(_L("GetNumClientsUsingResource returned %d\n"),r);
			channelTwo.Close();
			return r;
			}
		// To support the GetInfoOnClientsUsingResource testing, instantiate TClientInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TClientInfoBuf> infoPtrs(numClients-1);
		for(TUint i=0;i<(numClients-1);i++)
			{
			TClientInfoBuf *info = new TClientInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetInfoOnClientsUsingResource infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				channelTwo.Close();
				}
			}
		if((r=gChannel.GetInfoOnClientsUsingResource(gSharedResource, numClients, &infoPtrs, EFalse))!=KErrArgument)
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnClientsUsingResource for gLongLatencyResource returned %d\n"),r);
			channelTwo.Close();
			return KErrGeneral;
			}
		// Ensure misleading result is not returned
		r=KErrNone;
		infoPtrs.Close();
		}


	//
	//					Specific resource data tests
	//

	gTest.Printf(_L("TestTransientHandling: Client-specific data tests ...\n"));
	testNo=1;

	// These tests require a client name
	TBuf8<MAX_RESOURCE_NAME_LENGTH+1>name8Bit;
	name8Bit.Copy(gTestName);
	TClientName* clientName = (TClientName*)&name8Bit;
#if _DEBUG
	TBuf <MAX_CLIENT_NAME_LENGTH> clientName16Bit;
	clientName16Bit.Copy(*clientName);
	clientName16Bit.SetLength(clientName->Length());
	gTest.Printf(_L("Invoking TestTransientHandling client-specific data tests  with %S \n"),&clientName16Bit);
#endif
	// 13) Attempt to read the resource information without having previously loaded it.
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numResources = 0;
		if((r=gChannel.GetNumResourcesInUseByClient(*clientName, numResources, EFalse))!=KErrNone) // EFalse - don't load data
			{
			gTest.Printf(_L("GetNumResourcesInUseByClient returned %d\n"),r);
			return r;
			}
		// To support the GetInfoOnResourcesInUseByClient testing, instantiate TResourceInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TResourceInfoBuf> infoPtrs(numResources);
		for(TUint i=0;i<numResources;i++)
			{
			TResourceInfoBuf *info = new TResourceInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetAllResourcesInfo infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetInfoOnResourcesInUseByClient(*clientName, numResources, &infoPtrs))!=KErrNotReady)
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnClientsUsingResource returned %d\n"),r);
			return KErrGeneral;
			}
		else
			r=KErrNone;	// Ensure misleading result is not propagated
		infoPtrs.Close();
		}

	// 14) To support the following test need a third channel
	_LIT(tempStr2,"ExtraChan2");
	TBufC<16> tempName2(tempStr2);
	RBusDevResManUs channelThree;
	if((r=OpenChannel(tempName2, channelThree))!=KErrNone)
		{
		gTest.Printf(_L("Failed to open channelThree, %d\n"),r);
		channelTwo.Close();
		return r;
		}
	// Read the number of resources (and, by default, read the resource information)
	// Subsequent call to read the resource information (without re-loading) should return be successful
	// Call to read the information for a client name with zero resource requirements (without re-loading) should return KErrNone, numResources==0
	// Call to read the information for a client name with one or more resource requirements (without re-loading) should return KErrNotReady
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numResources = 0;
		if((r=gChannel.GetNumResourcesInUseByClient(*clientName, numResources))!=KErrNone) // load data
			{
			gTest.Printf(_L("GetNumResourcesInUseByClient returned %d\n"),r);
			return r;
			}
		// To support the GetInfoOnResourcesInUseByClient testing, instantiate TResourceInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TResourceInfoBuf> infoPtrs(numResources);
		for(TUint i=0;i<numResources;i++)
			{
			TResourceInfoBuf *info = new TResourceInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetAllResourcesInfo infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetInfoOnResourcesInUseByClient(*clientName, numResources, &infoPtrs))!=KErrNone)
			{
			gTest.Printf(_L("TestTransientHandling: gChannel GetInfoOnClientsUsingResource returned %d\n"),r);
			return KErrGeneral;
			}
		TUint dumResources=0;
		TBuf8<MAX_RESOURCE_NAME_LENGTH+1>name8Bit2;
		name8Bit2.Copy(tempName2);
		TClientName* clientName2 = (TClientName*)&name8Bit2;
		if((r=gChannel.GetInfoOnResourcesInUseByClient(*clientName2, dumResources, &infoPtrs))!=KErrNone)
			{
			gTest.Printf(_L("TestTransientHandling: tempName2 GetInfoOnClientsUsingResource returned %d\n"),r);
			return KErrGeneral;
			}
		if(dumResources!=0)
			{
			gTest.Printf(_L("TestTransientHandling: tempName2 GetInfoOnClientsUsingResource dumResources=%d\n"),dumResources);
			return KErrGeneral;
			}
		TBuf8<MAX_RESOURCE_NAME_LENGTH+1>name8Bit1;
		name8Bit1.Copy(tempName1);
		TClientName* clientName1 = (TClientName*)&name8Bit1;
		if((r=gChannel.GetInfoOnResourcesInUseByClient(*clientName1, numResources, &infoPtrs))!=KErrNotReady)
			{
			gTest.Printf(_L("TestTransientHandling: tempName1 GetInfoOnClientsUsingResource returned %d\n"),r);
			return KErrGeneral;
			}

		infoPtrs.Close();
		}


	// 15) Repeat the read the number of resources without reading the resource information
	// Subsequent call to read the resources information (without re-loading) should return error KErrNotReady
	// Then call to read the resources information (with re-loading) should be successful
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numResources = 0;
		if((r=gChannel.GetNumResourcesInUseByClient(*clientName, numResources, EFalse))!=KErrNone) // don't load data
			{
			gTest.Printf(_L("GetNumResourcesInUseByClient returned %d\n"),r);
			return r;
			}
		// To support the GetInfoOnResourcesInUseByClient testing, instantiate TResourceInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TResourceInfoBuf> infoPtrs(numResources);
		for(TUint i=0;i<numResources;i++)
			{
			TResourceInfoBuf *info = new TResourceInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetAllResourcesInfo infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetInfoOnResourcesInUseByClient(*clientName, numResources, &infoPtrs))!=KErrNotReady)
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnResourcesInUseByClient returned %d\n"),r);
			return KErrGeneral;
			}
		if((r=gChannel.GetInfoOnResourcesInUseByClient(*clientName, numResources, &infoPtrs, ETrue))!=KErrNone)
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnResourcesInUseByClient returned %d\n"),r);
			return r;
			}
		infoPtrs.Close();
		}


	// 16) Attempt to read the resource information (without re-loading) with a buffer of insufficient size should
	// return error KErrArgument
	gTest.Printf(_L("TestTransientHandling: test %d\n"),testNo++);
		{
		TUint numResources = 0;
		if((r=gChannel.GetNumResourcesInUseByClient(*clientName, numResources))!=KErrNone) // load data
			{
			gTest.Printf(_L("GetNumResourcesInUseByClient returned %d\n"),r);
			return r;
			}
		// To support the GetInfoOnResourcesInUseByClient testing, instantiate TResourceInfoBuf objects
		// and reference via an RSimplePointerArray
		RSimplePointerArray<TResourceInfoBuf> infoPtrs(numResources-1);
		for(TUint i=0;i<(numResources-1);i++)
			{
			TResourceInfoBuf *info = new TResourceInfoBuf();
			if((r=infoPtrs.Insert(info, i))!=KErrNone)
				{
				gTest.Printf(_L("TestTransientHandling test, GetAllResourcesInfo infoPtrs.Insert at index %d returned %d\n"),testNo,i,r);
				}
			}
		if((r=gChannel.GetInfoOnResourcesInUseByClient(*clientName, numResources, &infoPtrs))!=KErrArgument)
			{
			gTest.Printf(_L("TestTransientHandling: GetInfoOnResourcesInUseByClient returned %d\n"),r);
			return KErrGeneral;
			}

		// Ensure misleading result is not returned
		r=KErrNone;
		infoPtrs.Close();
		}

	channelTwo.Close();
	channelThree.Close();

	return r;
	}


EXPORT_C TInt E32Main()
//
// Main
//
	{
	gTest.Title();
	gTest.Start(_L("Test Power Resource Manager user side API\n"));

	// Test attempted load of PDD
	gTest.Next(_L("**Load PDD\n"));
	TInt r = User::LoadPhysicalDevice(PDD_NAME);
	gTest((r == KErrNone) || (r == KErrAlreadyExists));
	gTest.SetCleanupFlag(EPddLoaded);

	// Test attempted load of LDD
	gTest.Next(_L("**Load LDD\n"));
	r = User::LoadLogicalDevice(LDD_NAME);
	gTest((r == KErrNone) || (r == KErrAlreadyExists));
	r = KErrNone; // Re-initialise in case set to KErrAlreadyExists
	gTest.SetCleanupFlag(ELddLoaded);

	// test caps
	gTest(CheckCaps() == KErrNone);

	// Need a channel open for the following tests
	gTest.Next(_L("**OpenAndRegisterChannel\n"));
	r = OpenAndRegisterChannel();
	gTest(r == KErrNone);
	gTest.SetCleanupFlag(EChannelOpened);

	// Get the version of the ResourceController
	TUint version;
	r = gChannel.GetResourceControllerVersion(version);
	gTest.Printf(_L("TestTransientHandling: ResourceController version =0x%x\n"), version);
	gTest(r == KErrNone);

	gTest.Next(_L("**TestThreadExclusiveAccess\n"));
	r = TestThreadExclusiveAccess();
	gTest(r == KErrNone);

	gTest.Next(_L("**TestGetClientGetResourceInfo - initial state\n"));
	r = TestGetClientGetResourceInfo();
	gTest(r == KErrNone);

	gTest.Next(_L("**TestGetSetResourceStateOps\n"));
	r = TestGetSetResourceStateOps();
	gTest(r == KErrNone);

	gTest.Next(_L("**TestGetClientGetResourceInfo - after changing stateof Async resource\n"));
	r = TestGetClientGetResourceInfo();
	gTest(r == KErrNone);

	gTest.Next(_L("**TestGetSetResourceStateQuota\n"));
	r = TestGetSetResourceStateQuota();
	gTest(r == KErrNone);

	gTest.Next(_L("**TestNotificationOps\n"));
	r = TestNotificationOps();
	gTest(r == KErrNone);

	gTest.Next(_L("**TestNotificationQuota\n"));
	r = TestNotificationQuota();
	gTest(r == KErrNone);

	// Should be no change since last invocation (assuming that
	// no clients other than those in this test)
	gTest.Next(_L("**TestGetClientGetResourceInfo - last invocation\n"));
	r = TestGetClientGetResourceInfo();
	gTest(r == KErrNone);

	gTest.Next(_L("**TestAdditionalThread\n"));
	r = TestAdditionalThread();
	gTest(r == KErrNone);

	gTest.Next(_L("**TestTransientHandling\n"));
	r = TestTransientHandling();
	gTest(r == KErrNone);

	gTest.End();
	return KErrNone;
	}


