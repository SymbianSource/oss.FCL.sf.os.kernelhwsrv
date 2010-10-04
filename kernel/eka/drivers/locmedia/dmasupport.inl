// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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


inline TBool DDmaHelper::IsPageAligned(TLinAddr aAddr)		{ return !(aAddr & iPageSizeMsk); }
inline TLinAddr DDmaHelper::PageAlign(TLinAddr aAddr)	{ return aAddr & ~iPageSizeMsk; }
inline TLinAddr DDmaHelper::PageOffset(TLinAddr aAddr)		{ return aAddr & iPageSizeMsk; }


inline TInt DDmaHelper::MaxFragLength()	const { return iMaxPages << iPageSizeLog2; }
inline void DDmaHelper::SetFragLength(TInt aLength) { iFragLen = iFragLenRemaining = aLength; }
inline TInt DDmaHelper::FragLength()	const { return iFragLen; }
inline TInt DDmaHelper::LengthRemaining() const { return iReqLenClient - iLenConsumed; }


inline TBool DDmaHelper::IsDmaAligned(TLinAddr aAddr)		{ return !(aAddr & (iDmaAlignment-1)); }

inline TBool DDmaHelper::IsBlockAligned(TInt64 aPos)		{ return !(aPos & iMediaBlockSizeMask); }
inline TInt64 DDmaHelper::BlockAlign(TInt64 aPos)			{ return aPos & ~iMediaBlockSizeMask; }
inline TInt DDmaHelper::BlockOffset(TInt64 aPos)			{ return TInt(aPos & iMediaBlockSizeMask); }

inline TLinAddr DDmaHelper::LinAddress() const { return iLinAddressUser + ((TUint32) iReqRemoteDesOffset) + iLenConsumed; }

