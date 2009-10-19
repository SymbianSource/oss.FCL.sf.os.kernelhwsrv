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
// e32test\misc\ymodemu.h
// 
//

#ifndef __YMODEMU_H__
#define __YMODEMU_H__

#include "ymodem.h"
#include <d32comm.h>

class YModemU : public YModem
	{
public:
	virtual ~YModemU();
	static YModemU* NewL(TInt aPort, TBool aG);
private:
	YModemU(TBool aG);
	TInt Create(TInt aPort);
	virtual TInt ReadBlock(TDes8& aDest);
	virtual void WriteC(TUint aChar);
private:
	RBusDevComm iComm;
	RTimer iTimer;
	};

#endif
