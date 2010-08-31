// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// include/drivers/dmadefs.h
// DMA Framework - General class, enum, constant and type definitions.
//
//

/** @file
	@publishedPartner
*/

#ifndef __DMADEFS_H__
#define __DMADEFS_H__


#include <e32def.h>


/** The client request callback type.

*/
enum TDmaCallbackType
	{
	/** Transfer request completion callback

		@released
	*/
	EDmaCallbackRequestCompletion        = 0x01,
	/** Transfer request completion callback - source side

		@prototype
	*/
	EDmaCallbackRequestCompletion_Src    = 0x02,
	/** Transfer request completion callback - destination side

		@prototype
	*/
	EDmaCallbackRequestCompletion_Dst    = 0x04,

	/** Descriptor completion callback

		@prototype
	*/
	EDmaCallbackDescriptorCompletion     = 0x08,
	/** Descriptor completion callback - source side

		@prototype
	*/
	EDmaCallbackDescriptorCompletion_Src = 0x10,
	/** Descriptor completion callback - destination side

		@prototype
	*/
	EDmaCallbackDescriptorCompletion_Dst = 0x20,

	/** Frame completion callback

		@prototype
	*/
	EDmaCallbackFrameCompletion          = 0x40,
	/** Frame completion callback - source side

		@prototype
	*/
	EDmaCallbackFrameCompletion_Src      = 0x80,
	/** Frame completion callback - destination side

		@prototype
	*/
	EDmaCallbackFrameCompletion_Dst      = 0x100,

	/** H/W descriptor pause event callback

		@prototype
	*/
	EDmaCallbackLinkedListPaused        = 0x200,
	/** H/W descriptor pause event callback - source side

		@prototype
	*/
	EDmaCallbackLinkedListPaused_Src    = 0x400,
	/** H/W descriptor pause event callback - destination side

		@prototype
	*/
	EDmaCallbackLinkedListPaused_Dst    = 0x800
	};


/** The outcome of the transfer request.

	@released
*/
enum TDmaResult
	{
	/** Completed without error */
	EDmaResultOK = 0,
	/** There was an error */
	EDmaResultError
	};


/** To be used with address mode field of the DMA transfer config struct.

	@see TDmaTransferConfig::iAddrMode
*/
enum TDmaAddrMode
	{
	/** Constant addressing. The address remains the same for consecutive
		accesses.

		@released
	*/
	KDmaAddrModeConstant,
	/** Post-increment addressing. The address increases by the element size
		after each access.

		@released
	*/
	KDmaAddrModePostIncrement,
	/** Post-decrement addressing. The address decreases by the element size
		after each access.

		@prototype
	*/
	KDmaAddrModePostDecrement,
	/** 1D-index addressing. The address always increases by the element size
		plus the element skip value after each access.

		@prototype
	*/
	KDmaAddrMode1DIndex,
	/** 2D-index addressing. The address increases by the element size plus the
		element skip value - but only within a frame. Once a full frame has been
		transferred, the address increases by the element size plus the element
		skip value plus the frame skip value.

		@prototype
	*/
	KDmaAddrMode2DIndex
	};


/** To be used with the burst size field of the DMA transfer config struct.

	@see SDmacCaps::iBurstTransactions
	@see TDmaTransferConfig::iBurstSize

	@prototype
*/
enum TDmaBurstSize
	{
	/** Don't use burst transactions */
	KDmaNoBursts     = -1,
	/** Don't care (the default) */
	KDmaBurstSizeAny = 0x00,
	/** 4 bytes */
	KDmaBurstSize4   = 0x04,
	/** 8 bytes */
	KDmaBurstSize8   = 0x08,
	/** 16 bytes */
	KDmaBurstSize16  = 0x10,
	/** 32 bytes */
	KDmaBurstSize32  = 0x20,
	/** 64 bytes */
	KDmaBurstSize64  = 0x40,
	/** 128 bytes */
	KDmaBurstSize128 = 0x80
	};


/** To be used with the flags field of the DMA transfer config struct.

	@see TDmaTransferConfig::iFlags
*/
enum TDmaTransferFlags
	{
	/** Location is address of a memory buffer (as opposed to a peripheral or a
		register).

		@released
	*/
	KDmaMemAddr                      = 0x01,
	/** Address is a physical address (as opposed to a linear one).

		If it is a memory address then KDmaMemIsContiguous will need to be set
		as well.

		@released
	 */
	KDmaPhysAddr                     = 0x02,
	/** Target memory is known to be physically contiguous, hence there is
		no need for the framework to check for memory fragmentation.

		@released
	*/
	KDmaMemIsContiguous              = 0x04,
	/** Don't use packed access (if possible)

		@released
	*/
	KDmaDontUsePacked                = 0x08,
	/** Location is big endian (little endian if not set).

		To have any effect, this flag requires the DMAC to support endianness
		conversion.

		@see SDmacCaps::iEndiannessConversion

		@prototype
	*/
	KDmaBigEndian                    = 0x10,
	/** Don't do endianness conversion even if applicable.

		To have any effect, this flag requires the DMAC to support endianness
		conversion.

		@see SDmacCaps::iEndiannessConversion

		@prototype
	*/
	KDmaLockEndian                   = 0x20,
	/** Execute client request callback after each subtransfer (streaming /
		loop case).

		This option is only taken into account if the respective
		TDmaTransferConfig::iRepeatCount is non-zero.

		The callback will complete with a TDmaCallbackType of
		EDmaCallbackRequestCompletion (even if the repeat counts for source and
		destination are different), unless the flag
		TDmaPILFlags::KDmaAsymCompletionCallback is set too, in which case what
		is described there applies.

		@prototype
	*/
	KDmaCallbackAfterEveryTransfer   = 0x40,
	/** Execute client request callback after each completed hardware
		descriptor.

		Requires the DMAC to support this feature. Unless the DMAC supports
		asymmetric descriptor interrupts as well, this flag should not be set
		on only one (source or destination) side.

		@see SDmacCaps::iDescriptorInterrupt
		@see SDmacCaps::iAsymDescriptorInterrupt

		@prototype
	*/
	KDmaCallbackAfterEveryDescriptor = 0x80,
	/** Execute client request callback after each completed frame.

		Requires the DMAC to support this feature. Unless the DMAC supports
		asymmetric frame interrupts as well, this flag should not be set on
		only one (source or destination) side.

		@see SDmacCaps::iFrameInterrupt
		@see SDmacCaps::iAsymFrameInterrupt

		@prototype
	*/
	KDmaCallbackAfterEveryFrame      = 0x100
	};


/** To be used with the synchronization flags field of a DMA transfer
	config struct.

	@see SDmacCaps::iSynchronizationTypes
	@see TDmaTransferConfig::iSyncFlags

	@released
*/
enum TDmaTransferSyncFlags
	{
	/** Leave the decision on whether the transfer is hardware synchronized at
		this end (either source or destination) to the framework. This is the
		default.
	*/
	KDmaSyncAuto        = 0x00,
	/** Transfer is not hardware synchronized at this end (either source or
		destination).
	*/
	KDmaSyncNone        = 0x01,
	/** Transfer is hardware synchronized at this end (either source or
		destination). This option can also be used on its own, without any
		of the following sync sizes.
	*/
	KDmaSyncHere        = 0x02,
	/** H/W synchronized at this end: transfer one ELEMENT (a number of
		bytes, depending on the configured element size) per sync event.
	*/
	KDmaSyncSizeElement = 0x04,
	/** H/W synchronized at this end: transfer one FRAME (a number of
		elements, depending on the configured frame size) per sync event.
	*/
	KDmaSyncSizeFrame   = 0x08,
	/** H/W synchronized at this end: transfer one BLOCK (a number of
		frames, depending on the configured transfer size) per sync
		event. This is the most common use case.
	*/
	KDmaSyncSizeBlock   = 0x10,
	/** H/W synchronized at this end: transfer one PACKET (a number of
		elements, depending on the configured packet size) per sync event.
		In cases where the transfer block size is not a multiple of the
		packet size the last packet will consist of the remaining elements.
	*/
	KDmaSyncSizePacket  = 0x20
	};


/** To be used with the Graphics operation field of a DMA transfer request.

	@see TDmaTransferArgs::iGraphicsOps

	@prototype
*/
enum TDmaGraphicsOps
	{
	/** Don't use any graphics acceleration feature (the default) */
	KDmaGraphicsOpNone            = 0x00,
	/** Enable graphics acceleration feature 'Constant Fill' */
	KDmaGraphicsOpConstantFill    = 0x01,
	/** Enable graphics acceleration feature 'TransparentCopy' */
	KDmaGraphicsOpTransparentCopy = 0x02
	};


/** To be used with the PIL flags field of a DMA transfer request.

	@see TDmaTransferArgs::iFlags
*/
enum TDmaPILFlags
	{
	/** Request a different max transfer size (for instance for test
		purposes).

		@released
	*/
	KDmaAltTransferLength         = 0x01,
	/** Execute client request callback in ISR context instead of from a
		DFC.

		@released
	*/
	KDmaRequestCallbackFromIsr    = 0x02,
	/** Execute descriptor completion callback in ISR context instead of
		from a DFC. This option is to be used in conjunction with the
		TDmaTransferFlags::KDmaCallbackAfterEveryDescriptor flag.

		@prototype
	*/
	KDmaDescriptorCallbackFromIsr = 0x04,
	/** Execute frame completion callback in ISR context instead of
		from a DFC. This option is to be used in conjunction with the
		TDmaTransferFlags::KDmaCallbackAfterEveryFrame flag.

		@prototype
	*/
	KDmaFrameCallbackFromIsr      = 0x08,
	/** Execute the client request callback separately for source and
		destination subtransfers.

		This flag also determines the TDmaCallbackType value returned. If set,
		the callback will complete with EDmaCallbackRequestCompletion_Src or
		EDmaCallbackRequestCompletion_Dst, respectively, instead of with
		EDmaCallbackRequestCompletion.

		Requires the DMAC to support this feature.

		@see SDmacCaps::iAsymCompletionInterrupt

		@prototype
	*/
	KDmaAsymCompletionCallback    = 0x10,
	/** Execute the descriptor completion callback separately for source
		and destination subtransfers.

		This flag modifies the behaviour of the
		TDmaTransferFlags::KDmaCallbackAfterEveryDescriptor flag and also
		determines the TDmaCallbackType value returned. If set, the callback
		will complete with EDmaCallbackDescriptorCompletion_Src or
		EDmaCallbackDescriptorCompletion_Dst, respectively, instead of with
		EDmaCallbackDescriptorCompletion.

		Requires the DMAC to support this feature.

		@see SDmacCaps::iAsymDescriptorInterrupt

		@prototype
	*/
	KDmaAsymDescriptorCallback    = 0x20,
	/** Execute the frame completion callback separately for source and
		destination subtransfers.

		This flag modifies the behaviour of the
		TDmaTransferFlags::KDmaCallbackAfterEveryFrame flag. If set, the
		callback will complete with EDmaCallbackFrameCompletion_Src or
		EDmaCallbackFrameCompletion_Dst, respectively, instead of with
		EDmaCallbackFrameCompletion.

		Requires the DMAC to support this feature.

		@see SDmacCaps::iAsymFrameInterrupt

		@prototype
	*/
	KDmaAsymFrameCallback         = 0x40,
	/** This transfer (only) should use the channel priority indicated by
		TDmaTransferArgs::iChannelPriority.

		@prototype
	*/
	KDmaRequestChannelPriority    = 0x80
	};


/** Values which can be used with the priority field when opening a channel
	and/or when fragmenting a transfer request.

	@see TDmaChannel::SCreateInfo::iPriority
	@see TDmaTransferArgs::iChannelPriority

	@prototype
*/
enum TDmaPriority
	{
	/** No transfer priority preference (don't care value) */
	KDmaPriorityNone = 0x0,
	/** Platform-independent transfer priority 1 (lowest) */
	KDmaPriority1 = 0x80000001,
	/** Platform-independent transfer priority 2 */
	KDmaPriority2 = 0x80000002,
	/** Platform-independent transfer priority 3 */
	KDmaPriority3 = 0x80000003,
	/** Platform-independent transfer priority 4 */
	KDmaPriority4 = 0x80000004,
	/** Platform-independent transfer priority 5 */
	KDmaPriority5 = 0x80000005,
	/** Platform-independent transfer priority 6 */
	KDmaPriority6 = 0x80000006,
	/** Platform-independent transfer priority 7 */
	KDmaPriority7 = 0x80000007,
	/** Platform-independent transfer priority 8 (highest) */
	KDmaPriority8 = 0x80000008
	};


/** Contains the configuration values for either the source or the
	destination side of a DMA transfer.

	Note that some fields (notably iElementSize, iElementsPerFrame and
	iFramesPerTransfer) may only differ between source and destination if
	the underlying DMAC supports this.

	@see SDmacCaps::iSrcDstAsymmetry
	@see TDmaTransferArgs::iSrcConfig
	@see TDmaTransferArgs::iDstConfig

	@released
*/
struct TDmaTransferConfig
	{
friend struct TDmaTransferArgs;

	/** Default constructor.

		Initializes all fields with meaningful default values.
	*/
#ifdef DMA_APIV2
	KIMPORT_C
#endif
	TDmaTransferConfig();

	/**	Alternate constructor.

		Intended for general use ie. not 1D or 2D transfers
	*/
#ifdef DMA_APIV2
	KIMPORT_C
#endif
	TDmaTransferConfig (
		TUint32 aAddr,
		TUint aTransferFlags,
		TDmaAddrMode aAddrMode = KDmaAddrModePostIncrement,
		TUint aSyncFlags = KDmaSyncAuto,
		TDmaBurstSize aBurstSize = KDmaBurstSizeAny,
		TUint aElementSize = 0,
		TUint aElementsPerPacket = 0,
		TUint aPslTargetInfo = 0,
		TInt aRepeatCount = 0
		);

	/**	Alternate constructor.

		Intended for 1D and 2D transfers.
	*/
#ifdef DMA_APIV2
	KIMPORT_C
#endif
	TDmaTransferConfig (
		TUint32 aAddr,
		TUint aElementSize,
		TUint aElementsPerFrame,
		TUint aFramesPerTransfer,
		TInt aElementSkip,
		TInt aFrameSkip,
		TUint aTransferFlags,
		TUint aSyncFlags = KDmaSyncAuto,
		TDmaBurstSize aBurstSize = KDmaBurstSizeAny,
		TUint aElementsPerPacket = 0,
		TUint aPslTargetInfo = 0,
		TInt aRepeatCount = 0
		);

	/** Transfer start address */
	TUint32 iAddr;
	/** Address mode */
	TDmaAddrMode iAddrMode;
	/** Element size in bytes (1/2/4/8) */
	TUint iElementSize;
	/** Number of elements per frame */
	TUint iElementsPerFrame;
	/** Number of elements per packet */
	TUint iElementsPerPacket;
	/** Number of frames to transfer (result is the transfer block) */
	TUint iFramesPerTransfer;
	/** Element skip in bytes (for addr modes E1DIndex or E2DIndex) */
	TInt iElementSkip;
	/** Frame skip in bytes (for addr mode E2DIndex) */
	TInt iFrameSkip;
	/** Use burst transactions of the specified size (in bytes)
		@see TDmaBurstSize
	*/
	TInt iBurstSize;
	/** PIL src/dst config flags.
		@see TDmaTransferFlags
	*/
	TUint32 iFlags;
	/** Transfer synchronization flags.
		@see TDmaTransferSyncFlags
	*/
	TUint32 iSyncFlags;
	/** Information passed to the PSL */
	TUint iPslTargetInfo;
	/** How often to repeat this (sub-)transfer:
		 0     no repeat (the default)
		 1..n  once / n times
		-1     endlessly.

		@prototype
	*/
	TInt iRepeatCount;
	/** Structure contents delta vector.

		(usage tbd)

		@prototype
	 */
	TUint32 iDelta;
	/** Reserved for future use */
	TUint32 iReserved;

private:
	/** Private constructor.

		Initializes fields with the values passed in by the legacy version of
		the DDmaRequest::Fragment() call.
	*/
	TDmaTransferConfig(TUint32 aAddr, TUint aFlags, TBool aAddrInc);
	};


/** To be used by the client to pass DMA transfer request details to the
	framework.

	Also used internally by the framework as a pseudo descriptor if the
	controller doesn't support hardware descriptors (scatter/gather LLI).

	@see DDmaRequest::Fragment

	@released
*/
struct TDmaTransferArgs
	{
	friend class DDmaRequest;
	friend class TDmaChannel;
	friend class TDmac;
	friend class DmaChannelMgr;

	/** Default constructor.

		Initializes all fields with meaningful default values.
	*/
#ifdef DMA_APIV2
	KIMPORT_C
#endif
	TDmaTransferArgs();

	/**	Alternate constructor.

		Intended for transfers where src and dst TDmaTransferConfig structs
		share some of the same options, i.e. iDmaTransferFlags, iAddrMode,
		iSyncFlags, iBurstSize, and iElementSize.

		@param aSrcAddr
		@param aDstAddr
		@param aCount Number of bytes to transfer
		@param aDmaTransferFlags Bitmask of TDmaTransferFlags for src and dst
		@param aDmaSyncFlags Bitmask of TDmaTransferSyncFlags for src and dst
		@param aMode Address mode for src and dst
		@param aDmaPILFlags Bitmask of TDmaPILFlags
		@param aElementSize In bytes (1/2/4/8) for src and dst
		@param aChannelPriority
		@param aBurstSize for src and dst
		@param aPslRequestInfo Info word passed to the PSL
		@param aGraphicOp Graphics operation to be executed
		@param aColour Colour value for graphics operation
	*/
#ifdef DMA_APIV2
	KIMPORT_C
#endif
	TDmaTransferArgs (
		TUint aSrcAddr,
		TUint aDstAddr,
		TUint aCount,
		TUint aDmaTransferFlags,
		TUint aDmaSyncFlags = KDmaSyncAuto,
		TUint aDmaPILFlags = 0,
		TDmaAddrMode aMode = KDmaAddrModePostIncrement,
		TUint aElementSize = 0,
		TUint aChannelPriority = KDmaPriorityNone,
		TDmaBurstSize aBurstSize = KDmaBurstSizeAny,
		TUint aPslRequestInfo = 0,
		TDmaGraphicsOps aGraphicOp = KDmaGraphicsOpNone,
		TUint32 aColour = 0
		);

	/** Alternate constructor.

		Intended for transfers needing specific options for source and
		destination TDmaTransferConfig structs.

		@param aSrc Configuration values for the source
		@param aDst Configuration values for the destination
		@param aFlags @see TDmaPILFlags
		@param aChannelPriority Use for this request (only) the indicated
		channel priority. Requires KDmaRequestChannelPriority to be set in
		iFlags as well. @see TDmaPriority
		@param aPslRequestInfo Info word passed to the PSL
		@param aGraphicOp Graphics operation to be executed
		@param aColour Colour value for graphics operation
	*/
#ifdef DMA_APIV2
	KIMPORT_C
#endif
	TDmaTransferArgs (
		const TDmaTransferConfig& aSrc,
		const TDmaTransferConfig& aDst,
		TUint32 aFlags = 0,
		TUint aChannelPriority = KDmaPriorityNone,
		TUint aPslRequestInfo = 0,
		TDmaGraphicsOps aGraphicOp = KDmaGraphicsOpNone,
		TUint32 aColour = 0
		);

	/** Configuration values for the source */
	TDmaTransferConfig iSrcConfig;
	/** Configuration values for the destination */
	TDmaTransferConfig iDstConfig;

	/** Number of bytes to transfer (optional).

		A non-zero value here must be consistent with iElementSize,
		iElementsPerFrame and iFramesPerTransfer in iSrcConfig and iDstConfig
		if the latter are specified as well (or instead, they may be left at
		their default values of zero).

		If zero, the PIL will fill in a value calculated from multiplying
		iElementSize, iElementsPerFrame and iFramesPerTransfer in iSrcConfig,
		so that the PSL can rely on it being always non-zero and valid.
	 */
	TUint iTransferCount;
	/** Graphics operation to be executed */
	TDmaGraphicsOps iGraphicsOps;
	/** Colour value for graphics operations */
	TUint32 iColour;
	/** PIL common flags
		@see TDmaPILFlags
	*/
	TUint32 iFlags;
	/** Use for this request (only) the indicated channel priority.
		Requires KDmaRequestChannelPriority to be set in iFlags as well.
		@see TDmaPriority
	*/
	TUint iChannelPriority;
	/** Info word passed to the PSL */
	TUint iPslRequestInfo;
	/** Stores the PSL cookie returned by TDmaChannel::PslId() at request
		fragmentation time.

		The value PslId() is often (but not necessarily) identical with the
		client's TDmaChannel::SCreateInfo::iCookie, which gets passed by the
		PIL into DmaChannelMgr::Open() as 'aOpenId'.
	*/
	TUint32 iChannelCookie;
	/** Structure contents delta vector.

		(usage tbd)

		@prototype
	 */
	TUint32 iDelta;
	/** Reserved for future use */
	TUint32 iReserved1;

private:
	/** Private constructor.

		Initializes fields with the values passed in by the legacy version of
		the DDmaRequest::Fragment() call.
	*/
	TDmaTransferArgs(TUint32 aSrcAddr, TUint32 aDstAddr, TInt aCount,
					 TUint aFlags, TUint32 aPslInfo);
	/** Reserved for future use */
	TUint32 iReserved2;
	};


/** DMAC capabilities info structure.

	Instances are to be filled in by the PSL and then linked to via TDmaChannel
	objects after they have been opened.

	The contents may vary even between channels on the same DMAC (but should
	remain constant for a given channel for the duration that it is open),
	depending on static or dynamic factors which only the PSL knows about.

	@see TDmaChannel::Open
	@see TDmaChannel::DmacCaps

	@released
*/
struct SDmacCaps
	{
	/** DMAC supports n + 1 different channel priorities.
	 */
	TUint iChannelPriorities;
	/** DMAC supports the pausing and resuming of channels.
	 */
	TBool iChannelPauseAndResume;
	/** DMA addresses must be aligned on an element size boundary.
	 */
	TBool iAddrAlignedToElementSize;
	/** DMAC supports 1D (element) index addressing in hardware.
	 */
	TBool i1DIndexAddressing;
	/** DMAC supports 2D (frame) index addressing in hardware.
	 */
	TBool i2DIndexAddressing;
	/** DMAC supports these transfer synchronization types (bitmap of values).

		@see TDmaTransferSyncFlags
	*/
	TUint iSynchronizationTypes;
	/** DMAC supports burst transactions with these sizes (bitmap of values).

		@see TDmaBurstSize
	*/
	TUint iBurstTransactions;
	/** DMAC supports a 'h/w descriptor complete' interrupt.
	 */
	TBool iDescriptorInterrupt;
	/** DMAC supports a 'frame transfer complete' interrupt.
	 */
	TBool iFrameInterrupt;
	/** DMAC supports a 'linked-list pause event' interrupt.
	 */
	TBool iLinkedListPausedInterrupt;
	/** DMAC supports endianness conversion.
	 */
	TBool iEndiannessConversion;
	/** DMAC supports these graphics operations (bitmap of values).

		@see TDmaGraphicsOps
	*/
	TUint iGraphicsOps;
	/** DMAC supports repeated transfers (loops).
	 */
	TBool iRepeatingTransfers;
	/** DMAC supports logical channel linking (chaining).
	 */
	TBool iChannelLinking;
	/** DMAC supports scatter/gather mode (linked list items).
	 */
	TBool iHwDescriptors;
	/** DMAC supports asymmetric source and destination transfer
		parameters (such as element size).
	*/
	TBool iSrcDstAsymmetry;
	/** DMAC supports asymmetric h/w descriptor lists.

		ETrue here requires ETrue for iHwDescriptors and iSrcDstAsymmetry as
		well.
	*/
	TBool iAsymHwDescriptors;
	/** DMAC with asymmetric descriptor support has the limitation that the
		number of bytes transferred in source and destination must be equal in
		every link segment (i.e. in each src/dst descriptor pair).

		ETrue here requires ETrue for iAsymHwDescriptors as well.
	*/
	TBool iBalancedAsymSegments;
	/** DMAC supports separate transfer completion notifications for source and
		destination side subtransfers.

		This capability is required for the asymmetric transfer completion
		callback API feature.

		@see TDmaPILFlags::KDmaAsymCompletionCallback
	*/
	TBool iAsymCompletionInterrupt;
	/** DMAC supports separate descriptor completion notifications for source and
		destination side.

		This capability is required for the asymmetric descriptor completion
		callback API feature.

		ETrue here requires ETrue for both iDescriptorInterrupt and
		iAsymHwDescriptors as well.

		@see TDmaPILFlags::KDmaAsymDescriptorCallback
	*/
	TBool iAsymDescriptorInterrupt;
	/** DMAC supports separate frame completion notifications for source and
		destination side.

		This capability is required for the asymmetric frame completion
		callback API feature.

		ETrue here requires ETrue for iFrameInterrupt as well.

		@see TDmaPILFlags::KDmaAsymFrameCallback
	*/
	TBool iAsymFrameInterrupt;

	/** Reserved for future use */
	TUint32 iReserved[5];
	};


//////////////////////////////////////////////////////////////////////////////
// INTERFACE WITH TEST HARNESS
//////////////////////////////////////////////////////////////////////////////

/** Set of information used by test harness.

	@deprecated
*/
#ifdef DMA_APIV2
struct TDmaTestInfo
	{
	/** Maximum transfer size in bytes for all channels (ie. the minimum of all
		channels' maximum size)
	*/
	TUint iMaxTransferSize;
	/** 3->Memory buffers must be 4-byte aligned, 7->8-byte aligned, ... */
	TUint iMemAlignMask;
	/** Cookie to pass to DDmaRequest::Fragment for memory-memory transfer */
	TUint32 iMemMemPslInfo;
	/** Number of test single-buffer channels */
	TInt iMaxSbChannels;
	/** Pointer to array containing single-buffer test channel ids */
	TUint32* iSbChannels;
	/** Number of test double-buffer channels */
	TInt iMaxDbChannels;
	/** Pointer to array containing double-buffer test channel ids */
	TUint32* iDbChannels;
	/** Number of test scatter-gather channels */
	TInt iMaxSgChannels;
	/** Pointer to array containing scatter-gather test channel ids */
	TUint32* iSgChannels;
	};
#endif


/** Set of information used by test harness.

	@released
*/
struct TDmaV2TestInfo
	{
	enum {KMaxChannels=32};
	/** Maximum transfer size in bytes for all channels (ie. the minimum of all
		channels' maximum size)
	*/
	TUint iMaxTransferSize;
	/** 3->Memory buffers must be 4-byte aligned, 7->8-byte aligned, ... */
	TUint iMemAlignMask;
	/** Cookie to pass to DDmaRequest::Fragment for memory-memory transfer */
	TUint32 iMemMemPslInfo;
	/** Number of test single-buffer channels */
	TInt iMaxSbChannels;
	/** Pointer to array containing single-buffer test channel ids */
	TUint32 iSbChannels[KMaxChannels];
	/** Number of test double-buffer channels */
	TInt iMaxDbChannels;
	/** Pointer to array containing double-buffer test channel ids */
	TUint32 iDbChannels[KMaxChannels];
	/** Number of test scatter-gather channels */
	TInt iMaxSgChannels;
	/** Pointer to array containing scatter-gather test channel ids */
	TUint32 iSgChannels[KMaxChannels];
	};


#endif	// #ifndef __DMADEFS_H__
