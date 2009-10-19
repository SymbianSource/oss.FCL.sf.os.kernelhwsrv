// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfsrv\cl_blockmap.cpp
// 
//

#include "cl_std.h"

EXPORT_C TBlockMapEntry::TBlockMapEntry()
	{
	iNumberOfBlocks = 0;
	iStartBlock = 0;
	}

EXPORT_C void TBlockMapEntry::SetNumberOfBlocks( TUint aNumberOfBlocks )
	{
	iNumberOfBlocks = aNumberOfBlocks;	
	}

EXPORT_C void TBlockMapEntry::SetStartBlock( TUint aStartBlock )
	{
	iStartBlock = aStartBlock;
	}

