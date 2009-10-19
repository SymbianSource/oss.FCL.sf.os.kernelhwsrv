// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\debug\d_traceredirect.cpp
// Trace redirection LDD
// This device driver captures RDebug::Prints from user side code using the
// debug event handler.
// The text can be retrieved with the RTraceRedirect::NextTrace() function 
// TODO: Modify this LDD to use asynchronous retrieval.
// 
//

#include <kernel/kernel.h>
#include <kernel/kern_priv.h>
#include "d_traceredirect.h"

_LIT(KLddName,"D_TRACEREDIRECT");

const TInt KMajorVersionNumber = 0;
const TInt KMinorVersionNumber = 1;
const TInt KBuildVersionNumber = 1;

const TInt KMaxTraceEventLength=100;
const TInt KTraceBufferSize=256;
const TInt KTraceBufferSizeMask=0xff;


class TTraceEvent
	{
public:
	TInt iLength;
	TUint16 iText[KMaxTraceEventLength];
	};

class DTraceRedirect;

class DTraceEventHandler : public DKernelEventHandler
	{
public:
	DTraceEventHandler()
		: DKernelEventHandler(HandleEvent, this) {}
	TInt Create(DLogicalDevice* aDevice);
	~DTraceEventHandler();
	TInt GetNextEvent(TAny *outdes);
private:
	static TUint HandleEvent(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aThis);
	void HandleUserTrace(TText* aStr, TInt aLen);
public:
	DTraceRedirect *iChannel;
	TTraceEvent *iTraceBuffer;
	TInt iFront;
	TInt iBack;
	DMutex* iLock; // serialise calls to handler
	DLogicalDevice* iDevice;	// open reference to LDD for avoiding lifetime issues
	};

class DTraceRedirectFactory : public DLogicalDevice
	{
public:
	DTraceRedirectFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

class DTraceRedirect : public DLogicalChannelBase
	{
public:
	virtual ~DTraceRedirect();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual TInt Request(TInt aFunction, TAny* a1, TAny* a2);
	TInt NextTrace(TAny *outdes);
public:
	DTraceEventHandler *iEventHandler;
	};



DECLARE_STANDARD_LDD()
	{
	return new DTraceRedirectFactory;
	}

//
// DTraceRedirectFactory
//

DTraceRedirectFactory::DTraceRedirectFactory()
	{
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

TInt DTraceRedirectFactory::Create(DLogicalChannelBase*& aChannel)
/**
 Create a new DSchedhookTest on this logical device
*/
	{
	aChannel=new DTraceRedirect;
	return aChannel ? KErrNone : KErrNoMemory;
	}

TInt DTraceRedirectFactory::Install()
/**
 Install the LDD - overriding pure virtual
*/
	{
	return SetName(&KLddName);
	}

void DTraceRedirectFactory::GetCaps(TDes8& aDes) const
/**
 Get capabilities - overriding pure virtual
*/
	{
	TCapsTestV01 b;
	b.iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    Kern::InfoCopy(aDes,(TUint8*)&b,sizeof(b));
	}


//
// DSchedhookTest
//

TInt DTraceRedirect::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
/**
 Create channel
*/
	{
	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber),aVer))
		return KErrNotSupported;

	iEventHandler=new DTraceEventHandler;
	if (iEventHandler == NULL)
		return KErrNoMemory;
	return iEventHandler->Create(iDevice);
	}

DTraceRedirect::~DTraceRedirect()
	{
	iEventHandler->Close();
	}

TInt DTraceEventHandler::Create(DLogicalDevice* aDevice)
	{
	TInt r;
	r = aDevice->Open();
	if (r != KErrNone)
		return r;
	iDevice = aDevice;
	iFront=0;
	iBack=0;
	iTraceBuffer=new TTraceEvent[KTraceBufferSize];
	if (iTraceBuffer == NULL)
		return KErrNoMemory;
	_LIT(KDataMutexName, "EventHandlerMutex");
	r = Kern::MutexCreate(iLock, KDataMutexName, KMutexOrdDebug);
	if (r != KErrNone)
		return r;
	return Add();
	}

DTraceEventHandler::~DTraceEventHandler()
	{
	if (iLock)
		iLock->Close(NULL);
	delete [] iTraceBuffer;
	if (iDevice)
		iDevice->Close(NULL);
	}

TUint DTraceEventHandler::HandleEvent(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aThis)
	{
	if (aEvent==EEventUserTrace)
		{
		((DTraceEventHandler*)aThis)->HandleUserTrace((TText*)a1, (TInt)a2);
		return ETraceHandled;
		}
	return ERunNext;
	}

// called in CS
void DTraceEventHandler::HandleUserTrace(TText* aStr, TInt aLen)
	{
	Kern::MutexWait(*iLock);
	TInt frontplus1=(iFront+1)&KTraceBufferSizeMask;
	if (frontplus1!=iBack)  // check overflow
		{
		TTraceEvent *e=&iTraceBuffer[iFront];
		XTRAPD(r, XT_DEFAULT, kumemget(e->iText, aStr, aLen*2));
		if (r == KErrNone)
			e->iLength=aLen;
		else
			e->iLength = -1; // an error will be reported in GetNextEvent()
		iFront=frontplus1;
		}
	Kern::MutexSignal(*iLock);
	}

TInt DTraceEventHandler::GetNextEvent(TAny *outdes)
	{

	if (iBack==iFront)
		{ // buffer empty
		return KErrNone;
		}
	TTraceEvent *e=&iTraceBuffer[iBack];
	if (e->iLength == -1)
		return KErrCorrupt; // earlier error when copying trace data from userland
	TPtr ptr((TUint8*)e->iText, e->iLength, e->iLength);
	TInt r=KErrNone;
	Kern::KUDesPut(*(TDes *)outdes, ptr);
	iBack=(iBack+1)&KTraceBufferSizeMask;
	return r;
	}

TInt DTraceRedirect::NextTrace(TAny *outdes)
	{

	TInt r=iEventHandler->GetNextEvent(outdes);
	return r;
	}

TInt DTraceRedirect::Request(TInt aFunction, TAny* a1, TAny* /*a2*/)
/**
 Handle requests from the test program
*/
	{
	TInt r=KErrNone;
	switch (aFunction)
		{
		case RTraceRedirect::ENextTrace:
			r=NextTrace(a1);
			break;
		default:
			r=KErrNotSupported;
			break;
		}
	return r;
	}

