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
// e32test\device\t_usb.h
// 
//

#ifndef __T_USB_H__
#define __T_USB_H__

#include <e32cons.h>
#include <e32svr.h>
#include <e32std.h>
#include <f32file.h>

#include <d32usbc.h>
#include <d32otgdi.h>


static const TInt KMaxBufSize = 1024 * 1024;				// max data buffer size: 1MB
static const TInt KPreambleLength = 8;						// length of preamble data (bytes)


enum TXferMode
	{
	ENone,
	ELoop,
	ELoopComp,
	EReceiveOnly,
	ETransmitOnly
	};


class CActiveRW;
class CActiveStallNotifier;
class CActiveDeviceStateNotifier;

class CActiveConsole : public CActive
	{
public:
	static CActiveConsole* NewLC(CConsoleBase* aConsole, TBool aVerboseOutput);
	static CActiveConsole* NewL(CConsoleBase* aConsole, TBool aVerboseOutput);
	void ConstructL();
	~CActiveConsole();
	TInt SetupInterface();
	void RequestCharacter();

private:
	CActiveConsole(CConsoleBase* aConsole, TBool aVerboseOutput);
	// Defined as pure virtual by CActive;
	// implementation provided by this class.
	virtual void DoCancel();
	// Defined as pure virtual by CActive;
	// implementation provided by this class,
	virtual void RunL();
	void ProcessKeyPressL(TChar aChar);
	TInt QueryUsbClientL();
	void AllocateEndpointDMA(TEndpointNumber aEndpoint);
	void AllocateDoubleBuffering(TEndpointNumber aEndpoint);
	void DeAllocateEndpointDMA(TEndpointNumber aEndpoint);
	void DeAllocateDoubleBuffering(TEndpointNumber aEndpoint);
	void QueryEndpointState(TEndpointNumber aEndpoint);
#ifdef WITH_DUMP_OPTION
	void QueryRxBuffer();
#endif
	TInt SetupDescriptors();
	TInt ReEnumerate();

private:
	CConsoleBase* iConsole;									// a console to read from
	CActiveRW* iRW;											// the USB read/write active object
	CActiveStallNotifier* iStallNotifier;
	CActiveDeviceStateNotifier* iDeviceStateNotifier;
	RDevUsbcClient iPort;
	RUsbOtgDriver iOtgPort;
	TBool iBufferSizeChosen;
	TBool iBandwidthPriorityChosen;
	TBool iDMAChosen;
	TBool iAllocateDma;
	TBool iDoubleBufferingChosen;
	TBool iAllocateDoubleBuffering;
	TUint32 iBandwidthPriority;
	TBool iSoftwareConnect;
	TBool iHighSpeed;
	TBool iResourceAllocationV2;
	TBool iOtg;
	TBool iVerbose;
	};


class CActiveTimer;

class CActiveRW : public CActive
	{
public:
	enum TXferType
		{
		ENone,
		EPreamble,
		EReadXfer,
		EWriteXfer
		};
	static CActiveRW* NewL(CConsoleBase* aConsole, RDevUsbcClient* aPort, TBool aVerboseOutput);
	~CActiveRW();
	TInt ExchangeVersions();
	void SendPreamble();
	void SendData();
	void ReadData();
	void Stop();
	void SetMaxBufSize(TInt aBufSz);
	void SetMaxPacketSize(TInt aPktSz);
	TInt MaxBufSize() const;
	void SetTransferMode(TXferMode aMode);
	TInt WriteToDisk(TBool aEnable);
	TInt ReadFromDisk(TBool aEnable);

private:
	CActiveRW(CConsoleBase* aConsole, RDevUsbcClient* aPort, TBool aVerboseOutput);
	void ConstructL();
	virtual void RunL();
	virtual void DoCancel();
	TInt SendVersion();
	TInt ReceiveVersion();
	TBool CompareBuffers(TInt aLen);
	TInt SelectDrive();
	void WriteBufferToDisk(TDes8& aBuffer, TInt aLen);
	void ReadBufferFromDisk(TDes8& aBuffer, TInt aLen);

private:
	TBuf8<KPreambleLength> iPreambleBuf;				// 2 bytes transfer length + stuffing
	TBuf8<KMaxBufSize> iWriteBuf;
	TBuf8<KMaxBufSize> iReadBuf;
	CConsoleBase* iConsole;
	RDevUsbcClient* iPort;
	CActiveTimer* iTimeoutTimer;
	TInt iBufSz;
	TInt iMaxBufSz;
	TInt iMaxPktSz;
	TXferType iCurrentXfer;
	TXferMode iXferMode;
	TBool iDoStop;
	TUint32 iPktNum;
	TBool iVerbose;
	TBool iDiskAccessEnabled;
	RFs iFs;
	RFile iFile;
	TFileName iFileName;
	TInt iFileOffset;
	};


class CActiveStallNotifier : public CActive
	{
public:
	static CActiveStallNotifier* NewL(CConsoleBase* aConsole, RDevUsbcClient* aPort, TBool aVerboseOutput);
	~CActiveStallNotifier();
	void Activate();

private:
	CActiveStallNotifier(CConsoleBase* aConsole, RDevUsbcClient* aPort, TBool aVerboseOutput);
	void ConstructL();
	virtual void DoCancel();
	virtual void RunL();

private:
	CConsoleBase* iConsole;
	RDevUsbcClient* iPort;
	TUint iEndpointState;
	TBool iVerbose;
	};


class CActiveDeviceStateNotifier : public CActive
	{
public:
	static CActiveDeviceStateNotifier* NewL(CConsoleBase* aConsole, RDevUsbcClient* aPort, TBool aVerboseOutput);
	~CActiveDeviceStateNotifier();
	void Activate();

private:
	CActiveDeviceStateNotifier(CConsoleBase* aConsole, RDevUsbcClient* aPort, TBool aVerboseOutput);
	void ConstructL();
	virtual void DoCancel();
	virtual void RunL();

private:
	CConsoleBase* iConsole;
	RDevUsbcClient* iPort;
	TUint iDeviceState;
	TBool iVerbose;
	};


class CActiveTimer : public CActive
	{
public:
	static CActiveTimer* NewL(CConsoleBase* aConsole, RDevUsbcClient* aPort, TBool aVerboseOutput);
	~CActiveTimer();
	void Activate(TTimeIntervalMicroSeconds32 aDelay);

private:
	CActiveTimer(CConsoleBase* aConsole, RDevUsbcClient* aPort, TBool aVerboseOutput);
	void ConstructL();
	virtual void DoCancel();
	virtual void RunL();

private:
	CConsoleBase* iConsole;
	RDevUsbcClient* iPort;
	RTimer iTimer;
	TBool iVerbose;
	};


//
// Helpful Defines
//

#define TUSB_PRINT(string) \
		do { \
		iConsole->Printf(_L(string)); \
		iConsole->Printf(_L("\n")); \
		} while (0)

#define TUSB_PRINT1(string, a) \
		do { \
		iConsole->Printf(_L(string), (a)); \
		iConsole->Printf(_L("\n")); \
		} while (0)

#define TUSB_PRINT2(string, a, b) \
		do { \
		iConsole->Printf(_L(string), (a), (b)); \
		iConsole->Printf(_L("\n")); \
		} while (0)

#define TUSB_PRINT3(string, a, b, c) \
		do { \
		iConsole->Printf(_L(string), (a), (b), (c)); \
		iConsole->Printf(_L("\n")); \
		} while (0)

#define TUSB_PRINT5(string, a, b, c, d, e) \
		do { \
		iConsole->Printf(_L(string), (a), (b), (c), (d), (e)); \
		iConsole->Printf(_L("\n")); \
		} while (0)

#define TUSB_PRINT6(string, a, b, c, d, e, f) \
		do { \
		iConsole->Printf(_L(string), (a), (b), (c), (d), (e), (f)); \
		iConsole->Printf(_L("\n")); \
		} while (0)

#define TUSB_VERBOSE_PRINT(string) \
		do { \
		if (iVerbose) \
			{ \
			TUSB_PRINT(string); \
			} \
		} while (0)

#define TUSB_VERBOSE_PRINT1(string, a) \
		do { \
		if (iVerbose) \
			{ \
			TUSB_PRINT1(string, a); \
			} \
		} while (0)

#define TUSB_VERBOSE_PRINT2(string, a, b) \
		do { \
		if (iVerbose) \
			{ \
			TUSB_PRINT2(string, a, b); \
			} \
		} while (0)


#endif	// __T_USB_H__
