// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// SCSCI Protocol unit test cases classes
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef __T_MS_SCSI_H__
#define __T_MS_SCSI_H__

#include <e32def.h>
#include <f32fsys.h> // for Proxy drive

#include "scsiprot.h"
#include "drivemanager.h"


/**
	test scsi transport
*/
class TTestScsiTransport : public MTransportBase
	{
public:
	TTestScsiTransport();

	// MTransportBase methods
public:
	inline virtual void SetupReadData(TPtr8& aData);
	inline virtual void SetupReadData(TUint aLength);
	inline virtual void SetupWriteData(TPtrC8& aData);
	inline virtual TInt Start();
	inline virtual TInt Stop();
	inline virtual void RegisterProtocol(MProtocolBase& aProtocol);
	inline virtual TInt BytesAvailable();

	// test specific methods / members
public:
	void InitialiseReadBuf();
	
public:
	//new
	inline  virtual TInt InitialiseTransportL(TInt aTransportLddFlag); 
	inline	virtual void GetCommandBufPtr(TPtr8& aDes, TUint aLength); 
	inline	virtual void GetReadDataBufPtr(TPtr8& aDes);
	inline	virtual void GetWriteDataBufPtr(TPtrC8& aDes); 
	inline  virtual void ProcessReadData(TAny* aAddress);

public:
	/** pointer to the protocol */
	MProtocolBase* iProtocol;
	
	/** reference to Write Data protocol buffer */
	TPtrC8 iBufWrite;

	/** reference to Read Data protocol buffer */
	TBuf8<KMaxBufSize*2> iBufRead;
	
	/**reference to Command protocol buffer */
	TBuf8<36> iBufCmd;

	/** Number of bytes available in the protocol buffer */
	TUint iReadLength;
	};


/**
	test scsi proxy drive
*/
class CTestProxyDrive : public CProxyDrive
	{
public:
	CTestProxyDrive();
	virtual ~CTestProxyDrive();
	

	virtual TInt Read(TInt64 aPos,TInt aLength,TDes8& aTrg);
	
	virtual TInt Write(TInt64 aPos,const TDesC8& aSrc);
	
	virtual TInt Caps(TDes8& anInfo);
	
	virtual TInt Initialise();
	
	
	// not implemented methods
	virtual TInt Dismounted()
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt Enlarge(TInt)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt ReduceSize(TInt, TInt)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt Read(TInt64, TInt, const TAny*, TInt, TInt)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt Write(TInt64,TInt,const TAny*,TInt,TInt)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt Format(TFormatInfo&)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt Format(TInt64,TInt)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt SetMountInfo(const TDesC8*, TInt)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt ForceRemount(TUint)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt Unlock(TMediaPassword&, TBool)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt Lock(TMediaPassword&, TMediaPassword&, TBool)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt Clear(TMediaPassword&)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt ControlIO(const RMessagePtr2&,TInt,TAny*,TAny*)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt DeleteNotify(TInt64, TInt)
		{ ASSERT(EFalse); return KErrNone; }
	virtual TInt ErasePassword()
		{ ASSERT(EFalse); return KErrNone; }

	// test specific members / methods
public:
	/** Proxy drive Caps */
	TLocalDriveCapsV4 iCaps;
	
	/** Buffer with data for reading or writing*/
	TBuf8<KMaxBufSize*2> iMediaBuf;

	};

/**
	base class to encapsulate a single SCSI protocol test
*/
class CScsiTest : public CBase
	{
	// static memebers
public:
	/** 
	Defines Lun used for testing
	*/
	static TInt TestLun;

public:
	virtual ~CScsiTest();
	static CScsiTest* NewLC();

public:
	TPtrC8& GetSenseCodePtr();
	void MountTestDrive ();
	void DismountTestDrive ();
	CMassStorageDrive* TestDrive();

	// decode command
	inline TBool DecodePacket(TPtrC8& aCmd);

private:
	CScsiTest();
	void  ConstructL();

public:
	/** 
	Drive Manager
	*/
	CDriveManager* iDriveManager;
	/** 
	SCSI protocol
	*/
	CScsiProtocol* iScsiProt;
	/** 
	Transport object
	*/
	TTestScsiTransport iTransport;
	
	/**
	Test proxy drive
	*/
	CTestProxyDrive iProxyDrive;

	/** 
	drive map 
	*/
	RArray<TInt> iDriveMap;

	};



//
//	wrapper classes
//
/** RequestSense wrapper class */
class TRequestSenseData
	{
public:
	inline TRequestSenseData(TPtrC8 aData);
	
	inline TInt8 Key();
	inline TInt8 AdditionalCode();
	inline TInt8 AdditionalCodeQualifier();
		
public:
	/** pointer to internal data */
	TPtrC8 iData;
	};

	
/** Inquiry wrapper class */
class TInquiryData
	{
public:
	inline TInquiryData(TPtrC8 aData);
	
	inline TInt8 DeviceType(); 			// 5 lower bits in 1st byte
	inline TBool RMB(); 				// high bit in 2nd byte
	inline TInt8 Version();				// 3rd byte
	inline TInt8 RespDataFrmt();		// 4th byte 
    // TBuf<8>
	inline TPtr8 VendorId();
	// TBuf<16>
	inline TPtr8 ProductId();
	// TBuf<4>
	inline TPtr8 RevisionLevel();
	inline TInt8 PeripheralQualifier();	// 3 highest bits in 1st byte
	inline TInt8 Length(); 				// length of Inquiry data
	
public:
	/** pointer to internal data */
	TPtrC8 iData;
	};


// 
// Command wrappers
// Notes: Every command wrapper has internal buffer
//		  11 or 7 bytes long. This buffer is used for 
//		  10 or 6 bytes long commands. An extra byte is 
//		  used for the size of an command (see implementation
//		  of scsi protocol).

/** Wrapper class for READ(10) WRITE(10) VERIFY(10) command */
class TReadWrite10Cmd
	{
public:
	TReadWrite10Cmd();
	
	void SetBlockAddress(TUint32 aAddress);
	inline void SetTransferLength(TUint16 aLength);
	inline void SetProtect(TUint8 aProtect);
	
	inline void SetRead();
	inline void SetWrite();
	inline void SetVerify();
	inline void SetBytChk (TBool aSet);
	
public:
	/** buffer with command data */
	TBuf8<11> iCmd;
	};

/** Wrapper class for MODE SENSE(6) command */
class TModeSenseCmd
	{
public:
	TModeSenseCmd();
	inline void SetPC (TUint8 aPc);
	inline void SetPageCode (TUint8 aPc);
	
public:
	/** buffer with command data */
	TBuf8<7> iCmd;
	};

/** Wrapper class for INQUIRY command */
class TInquiryCmd
	{
public:
	TInquiryCmd();
	
public:
	/** buffer with command data */
	TBuf8<7> iCmd;
	};

/** Wrapper class for TEST UNIT READY command */
class TTestUnitReadyCmd
	{
public:
	TTestUnitReadyCmd();
	
public:
	/** buffer with command data */
	TBuf8<7> iCmd;
	};

/** Wrapper class for PREVENT ALLOW MEDIA REMOVAL command */
class TMediaRemovalCmd
	{
public:
	TMediaRemovalCmd();
	
public:
	/** buffer with command data */
	TBuf8<7> iCmd;
	};

/** Wrapper class for READ CAPACITY command */
class TReadCapacityCmd
	{
public:
	TReadCapacityCmd();
	
public:
	/** buffer with command data */
	TBuf8<11> iCmd;
	};

/** Wrapper class for READ CAPACITY response */
class TReadCapacityResponse : public TPtrC8
	{
public:
	TReadCapacityResponse(const TDesC8 &aDes);
	
	inline TUint32 LBAddress();
	inline TUint32 BlockLength();
	};
	
#include "t_ms_scsi.inl"
	
#endif //__T_MS_SCSI_H__
