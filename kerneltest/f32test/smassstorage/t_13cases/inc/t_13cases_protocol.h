// Copyright (c) 2004-2010 Nokia Corporation and/or its subsidiary(-ies).
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

Classes for bulk-only transport testing.
*/

#ifndef __T_13CASES_PROTOCOL_H__
#define __T_13CASES_PROTOCOL_H__

#include <e32base.h> 
#include <e32def.h>
#include <e32std.h>
	
#include "protocol.h"

class CScsiProtocol : public CBase, public MProtocolBase
	{
public:
	CScsiProtocol::CScsiProtocol();
	static CScsiProtocol* NewL();
	void RegisterTransport(MTransportBase* aTransport);
	TBool DecodePacket(TPtrC8& aData, TUint aLun);
	TInt ReadComplete(TInt aError);
	TInt Cancel();
	~CScsiProtocol();

private:
	TBool iReadingData;
	MTransportBase* iTransport;
	HBufC8* iReadData;
	HBufC8* iWriteData;
	};
	
#endif // __T_13CASES_PROTOCOL_H__

