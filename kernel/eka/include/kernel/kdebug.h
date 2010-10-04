// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\kernel\kdebug.h
// Public header for kernel debugger interface
// 
//

#ifndef __KDEBUG_H__
#define __KDEBUG_H__

#include <kernel/kern_priv.h>
#include <kernel/debug.h>
#include <e32ver.h>
#include <sm_debug_api.h>


/**
Defines the major version number of the stop-mode debug API

@publishedPartner
@released
*/
const TInt KDebuggerMajorVersionNumber	= 1;
/**
Defines the minor version number of the stop-mode debug API

@publishedPartner
@released
*/
#ifdef __MEMMODEL_MULTIPLE__
const TInt KDebuggerMinorVersionNumber	= 7;
#else
const TInt KDebuggerMinorVersionNumber	= 6;
#endif
/**
Defines the build number of the stop-mode debug API

@publishedPartner
@released
*/
const TInt KDebuggerBuildVersionNumber	= KE32BuildVersionNumber;

//
// Stop mode debugger change flags
//
/**
Flag to indicate that the process container has been modified by the kernel

@publishedPartner
@released
*/
const TUint32 KDebuggerChangeProcess	= 0x01;
/**
Flag to indicate that the thread container has been modified by the kernel

@publishedPartner
@released
*/
const TUint32 KDebuggerChangeThread		= 0x02;
/**
Flag to indicate that the library container has been modified by the kernel

@publishedPartner
@released
*/
const TUint32 KDebuggerChangeLibrary	= 0x04;
/**
Flag to indicate that the chunk container has been modified by the kernel

@publishedPartner
@released
*/
const TUint32 KDebuggerChangeChunk		= 0x08;
/**
Flag to indicate that the code segment list has been modified by the kernel

@publishedPartner
@released
*/
const TUint32 KDebuggerChangeCode		= 0x10;
/**
Flag to indicate that a PDD has been loaded or unloaded by the kernel

@publishedPartner
@released
*/
const TUint32 KDebuggerChangePdd		= 0x20;
/**
Flag to indicate that an LDD has been loaded or unloaded by the kernel

@publishedPartner
@released
*/
const TUint32 KDebuggerChangeLdd		= 0x40;

/**
Constant used to indicate that the given position in the debug offset
table is invalid

@publishedPartner
@released
*/
const TInt KDebuggerOffsetInvalid		= -1;

const TUint32 KFilterBufferSize = 16384;
const TUint32 KFilterBufferSignature = 0x4642444B;

/**
This structure will preceed any filter items in the filter buffer.

@publishedPartner
@prototype
*/
struct TFilterHeader
	{
	/**
	Must be set to KFilterBufferSignature for the filter buffer to be valid.
	If not, all events are reported and the filter is off.
	*/
	TUint32 iSignature;

	enum TFilterFlags
		{
		/** Set if we should notify about global events with no associated name */
		EGlobalEvents = 1 << 0
		};

	/**
	A bitwise OR of values from the TFilterFlags enum.
	@see TFilterFlags
	*/
	TUint32 iFlags;

	/**
	Number of filter objects that follow in the buffer
	@see TFilterObject
	*/
	TUint32 iNumItems;
	};

/**
This structure represents an object that the debugger wishes to be notified
about for certain events. Note these must be word aligned in the filter buffer

@publishedPartner
@prototype
*/
struct TFilterObject
	{
	/**
	The set of events for which this filter object applies
	Each bit in iEventMask maps onto each member of enum TKernelEvent.
	ie the Nth bit of iEvent indicates if we are interested in the
	Nth enum in TKernelEvent
	@see TKernelEvent
	*/
	TUint64 iEventMask;

	/** The length of the name of the filter object */
	TUint32 iLength;

	/** Spares */
	TUint32 iSpares[8];

	/** Padding to keep a 32 bit word at end of struct. Must be kept with iName, below*/
	TUint8 iPad[3];

	/** First character of the filter object name. this extends past the end of the struct */
	TUint8 iName[1];
	};

//
// Forward declarations
//
class DDebuggerInfo;

/**
Kernel debugger interface

@publishedPartner
@released
*/
class Debugger
	{
public:
	IMPORT_C static TVersion Version();
	IMPORT_C static TInt Install(DDebuggerInfo* aDebugger);
	IMPORT_C static DDebuggerInfo* DebuggerInfo();
public:
	static const TInt ObjectOffsetTable[];
	static const TInt VariantObjectOffsetTable[];
	};

/**
Stop-mode debugger interface

@publishedPartner
@released
*/
class DDebuggerInfo
	{
public:
	// construction
	DDebuggerInfo();

	// Functions to control access to the filter buffer
	inline void LockFilterBuffer();
	void ReleaseFilterBuffer();

public:
	// Points to offset table
	const TInt* const						iObjectOffsetTable;

	// Defines the size of the offset table
	TInt									iObjectOffsetTableCount;
	const TArmContextElement* const * const	iThreadContextTable;

	TVersion								iVersion;
	TVersion								iOSVersion;
	Debug::DStopModeExtension*				iStopModeExtension;

	// kernel objects
	volatile TUint32						iChange;
	DObjectCon* const *						iContainers;
	DMutex* 								iCodeSegLock;
	SDblQue*								iCodeSegGlobalList;
	TScheduler*								iScheduler;

	// shadow pages
	TPhysAddr*								iShadowPages;
	TInt									iShadowPageCount;

	// current thread
	NThread*								iCurrentThread;

	// mask for breakpoint-able event handler
	volatile TUint32						iEventMask[(EEventLimit + 31) >> 5];

	// breakpoint-able (RAM) address in the event handler
	TLinAddr								iEventHandlerBreakpoint;

	/** Identifies memory model deployed on the target and the content of Debugger::VariantObjectOffsetTable*/
	TMemoryModelType						iMemoryModelType;

	/** Points to variant specific offset table*/
	const TInt* const						iMemModelObjectOffsetTable;

	/** Defines the size of the variant specific offset table*/
	TInt									iMemModelObjectOffsetTableCount;

	/** @prototype Buffer through which the debugger can communicate the filter */
	TUint8*									iFilterBuffer;

	/** @prototype Size of memory allocated to the buffer */
	TUint32									iFilterBufferSize;

	/** @prototype Lock to control access to the filter buffer */
	volatile TBool							iFilterBufferInUse;
	};

#endif //__KDEBUG_H__
