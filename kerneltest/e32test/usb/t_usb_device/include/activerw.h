// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test/usb/t_usb_device/include/activerw.h
// 
//

#ifndef __ACTIVERW_H__
#define __ACTIVERW_H__


class CActiveTimer;

class CActiveRW : public CActive
	{
public:
	static CActiveRW* NewL(CConsoleBase* aConsole, RDEVCLIENT* aPort, RFs aFs, TUint16 aIndex, TBool aLastSetting);
	~CActiveRW();
	void SendData();
	TUint ReadData();
	void Stop();
	void SetTestParams(TestParamPtr aTpPtr);
	void SetTransferMode(TXferMode aMode);
	TInt WriteToDisk(TChar aDriveLetter);
	TInt ReadFromDisk(TChar aDriveLetter, TInt aMaxFileSize);
	void TestComplete(TBool aResult);
	void Suspend(TXferType aType);
	void Resume();
	void ResumeAltSetting(TUint aAltSetting);
	void SendWaitSetting();
	void StartOrSuspend();
	inline RDEVCLIENT* Ldd();
			
private:
	CActiveRW(CConsoleBase* aConsole, RDEVCLIENT* aPort, RFs aFs, TUint16 aIndex, TBool aLastSetting);
	void ConstructL();
	virtual void RunL();
	virtual void DoCancel();
	TBool CompareBuffers();
	void WriteBufferToDisk(TDes8& aBuffer, TInt aLen);
	void ReadBufferFromDisk(TDes8& aBuffer, TInt aLen);
	void ProcessWriteXfer();
	void ProcessReadXfer();
	void CActiveRW::Yield();

private:
	#ifdef USB_SC
	TEndpointBuffer iSCWriteBuf;
	TEndpointBuffer iSCReadBuf;
	TAny * iSCReadData;
	TBool iReadZlp;
	#else
	TPtr8 iWriteBuf;
	TPtr8 iReadBuf;
	#endif
	CConsoleBase* iConsole;
	RDEVCLIENT* iPort;
	CActiveTimer* iTimeoutTimer;
	TUint iBufSz;
	TUint iMaxPktSz;
	TUint iReadSize;
	TUint iReadOffset;
	TXferType iCurrentXfer;
	TXferMode iXferMode;
	TBool iDoStop;
	TUint32 iPktNum;
	TBool iDiskAccessEnabled;
	RFs iFs;
	RFile iFile;
	TFileName iFileName;
	TInt iFileOffset;
	TInt16 iRepeat;
	TestParamType iTestParams;
	TBool iComplete;
	TBool iResult;
	TUint16 iIndex;
	TBool iLastSetting;
	};

inline RDEVCLIENT* CActiveRW::Ldd()
	{
	return iPort;
	}

#endif	// __ACTIVERW_H__
