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
// e32\include\u32std.h
//
//

/**
 @file
 @internalComponent
 @released
*/

#ifndef __U32STD_H__
#define __U32STD_H__
#include <e32cmn.h>
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32cmn_private.h>
#endif
#include <e32hal.h>
#include <e32lmsg.h>
#include <e32event.h>
#include <e32ldr.h>
#include <e32ldr_private.h>
#include <e32power.h>
#include <e32shbufcmn.h>
#include <e32property.h>
#include <u32property.h>
#include <u32hal.h>

#include <cpudefs.h>

#ifdef __MARM__
#define	EKA2_ENTRY_POINT_VERSION_IDENTIFIER	\
	asm("tst pc, #%a0" : : "i" ((TInt)0) )
#endif

struct TUnicodeDataSet;                 // forward declaration
struct TCollationDataSet;               // forward declaration

/*
The LCharSet structure is used in Unicode builds to supply locale-specific
character attribute and collation data.

The structure is defined in both builds to avoid having to have a dummy ExecHandler::GetLocaleCharSet function
with a different signature in the 8-bit build.
*/
struct LCharSet
	{
	const TUnicodeDataSet* iCharDataSet;			// if non-null, character data overriding standard Unicode data
	const TCollationDataSet* iCollationDataSet;		// if non-null, locale-specific collation data
	};

extern const LCharSet* GetLocaleCharSet();

/** @internalTechnology */
const TInt KNumLocaleExports = 22;

//
// The bits in the type table (non-Unicode build only)
//
#ifndef _UNICODE

/** @internalTechnology */
const TUint __U=0x01; // Uppercase letter

/** @internalTechnology */
const TUint __L=0x02; // Lowercase letter

/** @internalTechnology */
const TUint __D=0x04; // Decimal digit

/** @internalTechnology */
const TUint __S=0x08; // Space

/** @internalTechnology */
const TUint __P=0x10; // Punctuation

/** @internalTechnology */
const TUint __C=0x20; // Control character

/** @internalTechnology */
const TUint __X=0x40; // Hex digit

/** @internalTechnology */
const TUint __B=0x80; // A blank character

#endif

//
// Time set mode parameters for setting system time and offset
//
enum TTimeSetMode
	{
	ETimeSetTime = 1,    // set the time to the value given, else leave it unchanged
	ETimeSetOffset = 2,  // set the offset to the value given, else leave it unchanged
	ETimeSetAllowTimeReversal = 4,  // allow time to go backwards
	ETimeSetNoTimeUpdate = 8,       // Don't restart second queue or notify changes - not valid with ESetTime, used early in boot only
	ETimeSetLocalTime = 16,			// Set time in local time, instead of UTC
	ETimeSetSecure = 32,  // use when setting the secure hardware clock
	};

//
enum TMatchType {EMatchNormal,EMatchFolded,EMatchCollated};

//
// Constants for descriptor implementation code
//
enum TDesType {EBufC,EPtrC,EPtr,EBuf,EBufCPtr};
const TUint KMaskDesLength=0xfffffff;
const TInt KShiftDesType=28;

//
// Constants for iFlags in DProcess and DThread
//
const TUint KThreadFlagProcessCritical		= 0x00000001;	// thread panic panics process
const TUint KThreadFlagProcessPermanent		= 0x00000002;	// thread exit of any kind causes process to exit (=main)
const TUint KThreadFlagSystemCritical		= 0x00000004;	// thread panic reboots entire system
const TUint KThreadFlagSystemPermanent		= 0x00000008;	// thread exit of any kind reboots entire system
const TUint KThreadFlagOriginal				= 0x00000010;
const TUint KThreadFlagLastChance			= 0x00000020;
const TUint KThreadFlagRealtime				= 0x00000040;	// thread will be panicked when using some non-realtime functions
const TUint KThreadFlagRealtimeTest			= 0x00000080;	// non-realtime functions only warn rather than panic
const TUint KThreadFlagLocalThreadDataValid	= 0x00000100;	// thread has valid local thread data
const TUint KProcessFlagPriorityControl		= 0x40000000;
const TUint KProcessFlagJustInTime			= 0x80000000;
const TUint KProcessFlagSystemCritical		= KThreadFlagSystemCritical;	// process panic reboots entire system
const TUint KProcessFlagSystemPermanent		= KThreadFlagSystemPermanent;	// process exit of any kind reboots entire system
//
const TUint KThreadHandle=0x40000000;
//
struct SPtrC8 {TInt length;const TUint8 *ptr;};
struct SBufC8 {TInt length;TUint8 buf[1];};
struct SPtr8 {TInt length;TInt maxLength;TUint8 *ptr;};
struct SBuf8 {TInt length;TInt maxLength;TUint8 buf[1];};
struct SBufCPtr8 {TInt length;TInt maxLength;SBufC8 *ptr;};

struct SPtrC16 {TInt length;const TUint16 *ptr;};
struct SBufC16 {TInt length;TUint16 buf[1];};
struct SPtr16 {TInt length;TInt maxLength;TUint16 *ptr;};
struct SBuf16 {TInt length;TInt maxLength;TUint16 buf[1];};
struct SBufCPtr16 {TInt length;TInt maxLength;SBufC16 *ptr;};

//
// Flags used for IPC copy functions
//
const TInt KChunkShiftBy0=0;
const TInt KChunkShiftBy1=KMinTInt;
const TInt KIpcDirRead=0;
const TInt KIpcDirWrite=0x10000000;

class TChunkCreate
	{
public:
	// Attributes for chunk creation that are used by both euser and the kernel
	// by classes TChunkCreateInfo and SChunkCreateInfo, respectively.
	enum TChunkCreateAtt
		{
		ENormal				= 0x00000000,
		EDoubleEnded		= 0x00000001,
		EDisconnected		= 0x00000002,
		ECache				= 0x00000003,
		EMappingMask		= 0x0000000f,
		ELocal				= 0x00000000,
		EGlobal				= 0x00000010,
		EData				= 0x00000000,
		ECode				= 0x00000020,
		EMemoryNotOwned		= 0x00000040,

		// Force local chunk to be named.  Only required for thread heap
		// chunks, all other local chunks should be nameless.
		ELocalNamed 		= 0x000000080,

		// Make global chunk read only to all processes but the controlling owner
		EReadOnly			= 0x000000100,

		// Paging attributes for chunks.
		EPagingUnspec		= 0x00000000,
		EPaged				= 0x80000000,
		EUnpaged			= 0x40000000,
		EPagingMask 		= EPaged | EUnpaged,

		EChunkCreateAttMask =	EMappingMask | EGlobal | ECode |
								ELocalNamed | EReadOnly | EPagingMask,
		};
public:
	TUint iAtt;
	TBool iForceFixed;
	TInt iInitialBottom;
	TInt iInitialTop;
	TInt iMaxSize;
	TUint8 iClearByte;
	};

enum TChunkRestrictions
	{
	// Keep this in sync with definitions in RChunk
	EChunkPreventAdjust = 0x01,  // Disallow Adjust, Commit, Allocate and Decommit
	};

class TChannelDoCreate
	{
public:
	TVersion iVer;
	const TDesC *iName;
	const TDesC *iPhysicalDevice;
	const TDesC8 *iInfo;
	};

class TCreateSession
	{
public:
	TVersion iVer;
	TInt iMessageSlots;
	};

enum TObjectType
	{
	EThread=0,
	EProcess,
	EChunk,
	ELibrary,
	ESemaphore,
	EMutex,
	ETimer,
	EServer,
	ESession,
	ELogicalDevice,
	EPhysicalDevice,
	ELogicalChannel,
	EChangeNotifier,
	EUndertaker,
	EMsgQueue,
	EPropertyRef,
	ECondVar,
	EShPool,
	EShBuf,
	ENumObjectTypes,	// number of DObject-derived types
	EObjectTypeAny=-1,

	EIpcMessageD=0x20,	// lookup IPC message handle, allow disconnect
	EIpcMessage=0x21,	// lookup IPC message handle, don't allow disconnect
	EIpcClient=0x22,	// lookup IPC message client, don't allow disconnect
	};

class TObjectOpenInfo
	{
public:
	TObjectType iObjType;
	TBool isReadOnly;
	};

class TChannelCreateInfo
	{
public:
	TVersion iVersion;
	TInt iUnit;
	const TDesC* iPhysicalDevice;
	const TDesC8* iInfo;
	};

#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
class TChannelCreateInfo8
	{
public:
	TVersion iVersion;
	TInt iUnit;
	const TDesC8* iPhysicalDevice;
	const TDesC8* iInfo;
	};
#else
typedef TChannelCreateInfo TChannelCreateInfo8;
#endif

const TInt KMaxThreadCreateInfo = 256;
struct SThreadCreateInfo
	{
	TAny* iHandle;
	TInt iType;
	TThreadFunction iFunction;
	TAny* iPtr;
	TAny* iSupervisorStack;
	TInt iSupervisorStackSize;
	TAny* iUserStack;
	TInt iUserStackSize;
	TInt iInitialThreadPriority;
	TPtrC iName;
	TInt iTotalSize;	// Size including any extras (must be a multiple of 8 bytes)
	};

enum TThreadCreationFlags
	{
	ETraceHeapAllocs 				= 0x00000001,
	EMonitorHeapMemory				= 0x00000002,

	EThreadCreateFlagPaged			= 0x00000004,
	EThreadCreateFlagUnpaged		= 0x00000008,
	EThreadCreateFlagPagingUnspec	= 0x00000000,
	EThreadCreateFlagPagingMask	= EThreadCreateFlagPaged | EThreadCreateFlagUnpaged,

	EThreadCreateFlagMask = ETraceHeapAllocs | EMonitorHeapMemory | EThreadCreateFlagPagingMask,
	};

struct SStdEpocThreadCreateInfo : public SThreadCreateInfo
	{
	SStdEpocThreadCreateInfo()
		: iFlags(0)	// Must be clear on creation.
		{
		};
	RAllocator* iAllocator;
	TInt iHeapInitialSize;
	TInt iHeapMaxSize;
	TUint iFlags;
	};

#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
struct SThreadCreateInfo8
	{
	TAny* iHandle;
	TInt iType;
	TThreadFunction iFunction;
	TAny* iPtr;
	TAny* iSupervisorStack;
	TInt iSupervisorStackSize;
	TAny* iUserStack;
	TInt iUserStackSize;
	TInt iInitialThreadPriority;
	TPtrC8 iName;
	TInt iTotalSize;	// size including any extras
	};

struct SStdEpocThreadCreateInfo8 : public SThreadCreateInfo8
	{
	SStdEpocThreadCreateInfo8()
		: iFlags(0) // Must be clear on creation.
		{
		};
	RAllocator* iAllocator;
	TInt iHeapInitialSize;
	TInt iHeapMaxSize;
	TUint iFlags;
	};
#else
typedef SThreadCreateInfo SThreadCreateInfo8;
typedef SStdEpocThreadCreateInfo SStdEpocThreadCreateInfo8;
#endif

struct SIpcCopyInfo
	{
	TUint8* iLocalPtr;
	TInt iLocalLen;
	TInt iFlags;
	};

enum TChunkAdjust
	{
	EChunkAdjust=0,
	EChunkAdjustDoubleEnded=1,
	EChunkCommit=2,
	EChunkDecommit=3,
	EChunkAllocate=4,
	EChunkUnlock=5,
	EChunkLock=6
	};

enum TMemModelAttributes
	{
	EMemModelTypeMask=0xf,					// bottom 4 bits give type of memory model
	EMemModelTypeDirect=0,					// direct memory model on hardware
	EMemModelTypeMoving=1,					// moving memory model on hardware
	EMemModelTypeMultiple=2,				// multiple memory model on hardware
	EMemModelTypeEmul=3,					// emulation using single host process
	EMemModelTypeFlexible=4,				// flexible memory model on hardware

	EMemModelAttrRomPaging=0x10,			// Demand paging of XIP ROM
	EMemModelAttrCodePaging=0x20,			// Demand paging of RAM loaded code
	EMemModelAttrDataPaging=0x40,			// Demand paging of all RAM
	EMemModelAttrPagingMask=0xf0,			// Mask for demand paging attributes

	EMemModelAttrNonExProt=(TInt)0x80000000,// accesses to nonexistent addresses are trapped
	EMemModelAttrKernProt=0x40000000,		// accesses to kernel memory from user mode are trapped
	EMemModelAttrWriteProt=0x20000000,		// addresses can be marked as read-only; writes to these are trapped
	EMemModelAttrVA=0x10000000,				// system supports virtual addresses
	EMemModelAttrProcessProt=0x08000000,	// accesses to other processes' memory are trapped
	EMemModelAttrSameVA=0x04000000,			// different processes map the same virtual address to different physical addresses
	EMemModelAttrSupportFixed=0x02000000,	// 'fixed' processes are supported
	EMemModelAttrSvKernProt=0x01000000,		// unexpected accesses to kernel memory within an executive call are trapped
	EMemModelAttrIPCKernProt=0x00800000,	// accesses to kernel memory via IPC are trapped
	EMemModelAttrIPCFullProt=0x00400000,	// accesses via IPC have same protection as user mode
	EMemModelAttrRamCodeProt=0x00200000,	// RAM-loaded code is only visible to processes which have loaded it
	};

/** @test */
enum TKernelHeapDebugFunction {EDbgMarkStart,EDbgMarkCheck,EDbgMarkEnd,EDbgSetAllocFail,EDbgSetBurstAllocFail,EDbgCheckFailure,EDbgGetAllocFail};

/** @test */
class TKernelHeapMarkCheckInfo
	{
public:
	TBool iCountAll;
	const TDesC8* iFileName;
	TInt iLineNum;
	};
//
class TTrapHandler;
class CActiveScheduler;
class TLocale;

//
//
//
// Handler below is used by test prints to trucate rather than panic the caller.
//
#if defined(_UNICODE) && !defined(__KERNEL_MODE__)
NONSHARABLE_CLASS(TestOverflowTruncate) : public TDes16Overflow
	{
public:
	virtual void Overflow(TDes16 &aDes);
	};
#else
NONSHARABLE_CLASS(TestOverflowTruncate) : public TDes8Overflow
	{
public:
	virtual void Overflow(TDes8 &aDes);
	};
#endif
//

/********************************************
 * Thread local storage entry
 ********************************************/
struct STls
	{
	TInt	iHandle;
	TInt	iDllUid;
	TAny*	iPtr;
	};

const TInt KDllUid_Default = 0;		// for ROM DLLs and direct calls to UserSvr::DllTls
const TInt KDllUid_Special = -1;	// used on emulator to instruct the kernel to get the DLL UID from the module handle

/********************************************
 * Entry point call values
 ********************************************/
const TInt	KModuleEntryReasonProcessInit		=0;		// Process start
const TInt	KModuleEntryReasonThreadInit		=1;		// Start new thread
const TInt	KModuleEntryReasonProcessAttach		=2;		// Process attach (init static data)
const TInt	KModuleEntryReasonProcessDetach		=3;		// Process detach (destroy static data)
const TInt	KModuleEntryReasonException			=4;		// Handle exception
const TInt	KModuleEntryReasonVariantInit0		=-3;	// Call variant static constructors

/** @publishedPartner
	@released
*/
const TInt	KModuleEntryReasonExtensionInit0	=-2;	// Extension early initialisation check

/** @publishedPartner
	@released
*/
const TInt	KModuleEntryReasonExtensionInit1	=-1;	// Extension initialisation

/**
	Flags returned by Exec::KernelConfigFlags()
*/
enum TKernelConfigFlags
	{
	EKernelConfigIpcV1Available = 1<<0,
	EKernelConfigPlatSecEnforcement = 1<<1,
	EKernelConfigPlatSecDiagnostics = 1<<2,
	EKernelConfigPlatSecProcessIsolation = 1<<3,
	EKernelConfigPlatSecEnforceSysBin = 1<<4,

	// paging policy values use by 2-bit code and data paging policy enums...
	EKernelConfigPagingPolicyNoPaging = 0,
	EKernelConfigPagingPolicyAlwaysPage = 1,
	EKernelConfigPagingPolicyDefaultUnpaged = 2,
	EKernelConfigPagingPolicyDefaultPaged = 3,

	EKernelConfigCodePagingPolicyShift			= 5,
	EKernelConfigCodePagingPolicyMask			= 3<<5,
	EKernelConfigCodePagingPolicyNoPaging		= EKernelConfigPagingPolicyNoPaging<<5,
	EKernelConfigCodePagingPolicyAlwaysPage		= EKernelConfigPagingPolicyAlwaysPage<<5,
	EKernelConfigCodePagingPolicyDefaultUnpaged	= EKernelConfigPagingPolicyDefaultUnpaged<<5,
	EKernelConfigCodePagingPolicyDefaultPaged	= EKernelConfigPagingPolicyDefaultPaged<<5,

	EKernelConfigPlatSecLocked = 1<<7,					// Primarily used by __PLATSEC_UNLOCKED__ (q.v.) test code

	EKernelConfigCrazyScheduling = 1<<8,				// Enables thread priority/timeslice craziness

	EKernelConfigDataPagingPolicyShift			= 9,
	EKernelConfigDataPagingPolicyMask			= 3<<9,
	EKernelConfigDataPagingPolicyNoPaging		= EKernelConfigPagingPolicyNoPaging<<9,
	EKernelConfigDataPagingPolicyAlwaysPage		= EKernelConfigPagingPolicyAlwaysPage<<9,
	EKernelConfigDataPagingPolicyDefaultUnpaged	= EKernelConfigPagingPolicyDefaultUnpaged<<9,
	EKernelConfigDataPagingPolicyDefaultPaged	= EKernelConfigPagingPolicyDefaultPaged<<9,

	EKernelConfigSMPUnsafeCompat = 1<<12,				// Enables compatibility mode for SMP-unsafe processes
	EKernelConfigSMPUnsafeCPU0   = 1<<13,				// Slow compatibility mode: all SMP-unsafe processes run on CPU 0 only
	EKernelConfigSMPCrazyInterrupts = 1<<14,			// Enables CPU target rotation for HW Interrupts.

	EKernelConfigSMPLockKernelThreadsCore0 = 1<< 15,    // locks all kernel side threads to CPU 0

	EKernelConfigDisableAPs = 1u<<30,

	EKernelConfigTest = 1u<<31,							// Only used by test code for __PLATSEC_UNLOCKED__
	};

/**
	If __PLATSEC_UNLOCKED__ is not defined, these flags must always
	be considered to be set.  See KernelConfigFlags() in kern_priv.h.

	@see KernelConfigFlags()

	@internalTechnology
*/
#ifdef __PLATSEC_UNLOCKED__
#define __PLATSEC_FORCED_FLAGS__	 0
#else
#define __PLATSEC_FORCED_FLAGS__	(EKernelConfigPlatSecEnforcement|EKernelConfigPlatSecProcessIsolation|EKernelConfigPlatSecEnforceSysBin)
#endif

/**
@internalTechnology
*/
enum TGlobalUserData
	{
	ELocaleDefaultCharSet,
	ELocalePreferredCharSet,
	EMaxGlobalUserData
	};

typedef void (*TGlobalDestructorFunc)(void);

// This must not conflict with any possible valid TLS keys
const TInt KGlobalDestructorTlsKey = -1;
const TInt KNestedEntryPointCallKey = -2;

GLREF_C void ExitCurrentThread(TExitType, TInt, const TDesC8*);

#ifndef __REMOVE_PLATSEC_DIAGNOSTICS__
/**
@internalTechnology
*/
class TPlatSecDiagnostic
	{
public:
	enum TType
		{
		ELoaderCapabilityViolation1,
		ELoaderCapabilityViolation2,
		EThreadCapabilityCheckFail,
		EProcessCapabilityCheckFail,
		EKernelSecureIdCheckFail,
		EKernelObjectPolicyCheckFail,
		EHandleCapabilityCheckFail,
		ECreatorCapabilityCheckFail,
		EMessageCapabilityCheckFail,
		EKernelProcessIsolationFail,
		EKernelProcessIsolationIPCFail,
		ECreatorPolicyCheckFail,
		};
public:
	inline TPlatSecDiagnostic();
	inline TPlatSecDiagnostic(TType aType);
	inline TPlatSecDiagnostic(TType aType, TInt aInt1, TInt aInt2, const SCapabilitySet& aCaps);
	inline TPlatSecDiagnostic(TType aType, TInt aInt1, const SSecurityInfo& aCaps);
	inline TPlatSecDiagnostic(TType aType, TInt aInt, const TDesC8& aString, const SCapabilitySet& aCaps);
	inline TPlatSecDiagnostic(TType aType, const TDesC8& aString1, const TDesC8& aString2, const SCapabilitySet& aCaps);
	inline TPlatSecDiagnostic(TType aType, TInt aInt1, TInt aInt2);
	inline TPlatSecDiagnostic(TType aType, TInt aInt1);
	inline const TDesC8* String1();
	inline const TDesC8* String2();
public:
	TType iType;
	TInt iArg1;
	TInt iArg2;
	const char* iContextText;
	TInt iContextTextLength;
	SSecurityInfo iSecurityInfo;
	};

inline TPlatSecDiagnostic::TPlatSecDiagnostic()
	{}

inline TPlatSecDiagnostic::TPlatSecDiagnostic(TType aType)
	: iType(aType)
	{}

inline TPlatSecDiagnostic::TPlatSecDiagnostic(TType aType,TInt aInt1)
	: iType(aType), iArg1(aInt1)
	{}

inline TPlatSecDiagnostic::TPlatSecDiagnostic(TType aType, TInt aInt1, TInt aInt2, const SCapabilitySet& aCaps)
	: iType(aType), iArg1(aInt1), iArg2(aInt2), iContextText(0)
	{
	iSecurityInfo.iSecureId = 0;
	iSecurityInfo.iVendorId = 0;
	iSecurityInfo.iCaps = aCaps;
	};
inline TPlatSecDiagnostic::TPlatSecDiagnostic(TType aType, TInt aInt1, const SSecurityInfo& aInfo)
	: iType(aType), iArg1(aInt1), iArg2(ECapability_None), iContextText(0), iSecurityInfo(aInfo)
	{
	};

inline TPlatSecDiagnostic::TPlatSecDiagnostic(TType aType, TInt aInt, const TDesC8& aString, const SCapabilitySet& aCaps)
	: iType(aType), iArg1(aInt), iArg2((TInt)&aString), iContextText(0)
	{
	iSecurityInfo.iSecureId = 0;
	iSecurityInfo.iVendorId = 0;
	iSecurityInfo.iCaps = aCaps;
	};

inline TPlatSecDiagnostic::TPlatSecDiagnostic(TType aType, const TDesC8& aString1, const TDesC8& aString2, const SCapabilitySet& aCaps)
	: iType(aType), iArg1((TInt)&aString1), iArg2((TInt)&aString2), iContextText(0)
	{
	iSecurityInfo.iSecureId = 0;
	iSecurityInfo.iVendorId = 0;
	iSecurityInfo.iCaps = aCaps;
	};

inline TPlatSecDiagnostic::TPlatSecDiagnostic(TType aType, TInt aInt1, TInt aInt2)
	: iType(aType), iArg1(aInt1), iArg2(aInt2)
	{
	iSecurityInfo.iSecureId = 0;
	iSecurityInfo.iVendorId = 0;
	iSecurityInfo.iCaps[0] = 0;
	iSecurityInfo.iCaps[1] = 0;
	};

inline const TDesC8* TPlatSecDiagnostic::String1()
	{ return (const TDesC8*)iArg1; }

inline const TDesC8* TPlatSecDiagnostic::String2()
	{ return (const TDesC8*)iArg2; }

inline TInt PlatSec::LoaderCapabilityViolation(const TDesC8& aImporterName, const TDesC8& aFileName, const SCapabilitySet& aMissingCaps)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::ELoaderCapabilityViolation2,aImporterName,aFileName,aMissingCaps);
	return EmitDiagnostic(d, NULL);
	}

#ifdef __KERNEL_MODE__

inline TInt PlatSec::CapabilityCheckFail(const DProcess* aViolatingProcess, TCapability aCapability, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EProcessCapabilityCheckFail,(TInt)aViolatingProcess,(TInt)aCapability);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::CapabilityCheckFail(const DThread* aViolatingThread, TCapability aCapability, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EThreadCapabilityCheckFail,(TInt)aViolatingThread,(TInt)aCapability);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::SecureIdCheckFail(const DProcess* aViolatingProcess, TSecureId aSid, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EKernelSecureIdCheckFail,(TInt)aViolatingProcess,(TInt)aSid);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::PolicyCheckFail(const DProcess* aProcess, const SSecurityInfo& aMissingSecurityInfo, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EKernelObjectPolicyCheckFail,(TInt)aProcess,(const SSecurityInfo&)aMissingSecurityInfo);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::PolicyCheckFail(const DThread* aThread, const SSecurityInfo& aMissingSecurityInfo, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EKernelObjectPolicyCheckFail,(TInt)aThread,(const SSecurityInfo&)aMissingSecurityInfo);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::ProcessIsolationFail(const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EKernelProcessIsolationFail);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::ProcessIsolationIPCFail(RMessageK* aMessage, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EKernelProcessIsolationIPCFail,(TInt)aMessage);
	return EmitDiagnostic(d,aContextText);
	}

#else // !__KERNEL_MODE__

inline TInt PlatSec::LoaderCapabilityViolation(RProcess aLoadingProcess, const TDesC8& aFileName, const SCapabilitySet& aMissingCaps)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::ELoaderCapabilityViolation1,aLoadingProcess.Handle(),aFileName,aMissingCaps);
	return EmitDiagnostic(d, NULL);
	}

inline TInt PlatSec::CreatorCapabilityCheckFail(TCapability aCapability, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::ECreatorCapabilityCheckFail,(TInt)0,aCapability);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::CreatorCapabilityCheckFail(const TCapabilitySet& aMissingCaps, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::ECreatorCapabilityCheckFail,(TInt)0,ECapability_None,(const SCapabilitySet&)aMissingCaps);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::CapabilityCheckFail(TInt aHandle, TCapability aCapability, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EHandleCapabilityCheckFail,aHandle,aCapability);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::CapabilityCheckFail(TInt aHandle, const TCapabilitySet& aMissingCaps, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EHandleCapabilityCheckFail,aHandle,ECapability_None,(const SCapabilitySet&)aMissingCaps);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::PolicyCheckFail(TInt aHandle, const SSecurityInfo& aMissingSecurityInfo, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EHandleCapabilityCheckFail,aHandle,(const SSecurityInfo&)aMissingSecurityInfo);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::CapabilityCheckFail(RMessagePtr2 aMessage, TCapability aCapability, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EMessageCapabilityCheckFail,(TInt)aMessage.Handle(),aCapability);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::CapabilityCheckFail(RMessagePtr2 aMessage, const TCapabilitySet& aMissingCaps, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EMessageCapabilityCheckFail,(TInt)aMessage.Handle(),ECapability_None,(const SCapabilitySet&)aMissingCaps);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::PolicyCheckFail(RMessagePtr2 aMessage, const SSecurityInfo& aMissing, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::EMessageCapabilityCheckFail,(TInt)aMessage.Handle(),(const SSecurityInfo&)aMissing);
	return EmitDiagnostic(d,aContextText);
	}

inline TInt PlatSec::CreatorPolicyCheckFail(const SSecurityInfo& aMissing, const char* aContextText)
	{
	TPlatSecDiagnostic d(TPlatSecDiagnostic::ECreatorPolicyCheckFail,(TInt)0,(const SSecurityInfo&)aMissing);
	return EmitDiagnostic(d,aContextText);
	}

#endif //__KERNEL_MODE__
#endif // !__REMOVE_PLATSEC_DIAGNOSTICS__

const TInt KTlsArrayGranularity=2;

#ifdef __CPU_HAS_CP15_THREAD_ID_REG

#define __USERSIDE_THREAD_DATA__

class TLocalThreadData
	{
public:
	void Close();
#ifndef __KERNEL_MODE__
	TAny* DllTls(TInt aHandle, TInt aDllUid);
	TInt DllSetTls(TInt aHandle, TInt aDllUid, TAny* aPtr);
	void DllFreeTls(TInt aHandle);
#endif
public:
	RAllocator* iHeap;				///< The thread's current heap
	CActiveScheduler* iScheduler;	///< The thread's current active scheduler
	TTrapHandler* iTrapHandler;		///< The thread's current trap handler
	TUint iThreadId;                ///< The thread's id
private:
	RAllocator* iTlsHeap; 			///< The heap that the DLL TLS data is stored on
	RArray<STls> iTls; 				///< DLL TLS data
	};

const TInt KLocalThreadDataSize = _ALIGN_UP(sizeof(TLocalThreadData), 8);

#endif

#ifdef __WINS__

enum TWin32RuntimeReason
	{
	// Same values as passed to DllMain
	EWin32RuntimeProcessAttach = 1,
	EWin32RuntimeThreadAttach = 2,
	EWin32RuntimeThreadDetach = 3,
	EWin32RuntimeProcessDetach = 4,
	};

typedef TBool (*TWin32RuntimeHook)(TWin32RuntimeReason);

#endif

struct SAtomicOpInfo64
	{
	TAny*		iA;
	TAny*		iQ;
	TUint64		i1;
	TUint64		i2;
	TUint64		i3;
	};

struct SAtomicOpInfo32
	{
	TAny*		iA;
	union
		{
		TAny*	iQ;
		TUint32	i0;
		};
	TUint32		i1;
	TUint32		i2;
	};

#endif //__U32STD_H__
