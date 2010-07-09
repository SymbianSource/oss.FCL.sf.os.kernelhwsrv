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
// e32test/iic/iic_psl/iic_testpsl.h
//

#ifndef IIC_TEST_PSL_H_
#define IIC_TEST_PSL_H_

_LIT(KPddName,"iic.pdd");

NONSHARABLE_CLASS(DIicPdd) : public DPhysicalDevice
	{
// Class to faciliate loading of the IIC classes
public:
	class TCaps
		{
	public:
		TVersion iVersion;
		};
public:
	DIicPdd();
	~DIicPdd();
	virtual TInt Install();
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void GetCaps(TDes8& aDes) const;
	inline static TVersion VersionRequired();
	};

#endif
