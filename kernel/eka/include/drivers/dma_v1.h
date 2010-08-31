// Copyright (c) 2002-2010 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0""
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
// include/drivers/dma_v1.h
// DMA Framework API v1
//
// NB: DMA clients should never include this file directly, but only ever the
// generic header file <drivers/dma.h>.
//

#ifndef __DMA_H__
#error "dma_v1.h must'n be included directly - use <drivers/dma.h> instead"
#endif	// #ifndef __DMA_H__

#ifndef __DMA_V1_H__
#define __DMA_V1_H__

#include <kernel/kern_priv.h>


//////////////////////////////////////////////////////////////////////////////
// Debug Support - KDmaPanicCat is defined in each source file

#define __DMA_ASSERTD(e) __ASSERT_DEBUG(e, Kern::Fault(KDmaPanicCat, __LINE__))
#define __DMA_ASSERTA(e) __ASSERT_ALWAYS(e, Kern::Fault(KDmaPanicCat, __LINE__))
#ifdef _DEBUG
#define __DMA_CANT_HAPPEN() Kern::Fault(KDmaPanicCat, __LINE__)
#define __DMA_DECLARE_INVARIANT public: void Invariant();
#define __DMA_INVARIANT() Invariant()
#else
#define __DMA_CANT_HAPPEN()
#define __DMA_DECLARE_INVARIANT
#define __DMA_INVARIANT()
#endif


//////////////////////////////////////////////////////////////////////////////
// INTERFACE EXPOSED TO DEVICE-DRIVERS
//////////////////////////////////////////////////////////////////////////////

/**
Bitmasks used for configuring a DMA request.

In general, specify KDmaMemSrc|KDmaIncSrc (resp. KDmaMemDest|KDmaIncDest) if
the source (resp. destination) is a memory buffer and clear
KDmaMemSrc|KDmaIncSrc (resp. KDmaMemDest|KDmaIncDest) if the source
(resp. destination) is a peripheral.

If the location is given as a physical address (rather than a linear one)
then also specify KDmaPhysAddrSrc and/or KDmaPhysAddrDest.

The EKA1 "Fill Mode" can be implemented by omitting KDmaIncSrc.

Some peripherals may require a post-increment address mode.

@see DDmaRequest::Fragment
@publishedPartner
@released
*/

enum TDmaRequestFlags
	{
	/** Source is address of memory buffer */
	KDmaMemSrc       = 0x01,
	/** Destination is address of memory buffer */
	KDmaMemDest      = 0x02,
	/** Source address must be post-incremented during transfer */
	KDmaIncSrc       = 0x04,
	/** Destination address must be post-incremented during transfer */
	KDmaIncDest      = 0x08,
	/** Source address is a physical address (as opposed to a linear one) */
	KDmaPhysAddrSrc  = 0x10,
	/** Destination address is a physical address (as opposed to a linear one) */
	KDmaPhysAddrDest = 0x20,
	/** Request a different max transfer size (for instance for test purposes) */
	KDmaAltTransferLen = 0x40
	};


//////////////////////////////////////////////////////////////////////////////

class TDmaChannel;
struct SDmaDesHdr;

/** A DMA request is a list of fragments small enough to be transferred in one go
	by the DMAC.

	In general, fragmentation is done in the framework by calling Fragment() but
	clients with special needs can allocate a blank descriptor list with
	ExpandDesList() and customise it to fit their needs.

	Clients should not set attributes directly, but should use the various functions
	instead.

	This class has not been designed to be called from several concurrent threads.
	Multithreaded clients must implement their own locking scheme (via DMutex).

	Fast mutexes are used internally to protect data structures accessed both
	by the client thread and the DFC thread.  Therefore no fast mutex can be held
	when calling a request function.

	@publishedPartner
	@released
 */
class DDmaRequest : public DBase
	{
	friend class TDmaChannel;
public:
	/** The outcome of the transfer */
	enum TResult {EBadResult=0, EOk, EError};
	/** The signature of the completion/failure callback function */
	typedef void (*TCallback)(TResult, TAny*);
public:
   
    /**
    Create a new transfer request. 

    @param aChannel The channel this request is bound to.
    @param aCb      Callback function called on transfer completion or failure (in channel
                    DFC context).  Can be NULL.
    @param aCbArg   Argument passed to callback function.
    @param aMaxTransferSize Maximum fragment size.  If not specified, defaults to the maximum size
           supported by the DMA controller for the type of transfer that is later scheduled.
    */
	IMPORT_C DDmaRequest(TDmaChannel& aChannel, TCallback aCb=NULL, TAny* aCbArg=NULL, TInt aMaxTransferSize=0);
	
	
	/**
    Destructor.

    Assume the request is not being transferred or pending.
    */
	IMPORT_C ~DDmaRequest();
	
	
	/**
    Split request into a list of fragments small enough to be fed to the DMAC.

    The size of each fragment is smaller than or equal to the maximum transfer size
    supported by the DMAC.  If the source and/or destination is memory, each
    fragment points to memory which is physically contiguous.

    The kind of transfer to perform is specified via a set of flags used by a PIL
    and a magic cookie passed to the PSL.  If the source (resp. destination) is a
    peripheral, aSrc (resp. aDest) is treated as a magic cookie by the PIL and
    passed straight to the PSL.

    The request can be uninitialised or may have been fragmented previously.  The
    previous configuration if any is lost whether or not the function succeeds.

    The client must ensure that any memory buffers involved in the transfer
    have been suitably prepared for DMA. For memory allocated on the kernel
    side or in a shared chunk this amounts to ensuring cache consistency
    before Queue() is called. However for memory that was allocated on the
    user side the client must also ensure that the memory is protected from
    both data paging and RAM defragmentation before Fragment() is called
    @see Kern::MapAndPinMemory(). Note however, that this function is only
    available if the flexible memory model (FMM) is in use.

    @param aSrc     Source memory buffer linear address or peripheral magic cookie.
    @param aDest    Destination memory buffer linear address or peripheral magic cookie.
    @param aCount   Number of bytes to transfer.
    @param aFlags   Bitmask characterising the transfer.
    @param aPslInfo Hardware-specific information passed to PSL.

    @return KErrNone if success. KErrArgument if aFlags and/or aPslInfo are invalid when finding
    the maximum transfer size. May also fail if running out of descriptors.

    @pre The request is not being transferred or pending.
    @pre The various parameters must be valid.  The PIL or PSL will fault the
    kernel if not.

    @see TDmaRequestFlags
    */
	IMPORT_C TInt Fragment(TUint32 aSrc, TUint32 aDest, TInt aCount, TUint aFlags, TUint32 aPslInfo);
	
	
	/**
    Transfer asynchronously this request.

    If this request's channel is idle, the request is transferred immediately.
    Otherwise, it is queued and transferred later.

    The client is responsible for ensuring cache consistency before and/or after the
    transfer if necessary.
    */
	IMPORT_C void Queue();
	

    /**
    Append new descriptor(s) to existing list.

    Clients needing to build a custom descriptor list should call this function to
    allocate the list and access the resulting list through iFirstHdr and iLastHdr.

    Clients should not change the value of iFirstHdr, iLastHdr and the iNext field
    of the descriptor headers to ensure descriptors can be deallocated. Clients
    are free to change hardware descriptors, including chaining, in whatever way
    suit them.

    Assume the request is not being transferred or pending.

    @param aCount Number of descriptors to append.

    @return KErrNone or KErrTooBig if not enough descriptors available.
    */
	IMPORT_C TInt ExpandDesList(TInt aCount=1);
	
	
	/**
    Free resources associated with this request.

    Assume the request is not being transferred or pending.
    */
	IMPORT_C void FreeDesList();
private:
	inline void OnDeque();
public:
	// WARNING: The following attributes are accessed both in client and DFC
	// context and so accesses must be protected with the channel lock.
	TDmaChannel& iChannel;		/**< The channel this request is bound to */
	volatile TCallback iCb;		/**< Called on completion/failure (can be NULL) */
	TAny* volatile iCbArg;		/**< Callback argument */
	TInt iDesCount;				/**< The number of fragments in list */
	SDmaDesHdr* iFirstHdr;		/**< The first fragment in the list (or NULL) */
	SDmaDesHdr* iLastHdr;		/**< The last fragment in the list (or NULL) */
	SDblQueLink iLink;			/**< The link on channel queue of pending requests */
	TBool iQueued;				/**< Indicates whether request is pending or being transferred */
	TInt iMaxTransferSize;		/**< Defaults to DMA controller max. transfer size */
	__DMA_DECLARE_INVARIANT
	};


//////////////////////////////////////////////////////////////////////////////

class TDmac;
class DmaChannelMgr;

/** DMA channel base class.

	This class has not been designed to be called from several concurrent
	client threads.  Multithreaded clients must implement their own locking
	scheme (via DMutex).

	Fast mutexes are used internally to protect data structures accessed both
	by the client thread and the DFC one.  Therefore no fast mutex can be held
	when calling a channel function.

	Must be allocated in BSS because it relies on being zeroed at
	creation-time.  If the PSL really needs to allocate channels on the kernel
	heap, it must manually zero-initialises the instances.  This can be
	achieved either by allocating raw memory and using placement new, or by
	wrapping channels into a DBase-derived wrapper.

	@publishedPartner
	@released
 */
class TDmaCancelInfo;
class TDmaChannel
	{
	friend class DDmaRequest;
	friend class TDmac;
	friend class DmaChannelMgr;
public:
	/**  Information passed by client when opening channel */
	struct SCreateInfo
		{
		/** Identifier used by PSL to select channel to open */
		TUint32 iCookie;
		/** Number of descriptors this channel can use */
		TInt iDesCount;
		/** DFC queue used to service DMA interrupts */
		TDfcQue* iDfcQ;
		/** DFC priority */
		TUint8 iDfcPriority;
		};
public:
    /**
 	Opens the DMA channel.

 	Channel selection is done by the hardware-specific layer using a cookie passed in
 	via aInfo.

 	The client should not delete the returned pointer as the framework owns
 	channel objects.  However, the client should explicitly close the channel when
 	finished with it.

	@param aInfo    Information passed by caller to select and configure channel.
 	@param aChannel Point to open channel on successful return.  NULL otherwise.

 	@return KErrNone or standard error code.
 	*/
	IMPORT_C static TInt Open(const SCreateInfo& aInfo, TDmaChannel*& aChannel);
	
	
	/**
 	Closes a previously opened DMA channel.

 	Assume the channel is idle and all requests have been deleted.
 	*/
	IMPORT_C void Close();
	
	
	/**
 	Cancels the current request and all the pending ones.
 	*/
	IMPORT_C void CancelAll();
	inline TBool IsOpened() const;
	inline TBool IsQueueEmpty() const;
	inline TUint32 PslId() const;
	inline TInt FailNext(TInt aFragmentCount);
	inline TInt MissNextInterrupts(TInt aInterruptCount);
	inline TInt Extension(TInt aCmd, TAny* aArg);
	
	/**
	This is a function that allows the Platform Specific Layer (PSL) to extend the DMA API
	with new channel-independent operations.

	@param aCmd Command identifier.  Negative values are reserved for Symbian use.
	@param aArg PSL-specific.
	
	@return KErrNotSupported if aCmd is not supported; a  PSL specific value otherwise.
 	*/
	IMPORT_C TInt StaticExtension(TInt aCmd, TAny* aArg);
	inline const TDmac* Controller() const;
	inline TInt MaxTransferSize(TUint aFlags, TUint32 aPslInfo);
	inline TUint MemAlignMask(TUint aFlags, TUint32 aPslInfo);
protected:
	// Interface with state machines
	TDmaChannel();
	virtual void DoQueue(DDmaRequest& aReq) = 0;
	virtual void DoCancelAll() = 0;
	virtual void DoUnlink(SDmaDesHdr& aHdr);
	virtual void DoDfc(DDmaRequest& aCurReq, SDmaDesHdr*& aCompletedHdr) = 0;
	/**
		This function allows the Platform Specific Layer (PSL) to control the
		power management of the channel or its controller by overriding the
		PIL's default implementation (which does nothing) and making
		appropriate use of the Power Resource Manager (PRM).

		The function gets called by the PIL whenever the channel's queued
		requests count has changed in a significant way, either before the
		channel's Transfer() method is invoked for a request on a previously
		empty request queue, or immediately after the request count has become
		zero because of request cancellation or completion.

		Depending on the current and previous observed values of
		iQueuedRequests, the PSL may power down or power up the channel.

		Note that iQueuedRequests gets accessed and changed by different
		threads, so the PSL needs to take the usual precautions when evaluating
		the variable's value. Also, due to the multithreaded framework
		architecture, there is no guarantee that the function calls always
		arrive at the PSL level in the strict chronological order of
		iQueuedRequests being incremented/decremented in the PIL, i.e. it might
		happen that the PSL finds iQueuedRequests to have the same value in two
		or more consecutive calls (that's why the previous observed value needs
		to be locally available and taken into account). It is however promised
		that before any actual transfer commences the PSL will find the request
		count to be greater than zero and that after the last request has
		finished it will be found to be zero.

		None of the internal DMA framework mutexes is being held by the PIL
		when calling this function.

		Here is an example implementation for a derived channel class:

		@code

		class TFooDmaChannel : public TDmaSgChannel
			{
			DMutex* iDmaMutex;
			TInt iPrevQueuedRequests;
			virtual void QueuedRequestCountChanged();
			};

		void TFooDmaChannel::QueuedRequestCountChanged()
			{
			Kern::MutexWait(*iDmaMutex);
			const TInt queued_now = __e32_atomic_load_acq32(&iQueuedRequests);
			if ((queued_now > 0) && (iPrevQueuedRequests == 0))
				{
				IncreasePowerCount(); // Base port specific
				}
			else if ((queued_now == 0) && (iPrevQueuedRequests > 0))
				{
				DecreasePowerCount(); // Base port specific
				}
			iPrevQueuedRequests = queued_now;
			Kern::MutexSignal(*iDmaMutex);
			}

		@endcode

		@see iQueuedRequests
	*/
	virtual void QueuedRequestCountChanged();
#if defined(__CPU_ARM) && !defined(__EABI__)
	inline virtual ~TDmaChannel() {}	// kill really annoying warning
#endif
private:
	static void Dfc(TAny*);
	void DoDfc();
	inline void Wait();
	inline void Signal();
	inline TBool Flash();
	void ResetStateMachine();
protected:
	TDmac* iController;										// DMAC this channel belongs to (NULL when closed)
	TUint32 iPslId;											// unique identifier provided by PSL
	NFastMutex iLock;										// for data accessed in both client & DFC context
	SDmaDesHdr* iCurHdr;									// fragment being transferred or NULL
	SDmaDesHdr** iNullPtr;									// Pointer to NULL pointer following last fragment
	TDfc iDfc;												// transfer completion/failure DFC
	TInt iMaxDesCount;										// maximum number of allocable descriptors
	TInt iAvailDesCount;									// available number of descriptors
	volatile TUint32 iIsrDfc;								// Interface between ISR and DFC:
	enum { KErrorFlagMask = 0x80000000 };					// bit 31 - error flag
	enum { KCancelFlagMask = 0x40000000 };					// bit 30 - cancel flag
	enum { KDfcCountMask = 0x3FFFFFFF };					// bits 0-29 - number of queued DFCs
	SDblQue iReqQ;											// being/about to be transferred request queue
	TInt iReqCount;											// number of requests attached to this channel
	TInt iQueuedRequests; 									// number of requests currently queued on this channel
	TBool iCallQueuedRequestFn;								// call QueuedRequestCountChanged? (default: true)
private:
	TDmaCancelInfo* iCancelInfo;
	__DMA_DECLARE_INVARIANT
	};


//////////////////////////////////////////////////////////////////////////////
// PIL-PSL INTERFACE
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


/**
Generic DMA descriptor used if the DMAC does not have support for hardware
descriptor.
@see DDmaRequest::Fragment
@publishedPartner
@released
*/

struct SDmaPseudoDes
	{
	/** Source linear address or peripheral cookie */
	TUint32 iSrc;
	/** Destination linear address or peripheral cookie */
	TUint32 iDest;
	/** Number of bytes to transfer */
	TInt iCount;
	/** @see TDmaRequestFlags */
	TUint iFlags;
	/** PSL-specific information provided by client */
	TUint32 iPslInfo;
	/** The same as TDmaChannel::SCreateInfo.iCookie */
	TUint32 iCookie;
	};


/**
Each hardware or pseudo descriptor is associated with a header.  Headers are
needed because hardware descriptors can not easily be extended to store
additional information.
@publishedPartner
@released
*/

struct SDmaDesHdr
	{
	SDmaDesHdr* iNext;
	};


/**
Interface used by PIL to open and close DMA channels.

Must be implemented by PSL.
@publishedPartner
@released
*/

class DmaChannelMgr
	{
public:
	/** Opens a channel using a client-provided identifier.
		This function must be implemented by the PSL.
		@param	aOpenId Magic cookie passed by client
				This may identify the channel (if a static channel
				allocation scheme is used) or may indicate some
				properties which the channel must possess (if a dynamic
				channel allocation scheme is used). It may be set to
				zero always if dynamic allocation is used and all
				channels are equivalent.
		@return	Pointer to channel if available, NULL otherwise.
		@pre	The PIL calls this function with a global fast mutex held to
				avoid race conditions.
		@post	If a non-NULL pointer is returned, the object pointed to has its
				iController and iPslId members set to valid states.
				iController should point to the controller handling that channel.
				iPslId should contain a value uniquely identifying the channel -
				it is used only for debug tracing by PIL. It can be given any
				convenient value by PSL	(channel index, I/O port address, ...).
	*/
	static TDmaChannel* Open(TUint32 aOpenId);

	/** Performs platform-specific operations when a channel is closed.
		This function must be implemented by the PSL but the implementation can be
		a no-op.
		@param aChannel The channel to close
		@pre The PIL calls this function with a global fast mutex held to
			avoid race conditions.
	*/
	static void Close(TDmaChannel* aChannel);

	/** Function allowing PSL to extend DMA API with new channel-independent operations.
		This function must be implemented by the PSL.
		@param aCmd Command identifier.  Negative values are reserved for Symbian use.
		@param aArg PSL-specific
		@return KErrNotSupported if aCmd is not supported.  PSL-specific value otherwise.
	 */
	static TInt StaticExtension(TInt aCmd, TAny* aArg);

	static inline void Wait();
	static inline void Signal();
private:
	static NFastMutex Lock;
	};


//////////////////////////////////////////////////////////////////////////////

/**
 Abstract base class representing a DMA controller.

 The class has two purposes.

 First, it is a container for channels, descriptors and descriptor headers.

 Second, it exposes a set of virtual functions implemented by
 the PSL (platform-specific layer).
 These functions are the main interfaces between
 the PIL (platform-independent layer) and PSL.

 Must be allocated in BSS because it relies on being zeroed at creation-time.

 @publishedPartner
 @released
 */

class TDmac
	{
	friend class DmaChannelMgr;
// protected: VC++ complains when building PSL if following decl is protected
public:
	/** Data required for creating a new instance */
	struct SCreateInfo
		{
		/** Number of channels in controller */
		TInt iChannelCount;
        /** Maximum number of descriptors (shared by all channels) */
		TInt iDesCount;
		/** Bitmask.  The only supported value is KCapsBitHwDes (hardware
			descriptors used). */
		TUint32 iCaps;
		/** Size of individual descriptors.  Use sizeof(SDmaPseudoDes) for
		 	single-buffer and double-buffer controllers. */
		TInt iDesSize;
		/** Bitmask used when creating the hardware chunk storing the descriptor
			pool. Used only for hardware descriptors. The access part must be
			EMapAttrSupRw.  If the chunk is cached and/or buffered, the PSL must
			flush the data cache and/or drain the write buffer in InitHwDes()
			and related functions.
		 	@see TMappingAttributes
		 */
		TUint iDesChunkAttribs;
		};
public:
	TInt Create(const SCreateInfo& aInfo);
	virtual ~TDmac();
	TInt ReserveSetOfDes(TInt aCount);
	void ReleaseSetOfDes(TInt aCount);
	void InitDes(const SDmaDesHdr& aHdr, TUint32 aSrc, TUint32 aDest, TInt aCount,
				 TUint aFlags, TUint32 aPslInfo, TUint32 aCookie);
	inline SDmaPseudoDes& HdrToDes(const SDmaDesHdr& aHdr) const;
	inline TAny* HdrToHwDes(const SDmaDesHdr& aHdr) const;
	inline TUint32 DesLinToPhys(TAny* aDes) const;
	inline void Wait();
	inline void Signal();
protected:
	TDmac(const SCreateInfo& aInfo);

public:
	/**
	Called by PIL when one fragment (single-buffer and double-buffer DMACs) or
	list of fragments (scatter/gather DMAC) is to be transferred.

	Called when	initiating a new transfer and also, for double-buffer DMACs, for
	configuring the next fragment to transfer while the current one is
	ongoing. Must always be implemented by PSL.
	@param aChannel The channel to use
	@param aHdr Header associated with fragment to transfer
	*/
	virtual void Transfer(const TDmaChannel& aChannel, const SDmaDesHdr& aHdr) = 0;

	/**
    Called by PIL to suspend transfer on a given channel.

    The suspension must occur synchronously as the PSL assumes the channel
    is suspended after calling this function. Must always be implemented by PSL.
	@param aChannel The channel to suspend
	*/
	virtual void StopTransfer(const TDmaChannel& aChannel) = 0;

	/**
	Called by PIL to check whether a DMA channel is idle.
	@param aChannel The channel to test
	@return ETrue if channel idle, EFalse if transferring.
	 */
	virtual TBool IsIdle(const TDmaChannel& aChannel) = 0;

	/**
	Called by PIL to retrieve from the PSL the maximum transfer size based on the
	parameters passed.
	@param aChannel Channel to be used for the transfer
	@param aFlags Bitmask characterising transfer
	@param aPslInfo Cookie passed by client and used by PSL
	@return 0 if invalid argument(s), -1 if transfer size not limited, the maximum
	transfer size otherwise.
	*/
	virtual TInt MaxTransferSize(TDmaChannel& aChannel, TUint aFlags, TUint32 aPslInfo) = 0;

	/**
	Called by PIL to retrieve from the PSL the memory alignment mask based on the
	parameters passed. Some DMA controllers impose alignment constraints on the base
	address of memory buffers. This mask is AND'ed against memory addresses computed
	during fragmentation.
	@param aChannel Channel to be used for the transfer
	@param aFlags Bitmask characterising transfer
	@param aPslInfo Cookie passed by client and used by PSL
	@return A value representing the alignment mask (e.g. 3 if buffer must be 4-byte aligned)
	*/
	virtual TUint MemAlignMask(TDmaChannel& aChannel, TUint aFlags, TUint32 aPslInfo) = 0;

	/**
    Called by PIL during fragmentation to initialise a hardware descriptor.

    The PSL must assume the descriptor is the last in the chain and so set the
	interrupt bit and set the next descriptor field to an end of chain marker.
	Must be implemented by PSL if and only if the DMAC supports hardware
	descriptors.
	@param aHdr Header associated with hardware descriptor to initialise
	@param aSrc Transfer source
	@param aDest Transfer destination
	@param aCount Number of bytes to transfer (<= max. size supported by DMAC)
	@param aFlags Bitmask characterising transfer
	@param aPslInfo Cookie passed by client and used by PSL
	@param aCookie the channel selection cookie
	@see DDmaRequest::Fragment
	*/
	virtual void InitHwDes(const SDmaDesHdr& aHdr, TUint32 aSrc, TUint32 aDest, TInt aCount,
						   TUint aFlags, TUint32 aPslInfo, TUint32 aCookie);

	/**
	Called by PIL, when fragmenting a request, to append a new hardware
	descriptor to an existing descriptor chain.

	Must clear the interrupt bit of	the descriptor associated with aHdr.
	Must be implemented by PSL if and only if the DMAC supports hardware descriptors.
	@param aHdr Header associated with last fragment in chain
	@param aNextHdr Header associated with fragment to append
	*/
	virtual void ChainHwDes(const SDmaDesHdr& aHdr, const SDmaDesHdr& aNextHdr);

	/**
	Called by PIL when queuing a new request while the channel is running.

	Must append the first hardware descriptor of the new request to the last
	descriptor in the existing chain. Must be implemented by PSL if and only if
	the DMAC supports hardware descriptors.
	@param aChannel The channel where the transfer takes place
	@param aLastHdr Header associated with last hardware descriptor in chain
	@param aNewHdr Header associated with first hardware descriptor in new request
	*/
	virtual void AppendHwDes(const TDmaChannel& aChannel, const SDmaDesHdr& aLastHdr,
							 const SDmaDesHdr& aNewHdr);

	/**
	Called by PIL when completing or cancelling a request to cause the PSL to unlink
	the last item in the h/w descriptor chain from a subsequent chain that it was
	possibly linked to. Must be implemented by the PSL if and only if the DMAC supports
	hardware descriptors.

	@param aChannel The channel where the request (and thus the descriptor) was queued
	@param aHdr Header associated with last h/w descriptor in completed/cancelled chain
	*/
	virtual void UnlinkHwDes(const TDmaChannel& aChannel, SDmaDesHdr& aHdr);

	/**
	Called by test harness to force an error when the next fragment is
	transferred.

	Must be implemented by the PSL only if possible.
	@param aChannel The channel where the error is to occur.
	@return KErrNone if implemented.  The default PIL implementation returns
	KErrNotSupported and the test harness knows how to deal with that.
	*/
	virtual TInt FailNext(const TDmaChannel& aChannel);

	/**
	Called by test harness to force the DMA controller to miss one or
	more interrupts.

	Must be implemented by the PSL only if possible.
	@param aChannel The channel where the error is to occur
	@param aInterruptCount The number of interrupt to miss.
	@return KErrNone if implemented.  The default PIL implementation returns
	KErrNotSupported and the test harness knows how to deal with that.
	*/
	virtual TInt MissNextInterrupts(const TDmaChannel& aChannel, TInt aInterruptCount);

	/** Function allowing platform-specific layer to extend channel API with
		new channel-specific operations.
		@param aChannel Channel to operate on
		@param aCmd Command identifier.  Negative values are reserved for Symbian use.
		@param aArg PSL-specific
		@return KErrNotSupported if aCmd is not supported.  PSL-specific value otherwise.
		@see TDmaChannel::Extension
	*/
	virtual TInt Extension(TDmaChannel& aChannel, TInt aCmd, TAny* aArg);

protected:
	static void HandleIsr(TDmaChannel& aChannel, TBool aIsComplete);
private:
	TInt AllocDesPool(TUint aAttribs);
	void FreeDesPool();
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
	const TUint iCaps;  		 /*< what is supported by DMA controller */
	enum {KCapsBitHwDes = 1};	 /*< hardware descriptors supported */
	SDmaDesHdr* iFreeHdr;		 /*< head of unallocated descriptors linked list */
#ifdef _DEBUG
	TBool IsValidHdr(const SDmaDesHdr* aHdr);
#endif
	__DMA_DECLARE_INVARIANT
	};


//////////////////////////////////////////////////////////////////////////////

/**
Single-buffer DMA channel.

Can be instantiated or further derived by PSL.  Not
intended to be instantiated by client device drivers.
@publishedPartner
@released
*/

class TDmaSbChannel : public TDmaChannel
	{
private:
	virtual void DoQueue(DDmaRequest& aReq);
	virtual void DoCancelAll();
	virtual void DoDfc(DDmaRequest& aCurReq, SDmaDesHdr*& aCompletedHdr);
private:
	TBool iTransferring;
	};


/**
Double-buffer DMA channel.

Can be instantiated or further derived by PSL.  Not
intended to be instantiated by client device drivers.
@publishedPartner
@released
*/

class TDmaDbChannel : public TDmaChannel
	{
private:
	virtual void DoQueue(DDmaRequest& aReq);
	virtual void DoCancelAll();
	virtual void DoDfc(DDmaRequest& aCurReq, SDmaDesHdr*& aCompletedHdr);
private:
	enum { EIdle = 0, ETransferring, ETransferringLast } iState;
	};


/**
Scatter-gather DMA channel.

Can be instantiated or further derived by PSL.
Not intended to be instantiated by client device drivers.
@publishedPartner
@released
*/

class TDmaSgChannel : public TDmaChannel
	{
private:
	virtual void DoQueue(DDmaRequest& aReq);
	virtual void DoCancelAll();
	virtual void DoUnlink(SDmaDesHdr& aHdr);
	virtual void DoDfc(DDmaRequest& aCurReq, SDmaDesHdr*& aCompletedHdr);
private:
	TBool iTransferring;
	};


//////////////////////////////////////////////////////////////////////////////
// INTERFACE WITH TEST HARNESS
//////////////////////////////////////////////////////////////////////////////

/**
Set of information used by test harness.
@publishedPartner
@released
*/

struct TDmaTestInfo
	{
	/** Maximum transfer size in bytes for all channels (ie. the minimum of all channels' maximum size)*/
	TInt iMaxTransferSize;
	/** 3->Memory buffers must be 4-byte aligned, 7->8-byte aligned, ... */
	TUint iMemAlignMask;
	/** Cookie to pass to DDmaRequest::Fragment for memory-memory transfer*/
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


/**
Provides access to test information structure stored in the PSL.

Must be implemented by the PSL.
@publishedPartner
@released
*/

IMPORT_C const TDmaTestInfo& DmaTestInfo();


//////////////////////////////////////////////////////////////////////////////

#include <drivers/dma_v1.inl>

#endif
