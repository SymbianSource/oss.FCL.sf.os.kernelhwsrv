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
// TTestScsiTransport
// setup read data 
// 
//

inline void TTestScsiTransport::SetupReadData (TPtr8& aData)
	{
	__FNLOG ("TTestScsiTransport::SetupReadData");

	// copy data from the begining of a buffer, always
	iReadLength = aData.Length();
	aData.Copy (&iBufRead[0], aData.Length());
	}

/** setup write data */
inline void TTestScsiTransport::SetupWriteData (TPtrC8& aData)
	{
	__FNLOG ("TTestScsiTransport::SetupWriteData");
	iBufWrite.Set(aData);
	}

/** start a transport */
inline TInt TTestScsiTransport::Start() 
	{
	__FNLOG ("TTestScsiTransport::Start");
	// not implemented
	ASSERT(EFalse);
	return 0;
	}

/** stop a transport */
inline TInt TTestScsiTransport::Stop()
	{
	__FNLOG ("TTestScsiTransport::Stop");
	// not implemented
	ASSERT(EFalse);
	
	return 0;
	}

inline void TTestScsiTransport::RegisterProtocol(MProtocolBase& aProtocol)
	{
	__FNLOG ("TTestScsiTransport::RegisterProtocol\n");
	iProtocol = &aProtocol;
	}

inline TInt TTestScsiTransport::BytesAvailable()
	{
	__FNLOG ("TTestScsiTransport::BytesAvailable\n");
	TUint retVal = iReadLength;
	iReadLength = 0;
	return(retVal);
	}

inline TInt TTestScsiTransport::InitialiseTransportL(TInt /*aTransportLddFlag*/)
	{
	return KErrNone;
	}

inline void TTestScsiTransport::GetCommandBufPtr(TPtr8& aDes, TUint aLength)
	{
	__FNLOG ("TTestScsiTransport::GetCommandBufPtr");
	aDes.Set((TUint8*)(iBufCmd.Ptr()), aLength, aLength);
	}

inline void TTestScsiTransport::GetReadDataBufPtr(TPtr8& aDes)
	{
	__FNLOG ("TTestScsiTransport::GetReadDataBufPtr");
	aDes.Set((TUint8*)(iBufRead.Ptr()),KMaxBufSize, KMaxBufSize);
	}
	
inline void TTestScsiTransport::GetWriteDataBufPtr(TPtrC8& aDes)
	{
	__FNLOG ("TTestScsiTransport::GetWriteDataBufPtr");
	aDes.Set((TUint8*)(iBufRead.Ptr()),iReadLength);
	}

inline void TTestScsiTransport::SetupReadData(TUint aLength)
	{
	__FNLOG ("TTestScsiTransport::SetupReadData");
	iReadLength = aLength;
	}

inline void TTestScsiTransport::ProcessReadData(TAny* /*aAddress*/)
	{
	return;
	}
//
// CTestProxyDrive
//
inline TInt CTestProxyDrive::Caps(TDes8& anInfo)
	{ 
	anInfo.Copy(TLocalDriveCapsV4Buf(iCaps)); 
	return KErrNone; 
	}


/**
A private structure that, when Connected, holds a pair of references to 
the CProxyDrive and the corresponding TBusLocalDrive's Media Changed flag.
*/
struct CMassStorageDrive::CLocalDriveRef : public CBase
	{
	CLocalDriveRef(CProxyDrive& aProxyDrive, TBool& aMediaChanged)
	    : iProxyDrive(aProxyDrive), iMediaChanged(aMediaChanged)
		{
		}
	CProxyDrive& iProxyDrive;
	TBool& iMediaChanged;
	TDriveState iDriveState;	
};


//
// CScsiTest
//
inline TBool CScsiTest::DecodePacket(TPtrC8& aCmd)
	{ 
	return iScsiProt->DecodePacket(aCmd, TestLun); 
	}




//
// command wrappers
//
TRequestSenseData::TRequestSenseData(TPtrC8 aData)
: iData(aData)
// SCSI-2 spec. p.136
	{ 
	ASSERT((iData.Length() >= 17) && (iData[7] + 8 == iData.Length())); 
	}
	
inline TInt8 TRequestSenseData::Key()
	{ 
	return static_cast<TInt8>(iData[2] & 0x0F); 
	}
		
inline TInt8 TRequestSenseData::AdditionalCode()
	{ 
	return iData[12]; 
	}
		
inline TInt8 TRequestSenseData::AdditionalCodeQualifier()
	{ 
	return iData[13]; 
	}


//
// class TInquiryData
//
TInquiryData::TInquiryData(TPtrC8 aData)
	{ 
	iData.Set(aData);
	}
	
inline TInt8 TInquiryData::DeviceType() 	// 5 lower bits in 1st byte
	{ 
	return static_cast<TInt8>(iData[0] & 0x1F); 
	}
		
inline TBool TInquiryData::RMB() 			// high bit in 2nd byte
	{ 
	return ((iData[1] & 0x80) == 0x80); 
	}
		
inline TInt8 TInquiryData::Version()		// 3rd byte
	{ 
	return iData[2]; 
	}
	
inline TInt8 TInquiryData::RespDataFrmt()	// 4th byte 
	{ 
	return (iData[3]); 
	}
	
inline TPtr8 TInquiryData::VendorId()
	{ 
	return TPtr8(const_cast<TUint8*>(&iData[8]), 8, 8); 
	}
	
inline TPtr8 TInquiryData::ProductId()
	{ 
	return TPtr8(const_cast<TUint8*>(&iData[16]), 16, 16); 
	}

inline TPtr8 TInquiryData::RevisionLevel()
	{ 
	return TPtr8(const_cast<TUint8*>(&iData[32]), 4, 4); 
	}
		
inline TInt8 TInquiryData::PeripheralQualifier()	// 3 highest bits in 1st byte
	{ 
	return static_cast<TInt8>((iData[0] & 0xE0) >> 5); 
	}

inline TInt8 TInquiryData::Length()			// length of Inquiry data
	{ 
	return static_cast<TInt8>(iData[4] + 4); 
	}

//
// class TReadWrite10Cmd
//
/** set transfer length */
inline void TReadWrite10Cmd::SetTransferLength(TUint16 aLength)
	{
	iCmd[8] = static_cast<TInt8>((aLength & 0xFF00) >> 8);
	iCmd[9] = static_cast<TInt8>(aLength & 0x00FF);
	}

/** set protect flag */
inline void TReadWrite10Cmd::SetProtect(TUint8 aProtect)
	{
	// Note: used only 3 lower bits
	//			clear flag    and set a new one
	iCmd[2] = static_cast<TInt8>((iCmd[2] & 0x1F) | (aProtect << 5));
	}

/** setup read command */
inline void TReadWrite10Cmd::SetRead()
	{
	iCmd[1] = 0x28; // READ(10) cmd
	}

/** setup write command */
inline void TReadWrite10Cmd::SetWrite()
	{
	iCmd[1] = 0x2A; // WRITE(10) cmd
	}

/** setup verify command */
inline void TReadWrite10Cmd::SetVerify()
	{
	iCmd[1] = 0x2F; // VERIFY(10) cmd
	}

/** set BYTCHK fiels. Valid for VERIFY10 cmd only */
inline void TReadWrite10Cmd::SetBytChk (TBool aSet)
	{
	if (0x2F == iCmd[1])	// if VERIFY(10)
		{
		iCmd[2] = static_cast<TInt8>((iCmd[2] & 0xFD) | (aSet << 1));
		}
	}

/** c'or */
inline TModeSenseCmd::TModeSenseCmd()
	{
	iCmd.FillZ (iCmd.MaxLength());
	
	iCmd[0] = static_cast<TInt8>(iCmd.MaxLength()-1);	// size of the Cmd
	iCmd[1] = 0x1A;
	}

/** set PC, 2 bits field */
inline void TModeSenseCmd::SetPC (TUint8 aPc)
	{
	iCmd[3] = static_cast<TInt8>((iCmd[3] & 0x3F) | (aPc << 6));
	}
	
/** set page code, 6 bits field */
inline void TModeSenseCmd::SetPageCode (TUint8 aPageCode)
	{
	iCmd[3] = static_cast<TInt8>((iCmd[3] & 0xC0) | aPageCode);
	}


inline TInquiryCmd::TInquiryCmd()
	{
	iCmd.FillZ (iCmd.MaxLength());
	iCmd[0] = static_cast<TInt8>(iCmd.MaxLength()-1);	// size of a command itself
	iCmd[1] = 0x12; 				// inquiry operation code
	iCmd[5] = 36;					// min required allocation length
	}

inline TTestUnitReadyCmd::TTestUnitReadyCmd()
	{
	iCmd.FillZ (iCmd.MaxLength());
	iCmd[0] = static_cast<TInt8>(iCmd.MaxLength()-1);	// size of a command itself
	iCmd[1] = 0x00; 				// TEST UNIT READY command
	}
	
inline TMediaRemovalCmd::TMediaRemovalCmd()
	{
	iCmd.FillZ (iCmd.MaxLength());
	iCmd[0] = static_cast<TInt8>(iCmd.MaxLength()-1);	// size of a command itself
	iCmd[1] = 0x1E; 				// PREVENT ALLOW MEDIUM REMOVAL command
	}
	
inline TReadCapacityCmd::TReadCapacityCmd()
	{
	iCmd.FillZ (iCmd.MaxLength());
	iCmd[0] = static_cast<TInt8>(iCmd.MaxLength()-1);	// size of a command itself
	iCmd[1] = 0x25; 				// READ CAPACITY command
	}

inline TReadCapacityResponse::TReadCapacityResponse(const TDesC8 &aDes)
	: TPtrC8(aDes)
	{
	}
	
/** accessor */
inline TUint32 TReadCapacityResponse::LBAddress()
	{
	return TUint32(AtC(0)<<24) | (AtC(1)<<16) | AtC(2)<<8 | AtC(3);
	}
	
/** accessor */
inline TUint32 TReadCapacityResponse::BlockLength()
	{
	return TUint32(AtC(4)<<24) | (AtC(5)<<16) | AtC(6)<<8 | AtC(7);
	}

