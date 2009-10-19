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
// e32\include\kernel\scmonitor.h
// Kernel System crash monitor header file
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalComponent
*/

#ifndef __SCMONITOR_H__
#define __SCMONITOR_H__

#include <plat_priv.h>
#include <kernel/monitor.h>
#include <e32des8.h>	
#include <arm.h>
#include <variant_norflash_layout.h>

#include <scmdatatypes.h>
#include <scmbytestreamutil.h>
#include <scmconfig.h>
#include <scmdatasave.h>

using namespace Debug;

class CrashFlash;

const TInt KFlashAlignment = sizeof(TInt32);

const TInt KFlashEraseAttempts = 10;
/**
 * System crash monitor responsible for writing crash data to flash in 
 * the event of a crash
 */
class SCMonitor: public Monitor
	{
	public:
		SCMonitor();	
		~SCMonitor();
		
		virtual void Print(const TDesC8& aDes);				
		virtual TInt Init2(TAny* aCategory, TInt aReason);
	
		void VariantInit();
		TInt InitFlash();
		
		void StableConstruction(); 		
		
	public:		
		CrashFlash* iFlash;		
			
	protected:	
		void DumpVariantSpecific();
	
		enum TSysCrashLogState
			{
			EUndefined = 0
			};
		
		
	private:	
		TInt ProcessCrash(const SCMCrashBlockEntry& aBlockEntry, const TUint aCrashId, TBool aCommit);
		TInt LogProcessMetaData(SCMDataSave::TDumpScope aDumpScope, TUint& aSizeDumped) const;
		TInt LogThreadMetaData(SCMDataSave::TDumpScope aDumpScope, TUint& aSizeDumped) const;
		TInt LogObjectContainers(TObjectType aObjectType, SCMDataSave::TDumpScope aDumpScope, const SCMDataSave::TDataToDump& aDataToDump, TUint& aSizeDumped) const;
		TInt GetNextCrashStartPoint(SCMCrashBlockEntry& aBlockEntry);
		void DoCrash(TAny* aCategory, TInt aReason);
		
	private:
		TInt HelpDumpStacks(DObject* aObject, TObjectType aObjectType, TUint& aSizeDumped, SCMDataSave::TStackType aStkType) const;
		TInt HelpDumpMetaData(DObject* aObject, TObjectType aObjectType, TUint& aSizeDumped) const;
		TInt EraseFlashBlock(const SCMCrashBlockEntry& aBlock);
		TInt EraseEntireFlashPartition();
		
	private:		
		SCMDataSave* iDataSave;
		Debug::SCMConfiguration* iScmConfig;
		SCMMultiCrashInfo* iMultiCrashInfo;
	};


	
#endif //__SCMONITOR_H__
//EOF scmonitor.h
