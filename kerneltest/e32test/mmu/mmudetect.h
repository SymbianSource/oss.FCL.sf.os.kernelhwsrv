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
// e32test\mmu\mmudetect.h
// 
//

#ifndef __MMUDETECT_H__
#define __MMUDETECT_H__

#include "u32std.h"
#include <e32rom.h>
#include <e32svr.h>

inline TUint32 MemModelAttributes()
	{ return (TUint32)UserSvr::HalFunction(EHalGroupKernel, EKernelHalMemModelInfo, NULL, NULL); }

inline TUint32 MemModelType()
	{ return MemModelAttributes() & EMemModelTypeMask; }

inline TBool HaveMMU()
	{ return MemModelAttributes()&EMemModelAttrVA; }

inline TBool HaveVirtMem()
	{ return MemModelAttributes()&EMemModelAttrVA; }

inline TBool HaveMultAddr()
	{ return (MemModelAttributes()&(EMemModelAttrProcessProt|EMemModelAttrSameVA))
									==(EMemModelAttrProcessProt|EMemModelAttrSameVA); }

inline TBool HaveProcessProt()
	{ return (MemModelAttributes()&EMemModelAttrProcessProt); }

inline TBool HaveDirectKernProt()
	{ return (MemModelAttributes()&EMemModelAttrKernProt); }

inline TBool HaveIPCKernProt()
	{ return (MemModelAttributes()&EMemModelAttrIPCKernProt); }

inline TBool HaveWriteProt()
	{ return (MemModelAttributes()&EMemModelAttrWriteProt); }

#ifdef __EPOC32__
inline TUint8* KernData()
	{
	const TRomHeader& romHdr=*(const TRomHeader*)UserSvr::RomHeaderAddress();
	return (TUint8*)romHdr.iKernDataAddress;
	}
#else
inline TUint8* KernData()
	{ return NULL; }
#endif
#endif
