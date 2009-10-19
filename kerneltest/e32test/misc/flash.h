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
// e32test\misc\flash.h
// 
//

#include <e32base.h>

typedef CBase Base;
class Flash : public Base
	{
public:
	static Flash* New(TUint32 anAddr);
	virtual TInt Read(TUint32 anAddr, TUint32 aSize, TUint8* aDest)=0;
	virtual TInt BlankCheck(TUint32 anAddr, TUint32 aSize)=0;
	virtual TInt Erase(TUint32 anAddr, TUint32 aSize)=0;
	virtual TInt Write(TUint32 anAddr, TUint32 aSize, const TUint8* aSrc)=0;
	};

