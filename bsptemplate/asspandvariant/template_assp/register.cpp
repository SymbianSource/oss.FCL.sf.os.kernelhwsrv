// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// naviengine_assp\assp.cpp
// 
//

#include <assp.h>

#ifdef __SMP__
TSpinLock AsspLock(TSpinLock::EOrderGenericIrqLow1);
#endif

///////////////////////////////////////////////////////////////////////////////
//
// MHA - Modular Hardware Adaption
//
// Register Access
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// We need spin locks around read, modify, write operations because another CPU
// may access the same memory in between operations and potentially cause
// memory corruption.
///////////////////////////////////////////////////////////////////////////////
EXPORT_C void AsspRegister::Modify8(TLinAddr aAddr, TUint8 aClearMask, TUint8 aSetMask)
	{
	TUint irq = __SPIN_LOCK_IRQSAVE(AsspLock);
	TUint8  value = *(volatile TUint8  *)aAddr;
	value &= ~aClearMask;
	value |= aSetMask;
	*(volatile TUint8  *)aAddr = value;
	__SPIN_UNLOCK_IRQRESTORE(AsspLock,irq);
	}

EXPORT_C void AsspRegister::Modify16(TLinAddr aAddr, TUint16 aClearMask, TUint16 aSetMask)
	{
	TUint irq = __SPIN_LOCK_IRQSAVE(AsspLock);
	TUint16 value = *(volatile TUint16 *)aAddr;
	value &= ~aClearMask;
	value |= aSetMask;
	*(volatile TUint16 *)aAddr = value;
	__SPIN_UNLOCK_IRQRESTORE(AsspLock,irq);
	}

EXPORT_C void AsspRegister::Modify32(TLinAddr aAddr, TUint32 aClearMask, TUint32 aSetMask)
	{
	TUint irq = __SPIN_LOCK_IRQSAVE(AsspLock);
	TUint32 value = *(volatile TUint32 *)aAddr;
	value &= ~aClearMask;
	value |= aSetMask;
	*(volatile TUint32 *)aAddr = value;
	__SPIN_UNLOCK_IRQRESTORE(AsspLock,irq);
	}

///////////////////////////////////////////////////////////////////////////////
// 64 bit operations may be more complex than 8/16/32 bit operations, depending
// upon hardware support for 64 bit accesses.
//
// For example, one platform required an assembly language function to prevent
// the compliler optimising the accesses into 2 x 32 bit accesses and causing a
// bus error.
//
// Spinlocks are required for non-atomic operations and are therefore
// recommended for 64 bit accesses on current platforms.
///////////////////////////////////////////////////////////////////////////////
extern TUint64 DoRead64(TLinAddr aAddr);

EXPORT_C TUint64 AsspRegister::Read64(TLinAddr aAddr)
	{
	TUint irq = __SPIN_LOCK_IRQSAVE(AsspLock);
	TUint64 value = DoRead64(aAddr);
	__SPIN_UNLOCK_IRQRESTORE(AsspLock,irq);
	return value;
	}

extern void DoWrite64(TLinAddr aAddr, TUint64 aValue);

EXPORT_C void AsspRegister::Write64(TLinAddr aAddr, TUint64 aValue)
	{
	TUint irq = __SPIN_LOCK_IRQSAVE(AsspLock);
	DoWrite64(aAddr, aValue);
	__SPIN_UNLOCK_IRQRESTORE(AsspLock,irq);
	}

EXPORT_C void AsspRegister::Modify64(TLinAddr aAddr, TUint64 aClearMask, TUint64 aSetMask)
	{
	TUint irq = __SPIN_LOCK_IRQSAVE(AsspLock);
	TUint64 value = DoRead64(aAddr);
	value &= ~aClearMask;
	value |= aSetMask;
	DoWrite64(aAddr, value);
	__SPIN_UNLOCK_IRQRESTORE(AsspLock,irq);
	}

