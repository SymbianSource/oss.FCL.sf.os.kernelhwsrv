// Copyright (c) 1999-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32utils\profiler\sampler.cpp
// 
//

#include "platform.h"

#include "sampler.h"
#include <kernel/kern_priv.h>		//temporary
#include <memmodel/epoc/plat_priv.h> // needed for DEpocCodeSeg
_LIT(KLddName,"Sampler");

TKName KiDFCThread = _L("Running from iDFC");
TKName KiDFCProcess = _L("N/A");
TUint  KiDFCId = (TUint)-1; //both process and thread assigned to iDFC will have 'fake' id=-1

const TInt KMajorVersionNumber=2;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=0;

const TInt KMinRate=10;
const TInt KMaxRate=1000;
const TInt KRawBufSize=256;
const TInt KCookedBufSize=0x2000;
const TInt KCodeBufSize=0x2000;

const TInt KMaxCreateCodeSegRecordSize = 300; //Max size of the encoded CodeSegCreate record.
const TInt KMaxErrorReportRecordSize = 18;    //Max size of the encoded ErrorReport record. (3 zeros and 3 integers)
const TInt KRequiredFreeSpace=512;

//Bit mask in report
const TInt KNonXIPModeActive=1; 
const TInt KNoDebugSupport=2;


#define PUT(p,x,e,s)	{*(p)++=(x); if ((p)==(e)) (p)-=(s);}
#define GET_A_BYTE(p,x,e,s)	{*(x)++=*(p)++; if ((p)==(e)) (p)-=(s);}

#define	TAG(obj)		(*(TUint32*)&(obj->iAsyncDeleteNext))

#define CODESEGBUFEND (iCodeSegBuffer+KCodeBufSize)
#define COOKEDBUFEND (iCookedBuf+KCookedBufSize)

extern TUint IntStackPtr();
extern TUint32 SPSR();
extern TUint IDFCRunning();

// global Dfc Que
TDynamicDfcQue* gDfcQ;

class DDeviceSampler : public DLogicalDevice
	{
public:
	DDeviceSampler();
	~DDeviceSampler();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

struct SRawSample
	{
	TLinAddr iPC;
	TUint32 iSampleCounter;
	TUint32 iThreadId;
	};

class DProfile : public DLogicalChannel
	{
public:
	DProfile();
	~DProfile();
protected:
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
	virtual void HandleMsg(TMessageBase* aMsg);
private:
	TInt GetSegments(TDes8* aDes);
	TInt StartSampling(TInt aRate);
	TInt StopSampling();
	TInt Reset(TBool aXIPOnly);
	TInt ResetSegments();
	TInt Drain(TDes8* aDes);
	TInt GetErrors(TDes8* aDes);
	TInt ProcessReadRequest();
	TInt DoDrainCooked();
	TInt Cook();
	void Complete(TInt aResult);
	inline TBool Running()
		{return iTimer.iState!=NTimer::EIdle;}
private:
	static void Sample(TAny*);
	static void Dfc(TAny*);
	static TUint KernelEventHandler(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aPrivateData);
	void LogCodeSegEvent(TKernelEvent aEvent, DEpocCodeSeg *pCodeSeg);

private:
	static TUint8* EncodeTag(TUint8* p, TUint8* e);
	static TUint8* EncodeInt(TUint8* p, TUint8* e, TInt aValue);
	static TUint8* EncodeUint(TUint8* p, TUint8* e, TUint aValue);
	static TUint8* EncodeText(TUint8* p, TUint8* e, const TDesC& aDes);
	static TUint8* EncodeRepeat(TUint8* p, TUint8* e, DProfile* aProfile);
	TUint8* EncodeThread(TUint8* p, TUint8* e, DThread* aThread);
	TUint8* EncodeIDFC(TUint8* p, TUint8* e);
	
	TUint8* PutStream(TUint8* p, TUint8* e, const TUint8* aSource, TInt aSize);
	TUint8* GetStream(TUint8* p, TUint8* e, TInt8* aDest, TInt aSize);	
	TBool CookCodeSeg(TBool aPutAll, TInt aSampleCounter);

private:
	TUint32 iStartTime;
	TInt iRepeat;
	SRawSample iLast;
	TInt iPeriod;
	NTimer iTimer;
	TDfc iDfc;
	TUint* iIntStackTop;
	DThread* iClient;
	TRequestStatus* iReqStatus;
	TInt iPos;			// client des pos
	TInt iRemain;		// space left in client des
	TDes8* iDes;		// client des pointer
	TUint8 iRPut;		// raw buffer put index
	TUint8 iRGet;		// raw buffer get index
	TUint8* iCPut;		// cooked buffer put
	TUint8* iCGet;		// cooked buffer get
	SRawSample iRawBuf[KRawBufSize];
	TUint8 iCookedBuf[KCookedBufSize];
	
	DKernelEventHandler* iKernelEvHandler;

	TInt iNextSampleCounter;
	TBool iXIPOnly;
	TBool iMarkedOnlySegments;	// True during GettingSegments phase in which event handler...
								// ... collects only the events from marked segments.
	TUint8 iCodeSegBuffer[KCodeBufSize];
	TUint8* iCSPut;		// CodeSeg buffer put
	TUint8* iCSGet;		// CodeSeg buffer get
	TUint iIDFCSeenBefore;
	struct TReport
		{
		TUint iRowBufferErrCounter;
		TUint iCodeSegErrCounter;
		TInt  iReportMask; 
		} iReport;
	};

DECLARE_STANDARD_LDD()
	{
	return new DDeviceSampler;
	}

DDeviceSampler::DDeviceSampler()
//
// Constructor
//
	{
	//iParseMask=0;
	//iUnitsMask=0;
	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
	}

const TInt KDSamplerThreadPriority = 27;
_LIT(KDSamplerThread,"DSamplerThread");

TInt DDeviceSampler::Install()
//
// Install the device driver.
//
	{
	// Allocate a kernel thread to run the DFC 
	TInt r = Kern::DynamicDfcQCreate(gDfcQ, KDSamplerThreadPriority, KDSamplerThread);

	if (r != KErrNone)
		return r; 	

	r=SetName(&KLddName);
	return r;
	}

void DDeviceSampler::GetCaps(TDes8& aDes) const
//
// Return the capabilities.
//
	{
	}

/**
  Destructor
*/
DDeviceSampler::~DDeviceSampler()
	{
	if (gDfcQ)
		gDfcQ->Destroy();
	}

TInt DDeviceSampler::Create(DLogicalChannelBase*& aChannel)
//
// Create a channel on the device.
//
	{
	aChannel=new DProfile;
	return aChannel?KErrNone:KErrNoMemory;
	}

DProfile::DProfile()
	:	iTimer(Sample,this),
		iDfc(Dfc,this,NULL,7)
//
// Constructor
//
	{
	}

DProfile::~DProfile()
//
// Destructor
//
	{
	Kern::SafeClose((DObject*&)iClient, NULL);
	}

TInt DProfile::DoCreate(TInt /*aUnit*/, const TDesC8* /*anInfo*/, const TVersion& aVer)
//
// Create the channel from the passed info.
//
	{
	if (!Kern::QueryVersionSupported(TVersion(1,0,1),aVer))
		return KErrNotSupported;
	iClient=&Kern::CurrentThread();
	iClient->Open();
	Kern::SetThreadPriority(24);
	iIntStackTop=(TUint*)IntStackPtr();
	SetDfcQ(gDfcQ);
	iDfc.SetDfcQ(iDfcQ);
	iMsgQ.Receive();
	return KErrNone;
	}

void DProfile::Complete(TInt aResult)
//Completes user request
	{
	DEBUG_PROFILER(Kern::Printf("C");)
	Kern::RequestComplete(iClient,iReqStatus,aResult);
	}

TInt DProfile::StartSampling(TInt aRate)
	{
	DEBUG_PROFILER(Kern::Printf("START");)
	//Activate timer
	aRate=Min(KMaxRate, Max(KMinRate, aRate));
	iPeriod=1000/aRate;
	if (!Running())
		iTimer.OneShot(iPeriod);
	
	DEBUG_PROFILER(Kern::Printf("START end");)
	return KErrNone;
	}

TInt DProfile::GetSegments(TDes8* aDes)
//
// Collects and marks all non-XIP segments.
//
	{
	DEBUG_PROFILER(Kern::Printf("GS");)
	TInt max=Kern::ThreadGetDesMaxLength(iClient,aDes);
	Kern::ThreadDesWrite(iClient,aDes,KNullDesC8,0,0,iClient);//Set length to zero
	TInt current = 0;

	Kern::AccessCode();
	
	// Take all records that are collected by event handler first. They may be only Delete CodeSeg 
	// events of tagged(marked) segments. On the first GetSegments call, cooked buffer also contains Profile Tag.

	CookCodeSeg(ETrue, 0); // Transfer/encode from CodeSeg buffer into cooked buffer
	current = iCPut-iCGet;
	if (current)
		{
		if (current < max)
			{//Copy data into user side descriptor
			TPtrC8 aPtr(iCGet, current);
			Kern::ThreadDesWrite(iClient,aDes,aPtr,0,KChunkShiftBy0,iClient);
			}
		else
			{	
			//This is very unlikely as in this stage we collect only CodeSeg Delete events of the marked segments.
			//It cannot happen on the first call, as there are no marked segments - which means that Profiler Tag is OK.
			iReport.iCodeSegErrCounter++;
			}
		}
	iCGet = iCPut = iCookedBuf; //Reset the cooked buffer
	
	//Collect all non-XIP segments that are not already marked.

	SDblQue* p = Kern::CodeSegList();
	SDblQueLink* anchor=&p->iA;
	SDblQueLink* a=anchor->iNext;
	for (; a!=anchor; a=a->iNext) 
		{
		DEpocCodeSeg* pSeg = (DEpocCodeSeg*) _LOFF(a, DCodeSeg, iLink);
		if (pSeg->iXIP || pSeg->iMark&DCodeSeg::EMarkProfilerTAG)
			continue;
		if (current > (max-KMaxCreateCodeSegRecordSize))
			break;//No more space. Finish now and wait for another GetSegments request.
			
		pSeg->iMark |= DCodeSeg::EMarkProfilerTAG;	//Mark this segment
		LogCodeSegEvent(EEventAddCodeSeg, pSeg); 	//Place this record into CodeSeg buffer ...
		CookCodeSeg(ETrue, 0);						//...and encode it into cooked buffer
		TPtrC8 aPtr(iCGet, iCPut-iCGet);
		Kern::ThreadDesWrite(iClient,aDes,aPtr,current,KChunkShiftBy0,iClient);//Copy record into user desc.
		current += iCPut-iCGet;
		iCPut = iCGet = iCookedBuf; //Reset cooked buffer
		}

	if (!current)//This will be the last GetSegments call. From now on, all events have to be recorded.
		iMarkedOnlySegments = EFalse;
	
	Kern::EndAccessCode();
	DEBUG_PROFILER(Kern::Printf("GS end %d",current);)
	return KErrNone;
	}

TInt DProfile::ResetSegments()
//
// Unmarks all non-XIP segments 
// Sets device into GettingSegments mode in which only the events of the marked Code Segments will be recorder
// 
	{
	DEBUG_PROFILER(Kern::Printf("RS");)
	if (iXIPOnly)
		return KErrGeneral;
	
	Kern::AccessCode();
	SDblQue* p = Kern::CodeSegList();
	SDblQueLink* anchor=&p->iA;
	SDblQueLink* a=anchor->iNext;
	for (; a!=anchor; a=a->iNext) 
		{
		DEpocCodeSeg* pSeg = (DEpocCodeSeg*) _LOFF(a, DCodeSeg, iLink);
		if (!pSeg->iXIP)
			pSeg->iMark &= ~DCodeSeg::EMarkProfilerTAG;
		}

	if (DKernelEventHandler::DebugSupportEnabled())
		{
		DEBUG_PROFILER(Kern::Printf("RS add handler");)
		iKernelEvHandler->Add();
		iReport.iReportMask|= KNonXIPModeActive;
		}
	else
		iReport.iReportMask|= KNoDebugSupport;	
	
	iMarkedOnlySegments = ETrue;	
	Kern::EndAccessCode();
	DEBUG_PROFILER(Kern::Printf("RS end");)
	return KErrNone;
	}

TInt DProfile::Reset(TBool aXIPOnly)
	{
//
// Resets the device. It is the first message sent by profiler application.
//	
	if (Running())
		return KErrGeneral;
	
	DEBUG_PROFILER(Kern::Printf("RST %d", aXIPOnly);)

	iXIPOnly = aXIPOnly;

	iTimer.Cancel();
	iDfc.Cancel();
	iLast.iPC=0;
	iLast.iSampleCounter=0;
	iLast.iThreadId=0;
	iRepeat=0;
	iPeriod=1;
	iReqStatus=NULL;
	iRPut=0;				// raw buffer put index
	iRGet=0;				// raw buffer get index
	iCPut=EncodeTag(iCookedBuf,COOKEDBUFEND); //cooked buffer put
	iCGet=iCookedBuf;		// cooked buffer get
	iPos=0;					// client des pos
	iDes=NULL;				// client des pointer
	iStartTime=NKern::TickCount();

	iReport.iRowBufferErrCounter = 0;
	iReport.iCodeSegErrCounter = 0;
	iReport.iReportMask = 0;
	iNextSampleCounter = 0;
	iCSPut=iCodeSegBuffer;	// CodeSeg buffer put
	iCSGet=iCodeSegBuffer;	// CodeSeg buffer get
	iMarkedOnlySegments = EFalse;
	iIDFCSeenBefore = EFalse;
	if (!iXIPOnly)
		iKernelEvHandler = new DKernelEventHandler(KernelEventHandler, this);
	
	DEBUG_PROFILER(Kern::Printf("RST end");)
	return KErrNone;
	}

TInt DProfile::StopSampling()
//
// Stops sampling
//
	{
	DEBUG_PROFILER(Kern::Printf("STOP");)
	if (Running())
		{
		iTimer.Cancel();
		Dfc(this);
		}
	if (iReqStatus)
		Complete(KErrNone);
	
	DEBUG_PROFILER(Kern::Printf("STOP end");)
	return KErrNone;
	}

TInt DProfile::GetErrors(TDes8* aDes)
//
// Returns error report and closes event handler
//
	{
	TInt r = KErrNone;
	TBuf8<KMaxErrorReportRecordSize> localBuf; //Enough space to encode 3 zeros and 3 integers
	DEBUG_PROFILER(Kern::Printf("GE");)

	TInt max=Kern::ThreadGetDesMaxLength(iClient,aDes);
	if (max<KMaxErrorReportRecordSize)
		return KErrArgument;
	
	Kern::ThreadDesWrite(iClient,aDes,KNullDesC8,0,0,iClient);//set zero length
	
	TUint8* p = (TUint8*)localBuf.Ptr();
	TUint8* e = p+KMaxErrorReportRecordSize;
	p = EncodeInt (p, e, 0);
	p = EncodeUint(p, e, 0);
	p = EncodeUint(p, e, 0);

	p = EncodeUint(p, e, iReport.iRowBufferErrCounter);
	p = EncodeUint(p, e, iReport.iCodeSegErrCounter);
	p = EncodeUint(p, e, iReport.iReportMask);

	localBuf.SetLength(p-localBuf.Ptr());
	r=Kern::ThreadDesWrite(iClient,aDes,localBuf,0,KChunkShiftBy0,iClient);
	
	if(iKernelEvHandler && iKernelEvHandler->IsQueued())
		iKernelEvHandler->Close();

	DEBUG_PROFILER(Kern::Printf("GE end %d %d %d", iReport.iRowBufferErrCounter, iReport.iCodeSegErrCounter, iReport.iReportMask);)
	return r;
	}


TInt DProfile::Drain(TDes8* aDes)
//
// Collects any remaining data
//
	{
	DEBUG_PROFILER(Kern::Printf("D");)		
	if (Running())
		return KErrGeneral;
	// we can assume read request is not pending
	TInt max=Kern::ThreadGetDesMaxLength(iClient,aDes);
	if (max<0)
		return max;
	if (max==0)
		return KErrArgument;
	TInt r=Kern::ThreadDesWrite(iClient,aDes,KNullDesC8,0,0,iClient);		// set client descriptor length to zero
	if (r!=KErrNone)
		return r;
	iDes=aDes;
	iRemain=max;
	iPos=0;
	iReqStatus=NULL;
	TInt n=-1;
	while (n)
		{
		r=DoDrainCooked();				// drain any cooked data if possible
		if (r<0 && r!=KErrUnderflow)
			return r;					// error writing client buffer
		n=Cook();						// cook the samples, return number cooked
		}

	// there might still be data left over
	DEBUG_PROFILER(Kern::Printf("D end");)
	return KErrNone;
	}

TInt DProfile::ProcessReadRequest()
	{
// If the profiler is stopped and there is available data, return it immediately and complete the request
// If the profiler is stopped and there is no data, wait.
// If the profiler is running, retrieve any data available now, if more is req'd set the trigger
	DEBUG_PROFILER(Kern::Printf("READ");)
	TInt max=Kern::ThreadGetDesMaxLength(iClient,iDes);
	if (max<0)
		return max;
	if (max==0)
		return KErrArgument;
	TInt r=Kern::ThreadDesWrite(iClient,iDes,KNullDesC8,0,0,iClient);		// set client descriptor length to zero
	if (r!=KErrNone)
		return r;
	iRemain=max;
	iPos=0;
	TInt n=-1;
	TBool read=EFalse;
	while (n)
		{
		r=DoDrainCooked();				// drain any cooked data if possible
		if (r!=KErrUnderflow)
			read=ETrue;					// we've got something
		if (r>0)
			return KErrNone;			// request completed, so finish
		if (r!=KErrNone && r!=KErrUnderflow)
			return r;					// error writing client buffer
		n=Cook();						// cook the samples, return number cooked
		}
	if (!Running() && read)
		return KErrCompletion;			// if stopped and data read, return it
	return KErrNone;					// wait
	}

TInt DProfile::DoDrainCooked()
//
// Copies encoded data from Cook buffer into user side descriptor (iDes).
// Returns:
// KErrUnderflow if all the data was already transfered or the desciptor was already full before the call.
// KErrNone if there is still remaining space available in the descriptor.
// 1  - descriptor is full and user request is completed.
// Error code other then KErrNone if writing to the user memory fails
//
	{
	TInt avail=iCPut-iCGet;
	if (avail<0)
		avail+=KCookedBufSize;
	TInt len=Min(avail,iRemain);
	if (len)
		{
		TUint8* pE=iCookedBuf+KCookedBufSize;
		TInt len1=Min(len,pE-iCGet);
		TPtrC8 local(iCGet,len1);
		TInt r=Kern::ThreadDesWrite(iClient, iDes, local, iPos, KChunkShiftBy0, iClient);
		if (r!=KErrNone)
			return r;
		len-=len1;
		TUint8* pG=iCGet+len1;
		if (pG==pE)
			pG=iCookedBuf;
		iCGet=pG;
		iRemain-=len1;
		iPos+=len1;
		if (len) // will be > 0 if there are remaining data at the beginning of Cooked buffer to be copied.
			{
			TPtrC8 local(iCGet,len);
			r=Kern::ThreadDesWrite(iClient, iDes, local, iPos, KChunkShiftBy0, iClient);
			if (r!=KErrNone)
				return r;
			iCGet+=len;
			iRemain-=len;
			iPos+=len;
			}
		if (iRemain==0 && iReqStatus)
			{
			Complete(KErrNone);
			return 1;
			}
		return KErrNone;
		}
	return KErrUnderflow;
	}

void DProfile::HandleMsg(TMessageBase* aMsg)
//
// Client requests
//
	{
	TInt r=KErrNone;
	TThreadMessage& m=*(TThreadMessage*)aMsg;
	TInt id=m.iValue;
	// Allow the client thread to send a message or system critical thread
	// to send a close message as this is probably the supervisor thread doing clean up
	if (m.Client()!=iClient && 
		!((m.Client()->iFlags&KThreadFlagSystemCritical) && id==(TInt)ECloseMsg))
		{
		m.PanicClient(_L("SAMPLER"),EAccessDenied);
		return;
		}
	if (id==(TInt)ECloseMsg)
		{
		DEBUG_PROFILER(Kern::Printf("CLOSE");)
		iTimer.Cancel();
		iDfc.Cancel();
		m.Complete(KErrNone,EFalse);
		iMsgQ.CompleteAll(KErrServerTerminated);
		DEBUG_PROFILER(Kern::Printf("CLOSE end");)
		return;
		}
	else if (id<0)
		{
		if (id!=~RSampler::ERequestRead)
			{
			TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
			Kern::RequestComplete(iClient,pS,KErrNotSupported);
			}
		if (iReqStatus)
			{
			m.PanicClient(_L("SAMPLER"),ERequestAlreadyPending);
			return;
			}
		iReqStatus=(TRequestStatus*)m.Ptr0();
		iDes=(TDes8*)m.Ptr1();
		r=ProcessReadRequest();
		if (r!=KErrNone)
			{
			if (r==KErrCompletion)
				r=KErrNone;
			Complete(r);
			}
		r=KErrNone;
		}
	else if (id==KMaxTInt)
		{
		TInt mask=m.Int0();
		if (mask & (1<<RSampler::ERequestRead))
			{
			Complete(KErrCancel);
			}
		}
	else
		{
		switch(id)
			{
			case RSampler::EControlGetSegments:
				r=GetSegments((TDes8*)m.Ptr0());
				break;
			case RSampler::EControlStartProfile:
				r=StartSampling(m.Int0());
				break;
			case RSampler::EControlStopProfile:
				r=StopSampling();
				break;
			case RSampler::EControlResetProfile:
				r=Reset((TBool)m.Ptr0());
				break;
			case RSampler::EControlResetSegments:
				r=ResetSegments();
				break;
			case RSampler::EControlDrain:
				r=Drain((TDes8*)m.Ptr0());
				break;
			case RSampler::EControlGetErrors:
				r=GetErrors((TDes8*)m.Ptr0());
				break;
			default:
				r=KErrNotSupported;
				break;
			}
		}
	m.Complete(r,ETrue);
	}

TUint8* DProfile::EncodeTag(TUint8* p, TUint8* e)
//
// Encode a tag and version to the trace data. This allows the offline analyser to 
// identify the sample data.
//
	{
	_LIT(KTraceTag,"profile");
	p=EncodeText(p,e,KTraceTag);
	p=EncodeUint(p,e,KMajorVersionNumber);
	return p;
	}

TUint8* DProfile::EncodeInt(TUint8* p, TUint8* e, TInt aValue)
//
// Encode a 32 bit signed integer into the data stream
// This has to deal with wrap around at the end of the buffer
//
	{
	TUint byte;
	for (;;)
		{
		byte = aValue & 0x7f;
		if ((aValue >> 6) == (aValue >> 7))
			break;
		aValue >>= 7;
		PUT(p,(TUint8)byte,e,KCookedBufSize);
		}
	PUT(p,(TUint8)(byte|0x80),e,KCookedBufSize);
	return p;
	}

TUint8* DProfile::EncodeUint(TUint8* p, TUint8* e, TUint aValue)
//
// Encode a 32 bit unsigned integer into the data stream
// This has to deal with wrap around at the end of the buffer
//
	{
	TUint byte;
	for (;;)
		{
		byte = aValue & 0x7f;
		aValue >>= 7;
		if (aValue == 0)
			break;
		PUT(p,(TUint8)byte,e,KCookedBufSize);
		}
	PUT(p,(TUint8)(byte|0x80),e,KCookedBufSize);
	return p;
	}

TUint8* DProfile::EncodeText(TUint8* p, TUint8* e, const TDesC& aDes)
//
// Encode a descriptor into the data stream
// This is currently limited to a descriptor that is up to 255 characters in length,
// and Unicode characters are truncated to 8 bits
//
	{
	TInt len=aDes.Length();
	PUT(p,(TUint8)len,e,KCookedBufSize);
	const TText* s = aDes.Ptr();
	while (--len >= 0)
		PUT(p,*s++,e,KCookedBufSize);
	return p;
	}

TUint8* DProfile::EncodeIDFC(TUint8* p, TUint8* e)
//
// iDFC samples do not really belong to any thread.
// However, the profiler protocol requires each sample to be associated to a particular thread.
// This method will encode 'fake' process ID & name and thread name for iDFC sample in the data stream.
// It will be embedded only for the very first sample from iDFCs.
// (For the rest of iDFCs samples, threadID is sufficient - as for the real threads.)
//
	{
	p=EncodeUint(p,e,KiDFCId);     //processID for iDFC
	p=EncodeText(p,e,KiDFCProcess);//process name for iDFC
	p=EncodeText(p,e,KiDFCThread); //thread name for iDFC
	return p;
	}

TUint8* DProfile::EncodeThread(TUint8* p, TUint8* e, DThread* aThread)
//
// Encode a thread name in the data stream.
// The thread is identified by its name, and the identity of its owning process.
// If the process has not been identified in the data stream already, it's name is
// also encoded.
//
	{
	DProcess* pP=aThread->iOwningProcess;
	TKName n;
	p=EncodeUint(p,e,pP->iId);
	if (TAG(pP)!=iStartTime)	// not seen this before
		{
		TAG(pP)=iStartTime;
		// Provide the name matching this process ID
		pP->Name(n);
		p=EncodeText(p,e,n);
		}
	aThread->Name(n);
	p=EncodeText(p,e,n);
	return p;
	}

TUint8* DProfile::EncodeRepeat(TUint8* p, TUint8* e, DProfile* aP)
//
// Encode a repeated sequence of samples
//
	{
	p=EncodeInt(p,e,0);
	p=EncodeUint(p,e,aP->iRepeat);
	aP->iRepeat = 0;
	return p;
	}

TInt DProfile::CookCodeSeg(TBool aPutAll, TInt aSampleCounter)
//
// Transfers and encodes CodeSeg Create/Delete records from CodeSeg buffer into Cooked buffer.
// If aPutAll = Etrue, all records will be transferred.
// If aPutAll = EFalse, only records at the beginning of the queue that matches aSampleCounter will be transferred.
// It stopps if there is less then KRequiredFreeSpace bytes available in cooked buffer.
// Returns the number of the transferred records.
//
	{
	if (iXIPOnly)
		return 0;
	
	TInt n = 0;
	TInt codeSampleCounter;//Will hold the sample counter of the record
	TInt runAddress;
	TInt codeSize;
	TInt8 textLen;
	TBuf<255> text;
	TUint8* localCSGet = iCSGet;

	FOREVER
		{
		//Check if there is any Code Seg record left.
		if (iCSGet==iCSPut)
			return n;//No records left
		
		//Check if the next record is due to be encoded. Both Create & Delete CodeSeg records start with sample counter.
		localCSGet = iCSGet;
		localCSGet = GetStream(localCSGet, CODESEGBUFEND, (TInt8*)(&codeSampleCounter), sizeof(TInt));
		if (!aPutAll && codeSampleCounter!=aSampleCounter)
			return n; //Still too early to insert the record into Cook Buffer
			
		//Check for the space in cook buffer
		TInt cookAvailable = (TInt)iCGet - (TInt)iCPut;
		if (cookAvailable <= 0)	
			cookAvailable+=KCookedBufSize;
		if (cookAvailable < KRequiredFreeSpace)
			return n;//No space in Cook Buffer.
		
		//At this point it is for sure that we have to transfer some record to cook buffer
		
		n++;
		iCSGet = localCSGet;
		//The next field for both Create & Delete CodeSeg records is run address:
		iCSGet = GetStream(iCSGet, CODESEGBUFEND, (TInt8*)(&runAddress), sizeof(TInt));
		
		if (runAddress & 1)//LSB in run address idenifies the type of the record
			{//CodeSegment Delete record. To be encoded as Int(0), UInt(0), UInt(RunAddress | 1)
			iCPut = EncodeInt (iCPut, COOKEDBUFEND, 0);
			iCPut = EncodeUint(iCPut, COOKEDBUFEND, 0);
			iCPut = EncodeUint(iCPut, COOKEDBUFEND, runAddress);
			}
		else
			{//CodeSegment Create record.
			iCSGet = GetStream(iCSGet, CODESEGBUFEND, (TInt8*)(&codeSize), sizeof(TInt));
			iCSGet = GetStream(iCSGet, CODESEGBUFEND, (TInt8*)(&textLen), sizeof(TInt8));
			iCSGet = GetStream(iCSGet, CODESEGBUFEND, (TInt8*)(text.Ptr()), textLen);
			text.SetLength(textLen);
			//To be encoded as Int(0), UInt(0), UInt(RunAddress), UInt(SegmentSize), Text(FileNeme)
			iCPut = EncodeInt(iCPut, COOKEDBUFEND, 0);
			iCPut = EncodeUint(iCPut, COOKEDBUFEND, 0);
			iCPut = EncodeUint(iCPut, COOKEDBUFEND, runAddress);
			iCPut = EncodeUint(iCPut, COOKEDBUFEND, codeSize);
			iCPut = EncodeText(iCPut, COOKEDBUFEND, text);
			}
		}
	}

TInt DProfile::Cook()
//
// Transfers/encodes row data and code segments record into cooked buffer.
// Returns the number of records (incl. both samples and codeSeg records) cooked.
//
	{
	TUint8* p=iCPut;
	TUint8* e=iCookedBuf+KCookedBufSize;
	TInt n=0;
	
	FOREVER
		{
		iCPut=p; //update iCPut before calling CookCodeSeg
		if ((iRGet==iRPut))
			{//No more samples.
			n+=CookCodeSeg(ETrue, 0); //Cook the remaining content of CodeSeg buffer.
			break;
			}

		SRawSample* s=iRawBuf+iRGet;	// pointer to the next sample to be cooked

		n+=CookCodeSeg(EFalse, s->iSampleCounter);//cook all codeSeg records than matches this sample counter
		p=iCPut; //CookCodeSeg might have changed iCPut

		TInt space=iCGet-p;
		if (space<=0)
			space+=KCookedBufSize;		// space remaining in cooked buffer
		if (space<KRequiredFreeSpace)
			break;						// if insufficient, finish

		//Cook the next sample record from Row buffer		
		++iRGet;
		++n;
		TBool newthread=s->iPC & 1;		// bit 0 of PC means so far unknown thread
		TLinAddr pc=s->iPC &~ 1;
		TUint rp = iRepeat;
		TInt diff=TInt(pc-iLast.iPC);
		if (!newthread)
			{
			if (s->iThreadId!=iLast.iThreadId)
				diff|=1;
			if (diff == 0)
				{
				iRepeat = rp + 1;	// Identical sample, bump up the repeat count
				continue;
				}
			if (rp)
				{
				// Encode the repeat data
				p = EncodeRepeat(p,e,this);
				}
			// Encode the PC difference
			p = EncodeInt(p, e, diff);
			if (diff & 1)
				{
				// Encode the new thread ID
				iLast.iThreadId = s->iThreadId;
				p = EncodeUint(p, e, s->iThreadId);
				}
			}
		else
			{
			if (rp)
				{
				// Encode the repeat data
				p = EncodeRepeat(p,e,this);
				}
			// Encode the PC difference
			p = EncodeInt(p, e, diff|1);

			if (s->iThreadId == KiDFCId)
				{
				// This is the first sample from iDFC. Encode 'threadID'
				iLast.iThreadId = KiDFCId;
				p = EncodeUint(p, e, KiDFCId);
				// and encode processID, processName & threadName for this sample
				p = EncodeIDFC(p, e);
				}
			else
				{
				// Encode the new thread ID
				DThread* pT=(DThread*)s->iThreadId;
				iLast.iThreadId = pT->iId;
				p = EncodeUint(p, e, pT->iId);
				// The thread is 'unknown' to this sample, so encode the thread name
				p = EncodeThread(p, e, pT);
				}
			}
		iLast.iPC=pc;
		}
	return n;
	}

void DProfile::Dfc(TAny* aPtr)
//
// Tranfers/encodes Row & CodeSeg buffers' content into Cook buffer (by Cook()), 
// and copies encoded data into user side descriptor (by DoDrainCooked())
//
	{
	DProfile& d=*(DProfile*)aPtr;
	TInt n=-1;
	while (n)
		{
		TInt r=d.DoDrainCooked();		// drain any cooked data if possible
		if (r<0 && r!=KErrUnderflow)
			{
			d.Complete(r);				// error writing client buffer
			break;
			}
		n=d.Cook();						// cook the samples, return number cooked
		}
	}

TUint8* DProfile::PutStream(TUint8* p, TUint8* e, const TUint8* aSource, TInt aSize)
//
// Put data into CodeSeg stream 
// This has to deal with wrap around at the end of the buffer
//
	{
	for (TInt i = 0 ; i< aSize ; i++)
		{
		PUT(p,(TUint8)(*aSource),e,KCodeBufSize);
		aSource++;
		}
	return p;
	}

TUint8* DProfile::GetStream(TUint8* p, TUint8* e, TInt8* aDest, TInt aSize)
//
// Get 32 bits from CodeSeg stream.
// This has to deal with wrap around at the end of the buffer
//
	{
	for (TInt i = 0 ; i< aSize ; i++)
		GET_A_BYTE(p,aDest,e,KCodeBufSize);
	return p;
	}

void DProfile::LogCodeSegEvent(TKernelEvent aEvent, DEpocCodeSeg *pCodeSeg)
//
// Records the event in CodeSeg buffer.
//
	{
///
///	
	TUint8* localCSPut = iCSPut;
	TInt available = KCodeBufSize + (TInt)iCSGet - (TInt)iCSPut;
	if (available > KCodeBufSize)
		available -= KCodeBufSize;

	switch (aEvent)
		{
	case EEventAddCodeSeg:
		{
		TInt textOffset = 0;
		TInt textSize= pCodeSeg->iFileName->Length();
		//Restrict file name to max 255 sharacters. If bigger, record the last 255 bytes only
		if (textSize > 255)
			{
			textOffset = textSize-255;
			textSize = 255;
			}
		if ((available -= 13+textSize) < 0) //13 bytes needed for sample counter, run address, size and text size) 
			{
			iReport.iCodeSegErrCounter++;
			return;
			}
		localCSPut = PutStream(localCSPut, CODESEGBUFEND, (TUint8*)(&iNextSampleCounter), sizeof(TInt));
		localCSPut = PutStream(localCSPut, CODESEGBUFEND, (TUint8*)(&pCodeSeg->iRunAddress), sizeof(TInt));
		localCSPut = PutStream(localCSPut, CODESEGBUFEND, (TUint8*)(&pCodeSeg->iSize), sizeof(TInt));
		localCSPut = PutStream(localCSPut, CODESEGBUFEND, (TUint8*)(&textSize), sizeof(TInt8));
		localCSPut = PutStream(	localCSPut, CODESEGBUFEND, pCodeSeg->iFileName->Ptr()+textOffset, textSize);
		iCSPut = localCSPut;

		DEBUG_PROFILER
			(
			TBuf<256> buf(textSize+1);
			buf.Copy(pCodeSeg->iFileName->Ptr()+textOffset,textSize); buf.SetLength(textSize+1); buf[textSize]=0;
			Kern::Printf("CREATE CS:%s, %x, %x,", buf.Ptr(), pCodeSeg->iRunAddress, pCodeSeg->iSize);
			)
		break;
		}
	case EEventRemoveCodeSeg:
		{
		if ((available-=8) < 0) //8 bytes needed for sample counter and run address
			{
			iReport.iCodeSegErrCounter++;
			return;
			}
		TInt runAddress = pCodeSeg->iRunAddress | 1; 
		localCSPut = PutStream(localCSPut, CODESEGBUFEND, (TUint8*)(&iNextSampleCounter), sizeof(TInt));
		localCSPut = PutStream(localCSPut, CODESEGBUFEND, (TUint8*)(&runAddress), sizeof(TInt));
		iCSPut = localCSPut;

		DEBUG_PROFILER(Kern::Printf("DELETE CS:%x", pCodeSeg->iRunAddress);)
		break;
		}
	default:
		return;
	}
	if (available < KCodeBufSize/2) //Start emptying CodeSeg (and Raw Buffer, as well) Buffer if more then a half full.
		iDfc.Enque();
}

TUint DProfile::KernelEventHandler(TKernelEvent aEvent, TAny* a1, TAny* a2, TAny* aPrivateData)
//
// Logs non-XIP CodeSeg Create/Delete events.
// In GettingSegments mode, it logs only deletion of the marked segments.
// Runs in the content of the Kernel scheduler
//
{
	if (aEvent==EEventAddCodeSeg || aEvent==EEventRemoveCodeSeg) 
	{
		DProfile *p = (DProfile*)aPrivateData;
		DEpocCodeSeg* pCodeSeg = (DEpocCodeSeg*)(DCodeSeg*)a1;

		Kern::AccessCode();
		if ((!pCodeSeg->iXIP) && (!p->iMarkedOnlySegments || pCodeSeg->iMark&DCodeSeg::EMarkProfilerTAG))
			p->LogCodeSegEvent(aEvent, pCodeSeg);
		Kern::EndAccessCode();

	}
	return DKernelEventHandler::ERunNext;
}


void DProfile::Sample(TAny* aPtr)
	{
	DProfile& d=*(DProfile*)aPtr;
	d.iTimer.Again(d.iPeriod);
	TUint8 next_put=(TUint8)(d.iRPut+1);
	if (next_put!=d.iRGet)		// space in raw buffer
		{
		DThread* pT=Kern::NThreadToDThread(NKern::CurrentThread());
		if (pT!=NULL)
			{
			SRawSample* p=d.iRawBuf+d.iRPut;
			d.iRPut=next_put;
			p->iPC=((d.iIntStackTop)[-1]) & ~1; //clear LSB bit (in case of Jazelle code)
			p->iSampleCounter=d.iNextSampleCounter++;
			
			if (IDFCRunning())
				{
				p->iThreadId=KiDFCId; //indicates iDFC running
				if (!d.iIDFCSeenBefore)
					{
					d.iIDFCSeenBefore = ETrue;
					p->iPC|=1;				// set bit 0 of PC to indicate new thread
					}
				else
					{
					TUint8 used=(TUint8)(d.iRPut-d.iRGet);
					if (used<=KRawBufSize/2)
						return;
					}
				}
			else
				{
				
				if (TAG(pT)!=d.iStartTime)	// not seen this before
					{
					TAG(pT)=d.iStartTime;
					p->iThreadId=(TUint)pT;
					p->iPC|=1;				// set bit 0 of PC to indicate new thread
					}
				else
					{
					p->iThreadId=pT->iId;
					TUint8 used=(TUint8)(d.iRPut-d.iRGet);
					if (used<=KRawBufSize/2)
						return;
					}
				}
			d.iDfc.Add();	// queue DFC if new thread seen or buffer more than half full
			}
		}
	else
		d.iReport.iRowBufferErrCounter++;
	}

