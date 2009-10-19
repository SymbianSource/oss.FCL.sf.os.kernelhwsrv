// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// LDD for testing operator new kernel side
// 
//

#ifndef __D_NEWLDD_H
#define __D_NEWLDD_H

#include <kernel/kernel.h>
#include <e32cmn.h>

#include "t_new_classes.h"



class DOperatorNewTestFactory : public DLogicalDevice
{
public:
	TInt Install();
	void GetCaps(TDes8& aDes) const;
	TInt Create(DLogicalChannelBase*& aChannel);
};

class DOperatorNewTest : public DLogicalChannelBase
{
public:
	virtual TInt Request(TInt aReqNo, TAny* a1, TAny* a2);
private:
	TInt TestNew();
	TInt TestPlacementNew();
	TInt TestPlacementVectorNew();
	TInt TestVectorNew();
};


#endif //__D_NEWLDD_H

