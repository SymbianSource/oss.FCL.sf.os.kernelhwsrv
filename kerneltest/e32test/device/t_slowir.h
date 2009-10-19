// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\device\t_slowir.h
// 
//

//#include <es_sock.h>
#include <e32test.h>
#include <e32cons.h>
#include <d32comm.h>

#ifndef __T_SLOWIR__
#define __T_SLOWIR__ 

//	Our own comms object with synchronous writes
class RCommDev : public RBusDevComm
	{
public:
	TInt WriteS(const TDesC8& aDes);
	TInt WriteS(const TDesC8& aDes,TInt aLength);
	TInt WriteSE(const TDes8& Entered, TInt aLength);
	};

enum TXferType
	{
    EWriteXfer,
    EReadXfer,
	EDiagXfer,
	EDiscardXfer
    };

class CActiveRW;
class CActiveConsole : public CActive
//-----------------------------------
	{
public:
	  // Construction
	CActiveConsole(CConsoleBase* aConsole);
	static CActiveConsole* NewLC(CConsoleBase* aConsole);
	void ConstructL();

	  // Destruction
	~CActiveConsole();

	  // Issue request
	void RequestCharacter();
	
	  // Cancel request.
	  // Defined as pure virtual by CActive;
	  // implementation provided by this class.
	void DoCancel();

	  // Service completed request.
	  // Defined as pure virtual by CActive;
	  // implementation provided by this class,
	void RunL();

	void ProcessKeyPressL(TChar aChar);

//	void GetFirRegs();
//	void GetDmaReaderRegs();
//	void GetDmaWriterRegs();
//	void GetReadBufInfo();
//	void GetWriteBufInfo();
//	void GetChunkInfo();
//	void GetMiscInfo();
	void Options1();
	void Options2();

public:
	 // Data members defined by this class
	CConsoleBase*   iConsole;	// A console for reading from
	CActiveRW*      iRW;

	RCommDev* iPort;
	TBool iInit1;
	TBool iInit2;
	};


class CActiveRW : public CActive
//-----------------------------------------------------------
	{
public:
	// Construction
	CActiveRW(CConsoleBase* aConsole,RCommDev* aPort);
	static CActiveRW* NewL(CConsoleBase* aConsole,RCommDev* aPort);
	void ConstructL();

	~CActiveRW();

	void Start(TBool StartWriting);
	void Stop();
private:
	void RunL ();
	void DoCancel ();
	TBool CompareBuffers(TInt aLen);
	TInt ErrorStats();

private:
	CConsoleBase* iConsole;
	RCommDev* iPort;
	TInt iLength;
	TInt iNextXfer;
	TInt iUnrecovered;
	TInt iRxCount;
	TInt iRxErrCount;
	};

#endif  // __T_SLOWIR__
