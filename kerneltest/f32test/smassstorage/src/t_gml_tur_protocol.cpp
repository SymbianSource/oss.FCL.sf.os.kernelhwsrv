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
//

#include "t_gml_tur_protocol.h"

/**
 Stub SCSI protocol class for testing the bulk only transport classes.
*/

CScsiProtocol* CScsiProtocol::NewL()
	{
	return new (ELeave) CScsiProtocol();
	}
	
void CScsiProtocol::RegisterTransport(MTransportBase* /*aTransport*/)
	{
	}
	
TBool CScsiProtocol::DecodePacket(TPtrC8& /*aData*/, TUint /*aLun*/)
	{
	return EFalse;
	}
	
TInt CScsiProtocol::ReadComplete(TInt /*aError*/)
	{
	return KErrNone;
	}
	
TInt CScsiProtocol::Cancel()
	{
	return KErrNone;
	}
	
CScsiProtocol::~CScsiProtocol()
	{
	}
