// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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



/**
 @file
 @internalTechnology
*/


inline TPos TLbaUtils::Pos(TLba aLba)
    {
    return static_cast<TInt64>(aLba) * KBlockSize;
    }

inline TUint32 TLbaUtils::Length(TLba aBlocks)
    {
    return static_cast<TUint32>(aBlocks) * KBlockSize;
    }

inline TPos RTargetDrive::StartPos() const
    {
    return static_cast<TInt64>(iStartLba) * KBlockSize;
    }


inline TPos RTargetDrive::TargetPos(TInt aPos) const
    {
    return StartPos() + aPos;
    }
