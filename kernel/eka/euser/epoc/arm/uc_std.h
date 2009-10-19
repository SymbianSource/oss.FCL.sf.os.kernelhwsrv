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
// e32\euser\epoc\arm\uc_std.h
// 
//

#if !defined(__UCSTD_H__)
#define __UCSTD_H__
#include <e32def.h>

#ifdef __GCC32__

#define	_CBASE_VPTR_OFFSET_				0

#define	_VTBL_INIT_SPARE_				8
#define _CBASE_DESTRUCT_OFFSET_			(_VTBL_INIT_SPARE_)
#define	_CBASE_VTBL_SIZE_				(_VTBL_INIT_SPARE_+8)
#define	_CACTIVESCHEDULER_WAIT_OFFSET_	(_CBASE_VTBL_SIZE_)
#define _CACTIVESCHEDULER_ERROR_OFFSET_	(_CBASE_VTBL_SIZE_+4)
#define _CACTIVESCHEDULER_VTBL_SIZE_	(_CBASE_VTBL_SIZE_+24)
#define	_CACTIVE_DOCANCEL_OFFSET_		(_CBASE_VTBL_SIZE_)
#define	_CACTIVE_RUNL_OFFSET_			(_CBASE_VTBL_SIZE_+4)
#define	_CACTIVE_RUNERROR_OFFSET_		(_CBASE_VTBL_SIZE_+8)
#define _CACTIVE_VTBL_SIZE_				(_CBASE_VTBL_SIZE_+12)
#define _CSESSION_CREATEL_OFFSET_		(_CBASE_VTBL_SIZE_)
#define _CSESSION_COUNT_OFFSET_			(_CBASE_VTBL_SIZE_+4)
#define	_CSESSION_SERVICEL_OFFSET_		(_CBASE_VTBL_SIZE_+8)
#define	_CSESSION2_SERVICEL_OFFSET_		(_CBASE_VTBL_SIZE_+8)
#define _TTRAPHANDLER_VPTR_OFFSET_		0
#define _TTRAPHANDLER_TRAP_OFFSET_		8
#define _TTRAPHANDLER_LEAVE_OFFSET_		16

#elif defined(__EABI__)

#define	_CBASE_VPTR_OFFSET_				0
#define _CBASE_DESTRUCT_OFFSET_			        4

#define	_CBASE_VTBL_SIZE_				12
#define	_CACTIVESCHEDULER_WAIT_OFFSET_	(_CBASE_VTBL_SIZE_) 
#define _CACTIVESCHEDULER_ERROR_OFFSET_	(_CBASE_VTBL_SIZE_+4) // *

#define	_CACTIVE_DOCANCEL_OFFSET_		(_CBASE_VTBL_SIZE_) // *
#define	_CACTIVE_RUNL_OFFSET_			(_CBASE_VTBL_SIZE_+4) // *
#define	_CACTIVE_RUNERROR_OFFSET_		(_CBASE_VTBL_SIZE_+8) // *
#define _CACTIVE_VTBL_SIZE_				(_CBASE_VTBL_SIZE_+12)
#define _CSESSION_CREATEL_OFFSET_		(_CBASE_VTBL_SIZE_)
#define _CSESSION_COUNT_OFFSET_			(_CBASE_VTBL_SIZE_+4)
#define	_CSESSION_SERVICEL_OFFSET_		(_CBASE_VTBL_SIZE_+8) // *
#define	_CSESSION2_SERVICEL_OFFSET_		(_CBASE_VTBL_SIZE_+8) // *


#define _TTRAPHANDLER_VPTR_OFFSET_		0
#define _TTRAPHANDLER_TRAP_OFFSET_		0
#define _TTRAPHANDLER_LEAVE_OFFSET_		8
#endif

#endif
