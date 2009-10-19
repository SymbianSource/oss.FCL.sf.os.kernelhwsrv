// Copyright (c) 1998-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\d32soundsc.h
// User side class definition for the shared chunk sound driver.
// 
//

/**
 @file
 @publishedPartner
 @prototype
*/

#ifndef __D32SOUNDSC_H__
#define __D32SOUNDSC_H__

#include <e32cmn.h>
#include <e32ver.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KDevSoundScName,"SoundSc");

const TInt KSoundScTxUnit0=0;
const TInt KSoundScTxUnit1=1;
/**
@capability UserEnvironment
@ref RSoundSc::Open An RSoundSc may not be opened for record without UserEnvironment
*/
const TInt KSoundScRxUnit0=4;
/**
@capability UserEnvironment
@ref RSoundSc::Open An RSoundSc may not be opened for record without UserEnvironment
*/
const TInt KSoundScRxUnit1=5;

// Note that the implementation in the sound driver LDD to calculate a default audio configuration assumes a 
// monotonic ordering of all caps bitfields. If it is necessary to break this ordering then this implementation
// will need to be reviewed and possibly changed.

/**
Sound driver capability constants - bitmasks of the audio channel configurations supported
by a unit. @see TSoundFormatsSupportedV02.
*/
/** The unit supports Mono (i.e. a single audio channel). */
const TUint KSoundMonoChannel = 0x0001;
/** The unit supports Stereo (i.e. two audio channels). */
const TUint KSoundStereoChannel = 0x0002;
/** The unit supports three audio channels. */
const TUint KSoundThreeChannel = 0x0004;
/** The unit supports four audio channels. */
const TUint KSoundFourChannel = 0x0008;
/** The unit supports five audio channels. */
const TUint KSoundFiveChannel = 0x0010;
/** The unit supports six audio channels. */
const TUint KSoundSixChannel = 0x0020;

/**
Sound driver capability constants - bitmasks of supported sample rates. @see TSoundFormatsSupportedV02.
*/
/** The device supports the 7.35KHz sample rate. */
const TUint KSoundRate7350Hz = 0x0001;
/** The device supports the 8KHz sample rate. */
const TUint KSoundRate8000Hz = 0x0002;
/** The device supports the 8.82KHz sample rate. */
const TUint KSoundRate8820Hz = 0x0004;
/** The device supports the 9.6KHz sample rate. */
const TUint KSoundRate9600Hz = 0x0008;
/** The device supports the 11.025KHz sample rate. */
const TUint KSoundRate11025Hz = 0x0010;
/** The device supports the 12KHz sample rate. */
const TUint KSoundRate12000Hz = 0x0020;
/** The device supports the 14.7KHz sample rate. */
const TUint KSoundRate14700Hz = 0x0040;
/** The device supports the 16KHz sample rate. */
const TUint KSoundRate16000Hz = 0x0080;
/** The device supports the 22.05KHz sample rate. */
const TUint KSoundRate22050Hz = 0x0100;
/** The device supports the 24KHz sample rate. */
const TUint KSoundRate24000Hz = 0x0200;
/** The device supports the 29.4KHz sample rate. */
const TUint KSoundRate29400Hz = 0x0400;
/** The device supports the 32KHz sample rate. */
const TUint KSoundRate32000Hz = 0x0800;
/** The device supports the 44.1KHz sample rate. */
const TUint KSoundRate44100Hz = 0x1000;
/** The device supports the 48KHz sample rate. */
const TUint KSoundRate48000Hz = 0x2000;

/**
Sound driver capability constants - bitmasks of supported encoding formats. @see TSoundFormatsSupportedV02.
*/
/** The device supports 8bit PCM encoding format. */
const TUint KSoundEncoding8BitPCM = 0x0001;
/** The device supports 16bit PCM encoding format. */
const TUint KSoundEncoding16BitPCM = 0x0002;
/** The device supports 24bit PCM encoding format. */
const TUint KSoundEncoding24BitPCM = 0x0004;

/**
Sound driver capability constants - bitmasks of the data formats supported
by the device. @see TSoundFormatsSupportedV02.
*/
/** The device supports an interleaved data format (i.e. data for each channel is interleaved throughout
each data buffer). */
const TUint KSoundDataFormatInterleaved = 0x0001;
/** The device supports a non-interleaved data format (i.e. data for each channel is arranged in
contiguous regions within each data buffer). */
const TUint KSoundDataFormatNonInterleaved = 0x0002;

/** The maximum setting possible for the record level / volume. */
const TInt KSoundMaxVolume = 255;

/**
Enumeration of possible data transfer directions. @see TSoundFormatsSupportedV02.
*/
enum TSoundDirection
	{
	/**	A unit for recording data. */
	ESoundDirRecord,
	/** A unit for playing data. */
	ESoundDirPlayback
	};

/**
Enumeration of possible sample rates. @see TCurrentSoundFormatV02.
*/	
enum TSoundRate
	{
	/** 7.35KHz sample rate. */
	ESoundRate7350Hz,
	/** 8KHz sample rate. */
	ESoundRate8000Hz,
	/** 8.82KHz sample rate. */
	ESoundRate8820Hz,
	/** 9.6KHz sample rate. */
	ESoundRate9600Hz,
	/** 11.025KHz sample rate. */
	ESoundRate11025Hz,
	/** 12KHz sample rate. */
	ESoundRate12000Hz,
	/** 14.7KHz sample rate. */
	ESoundRate14700Hz,
	/** 16KHz sample rate. */
	ESoundRate16000Hz,
	/** 22.05KHz sample rate. */
	ESoundRate22050Hz,
	/** 24KHz sample rate. */
	ESoundRate24000Hz,
	/** 29.4KHz sample rate. */
	ESoundRate29400Hz,
	/** 32KHz sample rate. */
	ESoundRate32000Hz,
	/** 44.1KHz sample rate. */
	ESoundRate44100Hz,
	/** 48KHz sample rate. */
	ESoundRate48000Hz
	};

/**
Enumeration of possible sound encoding formats. @see TCurrentSoundFormatV02.
*/
enum TSoundEncoding
	{
	/** 8bit PCM encoding format. */
	ESoundEncoding8BitPCM,
	/** 16bit PCM encoding format. */
	ESoundEncoding16BitPCM,
	/** 24bit PCM encoding format. */
	ESoundEncoding24BitPCM
	};
	
/**
Enumeration of possible sound data formats. @see TCurrentSoundFormatV02.
*/
enum TSoundDataFormat
	{
	/** The device supports an interleaved data format. */ 
	ESoundDataFormatInterleaved,
	/** The device supports a non-interleaved data format. */
	ESoundDataFormatNonInterleaved
	};	
	
/**
The general driver capabilites class - returned by the LDD factory in response to RDevice::GetCaps().
*/
class TCapsSoundScV01
	{
public:
	TVersion iVersion;
	};

/**
The main audio capabilities class. This is used by the LDD to get either the play or record capabilities
of a particular sound device once a channel to it has been opened.
*/
class TSoundFormatsSupportedV02
	{
public:
	/** The data transfer direction for this unit: play or record. @see TSoundDirection. */
	TSoundDirection iDirection;
	/** The audio channel configurations supported by this unit - a bitfield. */
	TUint32 iChannels;
	/** The sample rates supported - a bitfield. */
	TUint32 iRates;
	/** The encoding formats supported - a bitfield. */
	TUint32 iEncodings;
	/** The data formats supported - a bitfield. */
	TUint32 iDataFormats;
	/** The minimum request size that the device can support. All requests to play or record data must be of a
	length that is a multiple of this value. */
	TInt iRequestMinSize;
	/** The logarithm to base 2 of the alignment required for request arguments. All requests to play or
	record data must specify locations in the shared chunk which conform to this alignment.
	For example, iRequestAlignment of 1 is 2 byte aligned (2^1), 2 is 4 byte aligned (2^2) etc. */
	TInt iRequestAlignment;
	/** Indicates whether this unit is capable of detecting changes in its hardware configuration. */
	TBool iHwConfigNotificationSupport;
	/** Reserved field. */
	TInt iReserved1;
	};
typedef TPckgBuf<TSoundFormatsSupportedV02> TSoundFormatsSupportedV02Buf;

/**
The sound format configuration class. This is used to get and set the current configuration
of the sound device (both for playing and recording).
*/
class TCurrentSoundFormatV02
	{
public:
	/** The audio channel configuration: 1=Mono, 2=Stereo etc. */ 
	TInt iChannels;
	/** The sample rate. @see TSoundRate. */
	TSoundRate iRate;
	/** The encoding format. @see TSoundEncoding. */
	TSoundEncoding iEncoding;
	/** The data format: interleaved, non-interleaved etc. @see TSoundDataFormat. */
	TSoundDataFormat iDataFormat;
	/** Reserved field. */
	TInt iReserved1;
	};
typedef TPckgBuf<TCurrentSoundFormatV02> TCurrentSoundFormatV02Buf;

/**
A flag that can be passed via the 'aFlags' argument of the function RSoundSc::PlayData(). This being
set signifies that this particular play request is the last one of a series (ie EOF) and therefore an underflow is
expected after this request completes.
*/
const TUint KSndFlagLastSample=0x00000001;

/** Reserved settings used with CustomConfig(). */
const TInt KSndCustomConfigMaxReserved=0x0FFFFFFF;
#ifdef _DEBUG
const TInt KSndCustom_ForceHwConfigNotifSupported=0x00;
const TInt KSndCustom_CompleteChangeOfHwConfig=0x01;
const TInt KSndCustom_ForceStartTransferError=0x02;
const TInt KSndCustom_ForceTransferDataError=0x03;
const TInt KSndCustom_ForceTransferTimeout=0x04;
#endif

/** A structure used to assemble arguments for the function RSoundSc::PlayData() and to pass these to the driver. */
struct SRequestPlayDataInfo
	{
	TInt iBufferOffset;
	TInt iLength;
	TUint iFlags;
	};	

#ifndef __KERNEL_MODE__
typedef TPckgBuf<TTimeIntervalMicroSeconds> TTimeIntervalMicroSecondsBuf;
#endif

class RSoundSc : public RBusLogicalChannel
	{
private:	
	enum TRequest
	/**
	 Asynchronous request types
	 */
		{
		EMsgRequestPlayData,
		ERequestNotifyChangeOfHwConfig,
		EMsgRequestMax=3,				// All requests less than this value are handled in the driver DFC thread.
		ERequestRecordData,
		ENumRequests,
		EAllRequests = (1<<ENumRequests)-1
		};
		
	enum TControl
	/**
	 Synchronous request types
	*/
		{
		EMsgControlSetAudioFormat,
		EMsgControlSetBufChunkCreate,
		EMsgControlSetBufChunkOpen,
		EMsgControlSetVolume,
		EMsgControlPause,
		EMsgControlResume,
		EMsgControlCancelSpecific,
		EMsgControlCustomConfig,
		EControlTimePlayed,
		EControlTimeRecorded,
		EMsgControlMax=10,				// All requests less than this value are handled in the driver DFC thread.
		EControlGetCaps,
		EControlGetAudioFormat,
		EControlGetBufConfig,
		EControlGetVolume,
		EControlBytesTransferred,
		EControlResetBytesTransferred,
		EControlReleaseBuffer
		};
	friend class DSoundScLdd;	
public:
	/**
	Get the version number of sound driver interface.
    @return The sound driver interface version number.
	*/
	inline static TVersion VersionRequired();

#ifndef __KERNEL_MODE__
	/**
	Open a channel on a specified sound device. This driver allows more than one channel to be opened on each device.
	@capability MultimediaDD
	@capability Dependent UserEnvironment Is required when opening a record channel.
	@param aUnit The unit number of the sound device.
	@return KErrNone, if successful; KErrPermissionDenied, if process lacks required capabilities
        	otherwise one of the other system-wide error codes. 
	*/
	inline TInt Open(TInt aUnit=KNullUnit);
	
	/**
	Get the capabilities of the sound device.
	@param aCapsBuf A packaged object which will be filled with the capabilities of the device.
	@see TSoundFormatsSupportedV02.
	*/
	inline void Caps(TDes8& aCapsBuf);
	
	/**
  	Get the current audio format configuration.
  	@param aFormatBuf A packaged object which will be filled with the audio configuration settings.
    @see TCurrentSoundFormatV02.
	*/
	inline void AudioFormat(TDes8& aFormatBuf);
	
	/**
  	Set the current audio format configuration.
  	This can't be changed while the driver is currently playing or recording data.
	@param aFormatBuf A packaged object holding the new audio configuration settings to be used.
  	@return KErrNone if successful;
  			KErrInUse if the playing or recording of data is in progress;
          	KErrNotSupported if any configuration values are invalid;
		  	otherwise one of the other system-wide error codes.
	@see TCurrentSoundFormatV02.
	*/
	inline TInt SetAudioFormat(const TDesC8& aFormatBuf);
	
 	/**
  	Get the current buffer configuration.
	@param aConfigBuf A packaged TSharedChunkBufConfigBase derived object which will be filled with the buffer
					  configuration settings.
	@see TSharedChunkBufConfigBase.
	*/
 	inline void GetBufferConfig(TDes8& aConfigBuf);
 	
 	/**
  	Set the current buffer configuration - creating a shared chunk.
  	The driver will create a shared chunk according to the buffer specification supplied (and will commit
  	memory to it). This will replace any previous shared chunk created by this driver. A handle to
  	the chunk will then be created for the client thread which will be assigned to the RChunk object supplied
  	by the client.
  	The buffer configuration cannot be changed while the driver is currently playing or recording data.
  	Note that a minimum of three buffers are required when recording: one being loaded with record data by the driver,
  	another also owned by the driver - queued ready for the next transfer and a third owned by the client for processing.
  	Note that for record channels, the buffer size specified effectively determines the size of each record request.
  	Hence, for record channels the buffer size must be a multiple of the minimum request size as specified in the capabilities 
  	class. @see TSoundFormatsSupportedV02. It should also be a multiple of the number of bytes per audio sample (e.g. a multiple 
  	of 4 for 2 channels of 16bit PCM).  
	@param aConfigBuf A packaged TSharedChunkBufConfigBase derived object which must be set with the new buffer configuration
					  settings to be used. Note that this function will ignore any buffer offset information specified by the 
					  caller (i.e. setting the flag KScFlagBufOffsetListInUse has no effect).
	@param aChunk An RChunk object to which the chunk handle will be assigned. 
  	@return KErrNone if successful;
  			KErrInUse if the playing or recording of data is in progress;
          	KErrArgument if any configuration values are invalid;
          	KErrNoMemory if the driver failed to allocate memory for the shared chunk specified;
		  	otherwise one of the other system-wide error codes.
	@see TSharedChunkBufConfigBase.
	*/
 	inline TInt SetBufferChunkCreate(const TDesC8& aConfigBuf,RChunk& aChunk);
 	
 	/**
  	Set the current buffer configuration - using an existing shared chunk.
  	The client supplies an existing shared chunk which is to be used by the driver as the play or record buffer. 
  	Any shared chunk previously created by the driver will be closed by it.
  	The buffer configuration cannot be changed while the driver is currently playing or recording data.
  	See RSoundSc::SetBufferChunkCreate for details on the buffer configuration setting necessary for playback or record.
	@param aConfigBuf A packaged TSharedChunkBufConfigBase derived object which must be set with the buffer configuration
					  settings of the chunk supplied. This must include buffer offset information (i.e. the flag 
					  KScFlagBufOffsetListInUse must be set).
	@param aChunk A handle to the shared chunk which is to be used as the play or record buffer. (This must
				  be a valid handle for the calling thread).
  	@return KErrNone if successful;
  			KErrInUse if the playing or recording of data is in progress;
          	KErrArgument if any configuration values are invalid;
		  	otherwise one of the other system-wide error codes.
	@see TSharedChunkBufConfigBase.
	*/
 	inline TInt SetBufferChunkOpen(const TDesC8& aConfigBuf,RChunk& aChunk);
 	
 	/**
  	Get the current play volume or record level.
	@return The current play volume / record level - a value in the range 0 to 255. The value 255 equates to
	the maximum volume and each value below this equates to a 0.5dB step below it.
	*/
	inline TInt Volume();
	
	/**
	Set the current play volume or record level. The volume can be altered while the driver is in the process
	of playing or recording data.
	@param aVolume The play volume / record level to be set - a value in the range 0 to 255. The value 255
		equates to the maximum volume and each value below this equates to a 0.5dB step below it.
    @return KErrNone if successful;
          	KErrArgument if the specified volume is out of range;
		  	otherwise one of the other system-wide error codes.
	*/
	inline TInt SetVolume(TInt aVolume);
	
	/**
	Submit a request to play (write data) to the sound device from a buffer.
  	The buffer must be contained within the driver's shared chunk. A single play request may not span more than one
  	buffer even if two or more buffers are contiguous within the shared chunk.
  	The driver will allow more than one play request to be pending at any time. It will
  	process requests in FIFO order. This is an asynchronous operation with the driver completing the request and
  	returning the request status once all the specified data has been delivered to the sound device. The driver
  	will return a request status of KErrUnderflow each time it completes the last pending play request unless this
  	request was marked with the KSndFlagLastSample flag. If this flag is set, the driver will return
  	KErrNone in this situation. Therefore when it is required to play uninterrupted audio by issuing a series of play
  	requests, the client should mark the last play request using this flag.
	@param aStatus 		 The request status which is signalled when all of the specified data has been transferred to
						 the sound device (or when an error occurs). Possible values:
								KErrNone:	   if all the specified data was successfully transferred and no underflow
											   occured following this transfer (or if an underflow occurred but the
											   request was marked with the KSndFlagLastSample flag);
								KErrUnderflow: if data underflow occurred during or immediately following the transfer;
								otherwise one of the system-wide error codes.
	@param aBufferOffset Offset from the beginning of the shared chunk for the start of data to be played. This must be
						 aligned to a value conforming to the iRequestAlignment member of the capabilities structure.
						 @see TSoundFormatsSupportedV02.
  	@param aLength 		 Number of bytes of data to be played. This must be a multiple of the minimum request size as
  						 specified in the capabilities class. @see TSoundFormatsSupportedV02. It should also be a multiple
  						 of the number of bytes per audio sample (e.g. a multiple of 4 for 2 channels of 16bit PCM).
  	@param aFlags 		 Play request flags. Use KSndFlagLastSample to mark the last play request in a series of play
  						 requests.
	@pre The buffer must have been previously initialised by a call to SetBufferChunkN().
	*/
	inline void PlayData(TRequestStatus& aStatus,TInt aBufferOffset,TInt aLength,TUint aFlags=0);
	
	/**
	Get the next available record buffer. More than one request may be pending at any time. This is an asynchronous
	operation. If the driver is not already recording then the issuing of this request will start the capturing of
	record data from the sound device into a record buffer.
	@param aStatus	The request status which is signalled when buffer is available (or an error occurs). If
					the request is successful then this result value is set to the offset within the shared chunk
					where the buffer resides. Alternatively, if an error occurs, it will be set to one of
					the system wide error values. These include:
					KErrInUse: if the client needs to free up record buffers;
					KErrOverflow: if it was necessary for the driver to over-write unread record data since the
								  last record request;
					KErrCancel: if the device is paused and there is no un-read record data available.			  
					otherwise one of the system-wide error codes.
  	@param aLength	On return, this contains the number of record bytes successfully stored in the buffer. 
	@pre The record buffer must have been previously initialised by a call to SetBufferChunkN().
	*/
	inline void RecordData(TRequestStatus& aStatus, TInt& aLength);
	
	/**
	Release a record buffer - making it available again for the driver to record into.
	@param aChunkOffset: The chunk offset of the image to be released. This is the value that was returned by the
						 RecordData() request when the buffer was claimed by the client.
	@return KErrNone: if successful;
			KErrNotFound: if the chunk offset is invalid;
			otherwise one of the system-wide error codes.
	*/
	inline TInt ReleaseBuffer(TInt aChunkOffset);
	
	/**
	Cancel all outstanding play requests.
	This immediately stops the driver playing data and all outstanding play requests are completed with KErrCancel.
	*/
	inline void CancelPlayData();
	
	/**
	Cancel all outstanding record requests.
	This immediately stops the driver from capturing record data and all outstanding record requests are completed
	with KErrCancel.
	*/
	inline void CancelRecordData();
	
	/**
	Cancel a specific record or play request.
	The outstanding request completes with KErrCancel.
	@param aStatus The request status object associated with the request to be cancelled.
	*/			   
	inline void Cancel(const TRequestStatus& aStatus);
	
	/**
	For a play back units, this method returns the number of bytes delivered to the sound device hardware. For
	recording units, this method returns the number of bytes read from the sound device hardware. This is the
	count since the driver was opened or since the last ResetBytesTransferred() command was issued.	
	@return The number of bytes transferred.
	@see ResetBytesTransferred()
	*/
	inline TInt BytesTransferred();
	
	/**
	Reset the count of the number of bytes transferred by the sound device..
	@see BytesTransferred().
	*/
	inline void ResetBytesTransferred();
	
	/**
	Halt the playing or recording of data from the sound device.
	Playing or recording can be resumed using Resume().
	If an incomplete play request is outstanding when the device is paused, this request does not complete as a 
	result. Resuming play operation following a pause will cause the driver to re-commence playing data from the 
	next sample following the last one played - even of this is part way through a play buffer.
	However, if an incomplete record request is outstanding when recording is paused, this will be completed immediately
	as a result. Typically this will be a partially filled record buffer and the number of valid bytes in the buffer are
	indicated via the length argument of the RecordData() method. The client should release this buffer in the normal
	way in order to make it available again to the driver when recoding is resumed. Any other outstanding record
	requests are completed immediately when recording is halted with a return value of KErrCancel. Likewise, record 
	requests received after the device has been paused will complete immediately in the same way - either successfully
	if un-read data is available or KErrCancel if not. The device must be resumed before it can be primed to capture a
	new record buffer. The driver will capture no record data while it is in the paused state. If recording is resumed,
	the driver will begin storing the received data from the beginning of its next available record buffer.
	@return KErrNone if successful;
  			KErrNotReady: if the driver is not currently playing or recording;
		  	otherwise one of the other system-wide error codes.
	@see Resume()
	*/
	inline TInt Pause();
	
	/**
	Allow the sound device to resume playing or recording data.
	This is used resume the operation of the driver following a earlier Pause() command to pause play or record.
	@return KErrNone if successful;
  			KErrNotReady: if the driver is not currently paused;
		  	otherwise one of the other system-wide error codes.
	@see Pause()
	*/
	inline TInt Resume();
	
	/**
	Register for notification of a change in the hardware configuration of the device.
	This is an asynchronous operation. Only one request of this type can be outstanding per driver channel.
	@param aStatus The request status which is signalled when a change in hardware configuration occurs. Possible
		values returned:
			KErrNotSupported: The unit cannot detect changes in configuration;
			KErrInUse: A notification request is already outstanding for this driver channel.
			KErrNone: A normal change in hardware configuration has occured;
			otherwise one of the other system-wide error codes.
	@param aHeadsetPresent On return, this is set to ETrue if the unit is connected to a microphone or headset socket and
		this is now present. Otherwise it returns EFalse.
	*/
	inline void NotifyChangeOfHwConfig(TRequestStatus& aStatus,TBool& aHeadsetPresent);

	/**
	Cancel a request for notification of a change in the hardware configuration.
	This results in a pending request being completed immediately with KErrCancel.
	*/
	inline void CancelNotifyChangeOfHwConfig();
	
	/**
	Issue a custom configuration request.
	@param aFunction A number identifying the request. (Values below 0x10000000 are reserved).
	@param aParam A 32-bit value passed to the driver. Its meaning depends on the request.
	@return KErrNone if successful, otherwise one of the other system wide error codes.
	*/
	inline TInt CustomConfig(TInt aFunction,TAny* aParam=NULL);

	/**
	Calculates and returns the number of microseconds of data played since the start of playback.  If playback ends or
	if CancelPlayData() is called, this number will be reset.
	@param aTimePlayed A packaged TTimeIntervalMicroSecondsBuf into which to place the number of microseconds played.
	@return	KErrNone if successful.
			KErrNotSupported if unit is not open for playback.
			Otherwise one of the other system wide error codes.
	*/
	inline TInt TimePlayed(TTimeIntervalMicroSecondsBuf& aTimePlayed);

	/**
	Calculates and returns the number of microseconds of data recorded since the start of record.  If recording ends or
	if CancelRecord() is called, this number will be reset.
	@param aTimeRecorded A packaged TTimeIntervalMicroSecondsBuf into which to place the number of microseconds played.
	@return	KErrNone if successful.
			KErrNotSupported if unit is not open for recording.
			Otherwise one of the other system wide error codes.
	*/
	inline TInt TimeRecorded(TTimeIntervalMicroSecondsBuf& aTimeRecorded);

#endif	// __KERNEL_MODE__
	};

#include <d32soundsc.inl>

#endif // __D32SOUNDSC_H__

