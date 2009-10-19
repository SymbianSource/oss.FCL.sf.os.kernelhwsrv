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
// e32\include\drivers\pccd_vcc.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __PCCD_VCC_H__
#define __PCCD_VCC_H__
#include <pccd_ifc.h>

NONSHARABLE_CLASS(DPlatPcCardVcc) : public DPcCardVcc
	{
public:
	DPlatPcCardVcc(TInt aPsuNum, TInt aMediaChangeNum);
	virtual ~DPlatPcCardVcc();
	virtual void DoSetState(TPBusPsuState aState);
	virtual void DoCheckVoltage();
public:
    virtual void PsuInfo(TPBusPsuInfo &anInfo);
	};

#endif
