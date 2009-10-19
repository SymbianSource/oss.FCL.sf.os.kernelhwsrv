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
// e32/drivers/iic/iic_priv.h
//

#ifndef __IIC_PRIV_H__
#define __IIC_PRIV_H__

/**
@internalComponent
@prototype 9.6

Stub class for use by DIicBusController in searching iChannelArray

@see DIicBusController
*/
class DIicBusChannelSearcher : public DIicBusChannel
	{
public:
	inline DIicBusChannelSearcher(TChannelType aChanType, TBusType aBusType, TChannelDuplex aChanDuplex)
		: DIicBusChannel(aChanType, aBusType, aChanDuplex)
		{}	
	inline virtual TInt DoCreate() {return KErrNone;};
protected:
	virtual TInt CheckHdr(TDes8* /*aHdr*/) {return KErrNone;};
public:
	void SetChannelNumber(TInt8 aChanNum) {iChannelNumber=aChanNum;};
	};

#endif  // #ifndef __IIC__PRIV_H__

