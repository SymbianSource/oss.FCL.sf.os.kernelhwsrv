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
// Bootstrap Shadow Memory Region Test Driver
//


// -- INCLUDES ----------------------------------------------------------------

#include "d_trace.h"

#include <memmodel/epoc/platform.h>
#include <bootdefs.h>
#include <kernel/kernboot.h>
#include <kernel/kern_priv.h>
#include <platform.h>
#include <u32hal.h>
#include "d_smr.h"


// -- CLASSES -----------------------------------------------------------------


class DSMRTestFactory : public DLogicalDevice
	{
public:
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};


class DSMRTestChannel : public DLogicalChannelBase
	{
public:
	DSMRTestChannel();
	virtual ~DSMRTestChannel();
	
	//	Inherited from DObject
	virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
	
	// Inherited from DLogicalChannelBase
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
		
public:
	static void TestTrace(DSMRTestChannel* aSelf);
	
private:
	DThread* iClient;
    
	};

TInt OpenDumpCloseChunk(TUint32 aBase, TUint32 aSize);


// -- GLOBALS -----------------------------------------------------------------
//



// -- METHODS -----------------------------------------------------------------
//
// TEST FUNCTIONS
//

static TInt ECtrlCheckSMRIBPtr()
	{
	SMR_FUNC("DSMRTestChannel::Request::ECtrlCheckSMRIBPtr");

    TSuperPage& superPage = Kern::SuperPage();
    TLinAddr smrib = superPage.iSmrData;

    SMR_TRACE1("SMRIB - Virtual address %x", smrib);

    return smrib;
	}
	
static TInt ECtrlPrintSMRIB()
	{
	SMR_FUNC("DSMRTestChannel::Request::ECtrlPrintSMRIB");

    TSuperPage& superPage = Kern::SuperPage();
    TLinAddr smrib = superPage.iSmrData;

    SMR_TRACE1("SMRIB - Virtual address %x", smrib);

    if (smrib == KSuperPageAddressFieldUndefined)
    	SMR_LOGMSG_RETURN ("SMRIB Does not exist!", KErrBadHandle);
    
    SSmrBank* smrPtr = (SSmrBank*)(smrib);
    int x=0;
    while (smrPtr->iBase != 0)
        {
        SMR_TRACE6("SMRIB entry %d (0x%x): %x, %x, %x, %x", x, smrPtr, smrPtr->iBase, smrPtr->iSize, smrPtr->iPayloadUID, smrPtr->iPayloadFlags);
        x++;
        smrPtr++;
        }
    if (x==0)
    	SMR_TRACE0("SMRIB Zero, no valid entries");
    else
    	SMR_TRACE1("SMRIB Contained %d entries", x);

    return x;
	}


static TInt ECtrlAccessAllSMRs()
	{
	SMR_FUNC("DSMRTestChannel::Request::ECtrlAccessAllSMRs");

    TInt err=0; 
    TSuperPage& superPage = Kern::SuperPage();
    TLinAddr smrib = superPage.iSmrData;

    if (smrib == KSuperPageAddressFieldUndefined)
        return KErrBadHandle;
    
    SSmrBank* smrPtr = (SSmrBank*)(smrib);
    int x=0;
    while (smrPtr->iBase != 0)
        {
        SMR_TRACE6("SMRIB item %d (0x%x): %x, %x, %x, %x", x, smrPtr, smrPtr->iBase, smrPtr->iSize, smrPtr->iPayloadUID, smrPtr->iPayloadFlags);
        
        err = OpenDumpCloseChunk(smrPtr->iBase, smrPtr->iSize);
		if (err != KErrNone)
  			return err;
  			
        x++;
        smrPtr++;
        }
    if (x==0)
    	SMR_TRACE0("SMRIB Zero, no valid entries");
    else
    	SMR_TRACE1("SMRIB Contained %d entries", x);
   
	return x;
	}
	
	
static TInt ECtrlFreeHalfSMR1PhysicalRam()
	{
	SMR_FUNC("DSMRTestChannel::Request::ECtrlFreeHalfSMR1PhysicalRam");

    TInt err=0; 
    TSuperPage& superPage = Kern::SuperPage();
    TLinAddr smrib = superPage.iSmrData;
    

    if (smrib == KSuperPageAddressFieldUndefined)
        return KErrBadHandle;
    
    SSmrBank* smrPtr = (SSmrBank*)(smrib);
    int x=0;
    
    if ((smrPtr->iBase == 0) || (smrPtr->iSize == 0))
        SMR_LOGMSG_RETURN("SMRIB Does not contain one entry!", 0); 
        
    SMR_TRACE6("SMRIB item before %d (0x%x): %x, %x, %x, %x", x, smrPtr, smrPtr->iBase, smrPtr->iSize, smrPtr->iPayloadUID, smrPtr->iPayloadFlags);
	
	TInt halfSize = smrPtr->iSize >> 1;
	
	NKern::ThreadEnterCS();
    err = Epoc::FreePhysicalRam(smrPtr->iBase+halfSize, halfSize);
    NKern::ThreadLeaveCS();
    if (err != KErrNone)
        SMR_LOGMSG_RETURN("Epoc::FreePhysicalRam() gave error", err) 
    else
    	SMR_TRACE0("Success - half of physical ram freed for SMR 1");	
 
	smrPtr->iSize = halfSize;
	
    SMR_TRACE6("SMRIB item after %d (0x%x): %x, %x, %x, %x", x, smrPtr, smrPtr->iBase, smrPtr->iSize, smrPtr->iPayloadUID, smrPtr->iPayloadFlags);

    err = OpenDumpCloseChunk(smrPtr->iBase, smrPtr->iSize);
	if (err != KErrNone)
  		return err;

	return halfSize;
	}


static TInt ECtrlFreeAllSMR2PhysicalRam()
	{
	SMR_FUNC("DSMRTestChannel::Request::ECtrlFreeAllSMR2PhysicalRam");

    TInt err=0; 
    TSuperPage& superPage = Kern::SuperPage();
    TLinAddr smrib = superPage.iSmrData;

    if (smrib == KSuperPageAddressFieldUndefined)
        return KErrBadHandle;
    
    SSmrBank* smrPtr = (SSmrBank*)(smrib);
    int x=0;

	if ((smrPtr->iBase == 0) || (smrPtr->iSize == 0))
        SMR_LOGMSG_RETURN("SMRIB Does not contain first entry!", 0);
    
    smrPtr++; x++;
	if ((smrPtr->iBase == 0) || (smrPtr->iSize == 0))
        SMR_LOGMSG_RETURN("SMRIB Does not contain two entries!", 0);

    smrPtr++; x++;
	if ((smrPtr->iBase == 0) || (smrPtr->iSize == 0))
        SMR_LOGMSG_RETURN("SMRIB Does not contain three entries!", 0);

    SMR_TRACE6("SMRIB item before %d (0x%x): %x, %x, %x, %x", x, smrPtr, smrPtr->iBase, smrPtr->iSize, smrPtr->iPayloadUID, smrPtr->iPayloadFlags);        
   	TInt sizeToFree = smrPtr->iSize;
	
	NKern::ThreadEnterCS();
    err = Epoc::FreePhysicalRam(smrPtr->iBase, sizeToFree);
    NKern::ThreadLeaveCS();
    if (err != KErrNone)
        SMR_LOGMSG_RETURN("Epoc::FreePhysicalRam() gave error", err) 
    else
    	SMR_TRACE0("Success - all physical ram freed for SMR 2");	
        

	smrPtr->iBase = 0;
	smrPtr->iSize = 0;
	smrPtr->iPayloadUID = 0;
	smrPtr->iPayloadFlags = 0;
	
    SMR_TRACE6("SMRIB item after %d (0x%x): %x, %x, %x, %x", x, smrPtr, smrPtr->iBase, smrPtr->iSize, smrPtr->iPayloadUID, smrPtr->iPayloadFlags);
	return sizeToFree;
	}


TInt OpenDumpCloseChunk(TUint32 aBase, TUint32 aSize)
	{
	TInt err;
    TChunkCreateInfo cci;
    DChunk *chunkPtr;
    TLinAddr kernAddr = 0;
    TUint32 mapAttr = 0;
    
    cci.iType = TChunkCreateInfo::ESharedKernelSingle;
    cci.iMaxSize = 0x800000;
    cci.iMapAttr = EMapAttrCachedMax | EMapAttrSupRw;
    cci.iOwnsMemory = EFalse;
    cci.iDestroyedDfc = 0;
    
    NKern::ThreadEnterCS();
    err = Kern::ChunkCreate(cci, chunkPtr, kernAddr, mapAttr);
    NKern::ThreadLeaveCS();
    if (err != KErrNone)
    	SMR_LOGMSG_RETURN("Kern::ChunkCreate() gave error", err);
         
    NKern::ThreadEnterCS();
    err = Kern::ChunkCommitPhysical(chunkPtr, 0, aSize, aBase);
    NKern::ThreadLeaveCS();
    if (err != KErrNone)
    	SMR_LOGMSG_RETURN("Kern::ChunkCommitPhysical() gave error", err); 
        
    TUint32* setting = (TUint32*)(kernAddr); 
    SMR_TRACE1("SMR Image Memory Dump First Kb @ %08x", setting);
            
    for (TInt y=0; y < 0x80; y+=16)
        {
        SMR_TRACE5("  %08x:  %08x  %08x  %08x  %08x", setting, setting[0], setting[1], setting[2], setting[3]);
        setting+=4;
        }
        
    setting = (TUint32*)(kernAddr+aSize-0x80);
	SMR_TRACE1("SMR Image Memory Dump Last Kb @ %08x", setting);
            
    for (TInt y=0; y < 0x80; y+=16)
        {
        SMR_TRACE5("  %08x:  %08x  %08x  %08x  %08x", setting, setting[0], setting[1], setting[2], setting[3]);
        setting+=4;
        }
              
    NKern::ThreadEnterCS();
    TBool chunkRefCntZero = Kern::ChunkClose(chunkPtr);
    NKern::ThreadLeaveCS();
    if (chunkRefCntZero == 0)
		SMR_LOGMSG_RETURN("Kern::ChunkClose gave false result", KErrGeneral);
    
    return KErrNone;
	}

// -- METHODS -----------------------------------------------------------------
//
// DSMRTestFactory
//

TInt DSMRTestFactory::Install()
	{
    SMR_FUNC("DSMRTestFactory::Install");
	return SetName(&RSMRTest::Name());
	}

void DSMRTestFactory::GetCaps(TDes8& aDes) const
	{
    SMR_FUNC("DSMRTestFactory::GetCaps");
  	Kern::InfoCopy(aDes,0,0);
	}

TInt DSMRTestFactory::Create(DLogicalChannelBase*& aChannel)
	{
    SMR_FUNC("DSMRTestFactory::Create");
   
   	aChannel=new DSMRTestChannel();
	if(!aChannel)
		return KErrNoMemory;
	return KErrNone;
	}


// -- METHODS -----------------------------------------------------------------
//
// DSMRTestChannel
//

DSMRTestChannel::DSMRTestChannel()
	{
    SMR_FUNC("DSMRTestChannel");
   	}

DSMRTestChannel::~DSMRTestChannel()
	{
    SMR_FUNC("~DSMRTestChannel");
	}

TInt DSMRTestChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{
    SMR_FUNC("DSMRTestChannel::DoCreate");
   	
    iClient = &Kern::CurrentThread();
	return KErrNone;
	}

TInt DSMRTestChannel::RequestUserHandle(DThread* aThread, TOwnerType aType)
	{
    SMR_FUNC("DSMRTestChannel::RequestUserHandle");
    
	if (aType!=EOwnerThread || aThread!=iClient)
		return KErrAccessDenied;
	return KErrNone;
	}

TInt DSMRTestChannel::Request(TInt aReqNo, TAny* a1, TAny*)
	{
    SMR_FUNC("DSMRTestChannel::Request");
	TBool aEnforce = (TBool) a1;
	
	switch(aReqNo)
		{
		
	case RSMRTest::ECtrlCheckSMRIBPtr:
		{
		TInt rc = ECtrlCheckSMRIBPtr();
		if (rc < 0)
    		return rc;
		if (aEnforce && rc == 0)
    		return KErrBadHandle;
    		
		break; // fall through, return KErrNone
		}
		
	case RSMRTest::ECtrlPrintSMRIB:
		{
		TInt rc = ECtrlPrintSMRIB();
		if (rc < 0)
    		return rc;
		if (aEnforce && rc == 0)
    		return KErrNotFound;
    		
		break; // fall through, return KErrNone
		}
	
	case RSMRTest::ECtrlAccessAllSMRs:
		{
        TInt rc = ECtrlAccessAllSMRs();
		if (rc < 0)
    		return rc;
		if (aEnforce && rc == 0)
    		return KErrNotFound;
    		
    	break; // fall through, return KErrNone
		}
	
	case RSMRTest::ECtrlFreeHalfSMR1PhysicalRam:
		{
		TInt rc = ECtrlFreeHalfSMR1PhysicalRam();
		if (rc < 0)
    		return rc;
		if (aEnforce && rc == 0)
    		return KErrNotFound;
		
		break; // fall through, return KErrNone
		}
	
	case RSMRTest::ECtrlFreeAllSMR2PhysicalRam:
		{
		
		TInt rc = ECtrlFreeAllSMR2PhysicalRam();
		if (rc < 0)
    		return rc;
		if (aEnforce && rc == 0)
    		return KErrNotFound;
		
		break; // fall through, return KErrNone
		}
	
	default:
		return KErrNotSupported;
		}
		
	return KErrNone;
	}


// -- GLOBALS -----------------------------------------------------------------


DECLARE_STANDARD_LDD()
	{
    SMR_FUNC("D_SMR_DECLARE_STANDARD_LDD");

    const TRomHeader& romHdr = Epoc::RomHeader();
    
    TInt RHsize = sizeof(TRomHeader);
    SMR_TRACE2("RomHeader - addr %0x; size %d", &romHdr, RHsize);

    TSuperPage& superPage = Kern::SuperPage();
    TInt SPsize = sizeof(SSuperPageBase);
    
    TInt startupReason = superPage.iHwStartupReason;
    TLinAddr rootDirList = superPage.iRootDirList;
    
    SMR_TRACE2("SuperPage  - addr %0x; size %d", &superPage, SPsize);
    SMR_TRACE2("SuperPage - StartupReason: %0x; rootDirList %0x", startupReason, rootDirList);
                  
   	return new DSMRTestFactory;
	}

