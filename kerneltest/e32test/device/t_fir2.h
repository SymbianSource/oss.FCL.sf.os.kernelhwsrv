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
// e32test\device\t_fir2.h
// 
//

//#include <es_sock.h>
#include <e32test.h>
#include <e32cons.h>
#include <e32svr.h> 
#include <e32std.h> 
#include <d32comm.h>
#include <d32fir.h>

#ifndef __T_FIR2__
#define __T_FIR2__ 

#define PDD_NAME _L("difiba")
#define LDD_NAME _L("efir")


enum TXferType
	{
    EWriteXfer,
    EReadXfer
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

	void GetFirRegs();
	void GetDmaReaderRegs();
	void GetDmaWriterRegs();
	void GetReadBufInfo();
	void GetWriteBufInfo();
	void GetChunkInfo();
	void GetMiscInfo();
	void Options1();
	void Options2();
	void Options3();

public:
	 // Data members defined by this class
	CConsoleBase*   iConsole;	// A console for reading from
	CActiveRW*      iRW;

	RDevFir iPort;
	TBool iInit1;
	TBool iInit2;
	TBool iInit3;
	TBps iBaudRate;
	};


class CActiveRW : public CActive
//-----------------------------------------------------------
	{
public:
	// Construction
	CActiveRW(CConsoleBase* aConsole,RDevFir* aPort);
	static CActiveRW* NewL(CConsoleBase* aConsole,RDevFir* aPort);
	void ConstructL();

	~CActiveRW();

	void Start(TBool StartWriting);
	void Stop();
private:
	void RunL ();
	void DoCancel ();


private:
	CConsoleBase* iConsole;
	RDevFir* iPort;
	TInt iLength;
	TInt iNextXfer;
	};

#endif  // __T_FIR2__
