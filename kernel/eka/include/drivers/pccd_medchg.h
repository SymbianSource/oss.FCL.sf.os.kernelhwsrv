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
// e32\include\drivers\pccd_medchg.h
// 
//

/**
 @file
 @publishedPartner
 @released
*/

#ifndef __PCCD_MEDCHG_H__
#define __PCCD_MEDCHG_H__
#include <pccd_ifc.h>

NONSHARABLE_CLASS(DPlatPcCardMediaChange) : public DPcCardMediaChange
	{
public:
	DPlatPcCardMediaChange(TInt aMediaChangeNum);
	virtual TInt Create();
public:
	virtual void ForceMediaChange();
	virtual void DoDoorOpen();
	virtual void DoDoorClosed();
	virtual TMediaState MediaState();
	static void DelayCallBack(TAny* aPtr);
	static void Isr(TAny* aPtr);
public:
	TInt iMedChgIntId;
	NTimer iDelayCallBack;
	};

#endif
