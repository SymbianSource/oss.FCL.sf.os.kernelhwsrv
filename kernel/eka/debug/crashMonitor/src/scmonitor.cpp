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
// e32\debug\crashMonitor\src\scmonitor.cpp
// Core dump server - Kernel side crash monitor
// 
//

/**
 @file
 @internalTechnology
*/

#include <scmonitor.h>
#include <kernel/monitor.h>
#include <assp.h>
#include <drivers/crashflash.h>
#include <kernel/klib.h>
#include <crashlogwalker.h>
#include <scmconfigitem.h>

#include "scmdatasave.h"

GLDEF_D SCMonitor TheSCMonitor; //global definition of SCMonitor

//keep things 4 byte aligned
const TInt KRestartType = SCMonitor::ESoftRestart;

/**
SCMonitor constructor
*/
SCMonitor::SCMonitor()
	: iMultiCrashInfo(NULL)
	{
	}

SCMonitor::~SCMonitor()
	{
	delete iMultiCrashInfo;
	}

/**
 Print data to the corresponding output channel. Derived from monitor
 @param aDes the buffer containing the data
 */
void SCMonitor::Print (const TDesC8& aDes )
	{
	//intended to do nothing
	}

/**
 * Allocates resources for SCMonitor
 * cant fully construct in constructor as we are a kernel extension and resources are limited when we are created
 */
void SCMonitor::StableConstruction()
	{
	LOG_CONTEXT
	iDataSave = new SCMDataSave(this, TheSCMonitor.iFlash);	
	
	//Configuration object for use upon crash
	iScmConfig = new SCMConfiguration();
	TInt err = iScmConfig->SetDefaultConfig();	
	if(KErrNone != err)
		{
		CLTRACE1("SCMonitor::StableConstruction - Unable to set default config err = %d", err);
		}
	
	
#ifdef NO_MULTICRASHINFO	
	iMultiCrashInfo = NULL;
#else	

	//We need to take a look at the flash map from variant_norflash_layout.h
	iMultiCrashInfo = new SCMMultiCrashInfo();

	TUint numberBlocks = KCrashLogSize / KCrashLogBlockSize;
	for(TUint32 cnt = 0; cnt < numberBlocks; cnt++)
		{
		iMultiCrashInfo->AddBlock(new SCMCrashBlockEntry(cnt, cnt * KCrashLogBlockSize, KCrashLogBlockSize));
		}
#endif
	}

/**
 * Start a secondary DFC queue for the Flash and Init the flash in the variant(h4)
 * @param aAny
 */
void StartSecondary (TAny* )
	{
	LOG_CONTEXT
	//InitFlash is implemented in the variant as it creates a variant
	//specific derived CrashFlash
	TheSCMonitor.InitFlash ( );
	TheSCMonitor.StableConstruction();
	}

/**
 * Global method to create a dfc queue
 * @param Method to intialise the flash.
 * @param Null
 * @param Gets the address of the supervisor thread DFC queue
 * @param TDfcQ priority number
 * @return a DFC object
 */
GLDEF_C TDfc StartSecondaryDfc(&StartSecondary, NULL, Kern::SvMsgQue(), KMaxDfcPriority-1);

/**
 * Kernel Main module entry - Own implementation( similar to crash logger)
 * @param aReason reason to enter to the method
 * @return One of the system wide codes
 */
GLDEF_C TInt KernelModuleEntry(TInt aReason)
	{	
	if(aReason==KModuleEntryReasonVariantInit0)
		{
		new(&TheSCMonitor) SCMonitor;
		// We are going to register the system Crash monitor here so that the order
		// the monitor modules are placed in rom is preserved.  
		// The monitor is not fully intialised here.
		//the variant target is missing as we still have to finalise on the crash flash 
		//implementation. H2 & H4 doesnt support currently.
		LOG_CONTEXT		
		CLTRACE("Installing System Crash Monitor");
		Monitor::RegisterMonitorImpl (&TheSCMonitor );
		return KErrNone;
		}
	else if (aReason==KModuleEntryReasonExtensionInit0 )
		{
		return KErrNone;
		}
	else if (aReason==KModuleEntryReasonExtensionInit1 )
		{
		LOG_CONTEXT
		CLTRACE("Enqueing dfc to init crash flash for System Crash Monitor after all modules loaded");
		StartSecondaryDfc.Enque ( );
		return KErrNone;
		}
	return KErrArgument;
	}

/**
 Method to intialize the system crash monitor
 @param aCategory the fault category type
 @param aReason the reason for crash
 @return restart type
 */
TInt SCMonitor::Init2 (TAny* aCategory, TInt aReason )
	{
	LOG_CONTEXT
	__KTRACE_OPT(KALWAYS, Kern::Printf("\n\nSystem Crash Monitor Launched: To Analyse Crash Produced Use Core Dump Server\n"));

	//Start logging the data:	
	//Need to lock kernel to access object containers (it technically is anyway, but flag isnt set)
	NKern::Lock();
	DoCrash(aCategory, aReason);	
	NKern::Unlock();		
		
	__KTRACE_OPT(KALWAYS, Kern::Printf("System Crash Monitor Finished: Log Size = [%d]\n", iDataSave->iCrashInf.iLogSize));
	
	return KRestartType;
	}

/**
 * This is responsible for setting up any structures required for processing of the crash
 * @param aCategory the fault category type
 * @param aReason 
 */
void SCMonitor::DoCrash(TAny* aCategory, TInt aReason )
	{
	// get debug mask
	TInt dbgMask = Kern::SuperPage().iDebugMask[0];
	
	// if we are writing to the comm port then we need to turn off other debug messages
	if( iDataSave->GetWriteSelect() == SCMDataSave::EWriteComm)
		{
		Kern::SuperPage().iDebugMask[0] = 0;
		}
	
	if(!aCategory)
		{
		CLTRACE("\tNULL category retrieved and returning");
		TheSCMonitor.iFlash->EndTransaction();
		return;
		}

	iFrame = NULL;
	
	CLTRACE("\tAbout to set category -- note: can occasionaly crash board");	
	iFaultCategory = *(const TDesC8*)aCategory;  // this crashes the board sometimes		
	iFaultReason = aReason;
	Epoc::SetMonitorExceptionHandler ((TLinAddr)HandleException );
	
	// get the first start block
	// will retieve start of flash by default
	SCMCrashBlockEntry block;
	TInt err = GetNextCrashStartPoint(block);  // will also attempt to read iScmConfig

	if(KErrNone == err)
		{
		CLTRACE2("SCMonitor::DoCrash next crash will be written at blocknumber = %d offset  %d"
				, block.iBlockNumber, block.iBlockOffset);
		}
	else
		{
		CLTRACE1("SCMonitor::DoCrash Failed to find a valid block to write to, can not continue. err = [%d]", err);
		return;
		}
	
	TUint crashId = block.iBlockNumber;	
	iDataSave->iWriter->ResetBytesWritten();		
	
	//Write the crash (1st pass is to gather header data)
	TInt spaceRequired = ProcessCrash(block, crashId, EFalse);					
	
	// now do the real write	
	// prepare flash for data	
	TheSCMonitor.iFlash->StartTransaction();	
	TheSCMonitor.iFlash->SetWritePos(block.iBlockOffset);

	//write the crash this time
	ProcessCrash(block, crashId, ETrue);
	
	TheSCMonitor.iFlash->EndTransaction();	
	
	// restore debug mask
	Kern::SuperPage().iDebugMask[0] = dbgMask;
	}


/**
 * This walks the existing crash log and finds out where current crashes finish
 * @param aBlockEntry Block to use. Only valid if KErrNone is returned.
 * @return One of the OS wide codes
 */
TInt SCMonitor::GetNextCrashStartPoint(SCMCrashBlockEntry& aBlockEntry)
	{
	LOG_CONTEXT	
	
	//First thing is to try and read the config
	TBool configFound = (iDataSave->ReadConfig(*iScmConfig) == KErrNone);
	
	if( iMultiCrashInfo)	
		{				
		/**
		 * data save has been configured to use multicrash info to find the next block we are on we need to scan each
		 * block to see if it contains a valid header. if we find an empty block in our block list then that is the
		 * one we will use if we find no empty blocks then we have no room left	
		 */
		iMultiCrashInfo->Reset();
		SCMCrashBlockEntry* block = iMultiCrashInfo->GetNextBlock();
		TBool blockFound = EFalse;				
		
		//For any crashes in flash, we need to record where they end, so that we can then go to the next
		//block after the one in which it ends
		TInt crashEndPoint = 0;		
		
		while(block)
			{	
			CLTRACE1("SCMonitor::GetNextCrashStartPoint Processing block number %d", block->iBlockNumber );			
			
			//If we have already found our block, we should erase subsequent ones for use
			if(blockFound)
				{
				TInt err = EraseFlashBlock(*block);
				if(err != KErrNone)
					{					
					return err;
					}
				
				block = iMultiCrashInfo->GetNextBlock(); 
				continue;
				}	
			
			//is this block before a crash end? if it is, we cant use it as a crash can span multiple blocks
			if(block->iBlockOffset >= crashEndPoint)
				{
				//special condition if we have a config
				TUint startPos = block->iBlockOffset;
				TUint skipBytes = 0;
				if(configFound && block->iBlockOffset == 0)
					{
					startPos+=iScmConfig->GetSize();
					
					//must align to flash for read
					skipBytes = startPos % KFlashAlignment;	
					startPos -= skipBytes;
					}
				
				// try and read an info header at these flash coords
				TBuf8<TCrashInfoHeader::KSCMCrashInfoMaxSize + KFlashAlignment> buf;
				buf.SetLength(TCrashInfoHeader::KSCMCrashInfoMaxSize + KFlashAlignment);
	
				CLTRACE1("(SCMonitor::GetNextCrashStartPoint) reading at offset %d", block->iBlockOffset);
				
				TheSCMonitor.iFlash->SetReadPos(startPos);
				TheSCMonitor.iFlash->Read(buf);
				
				// create the buffer applying the offset of bytes skipped
				TByteStreamReader reader(const_cast<TUint8*> (buf.Ptr() + skipBytes));
				
				TCrashInfoHeader header;								
				TInt err = header.Deserialize(reader);
				
				if(err == KErrCorrupt)
					{
					CLTRACE2("(SCMonitor::GetNextCrashStartPoint) Found empty block blocknumber %d blockoffset = %d"
							, block->iBlockNumber, block->iBlockOffset);
										
					blockFound = ETrue;
					aBlockEntry = *block;
					
					continue; //Dont get next block, as next run will erase this current block for use
					}
				else
					{					
					crashEndPoint = header.iLogSize + startPos;
					CLTRACE3("(SCMonitor::GetNextCrashStartPoint) In block [%d] we found a valid crash header. This crash finishes at [%d] [0x%X]", block->iBlockNumber, crashEndPoint, crashEndPoint);
					}
				}
			
			block = iMultiCrashInfo->GetNextBlock();
			}									
			
		if(blockFound)
			{
			return KErrNone;
			}
		else
			{
			//CLTRACE("(SCMonitor::GetNextCrashStartPoint) No available blocks TREATING as NO MULTICRASH INFO will write to default block");
			//In this case should we just overwrite old crashes and return the first block as the comment above suggests
			//return blockFound;
			}
		}

	// no multi crash info supplied - use default first block settings
	TInt err = EraseEntireFlashPartition();
	if(err != KErrNone)
		{
		CLTRACE1("Unable to delete area required to log to flash. Aborting. Error - [%d]", err);
		return err;
		}
	
	aBlockEntry = SCMCrashBlockEntry(0,0,0);
	return KErrNone;
	}

/**
 * Handles the processing of the crash
 * @return The size of the crash log (including header) that has been/will be written
 */
TInt SCMonitor::ProcessCrash(const SCMCrashBlockEntry& aBlock, TUint aCrashId, TBool aCommit)
	{	
	LOG_CONTEXT
	CLTRACE5("aBlock.iBlockOffset = [%d]  [0x%X] aBlock.iBlockNumber = %d aBlock.iBlockSize = [%d]  [0x%X]",
			aBlock.iBlockOffset, aBlock.iBlockOffset, aBlock.iBlockNumber, aBlock.iBlockSize, aBlock.iBlockSize);		
	
	// reset writer for start of each crash
	iDataSave->iWriter->ResetBytesWritten();
	TInt logLevel = 0;

	if(aCommit)
		{
		logLevel = KALWAYS;
		iDataSave->iWriter->EnablePhysicalWriting();	
		}
	else
		{
#if defined(_DEBUG)
		logLevel = KDEBUGGER;
#else
		logLevel = KALWAYS; //Doesnt matter, KTRACE OPT is empty for rel builds 
		if(logLevel != KALWAYS)
			{
			//This is to avoid warning
			}
#endif
		
		iDataSave->iWriter->DisablePhysicalWriting();	
		}
	
	iDataSave->SetByteCount(aBlock.iBlockOffset);	
	if(aBlock.iBlockOffset == 0 && aBlock.iBlockNumber == 0)
		{	
		// this is the first crash - we need to save the config here first
		CLTRACE("(SCMonitor::ProcessCrash) - this is block 0 - WRITING CONFIG");
		iDataSave->LogConfig(*iScmConfig);	
		
		//Config is not part of crash so reset bytes written			
		iDataSave->SetCrashStartingPoint(iDataSave->iWriter->GetBytesWritten());		
		}	
	else
		{
		iDataSave->SetCrashStartingPoint(aBlock.iBlockOffset);
		}	
	
	iDataSave->iWriter->ResetBytesWritten();
	
	TUint32 logSize = 0;
	TUint sizeOfObjectDumped = 0;
		
	TInt err = iDataSave->LogCrashHeader(iFaultCategory, iFaultReason, aCrashId, sizeOfObjectDumped);		
	if(KErrNone != err)
		{
		CLTRACE("System Crash Monitor: Failed to create crash info header - (TCrashInfo)"); 
		return KRestartType;
		}	
	
	logSize += sizeOfObjectDumped;
	
	//Now we must read the configuration to use. This is held at the start of our flash partition
	//and managed by the iConfig object
	iScmConfig->ResetToHighestPriority();	
		
	//Always want the crash context
	iDataSave->iHdr.iCTFullRegOffset = logSize + iDataSave->GetCrashStartingPoint();
	
	err = iDataSave->LogCPURegisters(sizeOfObjectDumped);
	if(KErrNone != err)
		{
		CLTRACE1("\tError logging full registers = %d", err);
		}	
	
	logSize += sizeOfObjectDumped;
	
	CLTRACE("\tAbout to enter processing loop");		
	SCMDataSave::TDataToDump dump;
		
	for(;;)		
		{		
		//now we get each item by priority from the configuration
		TConfigItem* configItem = iScmConfig->GetNextItem();

		if(!configItem)
			{
			// end of list
			break;
			}
		
		CLTRACE1("\nLooking at item type [%d]", configItem->GetDataType());
		if(configItem->GetSpaceRequired() > iDataSave->SpaceRemaining())
			{
			__KTRACE_OPT(logLevel, Kern::Printf("\t\tFor Item Type [%d]: Unable to log [0x%X] [%d] bytes because we only have [0x%X] [%d] bytes left", configItem->GetDataType(), configItem->GetSpaceRequired(), configItem->GetSpaceRequired(), iDataSave->SpaceRemaining(), iDataSave->SpaceRemaining()));
			continue;
			}
		else
			{
			CLTRACE1("Will require [%d] bytes for this item", configItem->GetSpaceRequired());
			}
		
		// only interested in logging items with priority > 0
		if( configItem->GetPriority() <= 0)
			{
			CLTRACE1("\tIgnored config item type %d priority 0", configItem->GetDataType());
			continue;
			}
			
		//there are a lot of TUints in the hdr to record where we wrote this item. 
		//This will point to the one of interest for this configItem
		TUint32* offsetPointer = NULL;				
		
		//now we check the type of data we wish to dump
		switch(configItem->GetDataType())
			{
			case TConfigItem::ECrashedThreadMetaData:			
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: ECrashedThreadMetaData at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
				
				err = LogThreadMetaData(SCMDataSave::EThreadSpecific, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iCTMetaOffset);
				
				break;
				}
			case TConfigItem::EThreadsMetaData:			
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: EThreadsMetaData at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
				
				//record location we are writing to in the header
				iDataSave->iHdr.iTLstOffset = iDataSave->iWriter->GetBytesWritten();
				err = LogThreadMetaData(SCMDataSave::ESystemWide, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iTLstOffset);
				
				break;
				}
			case TConfigItem::ECrashedProcessMetaData:			
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: ECrashedProcessMetaData at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
					
				err = LogProcessMetaData(SCMDataSave::EProcessSpecific, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iCPMetaOffset);
				
				break;
				}
			case TConfigItem::EProcessMetaData:		
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: EProcessMetaData at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
								
				err = LogProcessMetaData(SCMDataSave::ESystemWide, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iPLstOffset);
				
				break;
				}
			case TConfigItem::ECrashedProcessUsrStacks:			
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: ECrashedProcessUsrStacks at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
								
				//define what we wish to dump
				dump.iMetaData = EFalse;
				dump.iCodeSegs = EFalse;
				dump.iStk = SCMDataSave::EUsrStack;
				dump.iReg = SCMDataSave::ERegSetNone;	
				err = LogObjectContainers(EThread, SCMDataSave::EProcessSpecific, dump, sizeOfObjectDumped);	
				offsetPointer = &(iDataSave->iHdr.iCTUsrStkOffset);
				
				break;
				}
			case TConfigItem::EThreadsUsrStack:			
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: EThreadsUsrStack at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
				
				//define what we wish to dump
				dump.iMetaData = EFalse;
				dump.iCodeSegs = EFalse;
				dump.iStk = SCMDataSave::EUsrStack;
				dump.iReg = SCMDataSave::ERegSetNone;

				err = LogObjectContainers(EThread, SCMDataSave::ESystemWide, dump, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iSysSvrStkOffset);
				
				break;
				}
			case TConfigItem::ECrashedProcessSvrStacks:			
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: ECrashedProcessSvrStacks at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
								
				//define what we wish to dump
				dump.iMetaData = EFalse;
				dump.iCodeSegs = EFalse;
				dump.iStk = SCMDataSave::ESvrStack;
				dump.iReg = SCMDataSave::ERegSetNone;
				
				err = LogObjectContainers(EThread, SCMDataSave::EProcessSpecific, dump, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iCTSvrStkOffset);
				
				break;
				}
			case TConfigItem::EThreadsSvrStack:			
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: EThreadsSvrStack at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
								
				//define what we wish to dump
				dump.iMetaData = EFalse;
				dump.iCodeSegs = EFalse;
				dump.iStk = SCMDataSave::ESvrStack;
				dump.iReg = SCMDataSave::ERegSetNone;
				
				err = LogObjectContainers(EThread, SCMDataSave::ESystemWide, dump, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iSysSvrStkOffset);
				
				break;
				}
			case TConfigItem::EThreadsUsrRegisters:		
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: EThreadsUsrRegisters at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
								
				//define what we wish to dump
				dump.iMetaData = EFalse;
				dump.iCodeSegs = EFalse;
				dump.iStk = SCMDataSave::EStackTypeNone;
				dump.iReg = SCMDataSave::EUserRegisters;
				
				err = LogObjectContainers(EThread, SCMDataSave::ESystemWide, dump, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iSysUsrRegOffset);
				
				break;
				}
			case TConfigItem::EThreadsSvrRegisters:		
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: EThreadsSvrRegisters at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
								
				//define what we wish to dump
				dump.iMetaData = EFalse;
				dump.iCodeSegs = EFalse;
				dump.iStk = SCMDataSave::EStackTypeNone;
				dump.iReg = SCMDataSave::ESupervisorRegisters;
				
				err = LogObjectContainers(EThread, SCMDataSave::ESystemWide, dump, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iSysSvrRegOffset);
				
				break;
				}
			case TConfigItem::EExceptionStacks:
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: EExceptionStacks at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
							
				err = iDataSave->LogExceptionStacks(sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iExcStkOffset);
				
				break;
				}
			case TConfigItem::ECrashedProcessCodeSegs:
				{	
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: ECrashedProcessCodeSegs at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
								
				//define what we wish to dump
				dump.iMetaData = EFalse;
				dump.iCodeSegs = ETrue;
				dump.iStk = SCMDataSave::EStackTypeNone;
				dump.iReg = SCMDataSave::ERegSetNone;				
				
				err = LogObjectContainers(EProcess, SCMDataSave::EProcessSpecific, dump, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iCPCodeSegOffset);
				
				break;
				}
			case TConfigItem::EProcessCodeSegs:
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: EProcessCodeSegs at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
								
				//define what we wish to dump
				dump.iMetaData = EFalse;
				dump.iCodeSegs = ETrue;
				dump.iStk = SCMDataSave::EStackTypeNone;
				dump.iReg = SCMDataSave::ERegSetNone;
				err = LogObjectContainers(EProcess, SCMDataSave::ESystemWide, dump, sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iSysCodeSegOffset);
				
				break;
				}
			case TConfigItem::ETraceData:
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: ETraceData at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
									
				err = iDataSave->LogTraceBuffer(configItem->GetSizeToDump(), sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iTraceOffset);
				
				break;
				}		
			case TConfigItem::ELocks:
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: ELocks at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));	
					
				err = iDataSave->LogLocks(sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iScmLocksOffset);
				
				break;
				}
			case TConfigItem::EKernelHeap:
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: EKernelHeap at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
				
				err = iDataSave->LogKernelHeap(sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iKernelHeapOffset);
				
				break;
				}
			case TConfigItem::EVariantSpecificData:
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: EVariantSpecificData at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
				
				err = iDataSave->LogVariantSpecificData(sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iVarSpecInfOffset);
				
				break;
				}
			case TConfigItem::ERomInfo:
				{
				__KTRACE_OPT(logLevel, Kern::Printf("\tDoing: ERomInfo at [%d] offset from [%d]", iDataSave->iWriter->GetBytesWritten(), iDataSave->GetCrashStartingPoint()));
				
				err = iDataSave->LogRomInfo(sizeOfObjectDumped);
				offsetPointer = &(iDataSave->iHdr.iRomInfoOffset);
				
				break;
				}
			//unknown configuration type - something bad is going on
			default: return 0;			
			}				
		
		if(KErrNone != err)
			{
			__KTRACE_OPT(logLevel, Kern::Printf("\tError logging data: [%d]   Type = [%d]", err, aBlock.iBlockOffset));
			continue;
			}
		
		//Set the space required so next time around we will know in advance how much space we need
		configItem->SetSpaceRequired(sizeOfObjectDumped);		
		
		//Note: the following steps are only required for the first time we call process crash. The second time,
		//when physical writing is enabled, these will have been written already and so they dont matter
		
		//update the offset and logsize if we are going to dump this item
		TUint32 absoluteLogPos = logSize + iDataSave->GetCrashStartingPoint();
		if(absoluteLogPos+sizeOfObjectDumped < iDataSave->MaxLogSize())
			{
			//now, we must record where in the crash log this item will be dumped
			*offsetPointer = absoluteLogPos;
			logSize += sizeOfObjectDumped;
			}
		}
	
	iDataSave->iCrashInf.iLogSize = logSize;	
	iDataSave->iWriter->FlushCache();		
	
	return iDataSave->iCrashInf.iLogSize;
	}

/**
 * Logs the meta data for processes
 * @param aCurrentProcess - scope to dump
 * @return one of the OS wide codes
 */
TInt SCMonitor::LogProcessMetaData(SCMDataSave::TDumpScope aScope, TUint& aSizeDumped) const
	{
	LOG_CONTEXT
	
	SCMDataSave::TDataToDump dump;
	dump.iMetaData = ETrue;
	
	return LogObjectContainers(EProcess, aScope, dump, aSizeDumped);
	}

/**
 * 
 * @param aCurrentThread -  to only do the current (crashed thread) or to do all the others
 * @return one of the OS wide codes
 */
TInt SCMonitor::LogThreadMetaData(SCMDataSave::TDumpScope aScope, TUint& aSizeDumped) const
	{
	LOG_CONTEXT
	
	SCMDataSave::TDataToDump dump; 
	dump.iMetaData = ETrue;
	
	return LogObjectContainers(EThread, aScope, dump, aSizeDumped);
	}

/**
 * Generic method that looks at all kernel objects of aObjectType
 * @param aObjectType
 * @param aDumpScope - if you wish to dump for the the current process, current thread or entire system
 * @param aDataToDump - data you wish to dump
 * @param aSizeDumped - records how much was dumped
 * @return
 */
TInt SCMonitor::LogObjectContainers(TObjectType aObjectType, SCMDataSave::TDumpScope aDumpScope, const SCMDataSave::TDataToDump& aDataToDump, TUint& aSizeDumped) const
	{
	aSizeDumped = 0;
	
	if(aObjectType >= ENumObjectTypes)
		{
		return KErrArgument;
		}
	
	//Get the object container for the given object type
	DObjectCon* objectContainer = Kern::Containers()[aObjectType];
	if(objectContainer == NULL)
		{		
		CLTRACE("tFailed to get object container");
		return KErrNotFound;
		}
	
	//Must check the mutex on this is ok otherwise the data will be in an inconsistent state
	if(objectContainer->iMutex->iHoldCount)
		{
		CLTRACE("\tObject Container is in an inconsistant state");
		return KErrCorrupt;
		}
	
	TInt numObjects = objectContainer->Count();
	TInt err = KErrNone;	
	
	for(TInt cnt = 0; cnt< numObjects; cnt ++)
		{	
		DObject* object = (*objectContainer)[cnt];
		
		//Are we interested in the object? scope only relevant for thread and process objects, for others, the scope is implicit
		if(aObjectType == EThread)			
			{
			switch(aDumpScope)
				{
				case SCMDataSave::EThreadSpecific :
					{
					//if we are interested in the current thread and this is not it, continue
					if(((DThread*)object) != &Kern::CurrentThread())
						continue;
					break;
					}
				case SCMDataSave::EProcessSpecific :
					{
					//if we are interested in the current proc and this is not it, continue
					if(((DThread*)object)->iOwningProcess != &Kern::CurrentProcess())
						continue;					
					break;
					}
				case SCMDataSave::ESystemWide :
				default: 
					break;
				}
			}
		else if(aObjectType == EProcess)
			{
			switch(aDumpScope)
				{
				case SCMDataSave::EProcessSpecific :
					{
					if((DProcess*)object != &Kern::CurrentProcess())
						continue;
					break;
					}
				case SCMDataSave::EThreadSpecific :  //thread specific process doesnt make sense
					return KErrArgument;				
				case SCMDataSave::ESystemWide :
				default:
					break;
				}
			}
		
		//Now we look at the data we have been asked to dump
		if(aDataToDump.iMetaData)
			{
			TUint dumped = 0;
			err = HelpDumpMetaData(object, aObjectType, dumped);
			if(KErrNone != err)
				{
				CLTRACE1("Failed to meta data: [%d]", err);
				return err;
				}
			aSizeDumped += dumped;
			}
		
		if(aDataToDump.iCodeSegs)
			{
			if(aObjectType != EProcess)
				{
				return KErrArgument;
				}
			
			TUint dumped = 0;
			err = iDataSave->LogCodeSegments((DProcess*)object, dumped);
			if(KErrNone != err)
				{
				CLTRACE1("Failed to log code segments: [%d]", err);
				return err;
				}
			aSizeDumped += dumped;
			}
		
		if(aDataToDump.iStk != SCMDataSave::EStackTypeNone)
			{
			TUint dumped = 0;
			err = HelpDumpStacks(object, aObjectType, dumped, aDataToDump.iStk);
			if(KErrNone != err)
				{
				CLTRACE1("Failed to log stacks: [%d]", err);
				return err;
				}
			aSizeDumped += dumped;
			}
		
		if(aDataToDump.iReg != SCMDataSave::ERegSetNone)
			{			
			if(aObjectType != EThread)
				{
				return KErrArgument;
				}
			TUint dumped = 0;
			err = iDataSave->LogRegisters((DThread*)object, aDataToDump.iReg, dumped);	
			if(KErrNone != err && KErrNotSupported !=err)  //we expect to send down a KErrNotSupported when we ask for Full CPU set for the non crashed thread - thats fine
				{
				CLTRACE1("Failed to log registers: [%d]", err);
				return err;
				}
			aSizeDumped += dumped;
			}
		}
	
	return KErrNone;
	}

/**
 * Helper method for dumping stacks. Looks to see what type of stack we want and then calls
 * appropriate method
 * @param aObject The DThread object whose stack we want
 * @param aObjectType The object type of this aObject. Anything other than EThread will give KErrArgument
 * @param aSizeDumped Holds the size of the stack dumped after processing 
 * @param aStkType The type of stack to be dumped
 * @see TObjectType
 * @see SCMDataSave::TStackType
 * @return One of the system wide codes
 */
TInt SCMonitor::HelpDumpStacks(DObject* aObject, TObjectType aObjectType, TUint& aSizeDumped, SCMDataSave::TStackType aStkType) const
	{
	//verify args
	if(aObjectType != EThread)
		{
		return KErrArgument;
		}
	
	switch(aStkType)
		{
		case SCMDataSave::EUsrStack:
			{
			return iDataSave->LogThreadUserStack((DThread*)aObject, ETrue, aSizeDumped);
			}
		case SCMDataSave::ESvrStack:
			{
			return iDataSave->LogThreadSupervisorStack((DThread*)aObject, ETrue, aSizeDumped);
			}
		default: return KErrArgument;
		}
	}

/**
 * Helper method to dump meta data about a DThread or a DProcess object
 * @param aObject DObject to use
 * @param aObjectType Type of DObject. Must be EThread or EProcess
 * @param aSizeDumped Holds the size of the stack dumped after processing 
 * @return
 */
TInt SCMonitor::HelpDumpMetaData(DObject* aObject, TObjectType aObjectType, TUint& aSizeDumped) const
	{
	aSizeDumped = 0;
	
	switch(aObjectType)
		{
		case EThread:
			{
			return iDataSave->LogThreadData((DThread*)aObject, aSizeDumped);	 
			}
		case EProcess:
			{
			return iDataSave->LogProcessData((DProcess*)aObject, aSizeDumped);
			}
		default: return KErrArgument;
		}
	}

/**
 * Wrapper method around the flash erase block fundtion to determine if the erase was succesful.
 * If the erase was not succesful we can't continue as we cannot write.
 * @param aBlockOffset Block to erase
 * @return One of the OS wide codes
 */
TInt SCMonitor::EraseFlashBlock(const SCMCrashBlockEntry& aBlock)
	{	
	iFlash->StartTransaction();
	
	TInt numAttempts = 0;
	while(numAttempts < KFlashEraseAttempts)
		{
		iFlash->SetWritePos(aBlock.iBlockOffset);
		iFlash->EraseFlashBlock(aBlock.iBlockOffset);
		
		//we will read the flash to make sure that it set the block to all 1's (well not all, just the start)
		TBuf8<sizeof(TUint32)> buf;
		buf.SetLength(sizeof(TUint32));
				
		iFlash->SetReadPos(aBlock.iBlockOffset);
		iFlash->Read(buf);
		
		volatile TUint32* result = (TUint32*)buf.Ptr();
		if(*result == 0xFFFFFFFF)
			{			
			__KTRACE_OPT(KALWAYS, Kern::Printf("Erase of block [0x%X] succesful after [%d] attempts", aBlock.iBlockOffset, numAttempts+1))			
			iFlash->EndTransaction();			
			return KErrNone;
			}
		
		numAttempts++;
		
		//Sometimes a write to the block helps the next erase
		TUint32 bytesWritten = 0;		
		while(bytesWritten < aBlock.iBlockSize)
			{
			TBuf8<sizeof(TUint8)> num;
			num.Append(0x0);
			iFlash->Write(num);
			bytesWritten++;
			}
		}		

	__KTRACE_OPT(KALWAYS, Kern::Printf("After %d attempts, we were unable to erase the flash block at [0x%X]. This could be because "
			"the driver is defective or because the flash has gone past its lifetime. Whatever it is though, "
			"we cannot continue.", KFlashEraseAttempts, aBlock.iBlockOffset));
	
	iFlash->EndTransaction();
	return KErrAbort;
	}

/**
 * This erases each block in the flash partition
 * @return One of the system wide codes
 */
TInt SCMonitor::EraseEntireFlashPartition()
	{
	if(iMultiCrashInfo)
		{
		iMultiCrashInfo->Reset();
		
		SCMCrashBlockEntry* block = iMultiCrashInfo->GetNextBlock();
		while(block)
			{
			TInt err = EraseFlashBlock(*block);
			if(KErrNone != err)
				{
				return err;
				}
			
			block = iMultiCrashInfo->GetNextBlock();
			}
		
		return KErrNone;
		}
	
	CLTRACE("SCMonitor::EraseEntireFlashPartition() -- No Flash MAP available, trying to use the raw driver to delete.");
	TheSCMonitor.iFlash->EraseLogArea();
	
	return KErrNone;
	}

//eof scmonitor.cpp

