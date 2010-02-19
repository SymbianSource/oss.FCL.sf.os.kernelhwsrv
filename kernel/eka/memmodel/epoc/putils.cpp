// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\memmodel\epoc\putils.cpp
// EPOC implementation of the ROM related parts of the system
// 
//

#include "plat_priv.h"
#include <e32uid.h>
#include "execs.h"
#include "cache_maintenance.h"

_LIT(KKernelFullPathNameSysBin,"z:\\sys\\bin\\ekern.exe");

#ifdef __MARM__
#define	CHECK_ROM_ENTRY_POINT(a)	__ASSERT_ALWAYS( ((a).iFlags & KRomImageEptMask) == KRomImageEpt_Eka2, PP::Panic(PP::EUnsupportedOldBinary) )
#else
#define	CHECK_ROM_ENTRY_POINT(a)
#endif

void PP::Panic(TPlatPanic aPanic)
	{
	Kern::Fault("PLAT",aPanic);
	}
 
void PP::InitSuperPageFromRom(TLinAddr aRomHeader, TLinAddr aSuperPage)
	{
	RomHeaderAddress = aRomHeader;
	SuperPageAddress = aSuperPage;

	TInt j;
	for (j = 0; j < KNumTraceMaskWords; j++)
		TheSuperPage().iDebugMask[j] = TheRomHeader().iTraceMask[j];

	for (j = 0; j < 8; j++)
		TheSuperPage().iInitialBTraceFilter[j] = TheRomHeader().iInitialBTraceFilter[j];

	Kern::SuperPage().iInitialBTraceBuffer = TheRomHeader().iInitialBTraceBuffer;
	Kern::SuperPage().iInitialBTraceMode = TheRomHeader().iInitialBTraceMode;

	TheSuperPage().SetKernelConfigFlags(TheRomHeader().iKernelConfigFlags);

	memcpy(&TheSuperPage().iDisabledCapabilities, &TheRomHeader().iDisabledCapabilities, sizeof(TheRomHeader().iDisabledCapabilities));
	}

TInt P::DefaultInitialTime()
	{
//
// Default implementation of the kernel hook for getting the initial system
// time, can be overriden by variant.
//
    TInt seconds;
	if (K::ColdStart || A::SystemTimeInSecondsFrom2000(seconds)!=KErrNone)
		return KErrCorrupt;
	else
		return seconds;
	}

TInt P::InitSystemTime()
	{
//
//  Initialise system time
//	Return the initial time in seconds from 00:00:00 01-01-2000
//

	// Reset the UTC offset (I assume this gets loaded from storage at some point after F32 loads)
	TUint dummy;
	K::SetSystemTimeAndOffset(0, 0, 0, dummy, ETimeSetOffset | ETimeSetNoTimeUpdate);

	// Read the hardware clock value. If this is negative it means it couldnt be read.
    TInt seconds = K::InitialTimeHandler()();	

	if (seconds >= 0)
		{
		K::SecureClockStatus |= ESecureClockPresent;
		__KTRACE_OPT(KBOOT,Kern::Printf("Read initial system time"));
		// now=Hardware RTC value
		}
	else 
		{
		__KTRACE_OPT(KBOOT,Kern::Printf("Could not read initial system time - using ROM timestamp to set system time"));
		TTimeK rom_time=*(const TTimeK*)&TheRomHeader().iTime;
		rom_time -= TTimeK(K::HomeTimeOffsetSeconds)*1000000;
		TInt s;
		TInt r=K::SecondsFrom2000(rom_time,s);
		if (r!=KErrNone)
			PP::Panic(PP::EInitialSystemTimeInvalid);
		seconds=s;

		// write the ROM timestamp to the hardware RTC
		A::SetSystemTimeInSecondsFrom2000(seconds);
		}
	return seconds;
	}


void FindRomRootDirectory()
	{
	TUint variant = TheSuperPage().iActiveVariant;
	TUint cpu = (variant >> 16) & 0xff;
	TUint asic = (variant >> 24);
	PP::RomRootDirAddress=0;
	TRomRootDirectoryList* pL=(TRomRootDirectoryList*)TheSuperPage().iRootDirList;
	if (!pL)
		pL=(TRomRootDirectoryList*)TheRomHeader().iRomRootDirectoryList;
	TInt i;
	for (i=0; i<pL->iNumRootDirs; i++)
		{
		if (THardwareVariant(pL->iRootDir[i].iHardwareVariant).IsCompatibleWith(cpu,asic,variant))
			{
			__KTRACE_OPT(KBOOT,Kern::Printf("Found ROM root dir index %d addr %08x",
				i, pL->iRootDir[i].iAddressLin));
			PP::RomRootDirAddress=pL->iRootDir[i].iAddressLin;
			return;
			}
		}
	}

typedef TInt (*TInitVarExtFn)(const TRomImageHeader&);

#ifdef KBOOT
void DumpRomFileInfo(const TRomEntry& aRomEntry)
	{
	TBuf8<128> name;
	TInt i;
	for (i=0; i<aRomEntry.iNameLength; ++i)
		{
		name.Append(TChar(aRomEntry.iName[i<<1]&0xff));
		}
	const TRomImageHeader& img = *(const TRomImageHeader*)aRomEntry.iAddressLin;
	__KTRACE_OPT(KBOOT,Kern::Printf("File %S[%08x]", &name, TUint(img.iHardwareVariant) ));
	}
#endif

void InitVarExt(TBool aVar, TInitVarExtFn aFn)
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("InitVarExt var=%d, fn=%08x", aVar, aFn));
	TUint variant = TheSuperPage().iActiveVariant;
	TUint cpu = (variant >> 16) & 0xff;
	TUint asic = (variant >> 24);
	__KTRACE_OPT(KBOOT,Kern::Printf("cpu=%d, asic=%d, variant=%x", cpu, asic, variant));
	const TRomHeader& rh = TheRomHeader();
	TRomEntry* pE = aVar ? (TRomEntry*)rh.iVariantFile : (TRomEntry*)rh.iExtensionFile;
	while(pE)
		{
#ifdef KBOOT
		DumpRomFileInfo(*pE);
#endif
		const TRomImageHeader& img = *(const TRomImageHeader*)pE->iAddressLin;
		if (THardwareVariant(img.iHardwareVariant).IsCompatibleWith(cpu,asic,variant))
			{
			__KTRACE_OPT(KBOOT,Kern::Printf("Processing"));
			(*aFn)(img);
			if (aVar)
				{
				__KTRACE_OPT(KBOOT,Kern::Printf("Variant installed"));
				return;
				}
			}
		pE=(TRomEntry*)img.iNextExtension;
		}
	if (aVar)
		Kern::Fault("NoVariant",0);
	}

TInt InitData(const TRomImageHeader& a)
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("InitData %08x+%x->%08x", a.iDataAddress, a.iDataSize, a.iDataBssLinearBase));
	CHECK_ROM_ENTRY_POINT(a);
	if (a.iDataSize)
		memcpy((TAny*)a.iDataBssLinearBase,(TAny*)a.iDataAddress,a.iDataSize);
	TLibraryEntry ep = (TLibraryEntry)a.iEntryPoint;
	__KTRACE_OPT(KBOOT,Kern::Printf("Calling entrypoint %08x(VariantInit0)", ep));
	TInt r = ep(KModuleEntryReasonVariantInit0);
	__KTRACE_OPT(KBOOT,Kern::Printf("Entrypoint returned %d",r));
	if(!(++K::ExtensionCount&0x7fffffff))
		K::Fault(K::ETooManyExtensions);
	return r;
	}

TInt InitVariant(const TRomImageHeader& a)
	{
	TInt r = InitData(a);
	__KTRACE_OPT(KBOOT,Kern::Printf("InitVariant: entry point returns %08x", r));
	if (r<0)
		Kern::Fault("VariantEntry",r);

	// Initialise and create the variant object
	r = A::CreateVariant(&a, r);
	if (r<0)
		Kern::Fault("VariantInit",r);
	return r;
	}

void P::CreateVariant()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("CreateVariant"));
	BTrace::Init0();
	InitVarExt(EFalse, &InitData);		// initialise .data for all extensions
	InitVarExt(ETrue, &InitVariant);	// find variant and initialise it
	FindRomRootDirectory();
	}

struct SExtInit1EntryPoint
	{
	inline SExtInit1EntryPoint() : iEntryPoint(0),iReturnCode(0)
		{}
	inline SExtInit1EntryPoint(TLibraryEntry aEp, TInt aVal) : iEntryPoint(aEp),iReturnCode(aVal)
		{}
	TLibraryEntry iEntryPoint;
	TInt iReturnCode;		// bits 7-0 used for extension startup priority order
	};

// This ordering function when used in conjunction with RArray<>::InsertInOrderAllowRepeats
// orders the array of extensions as follow:
//		highest priority -> lowest priority
//		if same priority -> first in, lowest index
//
TInt priorityOrder(const SExtInit1EntryPoint& aMatch, const SExtInit1EntryPoint& aEntry)
	{
	TUint8 l=(TUint8)aMatch.iReturnCode;
	TUint8 r=(TUint8)aEntry.iReturnCode;
	if(l>r)
		return -1;
	else if(l<r)
		return 1;
	else
		return 0;
	}

TInt InitExt0(const TRomImageHeader& a)
	{
	CHECK_ROM_ENTRY_POINT(a);
	TLibraryEntry ep = (TLibraryEntry)a.iEntryPoint;
	__KTRACE_OPT(KBOOT,Kern::Printf("InitExt0 %08x ep=%08x", &a, ep));
	TInt r = (*ep)(KModuleEntryReasonExtensionInit0);
	if (r<KErrNone)
		{
		__KTRACE_OPT(KBOOT,Kern::Printf("Already started"));
		return r;
		}
	SExtInit1EntryPoint s(ep,r);
	TInt count=K::ExtensionArray->Count();
	if(count==K::ExtensionCount)					// this function is only called if extensions exist, i.e. K::ExtensionCount>0
		K::Fault(K::EExtensionArrayOverflowed);		// the first insertion will allocate space for K::ExtensionCount entries and that is the maximum number of entries allowed
	TLinearOrder<SExtInit1EntryPoint> PriorityOrder(priorityOrder);
	if(K::ExtensionArray->InsertInOrderAllowRepeats(s,PriorityOrder)!=KErrNone)
		K::Fault(K::EInsertExtensionFailed);
	__KTRACE_OPT(KBOOT,Kern::Printf("Inserted at index %d, priority %d, last index %d", K::ExtensionArray->SpecificFindInOrder(s,PriorityOrder,EArrayFindMode_Last)-1, (TUint8)r, count));
	return KErrNone;
	}

void P::StartExtensions()
	{
	// start extensions...
	__KTRACE_OPT(KBOOT, Kern::Printf("Starting kernel extensions..."));

	K::ExtensionArray = new RArray<SExtInit1EntryPoint>(--K::ExtensionCount);	// ordered array of extensions excluding Variant
	if(!K::ExtensionArray)
		K::Fault(K::EExtensionArrayAllocationFailed);
	__KTRACE_OPT(KBOOT, Kern::Printf("Entry point array at %08x, max size %d",K::ExtensionArray,K::ExtensionCount));

	InitVarExt(EFalse, &InitExt0);		// populates the array of entry points in priority order

	for(TInt i=0; i<K::ExtensionArray->Count(); i++)	// call entry points in combined priority and temporal orders
		{
		TLibraryEntry ep = (*K::ExtensionArray)[i].iEntryPoint;
		__KTRACE_OPT(KBOOT,Kern::Printf("InitExt1: calling entrypoint %08x", ep));
		TInt r = ep(KModuleEntryReasonExtensionInit1);
		__KTRACE_OPT(KBOOT,Kern::Printf("Entrypoint returned %d", r));
		if (r!=KErrNone)
			K::Fault(K::EStartExtensionsFailed);
		}
	// preserve array of extensions, it contains the returned codes from ExtInit0 which may be useful for future use
	//delete K::ExtensionArray;
	//K::ExtensionArray=NULL;
	}

void P::KernelInfo(TProcessCreateInfo& aInfo, TAny*& aStack, TAny*& aHeap)
//
// Provide the initial supervisor data information from the ROM
//
	{
	aInfo.iFileName=KKernelFullPathNameSysBin;
	aInfo.iRootNameOffset=11;
	aInfo.iRootNameLength=9;
	aInfo.iExtOffset = 16;

	aInfo.iAttr=ECodeSegAttKernel|ECodeSegAttFixed;

	const TRomHeader& romHdr=TheRomHeader();
	const TRomEntry* primaryEntry=(const TRomEntry*)TheSuperPage().iPrimaryEntry;
	const TRomImageHeader* primaryImageHeader=(const TRomImageHeader*)primaryEntry->iAddressLin;
	Epoc::RomProcessInfo(aInfo, *primaryImageHeader);
	aStack = (TAny*)(romHdr.iKernDataAddress + Kern::RoundToPageSize(romHdr.iTotalSvDataSize));
	aHeap = (TAny*)(TLinAddr(aStack) + Kern::RoundToPageSize(aInfo.iStackSize));
	aInfo.iTotalDataSize=romHdr.iTotalSvDataSize;
	aInfo.iHeapSizeMin=TheSuperPage().iInitialHeapSize;
	}

EXPORT_C void Epoc::RomProcessInfo(TProcessCreateInfo& aInfo, const TRomImageHeader& a)
	{
	CHECK_PAGING_SAFE;
	aInfo.iUids=*(const TUidType*)&a.iUid1;
	aInfo.iCodeSize=a.iCodeSize;
	aInfo.iTextSize=a.iTextSize;
	aInfo.iDataSize=a.iDataSize;
	aInfo.iBssSize=a.iBssSize;
	aInfo.iTotalDataSize=a.iTotalDataSize;
	aInfo.iEntryPtVeneer=a.iEntryPoint;
	aInfo.iFileEntryPoint=a.iEntryPoint;
	aInfo.iDepCount=a.iDllRefTable ? a.iDllRefTable->iNumberOfEntries : 0;
	aInfo.iExportDir=a.iExportDir;
	aInfo.iExportDirCount=a.iExportDirCount;
	aInfo.iCodeLoadAddress=(TLinAddr)&a;//a.iCodeAddress;
	aInfo.iCodeRunAddress=a.iCodeAddress;
	aInfo.iDataLoadAddress=a.iDataAddress;
	aInfo.iDataRunAddress=a.iDataBssLinearBase;
	aInfo.iExceptionDescriptor = a.iExceptionDescriptor;
	aInfo.iHeapSizeMin=a.iHeapSizeMin;
	aInfo.iHeapSizeMax=a.iHeapSizeMax;
	aInfo.iStackSize=a.iStackSize;
	aInfo.iPriority=a.iPriority;
	aInfo.iHandle=NULL;
	aInfo.iS = a.iS;
	aInfo.iModuleVersion = a.iModuleVersion;
	if (a.iFlags&KRomImageFlagsKernelMask)
		aInfo.iAttr=ECodeSegAttKernel;
	else
		aInfo.iAttr=ECodeSegAttGlobal;
	if (a.iFlags&KRomImageFlagFixedAddressExe)
		aInfo.iAttr|=ECodeSegAttFixed;
	aInfo.iAttr &= ~ECodeSegAttABIMask;
	aInfo.iAttr |= (a.iFlags & KRomImageABIMask);
	if(a.iFlags&KRomImageSMPSafe)
		aInfo.iAttr |= ECodeSegAttSMPSafe;
	aInfo.iClientHandle = KCurrentThreadHandle;
	aInfo.iClientProcessHandle = 0;
	aInfo.iFinalHandle = 0;
	aInfo.iOwnerType = EOwnerProcess;
	aInfo.iFlags &= ~(TProcessCreateInfo::EDataPagingMask);
	if(a.iFlags&KRomImageFlagDataPaged)
		aInfo.iFlags |= TProcessCreateInfo::EDataPaged;
	if(a.iFlags&KRomImageFlagDataUnpaged)
		aInfo.iFlags |= TProcessCreateInfo::EDataUnpaged;
	CHECK_ROM_ENTRY_POINT(a);
	}

EXPORT_C void Epoc::SetMonitorEntryPoint(TDfcFn aFn)
	{
	if (aFn)
		{
		TUint32 x=(TUint32)aFn;
		PP::MonitorEntryPoint[0]=x;
		PP::MonitorEntryPoint[1]=~x;
		PP::MonitorEntryPoint[2]=((x>>2)*~x);
		}
	else
		{
		PP::MonitorEntryPoint[0]=0;
		PP::MonitorEntryPoint[1]=0;
		PP::MonitorEntryPoint[2]=0;
		}
	}

EXPORT_C void Epoc::SetMonitorExceptionHandler(TLinAddr aHandler)
	{
	TheScheduler.iMonitorExceptionHandler=aHandler;
	}

EXPORT_C TAny* Epoc::ExceptionInfo()
	{
#ifdef __SMP__
	return 0;	// separate for each CPU
#else
	return TheScheduler.i_ExcInfo;
#endif
	}

EXPORT_C const TRomHeader& Epoc::RomHeader()
	{
	return TheRomHeader();
	}

TLinAddr ExecHandler::RomHeaderAddress()
	{
	return ::RomHeaderAddress;
	}

TLinAddr ExecHandler::RomRootDirectoryAddress()
	{
	return PP::RomRootDirAddress;
	}

TBool M::IsRomAddress(const TAny* aPtr)
	{
    TLinAddr start=::RomHeaderAddress;
    TLinAddr end=start+TheRomHeader().iUncompressedSize;
	return ((TLinAddr)aPtr>=start) && ((TLinAddr)aPtr<end);
	}

void P::SetSuperPageSignature()
	{
	TUint32* sig = TheSuperPage().iSignature;
	const TUint32* time = (const TUint32*)&TheRomHeader().iTime;
	sig[0] = time[0] ^ 0xb504f333;
	sig[1] = time[1] ^ 0xf9de6484;
	}

TBool P::CheckSuperPageSignature()
	{
	const TUint32* sig = TheSuperPage().iSignature;
	const TUint32* time = (const TUint32*)&TheRomHeader().iTime;
	return ( (sig[0]^time[0])==0xb504f333 && (sig[1]^time[1])==0xf9de6484 );
	}

static const TUint32 KMapAttrType2 = 0x80000000;
static const TUint32 KMapAttrTypeShift 		 = 26;

EXPORT_C TMappingAttributes2::TMappingAttributes2(TMemoryType	aType,
														TBool	aUserAccess,
														TBool	aWritable,
														TBool	aExecutable,
														TInt	aShared,
														TInt	aParity)
	{
	//Sort out default values:
	if (aShared<0)
		#if defined	(__CPU_USE_SHARED_MEMORY)
		aShared = 1;
		#else
		aShared = 0;	
		#endif
	if (aParity<0)
		aParity = 0;
	
	// KMapAttrType2 bit marks the object as of TMappingAttributes2 type (as opposed to TMappingAttributes bitmask).
	// We have to make sure that these two types can work together.

	iAttributes =	KMapAttrType2				|	// Mark it as TMappingAttributes2 object
					EMapAttrReadSup				|	// All memory is readable from Kernel (Supervisor) mode
					(aType <<KMapAttrTypeShift)	|
					(aUserAccess ? EMapAttrReadUser : 0)|
					(aWritable	 ? EMapAttrWriteSup : 0)|
		((aWritable&&aUserAccess)? EMapAttrWriteUser: 0)|
#ifdef __MMU_USE_SYMMETRIC_ACCESS_PERMISSIONS
					(aExecutable   ? EMapAttrExecSup : 0)|
		((aExecutable&&aUserAccess)? EMapAttrExecUser: 0)|
#else
					(aExecutable ? EMapAttrExecUser|EMapAttrExecSup : 0)|
#endif
					(aShared	 ? EMapAttrShared	: 0)|
					(aParity	 ? EMapAttrUseECC	: 0);
	
	// Kernel relies on TMappingAttributes bitmask when dealing with various memory mappings.
	// Set cache attribute bits as they are in TMappingAttributes.
	iAttributes |= InternalCache::TypeToCachingAttributes(aType);
	}

TMappingAttributes2::TMappingAttributes2(TUint aMapAttr):iAttributes(aMapAttr)
	{
	};

TMemoryType TMappingAttributes2::Type()
	{
	if(iAttributes&KMapAttrType2)
		return (TMemoryType)(iAttributes>>KMapAttrTypeShift & 0x7); //three bits for memory type.

	switch(iAttributes&EMapAttrL1CacheMask)
		{
	case EMapAttrFullyBlocking:
		return EMemAttStronglyOrdered;

	case EMapAttrBufferedNC:
		return EMemAttDevice;

	case EMapAttrBufferedC:
	case EMapAttrL1Uncached:
	case EMapAttrCachedWTRA:
	case EMapAttrCachedWTWA:
	case EMapAttrAltCacheWTRA:
	case EMapAttrAltCacheWTWA:
		return EMemAttNormalUncached;

	case EMapAttrCachedWBRA:
	case EMapAttrCachedWBWA:
	case EMapAttrAltCacheWBRA:
	case EMapAttrAltCacheWBWA:
	case EMapAttrL1CachedMax:
		return EMemAttNormalCached;

	default:
		Panic(KErrArgument);
		return EMemAttNormalCached;
		}
	}

TBool TMappingAttributes2::UserAccess()	{return (iAttributes&EMapAttrUserRw   ?	(TBool)ETrue : (TBool)EFalse);}
TBool TMappingAttributes2::Writable()	{return (iAttributes&EMapAttrWriteMask? (TBool)ETrue : (TBool)EFalse);}
#ifdef __MMU_USE_SYMMETRIC_ACCESS_PERMISSIONS
TBool TMappingAttributes2::Executable()	{return (iAttributes&EMapAttrExecMask ? (TBool)ETrue : (TBool)EFalse);}
#else
TBool TMappingAttributes2::Executable()	{return (iAttributes&EMapAttrExecUser ? (TBool)ETrue : (TBool)EFalse);}
#endif
TBool TMappingAttributes2::Shared()		{return (iAttributes&EMapAttrShared   ?	(TBool)ETrue : (TBool)EFalse);}
TBool TMappingAttributes2::Parity()		{return (iAttributes&EMapAttrUseECC	  ?	(TBool)ETrue : (TBool)EFalse);}
TBool TMappingAttributes2::ObjectType2(){return (iAttributes&KMapAttrType2	  ?	(TBool)ETrue : (TBool)EFalse);}
void  TMappingAttributes2::Panic(TInt aPanic)	{Kern::Fault("TMappingAttributes2",aPanic);}


#ifdef __DEBUGGER_SUPPORT__
 /**
 Initialises the breakpoint pool.
 There is only one breakpoint pool in the system. The breakpoint pool should be initialised only once - usually from
 the run-mode debugger device driver.

 @param aCapabilities	On return this is set to a bitmask of values from enum DebugSupport::TType which represents the
 						supported breakpoint types. At the moment only DebugSupport::EBreakpointGlobal type is supported.
 @param aMaxBreakpoints The number of breakpoints for which resources should be reserved. It represents
                        the maximum number of the breakpoints at a time.

 @return KErrNoMemory, 		if not enough memory to reserve breakpoint resources.
   		 KErrInUse,    		if breakpoint pool already exists. Indicates that another debug tool might be using it at the moment.
   		 KErrNotSupported, 	if Kernel is not built with __DEBUGGER_SUPPORT__ option
   		 KErrNone,			on success.

 @pre   No fast mutex can be held.
 @pre   Kernel must be unlocked.
 @pre	Call in a thread context.
 @pre	Interrupts must be enabled.
 */
EXPORT_C TInt DebugSupport::InitialiseCodeModifier(TUint& aCapabilities, TInt aMaxBreakpoints)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DebugSupport::InitialiseCodeModifier");
	TInt err;
	NKern::ThreadEnterCS();
	Kern::MutexWait(CodeModifier::Mutex());

	if ( KErrNone == (err =CodeModifier::CreateAndInitialise(aMaxBreakpoints)))
		aCapabilities = EBreakpointGlobal;
	
	Kern::MutexSignal(CodeModifier::Mutex());
	NKern::ThreadLeaveCS(); 
	return err;
	}

 /**
 Restore all breakpoints and free resources.
 Must not be called before Initialise().

 @panic CodeModifier 0 if called before InitialiseCodeModifier().

 @pre   No fast mutex can be held.
 @pre   Kernel must be unlocked.
 @pre	Call in a thread context.
 @pre	Interrupts must be enabled.
 */
EXPORT_C void DebugSupport::CloseCodeModifier()
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DebugSupport::CloseCodeModifier");
	NKern::ThreadEnterCS(); 
	Kern::MutexWait(CodeModifier::Mutex());

	if (!TheCodeModifier)
		{
		Kern::MutexSignal(CodeModifier::Mutex());
		NKern::ThreadLeaveCS(); 
		CodeModifier::Fault(CodeModifier::EPanicNotInitialised);
		}
	TheCodeModifier->Close();

	Kern::MutexSignal(CodeModifier::Mutex());
	NKern::ThreadLeaveCS(); 
	}

/**
Write a single breakpoint.
I.e. store aValue at location aAddress in the address space of aThread.
If the address resides in XIP code (ROM image), the memory page is shadowed before the content of the aAddress is altered.

The breakpoint should be cleared/restored by DebugSupport::RestoreCode with matching aThread and aAddress.
The breakpoints are owned by the corresponding process. Therefore:
@code 
DebugSupport::ModifyCode(thread1, address, size, value, type);
and
DebugSupport::ModifyCode(thread2, address size, value, type);
@endcode
have the same effect if thread1 and thread2 belong to the same process.

Breakpoints of the diferent type(size) cannot overlap each other. For example:
@code 
DebugSupport::ModifyCode(thread, address,   4, value, type); //address is word aligned address
DebugSupport::ModifyCode(thread, address,   2, value, type); //will return KErrAccessDenied
DebugSupport::ModifyCode(thread, address+2, 2, value, type); //will return KErrAccessDenied
DebugSupport::ModifyCode(thread, address+1, 1, value, type); //will return KErrAccessDenied
@endcode

After the content of aAddress is altered, instruction cache invalidation is performed on the cache line that aAddress
belongs to. Therefore, the device driver doesn't have to call Cache::IMB_Range().

If a code segment (which a valid breakpoint belongs to) is removed from the given process, the breakpoint will be
automatically removed. This occures just before EEventRemoveCodeSeg event is issued with DProcess* matching
the breakpoint's process. This also applies to the terminating/killed process, as all breakpoints belonging to it will be removed too.

@param aThread 	The thread in who's address space the breakpoint is to be written.
@param aAddress The linear address of the breakpoint. Must be a multiple of aSize.
@param aSize 	The size, in bytes, of the breakpoint. Must be 1,2 or 4.
@param aValue 	The value to be stored at aAddress. This value is trucated to the
     			number of bits relevent to aSize.
@param aType 	The breakpoint type required. This is a bitmask of values from enum TType.
     			If this specifies more than one type, then the type with least scope
     			(i.e. EBreakpointLocal) is used when this is supported.

 @return KErrNoMemory,      if no resources are available.
   		 KErrAlreadyExists, if a breakpoint with the same address, size and the same owning process already exists in the pool.
   		 KErrAccessDenied, 	if an existing breakpoint of a different size and the same owning process overlaps the specified breakpoint.
   		 KErrNotSupported,  if none of the breakpoints types specified are supported or if Kernel is not built with __DEBUGGER_SUPPORT__ option
   		 Otherwise,         a positive value from enum TType which represents the type of breakpoint written.

 @panic CodeModifier 0 if called before InitialiseCodeModifier().
 @panic CodeModifier 1 if aSize value or aAdress alignement is invalid.

 @pre   No fast mutex can be held.
 @pre   Kernel must be unlocked.
 @pre	Call in a thread context.
 @pre	Interrupts must be enabled.
 */
EXPORT_C TInt DebugSupport::ModifyCode(DThread* aThread, TLinAddr aAddress, TInt aSize, TUint aValue, TUint aType)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DebugSupport::ModifyCode");
	switch(aSize) //Chack aSize and aValue
	{
	case CodeModifier::EByte:
		break;
	case CodeModifier::EHalfword:
		if ((TInt)aAddress & 1)
			CodeModifier::Fault(CodeModifier::EPanicInvalidSizeOrAlignment);
		break;
	case CodeModifier::EWord:	
		if ((TInt)aAddress & 3)
			CodeModifier::Fault(CodeModifier::EPanicInvalidSizeOrAlignment);
		break;
	default:
		CodeModifier::Fault(CodeModifier::EPanicInvalidSizeOrAlignment);
	}

	if (aType != DebugSupport::EBreakpointGlobal)//Check breakpoint type
		return KErrNotSupported;
	
	NKern::ThreadEnterCS(); 
	Kern::MutexWait(CodeModifier::Mutex());
	
	if (!TheCodeModifier)
		{
		Kern::MutexSignal(CodeModifier::Mutex());
		NKern::ThreadLeaveCS(); 
		CodeModifier::Fault(CodeModifier::EPanicNotInitialised);	
		}
	TInt r = TheCodeModifier->Modify(aThread, aAddress, aSize, aValue);

	Kern::MutexSignal(CodeModifier::Mutex());
	NKern::ThreadLeaveCS(); 

	if (r)
		return r;
	return EBreakpointGlobal;
	}

 /**
 Restore a previousely written breakpoint.
 I.e. restore the value at location aAddress in the address space of aProcess.

 After the content of aAddress is altered, instruction cache invalidation is performed on the cache line
 that aAddress belongs to. Therefore, the device driver doesn't have to call Cache::IMB_Range().

 If the address resides in shadowed memory, the memory page will be un-shadowed if this is the last breakpoint
 in the page. However, if the page had been already shadowed before the first breakpoint in the page was applied,
 the page will remain shadowed.

 @param aProcess The process in who's address space aAddress lies.
 @param aAddress The linear address of an existing breakpoint.

 @return KErrNotFound,		if the breakpoint hadn't been previously written. It is also returned if the breakpoint
                        	was previously removed from the list because the code segment (which the breakpoint belongs to) was
                        	unloaded/removed from the process associated with the breakpoint.
   		 KErrNotSupported, 	if Kernel is not built with __DEBUGGER_SUPPORT__ option
    	 KErrNone,			on success.

 @panic CodeModifier 0 if called before InitialiseCodeModifier().

 @pre   No fast mutex can be held.
 @pre   Kernel must be unlocked.
 @pre	Call in a thread context.
 @pre	Interrupts must be enabled.
 */
EXPORT_C TInt DebugSupport::RestoreCode(DThread* aThread, TLinAddr aAddress)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DebugSupport::RestoreCode");
	NKern::ThreadEnterCS(); 
	Kern::MutexWait(CodeModifier::Mutex());

	if (!TheCodeModifier)
		{
		Kern::MutexSignal(CodeModifier::Mutex());
		NKern::ThreadLeaveCS(); 
		CodeModifier::Fault(CodeModifier::EPanicNotInitialised);
		}
	TInt r = TheCodeModifier->Restore(aThread, aAddress);

	Kern::MutexSignal(CodeModifier::Mutex());
	NKern::ThreadLeaveCS(); 
	return r;
	}

 /**
 Terminates a specified process on behalf of a debugger.

 @param aProcess The process in who's address space aAddress lies.
 @param aReason The reason code to supply when terminating a process

 @return N/A.

 @pre   No fast mutex can be held.
 @pre   Kernel must be unlocked.
 @pre	Call in a thread context.
 @pre	Interrupts must be enabled.
 */
EXPORT_C void DebugSupport::TerminateProcess(DProcess* aProcess, const TInt aReason)
	{
	CHECK_PRECONDITIONS(MASK_THREAD_STANDARD,"DebugSupport::TerminateProcess");
	NKern::ThreadEnterCS(); 
	aProcess->Die(EExitTerminate,aReason,KNullDesC);
	NKern::ThreadLeaveCS(); 
	return;
	}

/**
Creates CodeModifier.
@param aMaxBreakpoints The number of breakpoints to be allocated.
@return KErrInUse 	 if code modifier already exists.
		KErrNoMemory if out of memory
		KErrNone 	 on success
@pre Calling thread must be in the critical section
@pre CodeSeg mutex held
*/
TInt CodeModifier::CreateAndInitialise(TInt aMaxBreakpoints)
	{
	if (TheCodeModifier)
		return KErrInUse;

	CodeModifier* modifier = new CodeModifier;
	if (!modifier)
		return KErrNoMemory;
	
	modifier->iBreakpoints = new TBreakpoint[aMaxBreakpoints];
	if (!modifier->iBreakpoints)
		{
		delete modifier;	
		return KErrNoMemory;
		};
		
	modifier->iPages = new TPageInfo[aMaxBreakpoints];
	if (!modifier->iPages)
		{
		delete[] modifier->iBreakpoints;
		delete modifier;	
		return KErrNoMemory;
		}

	modifier->iPoolSize = aMaxBreakpoints;
	modifier->iPageSize = Kern::RoundToPageSize(1);
	modifier->iPageMask = ~(modifier->iPageSize-1);

	TheCodeModifier = modifier;
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::CreateAndInitialise() Size:%d created", aMaxBreakpoints));
	return KErrNone;	
	}

/**
Sets breakpoint.
@pre Calling thread must be in the critical section
@pre CodeSeg mutex held
*/
TInt CodeModifier::Modify(DThread* aThread, TLinAddr aAddress, TInt aSize, TUint aValue)
	{
	TInt r;
	TUint oldValue;
	TBool overlap;
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::Modify() T:%x Addr:%x, Size:%d Val:%x", aThread, aAddress, aSize, aValue));
	
	TBreakpoint* brk =FindBreakpoint(aThread, aAddress,aSize,overlap);
	if (overlap)
		return KErrAccessDenied;
	if (brk)
		return KErrAlreadyExists;
	
	if(NULL==(brk = FindEmptyBrk()))
		return KErrNoMemory;
	
	//Find the page (if exists). Shadow the page if necessery.
	TInt pageIndex = -1;

#ifndef __DEMAND_PAGING__ 
	if (IsRom(aAddress))  // If no demand paging, only need to do this if the address is in rom
#endif
		{
		pageIndex = FindPageInfo(aAddress);
		if (pageIndex < 0)
			{
			pageIndex = FindEmptyPageInfo();
			if (pageIndex < 0)
				return KErrNoMemory;
			TPageInfo& page = iPages[pageIndex];
			memclr(&page, sizeof(page));
			page.iAddress = aAddress & iPageMask;
			
			if (IsRom(aAddress))
				{
				__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::Modify() - Shadowing Page"));
				r = Epoc::AllocShadowPage(aAddress & iPageMask);
				if (r==KErrAlreadyExists)
					page.iWasShadowed = ETrue;
				else if (r!=KErrNone)
					return r;
				}
#ifdef __DEMAND_PAGING__
			else
				{
				DDemandPagingLock* lock = new DDemandPagingLock;
				if (lock == NULL)
					return KErrNoMemory;
				r = lock->Alloc(iPageSize);
				if (r != KErrNone)
					{
					delete lock;
					return r;
					}
				lock->Lock(aThread, aAddress & iPageMask, iPageSize);
				page.iPagingLock = lock;
				}
#endif
			}
		iPages[pageIndex].iCounter++;
		}

	r = SafeWriteCode(aThread->iOwningProcess, aAddress, aSize, aValue, &oldValue);
	if (r != KErrNone)
		{//aAddress is invalid
		if (pageIndex >= 0)
			RestorePage(pageIndex);
		return r;
		}

	//All done. Update the internal structures.
	brk->iAddress = aAddress;
	brk->iProcessId = (aThread->iOwningProcess)->iId;
	brk->iOldValue = oldValue;
	brk->iSize = aSize;
	brk->iPageIndex = pageIndex;
	return KErrNone;
	}

/**
@pre Calling thread must be in the critical section
@pre CodeSeg mutex held
*/
TInt CodeModifier::Restore(DThread* aThread, TLinAddr aAddress)
	{
	TUint oldValue;
	TBool overlaps;
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::Restore() T:%x Addr:%x", aThread, aAddress));
	TInt r = KErrNone;;
	TBreakpoint* br = FindBreakpoint(aThread, aAddress, 0, overlaps);
	if (br==NULL)
		return KErrNotFound;
	
	r = SafeWriteCode(aThread->iOwningProcess, br->iAddress, br->iSize, br->iOldValue, &oldValue);
	if (r)
		r=KErrNotFound;
	
	br->iSize = (TUint)EEmpty;
		
	TInt pageIndex = br->iPageIndex;
	if (pageIndex>=0)
		RestorePage(pageIndex);
	
	return r;
	}

/*
@pre Calling thread must be in the critical section
@pre CodeSeg mutex held
*/
void CodeModifier::Close()
	{
	TUint oldValue;
	TInt brkIndex;

	TheCodeModifier = NULL;
	
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::Close()"));

	for (brkIndex=0; brkIndex<iPoolSize; brkIndex++)	
		{
		if (iBreakpoints[brkIndex].iSize ==(TUint16)EEmpty)
			continue;
		DProcess* process = Process(iBreakpoints[brkIndex].iProcessId);
		
		__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::Close() - Removing Brk:%x",iBreakpoints[brkIndex].iAddress));
		
		//Write back the original value
		if (process)
			SafeWriteCode(process, iBreakpoints[brkIndex].iAddress, iBreakpoints[brkIndex].iSize, iBreakpoints[brkIndex].iOldValue, &oldValue);
			
		iBreakpoints[brkIndex].iSize = (TUint)EEmpty;
		TInt pageIndex = iBreakpoints[brkIndex].iPageIndex;
		if (pageIndex>=0)
			RestorePage(pageIndex);
		}

	delete this;
	}

/*
Destructor. The object is deleted asynchroniously from Kernel Supervisor thread.
*/
CodeModifier::~CodeModifier()
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::~CodeModifier()"));
	delete[] iPages;
	delete[] iBreakpoints;
	}

/**
This is executed when a code segment is about to be unmapped from the process. It corresponds to EEventRemoveCodeSeg Kernel event.
Removes breakpoints that belong to the threads from aProcess. Also, removes shadow pages if there is no breakpoint left in them.

@param aCodeSeg Code Segment that is removed from aProcess.
@param aProcess Process from whom the code segment is removed.

@pre Calling thread must be in the critical section
@pre CodeSeg mutex held
*/
void CodeModifier::CodeSegRemoved(DCodeSeg* aCodeSeg, DProcess* aProcess)
	{
	if (!TheCodeModifier)
		return;
	TheCodeModifier->DoCodeSegRemoved(aCodeSeg, aProcess);
	}

void CodeModifier::DoCodeSegRemoved(DCodeSeg* aCodeSeg, DProcess* aProcess)
	{
	__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::CodeSegRemoved()"));
	
	TUint oldValue;
	TUint minAddr = aCodeSeg->iRunAddress;
	TUint maxAddr = aCodeSeg->iRunAddress + aCodeSeg->iSize;

	TBreakpoint* bp = iBreakpoints;
	TBreakpoint* bpEnd = bp+iPoolSize; //points right behind iBreakpoints
	for (; bp<bpEnd; ++bp)
		{
		if (bp->iSize == (TUint)EEmpty) continue;

		if (aProcess->iId == bp->iProcessId)
			{
			if (bp->iAddress >= minAddr && bp->iAddress <  maxAddr)   
				{
				__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::CodeSegRemoved()- a breakpoint"));

				//Remove breakpoint. Don't examine error code as there is nobody to report to.
				SafeWriteCode(aProcess, bp->iAddress, bp->iSize, bp->iOldValue, &oldValue);

				//Mark the slot as empty and decrease the counter of the shadow page slot (if there is any associated)
				bp->iSize = (TUint)EEmpty;
				if (bp->iPageIndex >= 0)
					RestorePage(bp->iPageIndex);
				}
			}
		}

	}

/*
Finds DProcess that matches to processId
@param aProcessId ProcessId
@return Pointer to matching DProcess or NULL
*/
DProcess* CodeModifier::Process(TUint aProcessId)
	{
	TInt i;
	DProcess* process = NULL;
	DObjectCon* processCon = Kern::Containers()[EProcess];
	processCon->Wait();

	for (i=0;i<processCon->Count();i++)
		{
		DProcess* pr = (DProcess*)(*processCon)[i];
		if (pr->iId == aProcessId)
			{
			process=(DProcess*)pr;
			break;
			}
		}

	processCon->Signal();
	return process;
	}

/*
Returns eTrue if given virtual address belongs to rom image, EFalse otherwise
*/
TBool CodeModifier::IsRom(TLinAddr aAddress)
	{
	TRomHeader romHeader = Epoc::RomHeader();
	if ( (aAddress >= romHeader.iRomBase ) && (aAddress < (romHeader.iRomBase + romHeader.iUncompressedSize)) )
		return ETrue;
	return EFalse;
	}

/*
Finds the first available(empty) breakpoint slot.
@return The pointer of the empty slot or NULL if all occupied.
*/
CodeModifier::TBreakpoint* CodeModifier::FindEmptyBrk()
	{
	TBreakpoint* bp = TheCodeModifier->iBreakpoints;
	TBreakpoint* bpEnd = bp+TheCodeModifier->iPoolSize; //points right behind iBreakpoints
	for (; bp<bpEnd; ++bp)
		if (bp->iSize == (TInt16)EEmpty)
			return bp;
		
	return NULL;	
	}
	
/*
Finds matching breakpoint.

@param aThread 		The thread who's process owns the breakpoint
@param aAddress 	Address of the breakpoint.
@param aSize 		The size of the breakpoint. Value 0, 1,2 or 4 is assumed. If 0, it doesn't check the size nor overlaps(used to remove breakpoint).
@param aOverlap 	On return, it is true if a breakpoint is found that doesn't match the size but overlaps with
					the specified breakpoint(i.e. address and process are the same but the size is different).

@return - The pointer to the breakpoint slot that matches the entry (adress, size and the owning process)
		- NULL - if could't find the matching breakpoint.
*/
CodeModifier::TBreakpoint* CodeModifier::FindBreakpoint(DThread* aThread, TLinAddr aAddress, TInt aSize, TBool& aOverlap)
	{
	TInt bytes=0;
	aOverlap = EFalse;
	TUint processId = aThread->iOwningProcess->iId;//processId of the thread that owns aThread

	if (aSize) //if size==0, we do not check overlaps.
		bytes = ((1<<aSize)-1)<<(aAddress&3);	//bits[3-0] marks the bytes that are contained in the breakpoint:
												//	address: ...00b size: 1 => bytes=0001b 
												//	address: ...01b size: 1 => bytes=0010b 
												//	address: ...10b size: 1 => bytes=0100b 
												//	address: ...11b size: 1 => bytes=1000b 
												//	address: ...00b size: 2 => bytes=0011b 
												//	address: ...10b size: 2 => bytes=1100b 
												//	address: ...00b size: 4 => bytes=1111b 

	TBreakpoint* bp = TheCodeModifier->iBreakpoints;
	TBreakpoint* bpEnd = bp+TheCodeModifier->iPoolSize; //points right behind iBreakpoints
	for (; bp<bpEnd; ++bp)
		{
		if (bp->iSize == (TInt16)EEmpty || bp->iProcessId != processId)
			continue;//Either empty or not matchng process. 

		if (!aSize)
			{ //Do not check the size. If the address does not match, do not check for overlap.
			if (bp->iAddress == aAddress)
				return bp;
			else
				continue;
			}
			
		if (bp->iAddress == aAddress && bp->iSize == aSize)
			return bp;//If we find a matching breakpoint, there cannot be another one that overlaps
		
		//Check if bp breakpoint overlaps with the specified one.
		if ((bp->iAddress^aAddress)>>2)
			continue;//Not in the same word
			
		if (((1<<bp->iSize)-1)<<(bp->iAddress&3)&bytes)
			{//Two brakpoints are within the same word with some overlaping bytes.
			aOverlap = ETrue;
			return NULL; //If we find an overlaping breakpoint, there cannot be another one that matches exactly.
			}
		}
	return NULL;	
	}

/*
Finds the first available(empty) page info slot.
@return The index of the slot or KErrNotFound if all occupied.
*/
TInt CodeModifier::FindEmptyPageInfo()
	{
	TInt i;
	for (i=0; i<iPoolSize; i++)
		if (!iPages[i].iCounter)
			return i;
	return KErrNotFound;
	}

/*
Finds the page info structure that contains given virtual address
@return The index of the page info slot or KErrNotFound.
*/
TInt CodeModifier::FindPageInfo(TLinAddr aAddress)
	{
	TInt i;
	aAddress &= iPageMask; //round down to the page base address
	for (i=0; i<iPoolSize; i++)
		if(iPages[i].iCounter)
			if (iPages[i].iAddress == aAddress)
				return i;
	return KErrNotFound;
	}

/**
Decrement the count of breakpoints associated with this page, and restores page
to its original state if there are none remaining.
*/
void CodeModifier::RestorePage(TInt aPageIndex)
	{
	TPageInfo& page = iPages[aPageIndex];
	if(--page.iCounter==0)
		{
		if (!page.iWasShadowed)
			{
			__KTRACE_OPT(KDEBUGGER,Kern::Printf("CodeModifier::Restore() - Freeing Shadow Page"));
			Epoc::FreeShadowPage(page.iAddress);
			}
#ifdef __DEMAND_PAGING__
		if (page.iPagingLock)
			{
			// Release lock and free resources
			delete page.iPagingLock;
			page.iPagingLock = NULL;
			}
#endif
		}
	}

void CodeModifier::Fault(TPanic aPanic)
	{
	Kern::Fault("CodeModifier", aPanic);
	}

#else //__DEBUGGER_SUPPORT__
EXPORT_C TInt DebugSupport::InitialiseCodeModifier(TUint& /*aCapabilities*/, TInt /*aMinBreakpoints*/)
	{
	return KErrNotSupported;	
	}
EXPORT_C void DebugSupport::CloseCodeModifier()
	{
	}
EXPORT_C TInt DebugSupport::ModifyCode(DThread* /*aProcess*/, TLinAddr /*aAddress*/, TInt /*aSize*/, TUint /*aValue*/, TUint /*aType*/)
	{
	return KErrNotSupported;	
	}
EXPORT_C TInt DebugSupport::RestoreCode(DThread* /*aProcess*/, TLinAddr /*aAddress*/)
	{
	return KErrNotSupported;	
	}
EXPORT_C void DebugSupport::TerminateProcess(DProcess* /*aProcess*/, const TInt /*aReason*/)
	{
	}
#endif

