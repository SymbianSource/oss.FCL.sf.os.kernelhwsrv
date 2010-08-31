// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// Device driver for kernel side stop mode debugging
//

#ifdef __WINS__
#error - this driver cannot be built for emulation
#endif

#include <e32def.h>
#include <e32def_private.h>
#include <e32cmn.h>
#include <e32cmn_private.h>
#include <e32ldr.h>
#include <u32std.h>
#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include <nk_trace.h>
#include <arm.h>
#include <kernel/cache.h>
#include <platform.h>
#include <nkern.h>
#include <u32hal.h>
#include <kernel/kdebug.h>


#include "debug_logging.h"
#include "d_debug_functionality.h"
#include "debug_utils.h"
#include "d_buffer_manager.h"


using namespace Debug;


DStopModeExtension* TheStopModeExtension = NULL;

/**
  This value is used as an initialiser for the size of the Stop-Mode Debug API's
  default request buffer.
  */
const TInt KRequestBufferSize = 0x200;
/**
  This value is used as an initialiser for the size of the Stop-Mode Debug API's
  default response buffer.
  */
const TInt KResponseBufferSize = 0x1000;

DECLARE_STANDARD_EXTENSION()
	{
	__KTRACE_OPT(KBOOT,Kern::Printf("Starting Stop Mode Debugger V2"));

	// get a reference to the DDebuggerInfo and to the DStopModeExtension
	TSuperPage& superPage = Kern::SuperPage();

	if(!superPage.iDebuggerInfo)
		{
		//kdebug has not been installed so create DDebuggerInfo using our stub constructor
		superPage.iDebuggerInfo = new DDebuggerInfo();
		}

	if(!TheStopModeExtension)
		{
		TheStopModeExtension = new DStopModeExtension();
		}

	// create the request buffer and store a reference to it
	TTag tag;
	tag.iTagId = EBuffersRequest;
	tag.iType = ETagTypePointer;
	tag.iSize = KRequestBufferSize;
	TInt err = TheDBufferManager.CreateBuffer(tag);
	if(KErrNone != err)
		{
		return KErrNone;
		}

	// create the response buffer and store a reference to it
	tag.iTagId = EBuffersResponse;
	tag.iSize = KResponseBufferSize;
	err = TheDBufferManager.CreateBuffer(tag);
	if(KErrNone != err)
		{
		return KErrNone;
		}
	// create the debug functionality buffer and store a reference to it
	TDebugFunctionality df;
	TUint dfSize = df.GetStopModeFunctionalityBufSize();
	tag.iTagId = EBuffersFunctionality;
	tag.iSize = dfSize;
	err = TheDBufferManager.CreateBuffer(tag);
	if(KErrNone != err)
		{
		return KErrNone;
		}

	// fill the functionality buffer with the functionality data and store it in
	// the super page
	TPtr8 dfBlockPtr((TUint8*)tag.iValue, dfSize);
	if(!df.GetStopModeFunctionality(dfBlockPtr))
		{
		return KErrNone;
		}
	TheStopModeExtension->iFunctionalityBlock = (DFunctionalityBlock*)tag.iValue;

	DStopModeExtension::Install(TheStopModeExtension);

	return KErrNone;
	}

/**
 * This stub constructor is intended to be used in the case where the old deprecated
 * stop mode api, kdebug, is not in place. It will initialise all values to NULL except
 * the pointer to the new stop mode api extension. This allows the new stop mode solution
 * to both co-exist and exist independantly of the existing one *
 */
DDebuggerInfo::DDebuggerInfo():
	iObjectOffsetTable(NULL),
	iObjectOffsetTableCount(NULL),
	iThreadContextTable(NULL),
	iStopModeExtension(new DStopModeExtension()),
	iContainers(NULL),
	iCodeSegLock(NULL),
	iCodeSegGlobalList(NULL),
	iScheduler(NULL),
	iShadowPages(NULL),
	iShadowPageCount(0),
	iCurrentThread(NULL),
	iEventMask(),
	iEventHandlerBreakpoint(0),
	iMemModelObjectOffsetTable(NULL),
	iMemModelObjectOffsetTableCount(0)
	{
	}

/**
 * Installs the stop-mode debugger extension
 * Make the stop-mode API visible to a JTAG debugger, by publishing its
 * existence in the superpage
*/
void DStopModeExtension::Install(DStopModeExtension* aExt)
	{
	Kern::SuperPage().iDebuggerInfo->iStopModeExtension = aExt;
	}
