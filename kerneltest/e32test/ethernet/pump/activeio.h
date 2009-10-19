/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#if (!defined __ACTIVEIO_H__)
#define __ACTIVEIO_H__

#include <e32base.h>

class CIOBuffer : public CBase
	{
public:
	~CIOBuffer();
	HBufC8*	Data() const;
	void FreeData();
	TPtr8& Ptr();
	void Assign(HBufC8* aBuffer = NULL);
    static CIOBuffer* NewL(HBufC8* aBuf = NULL);
    static CIOBuffer* NewL(const TInt aSize);
	static TInt LinkOffset();

private:
	CIOBuffer();
	void ConstructL(const TInt aSize);
	void ConstructL(HBufC8* aBuffer);
	
	TSglQueLink iLink;
	HBufC8* iBuf;
	TPtr8 iBufPtr;
	};

///////
// Pure Abstract 'M' interface classes that CDemoControl derives from
class MWriterNotify
	{
public:
	virtual void WriteCompleteL(const TInt aStatus) = 0;
	};

class MReaderNotify
	{
public:
	virtual void ReadCompleteL(const TInt aStatus) = 0;
	};
///////

class CDemoWriter : public CActive
// Active object class for writing to the server
	{
public:
	~CDemoWriter();

	static CDemoWriter* NewL(MWriterNotify& aNotify,RBusDevEthernet& aCard);

	void WriteL(const TDesC8& aBuffer);
	void RunL();
	void DoCancel();
private:
	// Construct with pointer to the notifier and reference to the server session


	void ConstructL(MWriterNotify& aNotify,RBusDevEthernet& aCard);


	CDemoWriter(TInt aPriority) : CActive(aPriority){};
private:


	RBusDevEthernet *iCard;

	MWriterNotify* iNotify;
	};

class CDemoReader : public CActive
	{
public:
	~CDemoReader();


	static CDemoReader* NewL(MReaderNotify& aNotify,RBusDevEthernet& aCard);

	void RunL();
	void DoCancel();
	void ReadL(TDes8& aBuffer);
private:


	void ConstructL(MReaderNotify& aNotify,RBusDevEthernet& aCard);


	CDemoReader(TInt aPriority) : CActive(aPriority){};
private:
	MReaderNotify* iNotify;


	RBusDevEthernet *iCard;

	};


// C Class derived fron CActive
// CActive derived from CBase
// See PSP Chapter 18 Active Objects
class CDemoControl : public CActive , public MReaderNotify , public MWriterNotify
	{
public:
	~CDemoControl();
	static CDemoControl* NewLC();
	// Mandatory Overrides of CActive pure virtuals
	void RunL();
	void DoCancel();

	static TInt Callback(TAny* aCtrl);
	void RequestCharacter();
	virtual void WriteCompleteL(const TInt aStatus);
	virtual void ReadCompleteL(const TInt aStatus);

	void ReadAndSetDestMacL();
	void ReadAndDisplaySettings();
	CIOBuffer* CreateRandomPacketL(TInt aOffset);
	void SendAndCompareEchoL();
	void CompareEcho();
	void HandleWriteCompleteSndCmpEchoModeL();
	void HandleReadCompleteSndCmpEchoModeL();
	
private:
	void ConstructL();
	CDemoControl(TInt aPriority) : CActive(aPriority){};
	void ProcessKeyPress(TChar aChar);
	void HelpText() const;
	void StartCardL();
	void StopCard();
	void EchoL();
	void PumpL();
	void ReadL();
	void StopL();
	void PrintError(TChar aChar);
	void EmptyWriteQueue();
	void HandleWriteCompleteEchoModeL();
	void HandleReadCompleteEchoModeL();
	void HandleWriteCompletePumpModeL();
	void HandleReadCompletePumpModeL();
	void HandleReadCompleteReadModeL();
	void FlipMacAddresses(TDes8& aBuf);
	CIOBuffer*  CreateSendPacketL();

private:
	enum TIfState {EIdle,EEcho,ERead,EPump,ESendAndCmpEcho};
	TIfState iIfState;

	CDemoWriter* iWriter;
	CDemoReader* iReader;
	TInt		iPacketsWritten;
	TInt		iPacketsRead;
	CPeriodic*	iTimer;

	TBuf8<1600> iReadBuffer;


	RBusDevEthernet iCard;
	TBuf8<32>	iConfig;

	TSglQue<CIOBuffer> iWriteQueue;

	TBool iSendAndEchoSame;
	TInt iIntRandomOffset;
	TInt64 iIntSeed;
	static TInt iSendAndEchoCmpCounter;
	};

#endif
