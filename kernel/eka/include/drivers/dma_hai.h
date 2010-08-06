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
// include/drivers/dma_hai.h
// DMA Framework - Symbian Hardware Abstraction Interface (SHAI).
//
//

/** @file
	@publishedPartner
	@released
*/

#ifndef __DMA_HAI_H__
#define __DMA_HAI_H__


#include <kernel/kern_priv.h>


//////////////////////////////////////////////////////////////////////////////


/** Interface used by PIL to open and close DMA channels.

	Must be implemented by the PSL.
*/
class DmaChannelMgr
	{

public:
	/** Opens a channel using a client-provided identifier.

		This function must be implemented by the PSL.

		@param aOpenId PSL-specific magic cookie passed by client. This could
		identify the channel exactly (by being just the channel number), or at
		least sufficiently (for example for use with a certain peripheral), or
		it may indicate some properties which the channel must possess. It may
		be set to zero always if all channels are equivalent.

		@param aDynChannel ETrue if the Open call is for a dynamic channel. A
		dynamic channel is not exclusively reserved for just one client, and
		further Open calls for more dynamic channels should succeed as long as
		certain resources (but not including the number of available physical
		channels) are not exceeded. Different transfer requests on this dynamic
		channel may be serviced using different actual channels.

		@param aPriority The desired channel priority as requested by the
		client. This may be an actual hardware priority or a
		platform-independent value. Not being able to satisfy the requested
		value is not a reason for the PSL to return NULL. This parameter may be
		ignored if aDynChannel is passed as ETrue. An overriding per-transfer
		priority may be requested by a client later via
		TDmaTransferArgs::iChannelPriority.
		@see SDmacCaps::iChannelPriorities
		@see TDmaPriority

		@return Pointer to channel if available, NULL otherwise. It should not
		be NULL if the Open call was for a dynamic channel unless a processing
		error occurred.

		@pre The PIL calls this function with a global fast mutex held to avoid
		race conditions.

		@post If a non-NULL pointer is returned, the object pointed to has its
		iController, iDmacCaps, iPslId, iDynChannel and iPriority members set
		to valid states.

		iController should point to the controller handling the
		channel.

		iDmacCaps should point to a SDmacCaps structure containing values
		relating to this particular channel.

		iPslId should contain a value uniquely identifying the channel - the
		PIL assigns this value later during request fragmentation to
		TDmaTransferArgs::iChannelCookie. It can be given any convenient value
		by the PSL (channel index, I/O port address, etc.).

		iDynChannel should be set to ETrue by the PSL if a dynamic channel was
		requested and has been opened.

		If applicable, iPriority should contain the actual hardware priority
		that has been configured or reserved. Otherwise it may be left at its
		default value TDmaPriority::KDmaPriorityNone.
	*/
	static TDmaChannel* Open(TUint32 aOpenId, TBool aDynChannel, TUint aPriority);


	/** Performs platform-specific operations when a channel is closed.

		If aChannel was opened as a dynamic channel then this call is a sign
		that there is a client which does not intend to queue any further
		transfer requests via this channel.

		This function must be implemented by the PSL but the implementation can
		be a no-op.

		@param aChannel The channel to close

		@pre The PIL calls this function with a global fast mutex held to avoid
		race conditions.
	*/
	static void Close(TDmaChannel* aChannel);


	/** Function allowing PSL to extend DMA API with new channel-independent
		operations.

		This function must be implemented by the PSL.

		@param aCmd Command identifier. Negative values are reserved for FW
		internal use.

		@param aArg PSL-specific

		@return KErrNotSupported if aCmd is not supported. PSL-specific value
		otherwise.
	*/
	static TInt StaticExtension(TInt aCmd, TAny* aArg);


	/** Acquires the channel manager lock. Called by the PIL before opening and
		closing a channel.
	*/
	static void Wait();


	/** Releases the channel manager lock. Called by the PIL after opening and
		closing a channel.
	*/
	static void Signal();

private:
	/** Declared, defined, and called by PSL's DECLARE_STANDARD_EXTENSION(). */
	friend TInt InitExtension();

	/** Must be called in the PSL's DECLARE_STANDARD_EXTENSION(). */
	static TInt Initialise();

	static NFastMutex Lock;
	};


//////////////////////////////////////////////////////////////////////////////


/** Abstract base class representing a DMA controller.

	The class has two purposes.

	First, it is a container for channels, descriptors and descriptor headers.

	Second, it exposes a set of virtual functions implemented by the PSL
	(platform-specific layer).

	These functions are the main interfaces between the PIL
	(platform-independent layer) and PSL.
*/
class TDmac
	{
friend class DmaChannelMgr;

// The following friend declaration will become obsolete once header
// pool manipulation functionality is owned and provided by the controller
// class instead of the request class.
// (TDmac::iFreeHdr could then also be made private.)
friend class DDmaRequest;

friend class TSkelDmac;

protected:
	/** Data required for creating a new instance. */
	struct SCreateInfo
		{
		/** True if DMAC uses hardware descriptors (i.e. supports
			scatter/gather mode).
		*/
		TBool iCapsHwDes;

		/** Initial maximum number of descriptors and headers (shared by all
			channels) to be allocated by the PIL. If at run time more
			descriptors are needed then they will be dynamically allocated and
			added to the available pool.

			The PSL may consider a number of factors when providing this
			initial value, such as the number of channels on this controller,
			the maximum transfer size per descriptor and also likely usage
			scenarios for the platform or device (number of drivers using DMA,
			their traffic patterns, simultaneity of operations, etc.).

			(NB: Dynamic growing of the descriptor pool is not yet implemented
			in the PIL, so this value is currently also the final maximum
			number of descriptors.)
		*/
		TInt iDesCount;

		/** Size of an individual descriptor.

			Use sizeof(TDmaTransferArgs) for single-buffer and double-buffer
		 	(i.e. non-s/g) controllers.
		*/
		TInt iDesSize;

		/** Bitmask used when creating the memory chunk storing the descriptor
			pool in the PIL. Used only for hardware descriptors.

			The access part must be EMapAttrSupRw. If the chunk is cached
			and/or buffered, the PSL must flush the data cache and/or drain the
			write buffer in InitHwDes() and related functions.

			The physical start address of the chunk will always be MMU page
		 	size aligned.

		 	@see TMappingAttributes
		 */
		TUint iDesChunkAttribs;
		};

	/** Base class constructor.
	 */
	TDmac(const SCreateInfo& aInfo);

	/** Base class 2nd-phase constructor.
	 */
	TInt Create(const SCreateInfo& aInfo);

public:
	/** Base class virtual destructor.
	 */
	virtual ~TDmac();


	/** Allocates a number of headers (and hence also descriptors) from the
		header/descriptor pools. Called by the PIL but may also be used by the
		PSL.
	*/
	TInt ReserveSetOfDes(TInt aCount);


	/** Returns previously allocated headers (and hence also descriptors) to
		the header/descriptor pools. Called by the PIL but may also be used by
		the PSL.
	*/
	void ReleaseSetOfDes(TInt aCount);


	/** Called by the PIL during request fragmentation to fill a descriptor or
		pseudo descriptor with transfer arguments.
	*/
	TInt InitDes(const SDmaDesHdr& aHdr, const TDmaTransferArgs& aTransferArgs);


	/** Called by the PIL in TDmaChannel::IsrRedoRequest() if any of the
		latter's arguments is non-zero.
	*/
	TInt UpdateDes(const SDmaDesHdr& aHdr, TUint32 aSrcAddr, TUint32 aDstAddr,
				   TUint aTransferCount, TUint32 aPslRequestInfo);


	/** Returns a reference to the associated pseudo descriptor for a given
		descriptor header. For use by PIL and PSL.
	*/
	inline TDmaTransferArgs& HdrToDes(const SDmaDesHdr& aHdr) const;


	/** Returns a reference to the associated hardware descriptor for a given
		descriptor header. For use by PIL and PSL.
	*/
	inline TAny* HdrToHwDes(const SDmaDesHdr& aHdr) const;


	/** Returns the physical address of the hardware descriptor
		pointed to by aDes. For use by PIL and PSL.
	*/
	inline TUint32 HwDesLinToPhys(TAny* aDes) const;


	/** Called by PIL when one fragment (single-buffer and double-buffer DMACs)
		or list of fragments (scatter/gather DMAC) is to be transferred.

		Called when initiating a new transfer and also, for double-buffer
		DMACs, for configuring the next fragment to transfer while the current
		one is ongoing.

		The function must be implemented by the PSL if
		SCreateInfo::iCaps::iAsymHwDescriptors is reported as false.

		@note This function may be called in thread or ISR context by the PIL

		@param aChannel The channel to use.
		@param aHdr Header associated with fragment to transfer.
	*/
	virtual void Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aHdr);


	/** Called by PIL when two lists of fragments (scatter/gather DMAC with
		asymmetrical linked-list capability) are to be transferred.

		Called when initiating a new transfer.

		The function must be implemented by the PSL if
		SDmaCaps::iAsymHwDescriptors is reported as true.

		@note This function may be called in thread or ISR context by the PIL

		@param aChannel The channel to use.
		@param aSrcHdr Header associated with descriptor to transfer on the
		source side.
		@param aDstHdr Header associated with descriptor to transfer on the
		destination side.
	*/
	virtual void Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aSrcHdr,
						  const SDmaDesHdr& aDstHdr);


	/** Called by PIL to stop a transfer on a given channel.

		The stopping must occur synchronously as the PIL assumes the channel
		is halted after calling this function. A channel stopped via this
		function is not intended to be resumed. Function must always be
		implemented by the PSL.

		@param aChannel The channel to stop
		@post The channel will be idle
		@post No interrupt will occur from this channel until a new
		request is queued.
	*/
	virtual void StopTransfer(const TDmaChannel& aChannel) = 0;


	/** Called by PIL to pause (suspend) a transfer on a given channel.

		A paused channel transfer must be able to be resumed by calling
		ResumeTransfer().

		The function must be implemented by the PSL if
		SDmacCaps::iChannelPauseAndResume is reported as true.

		@return KErrNone if the transfer has been paused successfully,
		KErrCompletion if the transfer was already paused, KErrGeneral
		if a general error occurred preventing a successful outcome.

		@post No interrupt will occur from this channel until it is
		resumed.
	*/
	virtual TInt PauseTransfer(const TDmaChannel& aChannel);


	/** Called by PIL to resume a paused (suspended) transfer on a given
		channel.

		Resume() can be called when the transfer is paused as a result of a
		previous call to PauseTransfer() or because the DMAC has encountered a
		Pause bit in a H/W descriptor.

		The function must be implemented by the PSL if
		SDmacCaps::iChannelPauseAndResume or
		SDmacCaps::iLinkedListPausedInterrupt is reported as true.

		@return KErrNone if the transfer has been resumed successfully,
		KErrCompletion if there was no paused transfer, KErrGeneral
		if a general error occurred preventing a successful outcome.
	*/
	virtual TInt ResumeTransfer(const TDmaChannel& aChannel);


	/** Called by PIL to check whether a DMA channel is idle.

		'Idle' here means that the channel is ultimately stopped, for example
		because the transfer has finished, or an error was encountered, or it
		was manually stopped, but not because it was manually suspended (aka
		'paused'), or it is waiting for a request line assertion to start the
		transfer.

		@param aChannel The channel to test

		@return ETrue if channel idle, EFalse if transferring.
	*/
	virtual TBool IsIdle(const TDmaChannel& aChannel) = 0;


	/** Called by PIL to retrieve from the PSL the maximum transfer length
		based on the parameters passed.

		@param aChannel Channel to be used for the transfer
		@param aSrcFlags Bitmask characterising transfer source
		@see TDmaTransferArgs::iSrcConfig::iFlags
		@param aDstFlags Bitmask characterising transfer destination
		@see TDmaTransferArgs::iDstConfig::iFlags
		@param aPslInfo Cookie passed by client and used by the PSL
		@see TDmaTransferArgs::iPslRequestInfo

		@return 0 if transfer length is not limited, the maximum transfer
		length in bytes otherwise.
	*/
	virtual TUint MaxTransferLength(TDmaChannel& aChannel, TUint aSrcFlags,
									TUint aDstFlags, TUint32 aPslInfo) = 0;


	/** Called by PIL to retrieve from the PSL the memory alignment mask based
		on the parameters passed. Some DMA controllers impose alignment
		constraints on the base address of memory buffers. This mask is AND'ed
		against memory addresses computed during fragmentation.

		The PIL will call this function separately for source and destination.

		An assumption is that the PSL doesn't need to know if a call to this
		function is for the source or the destination side, i.e. both ports
		are, as far as the alignment is concerned, equivalent. All that matters
		are the values of the relevant configuration parameters.

		Another assumption is that the alignment requirement for a port on a
		DMAC with potentially different values for source and destination does
		not depend on the configuration of the respective other port.

		@param aChannel Channel used for the transfer
		@param aTargetFlags Bitmask characterising transfer source or
		destination
		@see TDmaTransferArgs::iSrcConfig::iFlags
		@see TDmaTransferArgs::iDstConfig::iFlags
		@param aElementSize Element size used for the transfer. May be zero if
		not known or 'don't care'.
		@param aPslInfo Cookie passed by client and used by the PSL
		@see TDmaTransferArgs::iPslRequestInfo

		@return A value representing the alignment mask (e.g. 3 if buffer must
		be 4-byte aligned)
	*/
	virtual TUint AddressAlignMask(TDmaChannel& aChannel, TUint aTargetFlags,
								   TUint aElementSize, TUint32 aPslInfo) = 0;


	/** Called by PIL during fragmentation to initialise a hardware descriptor.

		The PSL must assume the descriptor is the last in the chain and so set
		the interrupt bit and set the next descriptor field to an end of chain
		marker.

		The function must be implemented by the PSL if and only if the DMAC
		supports hardware descriptors and SDmaCaps::iAsymHwDescriptors is
		reported as false.

		@param aHdr Header associated with the hardware descriptor to
		initialise
		@param aTransferArgs The transfer parameters for this descriptor

		@return KErrNone if the descriptor was successfully initialized,
		KErrArgument if any of the transfer arguments were detected to be
		invalid, KErrGeneral if a general error occurred preventing a
		successful outcome.
	*/
	virtual TInt InitHwDes(const SDmaDesHdr& aHdr, const TDmaTransferArgs& aTransferArgs);


	/** Called by PIL during fragmentation to initialise a hardware descriptor
		on the source side of an asymmetric linked list.

		The function must be implemented by the PSL if
		SDmaCaps::iAsymHwDescriptors is reported as true.

		@param aHdr Header associated with the hardware descriptor to
		initialise
		@param aTransferArgs The transfer parameters for this descriptor. Only
		the elements relating to the source side should be relevant to the
		implementation.

		@return KErrNone if the descriptor was successfully initialized,
		KErrArgument if any of the transfer arguments were detected to be
		invalid, KErrGeneral if a general error occurred preventing a
		successful outcome.
	*/
	virtual TInt InitSrcHwDes(const SDmaDesHdr& aHdr, const TDmaTransferArgs& aTransferArgs);


	/** Called by PIL during fragmentation to initialise a hardware descriptor
		on the destination side of an asymmetric linked list.

		The function must be implemented by the PSL if
		SDmaCaps::iAsymHwDescriptors is reported as true.

		@param aHdr Header associated with the hardware descriptor to
		initialise
		@param aTransferArgs The transfer parameters for this descriptor. Only
		the elements relating to the destination side should be relevant to the
		implementation.

		@return KErrNone if the descriptor was successfully initialized,
		KErrArgument if any of the transfer arguments were detected to be
		invalid, KErrGeneral if a general error occurred preventing a
		successful outcome.
	*/
	virtual TInt InitDstHwDes(const SDmaDesHdr& aHdr, const TDmaTransferArgs& aTransferArgs);


	/** Called by the PIL in ISR context to change specific fields in a
		hardware descriptor.

		The function must be implemented by the PSL if and only if the DMAC
		supports hardware descriptors and SDmaCaps::iAsymHwDescriptors is
		reported as false.

		@param aHdr Header associated with the hardware descriptor to be
		updated
		@param aSrcAddr @see TDmaTransferArgs::iSrcConfig::iAddr
		@param aDstAddr @see TDmaTransferArgs::iDstConfig::iAddr
		@param aTransferCount @see TDmaTransferArgs::iTransferCount
		@param aPslRequestInfo @see TDmaTransferArgs::iPslRequestInfo

		Since Epoc::LinearToPhysical() cannot be called in ISR context the
		addresses passed into this function are always physical ones, i.e.
		TDmaTransferFlags::KDmaPhysAddr is implied.

		@return KErrNone if the descriptor was successfully modified,
		KErrArgument if any of the transfer arguments were detected to be
		invalid, KErrGeneral if a general error occurred preventing a
		successful outcome.
	*/
	virtual TInt UpdateHwDes(const SDmaDesHdr& aHdr, TUint32 aSrcAddr, TUint32 aDstAddr,
							 TUint aTransferCount, TUint32 aPslRequestInfo);


	/** Called by the PIL in ISR context to change specific fields in a
		hardware descriptor.

		The function must be implemented by the PSL if
		SDmaCaps::iAsymHwDescriptors is reported as true.

		@param aHdr Header associated with the hardware descriptor to be
		updated
		@param aSrcAddr @see TDmaTransferArgs::iSrcConfig::iAddr
		@param aTransferCount @see TDmaTransferArgs::iTransferCount
		@param aPslRequestInfo @see TDmaTransferArgs::iPslRequestInfo

		Since Epoc::LinearToPhysical() cannot be called in ISR context the
		address passed into this function is always a physical ones, i.e.
		TDmaTransferFlags::KDmaPhysAddr is implied.

		@return KErrNone if the descriptor was successfully modified,
		KErrArgument if any of the transfer arguments were detected to be
		invalid, KErrGeneral if a general error occurred preventing a
		successful outcome.
	*/
	virtual TInt UpdateSrcHwDes(const SDmaDesHdr& aHdr, TUint32 aSrcAddr,
								TUint aTransferCount, TUint32 aPslRequestInfo);


	/** Called by the PIL in ISR context to change specific fields in a
		hardware descriptor.

		The function must be implemented by the PSL if
		SDmaCaps::iAsymHwDescriptors is reported as true.

		@param aHdr Header associated with the hardware descriptor to be
		updated
		@param aDstAddr @see TDmaTransferArgs::iDstConfig::iAddr
		@param aTransferCount @see TDmaTransferArgs::iTransferCount
		@param aPslRequestInfo @see TDmaTransferArgs::iPslRequestInfo

		Since Epoc::LinearToPhysical() cannot be called in ISR context the
		address passed into this function is always a physical ones, i.e.
		TDmaTransferFlags::KDmaPhysAddr is implied.

		@return KErrNone if the descriptor was successfully modified,
		KErrArgument if any of the transfer arguments were detected to be
		invalid, KErrGeneral if a general error occurred preventing a
		successful outcome.
	*/
	virtual TInt UpdateDstHwDes(const SDmaDesHdr& aHdr, TUint32 aDstAddr,
								TUint aTransferCount, TUint32 aPslRequestInfo);


	/** Called by PIL, when fragmenting a request, to append a new hardware
		descriptor to an existing descriptor chain. May also be called by
		clients who wish to create their own descriptor chains.

		Must clear the interrupt bit of the descriptor associated with aHdr.

		The function must be implemented by the PSL if and only if the DMAC
		supports hardware descriptors.

		@param aHdr Header associated with last fragment in chain
		@param aNextHdr Header associated with fragment to append
	*/
	virtual void ChainHwDes(const SDmaDesHdr& aHdr, const SDmaDesHdr& aNextHdr);


	/** Called by PIL when queuing a new request while the channel is running.

		Must append the first hardware descriptor of the new request to the
		last descriptor in the existing chain.

		The function must be implemented by the PSL if and only if the DMAC
		supports hardware descriptors.

		@param aChannel The channel where the transfer takes place
		@param aLastHdr Header associated with last hardware descriptor in
		chain
		@param aNewHdr Header associated with first hardware descriptor in new
		request
	*/
	virtual void AppendHwDes(const TDmaChannel& aChannel, const SDmaDesHdr& aLastHdr,
							 const SDmaDesHdr& aNewHdr);


	/** Called by PIL when queuing a new request while the channel is running.

		Must append the first hardware descriptor of the new request to the
		last descriptor in the existing chain.

		The function must be implemented by the PSL if
		SDmaCaps::iAsymHwDescriptors is reported as true.

		@param aChannel The channel where the transfer takes place
		@param aSrcLastHdr Header associated with the last descriptor in the
		source side chain
		@param aSrcNewHdr Header associated with the first source side
		descriptor of the new request
		@param aDstLastHdr Header associated with the last descriptor in the
		destination side chain
		@param aDstNewHdr Header associated with the first destination side
		descriptor of the new request
	*/
	virtual void AppendHwDes(const TDmaChannel& aChannel,
							 const SDmaDesHdr& aSrcLastHdr, const SDmaDesHdr& aSrcNewHdr,
							 const SDmaDesHdr& aDstLastHdr, const SDmaDesHdr& aDstNewHdr);


	/** Called by PIL when completing or cancelling a request to cause the PSL
		to unlink the last item in the h/w descriptor chain from a subsequent
		chain that it was possibly linked to.

		The function must be implemented by the PSL if and only if the DMAC
		supports hardware descriptors.

		@param aChannel The channel where the request (and thus the descriptor)
		was queued
		@param aHdr Header associated with last h/w descriptor in
		completed / cancelled chain
	*/
	virtual void UnlinkHwDes(const TDmaChannel& aChannel, SDmaDesHdr& aHdr);


	/** Called by PIL when freeing descriptors back to the shared pool in
		FreeDesList(). The PSL inside ClearHwDes() can clear the contents of
		the h/w descriptor.

		This may be necessary if the PSL implementation uses the h/w descriptor
		as another header which in turn points to the actual DMA h/w descriptor
		(aka LLI).

		The function may be implemented by the PSL if the DMAC supports
		hardware descriptors.

		@param aHdr Header associated with the h/w descriptor being freed.
	*/
	virtual void ClearHwDes(const SDmaDesHdr& aHdr);


	/** Called by PIL to logically link two physical channels.

		The function must be implemented by the PSL if the DMAC supports
		logical channel linking.

		@see SDmacCaps::iChannelLinking

		@param a1stChannel The channel which is to be linked to another channel
		@param a2ndChannel The channel the first one is to be linked to

		@return KErrNone if the two channels have been linked successfully,
		KErrCompletion if a1stChannel was already linked to a2ndChannel,
		KErrArgument if a1stChannel was already linked to a different channel,
		KErrGeneral if a general error occurred preventing a successful
		outcome. The default PIL implementation returns KErrNotSupported.
	*/
	virtual TInt LinkChannels(TDmaChannel& a1stChannel, TDmaChannel& a2ndChannel);


	/** Called by PIL to logically unlink a physical channel from its linked-to
		successor.

		The function must be implemented by the PSL if the DMAC supports
		logical channel linking.

		@see SDmacCaps::iChannelLinking

		@param aChannel The channel which is to be unlinked from its successor

		@return KErrNone if the channel has been unlinked successfully,
		KErrCompletion if the channel was not linked to another channel,
		KErrGeneral if a general error occurred preventing a successful
		outcome. The default PIL implementation returns KErrNotSupported.
	*/
	virtual TInt UnlinkChannel(TDmaChannel& aChannel);


	/** Called by a test harness to force an error when the next fragment is
		transferred.

		Must be implemented by the PSL only if possible.

		@param aChannel The channel where the error is to occur.

		@return KErrNone if implemented. The default PIL implementation
		returns KErrNotSupported.
	*/
	virtual TInt FailNext(const TDmaChannel& aChannel);


	/** Called by a test harness to force the DMA controller to miss one or
		more interrupts.

		The function must be implemented by the PSL only if possible.

		@param aChannel The channel where the error is to occur
		@param aInterruptCount The number of interrupt to miss.

		@return KErrNone if implemented. The default PIL implementation
		returns KErrNotSupported.
	*/
	virtual TInt MissNextInterrupts(const TDmaChannel& aChannel, TInt aInterruptCount);


	/** Function allowing platform-specific layer to extend channel API with
		new channel-specific operations.

		@see TDmaChannel::ChannelExtension

		@param aChannel Channel to operate on
		@param aCmd Command identifier. Negative values are reserved for use by
		Nokia.
		@param aArg PSL-specific argument

		@return KErrNotSupported if aCmd is not supported. PSL-specific value
		otherwise.
	*/
	virtual TInt Extension(TDmaChannel& aChannel, TInt aCmd, TAny* aArg);


	/** Called by the PIL to query the number of elements that have so far been
		transferred by the hardware descriptor associated with aHdr at the
		source port.

		If SDmacCaps::iAsymHwDescriptors is true then the PIL will call this
		function only for source-side descriptors, and the PSL should fault the
		kernel if this is not the case.

		The function must be implemented (i.e. overridden) by the PSL if and
		only if the DMAC supports hardware descriptors.

		@param aHdr Descriptor header associated with the hardware descriptor
		to be queried

		@return The number of elements that have been transferred by the
		hardware descriptor associated with aHdr at the source port
	*/
	virtual TUint32 HwDesNumSrcElementsTransferred(const SDmaDesHdr& aHdr);


	/** Called by the PIL to query the number of elements that have so far been
		transferred by the hardware descriptor associated with aHdr at the
		destination port.

		If SDmacCaps::iAsymHwDescriptors is true then the PIL will call this
		function only for destination-side descriptors, and the PSL should
		panic if this is not the case.

		The function must be implemented (i.e. overridden) by the PSL if and
		only if the DMAC supports hardware descriptors.

		@param aHdr Descriptor header associated with the hardware descriptor
		to be queried

		@return The number of elements that have been transferred by the
		hardware descriptor associated with aHdr at the destination port
	*/
	virtual TUint32 HwDesNumDstElementsTransferred(const SDmaDesHdr& aHdr);

protected:
	/** Called by the PSL in interrupt context upon a channel interrupt event.

		@param aChannel The channel the ISR relates to
		@param aEventMask Bitmask of one or more TDmaCallbackType values
		@param aIsComplete Set to ETrue if no error was encountered
	*/
	static void HandleIsr(TDmaChannel& aChannel, TUint aEventMask, TBool aIsComplete);

private:
	/** Called in Create() */
	TInt AllocDesPool(TUint aAttribs);

	/** Called in ~TDmac() */
	void FreeDesPool();

	/** Called by the PIL to acquire the controller lock which protects the
		header and descriptor pools.
	*/
	inline void Wait();

	/** Called by the PIL to release the controller lock which protects the
		header and descriptor pools.
	*/
	inline void Signal();

private:
	NFastMutex iLock;			 // protect descriptor reservation and allocation
	const TInt iMaxDesCount;	 // initial number of descriptors and headers
	TInt iAvailDesCount;		 // current available number of descriptors and headers
	SDmaDesHdr* iHdrPool;		 // descriptor header dynamic array
#ifndef __WINS__
	DPlatChunkHw* iHwDesChunk;	 // chunk for hardware descriptor pool
#endif
	TAny* iDesPool;				 // hardware or pseudo descriptor dynamic array
	const TInt iDesSize;		 // descriptor size in bytes

public:
	const TBool iCapsHwDes;		 /*< True if DMAC uses h/w descriptors */
	SDmaDesHdr* iFreeHdr;		 /*< head of unallocated descriptors linked list */

#ifdef _DEBUG
	/** Tests whether aHdr points into the descriptor header array. */
	TBool IsValidHdr(const SDmaDesHdr* aHdr);
#endif
	__DMA_DECLARE_INVARIANT
	};


//////////////////////////////////////////////////////////////////////////////


/** Single-buffer DMA channel.

	Can be instantiated or further derived by the PSL.
*/
class TDmaSbChannel : public TDmaChannel
	{
private:
	virtual void DoQueue(const DDmaRequest& aReq);
	virtual void DoCancelAll();
	virtual void DoDfc(const DDmaRequest& aCurReq, SDmaDesHdr*& aCompletedHdr);

protected:
	enum {EIdle = 0, ETransferring} iState;
	};


/** Double-buffer DMA channel.

	Can be instantiated or further derived by the PSL.
*/
class TDmaDbChannel : public TDmaChannel
	{
private:
	virtual void DoQueue(const DDmaRequest& aReq);
	virtual void DoCancelAll();
	virtual void DoDfc(const DDmaRequest& aCurReq, SDmaDesHdr*& aCompletedHdr);

protected:
	enum {EIdle = 0, ETransferring, ETransferringLast} iState;
	};


/** Scatter-gather DMA channel.

	Can be instantiated or further derived by the PSL.
*/
class TDmaSgChannel : public TDmaChannel
	{
private:
	virtual void DoQueue(const DDmaRequest& aReq);
	virtual void DoCancelAll();
	virtual void DoUnlink(SDmaDesHdr& aHdr);
	virtual void DoDfc(const DDmaRequest& aCurReq, SDmaDesHdr*& aCompletedHdr);

protected:
	enum {EIdle = 0, ETransferring} iState;
	};


/** Scatter-gather DMA channel with asymmetric linked-lists.

	Can be instantiated or further derived by the PSL.

	@prototype
*/
class TDmaAsymSgChannel : public TDmaChannel
	{
public:
	TDmaAsymSgChannel();

private:
	virtual void DoQueue(const DDmaRequest& aReq);
	virtual void DoCancelAll();
	virtual void DoUnlink(SDmaDesHdr& aHdr);
	virtual void DoDfc(const DDmaRequest& aCurReq, SDmaDesHdr*& aSrcCompletedHdr,
					   SDmaDesHdr*& aDstCompletedHdr);
	virtual void SetNullPtr(const DDmaRequest& aReq);
	virtual void ResetNullPtr();

protected:
	SDmaDesHdr* iSrcCurHdr;	  // source fragment being transferred or NULL
	SDmaDesHdr** iSrcNullPtr; // Pointer to NULL pointer following last source fragment
	SDmaDesHdr* iDstCurHdr;	  // destination fragment being transferred or NULL
	SDmaDesHdr** iDstNullPtr; // Pointer to NULL pointer following last destination fragment
	enum {EIdle = 0, ETransferring} iState;

	__DMA_DECLARE_VIRTUAL_INVARIANT
	};


//////////////////////////////////////////////////////////////////////////////

// Trace macros intended for use by the DMA PSL
#define DMA_PRINTF(MSG) __KTRACE_OPT(KDMA, Kern::Printf((MSG)))
#define DMA_PRINTF1(MSG, ARG1) __KTRACE_OPT(KDMA, Kern::Printf((MSG), (ARG1)))
#define DMA_PRINTF2(MSG, ARG1, ARG2) __KTRACE_OPT(KDMA, Kern::Printf((MSG), (ARG1), (ARG2)))

#define DMA_PSL_MESG "DMA PSL: "

// General PSL tracing
#define DMA_PSL_TRACE(MSG) DMA_PRINTF(DMA_PSL_MESG MSG)
#define DMA_PSL_TRACE1(MSG, ARG1) DMA_PRINTF1(DMA_PSL_MESG MSG, (ARG1))
#define DMA_PSL_TRACE2(MSG, ARG1, ARG2) DMA_PRINTF2(DMA_PSL_MESG MSG, (ARG1), (ARG2))


#define DMA_PSL_CHAN_MESG DMA_PSL_MESG "ChanId %d: "
#define DMA_PSL_CHAN_ARGS(CHAN) ((CHAN).PslId())

// For channel specific tracing (where CHAN is a TDmaChannel)
#define DMA_PSL_CHAN_TRACE_STATIC(CHAN, MSG) DMA_PRINTF1(DMA_PSL_CHAN_MESG MSG, DMA_PSL_CHAN_ARGS(CHAN))
#define DMA_PSL_CHAN_TRACE_STATIC1(CHAN, MSG, ARG1) DMA_PRINTF2(DMA_PSL_CHAN_MESG MSG, DMA_PSL_CHAN_ARGS(CHAN), (ARG1))

// For channel specific tracing, for use within methods of TDmaChannel derived
// class
#define DMA_PSL_CHAN_TRACE(MSG) DMA_PSL_CHAN_TRACE_STATIC(*this, MSG)
#define DMA_PSL_CHAN_TRACE1(MSG, ARG1) DMA_PSL_CHAN_TRACE_STATIC1(*this, MSG, (ARG1))



#include <drivers/dma_hai.inl>


#endif	// #ifndef __DMA_HAI_H__
