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
// f32test\loader\security\t_hash.h
// 
//


#include <e32base.h>

const TUint SHA1_LBLOCK=16;
const TUint SHA1_HASH=20;


class CSHA1:public CBase
	{
public:
		static CSHA1* NewL(void);
		void Update(const TDesC8& aMessage);
		TPtrC8 Final(void);
		~CSHA1(void);
		void Reset(void);
	private:
		CSHA1(void);
		TUint iA;
		TUint iB;
		TUint iC;
		TUint iD;
		TUint iE;
		TBuf8<SHA1_HASH> iHash;
		TUint iNl;
		TUint iNh;
		TUint iData[SHA1_LBLOCK*5];
		void DoUpdate(const TUint8* aData,TUint aLength);
		void DoFinal(void);
		void Block();
		void ConstructL(void);
		TUint8* iTempData;
	};
