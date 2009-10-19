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
// e32\memmodel\epoc\direct\mglobals.cpp
// 
//

#include <memmodel.h>

TLinAddr MM::UserDataSectionBase;
TLinAddr MM::UserRomDataSectionEnd;
TLinAddr MM::UserDataSectionEnd;
TLinAddr MM::RomLinearBase;
TBitMapAllocator* MM::RamAllocator;
TBitMapAllocator* MM::SecondaryAllocator;
DMutex* MM::RamAllocatorMutex;
TInt MM::RamBlockSize;
TInt MM::RamBlockShift;
TInt MM::InitialFreeMemory;
TBool MM::AllocFailed=EFalse;

