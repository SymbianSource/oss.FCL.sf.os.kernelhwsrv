// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "debug_logging.h"

// Print breakpoints disabled for stepping
inline void D_RMD_Breakpoints::print_BreakpointsDisabledForStep()
	{
	for (TInt i = 0; i < iBreakPointList.Count(); i++)
		{
		if(iBreakPointList[i].iDisabledForStep)
			{
				LOG_MSG2("Breakpoint disabled for stepping: iBreakPointList[%d]", i);
				LOG_MSG4("iBreakId = %x, iId = %d, iAddress = %x", iBreakPointList[i].iBreakId, iBreakPointList[i].iId, iBreakPointList[i].iAddress );
			}
		}
	}

// Print breakpoint list
inline void D_RMD_Breakpoints::print_BreakpointsList()
	{
	for (TInt i = NUMBER_OF_TEMP_BREAKPOINTS; i < iBreakPointList.Count(); i++)
		{
			LOG_MSG2("Breakpoint list: iBreakPointList[%d]", i);
			LOG_MSG4("iBreakId = %x, iId = %d, iAddress = %x", iBreakPointList[i].iBreakId, iBreakPointList[i].iId, iBreakPointList[i].iAddress );
		}
	}
