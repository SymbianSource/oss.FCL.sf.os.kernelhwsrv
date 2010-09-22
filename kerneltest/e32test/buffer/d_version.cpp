// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\buffer\d_version.cpp
// LDD for testing kernel side of TVersion
// 
//

#include <kernel/kern_priv.h>
#include "d_version.h"

class DVersionTestFactory : public DLogicalDevice
//
// VersionTest LDD factory
//
	{
public:
	DVersionTestFactory();
	~DVersionTestFactory();
	virtual TInt Install();						//overriding pure virtual
	virtual void GetCaps(TDes8& aDes) const;	//overriding pure virtual
	virtual TInt Create(DLogicalChannelBase*& aChannel);	//overriding pure virtual
	};

class DVersionTest : public DLogicalChannelBase
//
// VersionTest LDD channel
//
	{
public:
	DVersionTest();
	~DVersionTest();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
private:
	TBool QVS(TInt aCurrent,TInt aRequested);
    TVersion* iTV[KNumTVersions];
    TVersion iDefTV;     // tests default constructor
	};

LOCAL_D const TText* Names[]=
    {
    _S("0.00(0)"),
    _S("0.00(0)"),
    _S("0.00(1)"),
    _S("0.00(999)"),
    _S("0.01(0)"),
    _S("0.01(1)"),
    _S("0.01(999)"),
    _S("0.99(0)"),
    _S("0.99(1)"),
    _S("0.99(999)"),
    _S("1.00(0)"),
    _S("1.00(1)"),
    _S("1.00(999)"),
    _S("1.01(0)"),
    _S("1.01(1)"),
    _S("1.01(999)"),
    _S("1.99(0)"),
    _S("1.99(1)"),
    _S("1.99(999)"),
    _S("99.00(0)"),
    _S("99.00(1)"),
    _S("99.00(999)"),
    _S("99.01(0)"),
    _S("99.01(1)"),
    _S("99.01(999)"),
    _S("99.99(0)"),
    _S("99.99(1)"),
    _S("99.99(999)")
    };

DVersionTestFactory::DVersionTestFactory()
//
// Constructor
//
    {
    }

//
// Destructor
//
DVersionTestFactory::~DVersionTestFactory()
	{
	}

TInt DVersionTestFactory::Create(DLogicalChannelBase*& aChannel)
//
// Create 
//
    {
	aChannel=new DVersionTest;
	return aChannel?KErrNone:KErrNoMemory;
    }

TInt DVersionTestFactory::Install()
//
// Install the LDD - overriding pure virtual
//
    {
	return SetName(&KVersionTestLddName);
    }

void DVersionTestFactory::GetCaps(TDes8& /*aDes*/) const
//
// Get capabilities - overriding pure virtual
//
    {
    // Not used but required as DLogicalDevice::GetCaps is pure virtual
    }

DECLARE_STANDARD_LDD()
	{
    return new DVersionTestFactory;
    }

DVersionTest::DVersionTest()
//
// Constructor
//
    {
    iTV[0]=&iDefTV;
    TInt i=1;
    TInt major=0;
    FOREVER
        {
        TInt minor=0;
        FOREVER
            {
            TInt build=0;
            FOREVER
                {
                iTV[i++]=new TVersion(major,minor,build);
                if (build==999)
                    {
                    break;
                    }
                build=(build==1? 999: 1);
                }
            if (minor==99)
                {
                break;
                }
            minor=(minor==1? 99: 1);
            }
        if (major==99)
            {
            break;
            }
        major=(major==1? 99: 1);
        }
	}
	
TInt DVersionTest::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& /*aVer*/)
//
// Create channel
//
    {
	return KErrNone;
	}

DVersionTest::~DVersionTest()
//
// Destructor
//
    {
    }

TInt DVersionTest::Request(TInt aFunction, TAny* /*a1*/, TAny* /*a2*/)
	{
	TInt r=KErrNone;
	switch (aFunction)
		{
		case RVersionTest::EVersionTestName:
			{
			//
			// Test the version name
			//
			{
			for (TInt i=0; i<KNumTVersions; i++)
			    {
                TPtrC Name=(TPtrC)Names[i];
			    if (iTV[i]->Name().Compare(Name))
			        {
                    r=KErrNotSupported;
			        return r;
			        }
			    }
			}
			break;
			}
		case RVersionTest::EVersionTestQVS:
			{
			//
			// Check QueryVersionSupported()
			//
			for (TInt i=0; i<KNumTVersions; i++)
			    {
			    for (TInt j=0; j<KNumTVersions; j++)
			        {
			        if (Kern::QueryVersionSupported(*iTV[i],*iTV[j])!=QVS(i,j))
			            {
                        r=KErrNotSupported;
			            return r;
			            }
			        }
			    }
			    
			break;
			}
		default:
			break;
		}
	return r;
	}

TBool DVersionTest::QVS(TInt aCurrent,TInt aRequested)
//
// An independent calculation of what QueryVersionSupported should return
//
    {
    if (aCurrent)
        aCurrent--;
    if (aRequested)
        aRequested--;
    aCurrent/=3;
    aRequested/=3;
    return(aCurrent>=aRequested);
    }
