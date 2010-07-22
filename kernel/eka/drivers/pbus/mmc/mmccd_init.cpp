// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\drivers\pbus\mmc\mmccd_init.cpp
// 
//

#include <drivers/mmccd_ifc.h>
#include <drivers/pbusmedia.h>




/**
Starts the sequence of operations that creates the platform specific objects,
registers the socket, and initialises the stack.

The function is called in the Variant DLL's entry point code, i.e. when
the kernel extension is initialised.

Note that the Variant DLL is a kernel extension.

@return KErrNone if creation and initialisation is successful;
        one of the other system wide error codes otherwise.
        
@see TMMCardControllerInterface::NewStack()
@see TMMCardControllerInterface::NewMediaChange()
@see TMMCardControllerInterface::NewVcc()
@see TMMCardControllerInterface::Init()
@see TMMCardControllerInterface::IsMMCStack()
@see TMMCardControllerInterface::MediaChangeID()
@see TMMCardControllerInterface::VccID()
*/	 
EXPORT_C TInt TMMCardControllerInterface::Create()
//
// Allocate any resources. Only done once on kernel initialization so don't
// worry about cleanup if it fails.
//
	{
	TInt r=KErrNone;
	__KTRACE_OPT(KPBUS1,Kern::Printf(">TMMCardControllerInterface::Create"));

	// Create the password store (a single password store
	//  is allocated for the use of all MMC stacks

	TMMCPasswordStore* theMmcPasswordStore = (TMMCPasswordStore*)LocDrv::PasswordStore();
	if(theMmcPasswordStore == NULL)
		{
		theMmcPasswordStore = new TMMCPasswordStore();
		if(theMmcPasswordStore)
			{
			if((r = theMmcPasswordStore->Init()) == KErrNone)
				r = LocDrv::RegisterPasswordStore(theMmcPasswordStore);					

			if(r != KErrNone)
				delete theMmcPasswordStore;
			}
		else
			{
			r = KErrNoMemory;
			}
		}

	if(r!= KErrNone)
		return r;

	r = Init();
	if (r != KErrNone)
		return r;


	SMediaDeviceInfo mdi;
	TInt i;
	for (i=0; i<KMaxPBusSockets && r==KErrNone; i++)
		{
		if (IsMMCSocket(i,mdi))
			{
			__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d is MMC card",i));

			// Allocate a new socket
			DMMCSocket* pS = NewSocket(i, theMmcPasswordStore);
			if (!pS)
				{
				r=KErrNoMemory;
				break;
				}
			TheSockets[i]=pS;

			// Allocate a variant specific stack object via the interface
			DMMCStack* pStack=NewStack(i, pS);
			if (!pStack)
				{
				r=KErrNoMemory;
				break;
				}

			pS->iStack=pStack;

			TInt mcid=MediaChangeID(i);
			__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d Media Change %d",i,mcid));
			DMMCMediaChange* pM=(DMMCMediaChange*)TheMediaChanges[mcid];
			if (!pM)
				{
				__KTRACE_OPT(KPBUS1,Kern::Printf("New Media Change"));
				pM=NewMediaChange(mcid);
				if (!pM)
					{
					r=KErrNoMemory;
					break;
					}
				TheMediaChanges[mcid]=pM;
				__KTRACE_OPT(KPBUS1,Kern::Printf("Media Change %d at %08x",mcid,pM));
				r=pM->Create();
				if (r!=KErrNone)
					break;
				}
			else
				{
				__KTRACE_OPT(KPBUS1,Kern::Printf("Media Change %d already exists at %08x",mcid,pM));
				++pM->iReplyCount;
				}
			TInt vcc=VccID(i);
			__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d Vcc %d",i,vcc));
			DMMCPsu* pV=(DMMCPsu*)TheVccs[vcc];
			if (!pV)
				{
				__KTRACE_OPT(KPBUS1,Kern::Printf("New Vcc"));
				pV=NewVcc(vcc,mcid);
				if (!pV)
					{
					r=KErrNoMemory;
					break;
					}
				TheVccs[vcc]=pV;
				
				// Assign Socket here such that iDFc can be obtained
				pV->iSocket=pS;
				
				__KTRACE_OPT(KPBUS1,Kern::Printf("Vcc %d at %08x",vcc,pV));
				r=pV->Create();
				if (r!=KErrNone)
					break;
				}
			else 
				{
				__KTRACE_OPT(KPBUS1,Kern::Printf("Vcc %d already exists at %08x, mcid=%d",vcc,pV,pV->iMediaChangeNum));
// DISALLOW SHARED PSUs UNTIL SOMEONE NEEDS THEM
//				if (pV->iMediaChangeNum!=mcid)
//					{
					r=KErrInUse;
//					break;
//					}
				}
			
			DMMCPsu* pVCore=(DMMCPsu*)TheVccCores[vcc];
			// N.B. Assume paired vcc & vccQ unit are numbered identically!
			pVCore=NewVccCore(vcc,mcid);
			if (pVCore)
				{
				TheVccCores[vcc]=pVCore;
				__KTRACE_OPT(KPBUS1,Kern::Printf("VccCore %d at %08x",vcc,pVCore));
				
				// Assign Socket here such that iDFcQ can be obtained
				pVCore->iSocket=pS;
				
				r=pVCore->Create();
				if (r!=KErrNone)
					break;
				
				// VccCore must issue sleep instead of power down 
				pVCore->iPwrDownCheckFn = DMMCPsu::SleepCheck;				
				}
			//else do nothing doesn't matter if its not supported
			
			r=pS->Create(mdi.iDeviceName);
			if (r!=KErrNone)
				break;

			pS->iMediaChangeNumber=mcid;
			pS->iMediaChange=pM;
			pS->iVcc=pV;
			if (pVCore)
				{
				pS->iVccCore = pVCore;
				}

			r=pS->Init();
			if (r!=KErrNone)		
				break;

			r = RegisterMediaDevices(i);
			if(r != KErrNone)
			   break;
			
			__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d Created OK",i));
			}
		else
			__KTRACE_OPT(KPBUS1,Kern::Printf("Socket %d not MMC card",i));
		}
		
	__KTRACE_OPT(KPBUS1,Kern::Printf("<TMMCardControllerInterface::Create, ret %d",r));
	return r;
	}

EXPORT_C TInt TMMCardControllerInterface::RegisterMediaDevices(TInt aSocket)
/**
Registers the media devices for the specified socket.
By default, All MMC derivatives register at least an MMC driver
@internal
*/
	{
	TInt err = KErrNone;
	
	SMediaDeviceInfo mdi;

	if (IsMMCSocket(aSocket, mdi) == EFalse)
		return(KErrNotSupported);
		
	DMMCSocket* pS = (DMMCSocket*) TheSockets[aSocket];
	if(pS == NULL)
		return(KErrNoMemory);

	TMMCMachineInfo mi;
	pS->iStack->MachineInfo(mi);
	
	// There may be more than one physical card slot for this socket/stack;
	// if this is the case, then we have to create a separate DPBusPrimaryMedia 
	// for each one. This is the only way to get the local media sub-system to iterate 
	// through the installed set of media drivers repeatedly for each card slot
	// (rather than once for the entire stack). We also need to share the number of 
	// drives/partitions (SMediaDeviceInfo.iDriveCount) and the number of media 
	// (SMediaDeviceInfo.iNumMedia) equally between the slots...
	__ASSERT_ALWAYS(mdi.iDriveCount >= mi.iTotalSockets, DMMCSocket::Panic(DMMCSocket::EMMCInvalidNumberOfCardSlots) );
	__ASSERT_ALWAYS(mdi.iNumMedia >= mi.iTotalSockets, DMMCSocket::Panic(DMMCSocket::EMMCInvalidNumberOfCardSlots) );

	TInt physicalCardSlots = mi.iTotalSockets;
	TInt drivesPerSlot = mdi.iDriveCount / physicalCardSlots;
	TInt numMediaPerSlot = mdi.iNumMedia / physicalCardSlots;
	TInt driveListIndex = 0;

#if !defined(__WINS__)
	DMMCStack::TDemandPagingInfo demandPagingInfo;
	demandPagingInfo.iPagingDriveList = NULL;
	demandPagingInfo.iDriveCount = 0;
	TBool demandPagingSupported = 
		pS->iStack->DemandPagingInfo(demandPagingInfo) == KErrNone ? (TBool)ETrue : (TBool)EFalse;

#endif // __WINS__


	for (TInt i=0; i<physicalCardSlots ; i++)	
		{
		DPBusPrimaryMedia* pMedia = new DPBusPrimaryMedia(pS);
		if (pMedia == NULL)
			return(KErrNoMemory);
		// store the slot number in DPBusPrimaryMedia so that it can 
		// subsequently be passed to the media driver.
		pMedia->iSlotNumber = i;

		err = LocDrv::RegisterMediaDevice(
				mdi.iDevice,
				drivesPerSlot,
				mdi.iDriveList+driveListIndex,
				pMedia,
				numMediaPerSlot,
				*mdi.iDeviceName);
		if (err != KErrNone)
			break;

#if !defined(__WINS__)
        if ((mi.iFlags & TMMCMachineInfo::ESupportsDMA) == TMMCMachineInfo::ESupportsDMA)
            {
            err = LocDrv::RegisterDmaDevice(pMedia,
                                            KMMCardHighCapBlockSize, 
                                            pS->MaxDataTransferLength(), 
                                            pS->DmaAlignment());
            if (err != KErrNone)
                break;
            }		
		
		if (demandPagingSupported && demandPagingInfo.iSlotNumber == i)
			{
			err = LocDrv::RegisterPagingDevice(
				pMedia, 
				demandPagingInfo.iPagingDriveList,
				demandPagingInfo.iDriveCount,
				demandPagingInfo.iPagingType,
				demandPagingInfo.iReadShift,
				demandPagingInfo.iNumPages);

			// Ignore error if demand paging not supported by kernel
			if (err == KErrNotSupported)
				err = KErrNone;

			if (err != KErrNone)
				break;
			}
#endif // __WINS__

		driveListIndex+= drivesPerSlot;
		}
	
	return(err);
	}
