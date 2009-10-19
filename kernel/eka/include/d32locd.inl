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
// e32\include\d32locd.inl
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without noticed. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

// class RLocalDrive
inline TVersion RLocalDrive::VersionRequired() const
	{ return TVersion(KLocalDriveMajorVersion,KLocalDriveMinorVersion,KLocalDriveBuildVersion); }
/**
@capability TCB
*/
inline TInt RLocalDrive::Connect(TInt aDriveNumber, TBool& aChangedFlag)
	{ return DoCreate(KLitLocalDriveLddName,VersionRequired(),aDriveNumber,NULL,(const TDesC8*)&aChangedFlag,EOwnerProcess); }
inline TInt RLocalDrive::Enlarge(TInt aLength)
	{ return DoControl(EControlEnlarge, (TAny*)aLength); }
inline TInt RLocalDrive::Reduce(TInt aPos, TInt aLength)
	{ return DoControl(EControlReduce, (TAny*)aPos, (TAny*)aLength); }
inline TInt RLocalDrive::Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt aMessageHandle, TInt aOffset, TInt aFlags)
	{ TLocalDriveMessageData d(aPos,aLength,aTrg,aMessageHandle,aOffset,aFlags); return DoControl(EControlRead, &d); }
inline TInt RLocalDrive::Read(TInt64 aPos, TInt aLength, const TAny* aTrg, TInt aMessageHandle, TInt anOffset)
	{ TLocalDriveMessageData d(aPos,aLength,aTrg,aMessageHandle,anOffset,0); return DoControl(EControlRead, &d); }
inline TInt RLocalDrive::Read(TInt64 aPos, TInt aLength, TDes8& aTrg)
	{ TLocalDriveMessageData d(aPos,aLength,&aTrg,KLocalMessageHandle,0,ELocDrvMetaData); return DoControl(EControlRead, &d); }
inline TInt RLocalDrive::Write(TInt64 aPos, TInt aLength, const TAny* aSrc, TInt aMessageHandle, TInt aOffset, TInt aFlags)
	{ TLocalDriveMessageData d(aPos,aLength,aSrc,aMessageHandle,aOffset,aFlags); return DoControl(EControlWrite, &d); }
inline TInt RLocalDrive::Write(TInt64 aPos, TInt aLength, const TAny* aSrc, TInt aMessageHandle, TInt anOffset)
	{ TLocalDriveMessageData d(aPos,aLength,aSrc,aMessageHandle,anOffset,0); return DoControl(EControlWrite, &d); }
inline TInt RLocalDrive::Write(TInt64 aPos, const TDesC8& aSrc)
	{ TLocalDriveMessageData d(aPos,aSrc.Length(),&aSrc,KLocalMessageHandle,0,ELocDrvMetaData); return DoControl(EControlWrite, &d); }
inline TInt RLocalDrive::Caps(TDes8& anInfo)
	{ return DoControl(EControlCaps, &anInfo); }
inline TInt RLocalDrive::Format(TInt64 aPos, TInt aLength)
	{ TLocalDriveMessageData d(aPos,aLength,NULL,KLocalMessageHandle,0,0); return DoControl(EControlFormat, &d); }
inline TInt RLocalDrive::ForceMediaChange(TInt aMode)
	{ return DoControl(EControlForceMediaChange, (TAny*)aMode); }
inline void RLocalDrive::NotifyChange(TRequestStatus* aStatus)
	{ *aStatus=KRequestPending; DoControl(EControlNotifyChange, aStatus); }
inline void RLocalDrive::NotifyChangeCancel()
	{ DoControl(EControlNotifyChangeCancel); }
inline TMediaDevice RLocalDrive::MediaDevice()
	{ return (TMediaDevice)DoControl(EControlMediaDevice); }
inline TInt RLocalDrive::SetMountInfo(const TDesC8* aInfo,TInt aMessageHandle)
	{ TLocalDriveMessageData d(0,0,aInfo,aMessageHandle,0,0); return DoControl(EControlSetMountInfo, &d); }
inline TInt RLocalDrive::IsRemovable(TInt& aSocketNum)
	{ return DoControl(EControlIsRemovable,&aSocketNum); }
inline TInt RLocalDrive::ControlIO(TInt aCommand, TAny* aParam1, TAny* aParam2)
	{ TLocalDriveControlIOData d(aCommand,aParam1,aParam2,KLocalMessageHandle); return DoControl(EControlControlIO,&d); }
inline TInt RLocalDrive::ControlIO(TInt aCommand, TDes8& aBuf, TInt aParam)
	{ TLocalDriveControlIOData d(aCommand, (TUint8*) aBuf.Ptr(), aParam, aBuf.MaxLength()); return DoControl(EControlControlIO,&d); }
inline TInt RLocalDrive::ControlIO(TInt aCommand, TDesC8& aBuf, TInt aParam)
	{ TLocalDriveControlIOData d(aCommand, (TUint8*) aBuf.Ptr(), aParam, aBuf.Length()); return DoControl(EControlControlIO,&d); }
inline TInt RLocalDrive::ControlIO(TInt aCommand, TInt aParam1, TInt aParam2)
	{ TLocalDriveControlIOData d(aCommand,(TAny*) aParam1,(TAny*) aParam2,0); return DoControl(EControlControlIO,&d); }


// RLocalDrive Password Control (Set/Lock/Unlock)
inline TInt RLocalDrive::Unlock(const TDesC8& aPassword, TBool aStorePassword)
	{ TLocalDrivePasswordData d((TDesC8&)aPassword, (TDesC8&)aPassword, aStorePassword); return DoControl(EControlPasswordUnlock, &d); }
inline TInt RLocalDrive::SetPassword(const TDesC8& aOldPassword, const TDesC8& aNewPassword, TBool aStorePassword)
	{ TLocalDrivePasswordData d((TDesC8&)aOldPassword, (TDesC8&)aNewPassword, aStorePassword); return DoControl(EControlPasswordLock, &d); }
inline TInt RLocalDrive::Clear(const TDesC8& aPassword)
	{ TLocalDrivePasswordData d((TDesC8&)aPassword, (TDesC8&)aPassword, EFalse); return DoControl(EControlPasswordClear, &d); }
inline TInt RLocalDrive::ErasePassword()
	{ return DoControl(EControlPasswordErase); }

// RLocalDrive Password Store Control (Read/Write/Length)
inline TInt RLocalDrive::ReadPasswordData(TDesC8& aStoreData)
	{ return DoControl(EControlReadPasswordStore, (TDesC8*)&aStoreData); }
inline TInt RLocalDrive::WritePasswordData(const TDesC8& aStoreData)
	{ return DoControl(EControlWritePasswordStore, (TDesC8*)&aStoreData); }
inline TInt RLocalDrive::PasswordStoreLengthInBytes()
	{ TInt length=0; return DoControl(EControlPasswordStoreLengthInBytes, (TAny*)&length)==KErrNone?length:0; }

inline TInt RLocalDrive::DeleteNotify(TInt64 aPos, TInt aLength)
	{ TLocalDriveMessageData d(aPos,aLength,0,KLocalMessageHandle,0,0); return DoControl(EControlDeleteNotify, &d); }

// Get Last Error Info
inline TInt RLocalDrive::GetLastErrorInfo(TDesC8& aErrorInfo)
	{ return DoControl(EControlGetLastErrorInfo, (TDesC8*)&aErrorInfo); }


inline TInt RLocalDrive::QueryDevice(TQueryDevice aQueryDevice, TDes8 &aBuf)
	{ return DoControl(EControlQueryDevice, (TAny*) aQueryDevice, &aBuf); }

inline TLDFormatInfo::TLDFormatInfo()
                     :iCapacity(0),iSectorsPerCluster(0),iSectorsPerTrack(0),iNumberOfSides(0),iFATBits(EFBDontCare),
                     iReservedSectors(0), iFlags(0), iPad(0)
    {
    }

inline TInt64 TLocalDriveCapsV4::MediaSizeInBytes()
	{
	//
	// Return the actual size of the media (as opposed to the partition size)
	//
	// Note : We calculate including iNumPagesPerBlock to maintain compatibility with NAND flash
	//		  where iNumberOfSectors == iNumOfBlocks and iSectorSizeInBytes == iNumBytesMain.
	//		  The local media subsystem sets a default value of iNumPagesPerBlock == 1.
	//
	return(TInt64(iNumberOfSectors) * iSectorSizeInBytes * iNumPagesPerBlock);
	}
