// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\d32soundsc.inl
// User side inline header file for the shared chunk sound driver.
// 
//

/**
 @file
 @publishedPartner
 @prototype
*/

inline TVersion RSoundSc::VersionRequired()
	{
	const TInt KSoundScMajorVersionNumber=1;
	const TInt KSoundScMinorVersionNumber=0;
	const TInt KSoundScBuildVersionNumber=KE32BuildVersionNumber;
	return TVersion(KSoundScMajorVersionNumber,KSoundScMinorVersionNumber,KSoundScBuildVersionNumber);
	}

#ifndef __KERNEL_MODE__

inline TInt RSoundSc::Open(TInt aUnit)
	{return(DoCreate(KDevSoundScName,VersionRequired(),aUnit,NULL,NULL,EOwnerProcess));}
	
inline void RSoundSc::Caps(TDes8& aCapsBuf)
	{DoControl(EControlGetCaps,(TAny*)&aCapsBuf);}
	
inline void RSoundSc::AudioFormat(TDes8& aFormatBuf)
	{DoControl(EControlGetAudioFormat,(TAny*)&aFormatBuf);}
	
inline TInt RSoundSc::SetAudioFormat(const TDesC8& aFormatBuf)
	{return(DoControl(EMsgControlSetAudioFormat,(TAny*)&aFormatBuf));}

inline void RSoundSc::GetBufferConfig(TDes8& aConfigBuf)
	{DoControl(EControlGetBufConfig,(TAny*)&aConfigBuf);}
 	
inline TInt RSoundSc::SetBufferChunkCreate(const TDesC8& aConfigBuf,RChunk& aChunk)
 	{return(aChunk.SetReturnedHandle(DoControl(EMsgControlSetBufChunkCreate,(TAny*)&aConfigBuf)));}
 		
inline TInt RSoundSc::SetBufferChunkOpen(const TDesC8& aConfigBuf,RChunk& aChunk)
 	{return(DoControl(EMsgControlSetBufChunkOpen,(TAny*)&aConfigBuf,(TAny*)aChunk.Handle()));}
 	
inline TInt RSoundSc::Volume()
	{return(DoControl(EControlGetVolume));}
	
inline TInt RSoundSc::SetVolume(TInt aVolume)
	{return(DoControl(EMsgControlSetVolume,(TAny*)aVolume));}
	
inline void RSoundSc::PlayData(TRequestStatus& aStatus,TInt aBufferOffset,TInt aLength,TUint aFlags)
	{
	SRequestPlayDataInfo info = {aBufferOffset,aLength,aFlags};
	DoRequest(EMsgRequestPlayData,aStatus,&info);
	}
	
inline void RSoundSc::RecordData(TRequestStatus& aStatus, TInt& aLength)
	{DoRequest(ERequestRecordData,aStatus,(TAny*)&aLength);}	

inline TInt RSoundSc::ReleaseBuffer(TInt aChunkOffset)
	{return(DoControl(EControlReleaseBuffer,(TAny*)aChunkOffset));}
		
inline void RSoundSc::CancelPlayData()
	{DoCancel(1<<EMsgRequestPlayData);}
	
inline void RSoundSc::CancelRecordData()
	{DoCancel(1<<ERequestRecordData);}	

inline void RSoundSc::Cancel(const TRequestStatus& aStatus)
	{DoControl(EMsgControlCancelSpecific,(TAny*)&aStatus);}
		
inline TInt RSoundSc::BytesTransferred()
	{return(DoControl(EControlBytesTransferred));}
	
inline void RSoundSc::ResetBytesTransferred()
	{DoControl(EControlResetBytesTransferred);}
	
inline TInt RSoundSc::Pause()
	{return(DoControl(EMsgControlPause));}
	
inline TInt RSoundSc::Resume()
	{return(DoControl(EMsgControlResume));}
	
inline void RSoundSc::NotifyChangeOfHwConfig(TRequestStatus& aStatus,TBool& aHeadsetPresent)
	{DoRequest(ERequestNotifyChangeOfHwConfig,aStatus,(TAny*)&aHeadsetPresent);}

inline void RSoundSc::CancelNotifyChangeOfHwConfig()
	{DoCancel(1<<ERequestNotifyChangeOfHwConfig);}
	
inline TInt RSoundSc::CustomConfig(TInt aFunction,TAny* aParam)
	{return(DoControl(EMsgControlCustomConfig,(TAny*)aFunction,(TAny*)aParam));}

inline TInt RSoundSc::TimePlayed(TTimeIntervalMicroSecondsBuf& aTimePlayed)
	{return(DoControl(EControlTimePlayed,(TAny*)&aTimePlayed));}

inline TInt RSoundSc::TimeRecorded(TTimeIntervalMicroSecondsBuf& aTimeRecorded)
	{return(DoControl(EControlTimeRecorded,(TAny*)&aTimeRecorded));}

#endif	// __KERNEL_MODE__

