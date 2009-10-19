// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// protocol.h
// 
//

/**
 @file 
 @internalTechnology
*/

#ifndef __T_GML_TUR_PROTOCOL_H__
#define __T_GML_TUR_PROTOCOL_H__

#include <e32base.h>			// C Class Definitions, Cleanup Stack 
#include <e32def.h>				// T Type  Definitions
#include <e32std.h>    			// ELeave definition
#include "protocol.h"

class CScsiProtocol : public CBase, public MProtocolBase
	{
public:
	static CScsiProtocol* NewL();
	void  RegisterTransport(MTransportBase* aTransport);
	TBool DecodePacket(TPtrC8& aData, TUint aLun);
	TInt  ReadComplete(TInt aError);
	TInt Cancel();
	~CScsiProtocol();
	};
	
#endif // __T_GML_TUR_PROTOCOL_H__
