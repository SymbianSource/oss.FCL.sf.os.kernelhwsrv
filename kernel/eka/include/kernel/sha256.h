/**
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL " http://www.eclipse.org/legal/epl-v10.html ".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* kernel/eka/include
*
*/
/**
 @file
 @internalTechnology
 */

#ifndef __SHA256_H__
#define __SHA256_H__

#include <e32cmn.h>
#include <e32des8.h>
#include <e32def.h>


const TUint KSHA256BlockSize = 64;
const TInt KSHA256HashSize = 32;
			
class SHA256
	{	
public:
	SHA256(); // constructor
	void Reset();
	const TDesC8& Final(void);
	void Update(const TUint8* aData,TUint aLength);

private:
	inline void AddLength(const TUint aLength);
	inline void CopyWordToHash(TUint aVal, TInt aIndex);
	void Block();
	void PadMessage();
	
private:
	TBuf8<KSHA256HashSize> iHash;
	TUint iA;
	TUint iB;
	TUint iC;
	TUint iD;
	TUint iE;
	TUint iF;
	TUint iG;
	TUint iH;
	TUint iData[KSHA256BlockSize];
	TUint iNl;
	TUint64 iNh;
	};
	
#endif
