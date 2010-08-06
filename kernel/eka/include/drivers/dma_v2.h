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
// include/drivers/dma_v2.h
// DMA Framework - Client API v2 definition.
//
// NB: DMA clients should never include this file directly, but only ever the
// generic header file <drivers/dma.h>.
//

/** @file
	@publishedPartner
*/

#ifndef __DMA_H__
#error "dma_v2.h must'n be included directly - use <drivers/dma.h> instead"
#endif	// #ifndef __DMA_H__

#ifndef __DMA_V2_H__
#define __DMA_V2_H__


#include <kernel/kernel.h>
#include <drivers/dmadefs.h>


//////////////////////////////////////////////////////////////////////////////
// Debug Support - KDmaPanicCat is defined in each source file

#define __DMA_ASSERTD(e) __ASSERT_DEBUG(e, Kern::Fault(KDmaPanicCat, __LINE__))
#define __DMA_ASSERTA(e) __ASSERT_ALWAYS(e, Kern::Fault(KDmaPanicCat, __LINE__))
#ifdef _DEBUG
#define __DMA_CANT_HAPPEN() Kern::Fault(KDmaPanicCat, __LINE__)
#define __DMA_DECLARE_INVARIANT public: void Invariant();
#define __DMA_DECLARE_VIRTUAL_INVARIANT public: virtual void Invariant();
#define __DMA_INVARIANT() Invariant()
#else
#define __DMA_CANT_HAPPEN()
#define __DMA_DECLARE_INVARIANT
#define __DMA_DECLARE_VIRTUAL_INVARIANT
#define __DMA_INVARIANT()
#endif

#ifdef __DMASIM__
#ifdef __PRETTY_FUNCTION__
#define __DMA_UNREACHABLE_DEFAULT() DMA_PSL_TRACE1("Calling default virtual: %s", __PRETTY_FUNCTION__)
#else
#define __DMA_UNREACHABLE_DEFAULT() DMA_PSL_TRACE("Calling default virtual function")
#endif
#else
#define __DMA_UNREACHABLE_DEFAULT() __DMA_CANT_HAPPEN()
#endif

//////////////////////////////////////////////////////////////////////////////
// INTERFACE EXPOSED TO DEVICE-DRIVERS
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

/** Bitmasks used for configuring a DMA request.

	In general, specify KDmaMemSrc|KDmaIncSrc (resp. KDmaMemDest|KDmaIncDest)
	if the source (resp. destination) is a memory buffer and clear
	KDmaMemSrc|KDmaIncSrc (resp. KDmaMemDest|KDmaIncDest) if the source
	(resp. destination) is a peripheral.

	If the location is given as a physical address (rather than a linear one)
	then also specify KDmaPhysAddrSrc and/or KDmaPhysAddrDest.

	The EKA1 "Fill Mode" can be implemented by omitting KDmaIncSrc.

	Some peripherals may require a post-increment address mode.

	@see DDmaRequest::Fragment()

	Note: This enum is only required for backwards compatibility with the old
	DMA framework, it can be removed once this is no longer needed.

	@deprecated
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


/** Each hardware or pseudo descriptor is associated with a header.  Headers
	are needed because hardware descriptors can not easily be extended to store
	additional information.

	@released
*/
struct SDmaDesHdr
	{
	SDmaDesHdr* iNext;
	};


/** Pointer to signature of the new extended callback function.

	TUint       - bitmask of one or more TDmaCallbackType values
	TDmaResult  - just that
	TAny*       - was provided by client in DDmaRequest constructor
	SDmaDesHdr* - points to header (and thus descriptor) which caused a
	'descriptor completed' or 'descriptor paused' event

	@released
 */
typedef void (*TDmaCallback)(TUint, TDmaResult, TAny*, SDmaDesHdr*);


class TDmaChannel;


/** A DMA request is a list of fragments small enough to be transferred in one go
	by the DMAC.

	In general, fragmentation is done in the framework by calling Fragment() but
	clients with special needs can allocate a blank descriptor list with
	ExpandDesList() and customise it to fit their needs.

	Clients should not set attributes directly, but should use the various functions
	instead.

	This class has not been designed to be called from several concurrent threads.
	Multithreaded clients must implement their own locking scheme (via DMutex).

	Mutexes are used internally to protect data structures accessed both by the
	client thread and the DFC thread. Therefore no fast mutex can be held when
	calling a request function.
*/
class DDmaRequest : public DBase
	{
	friend class TDmaChannel;

public:
	/** The outcome of the transfer

		@see TDmaResult

		@deprecated
	*/
	enum TResult {EBadResult=0, EOk, EError};

	/** The signature of the completion/failure callback function

		@see TDmaCallback

		@deprecated
	*/
	typedef void (*TCallback)(TResult, TAny*);

public:
    /** Constructor.

		Create a new transfer request.

		@param aChannel The channel this request is bound to.
		@param aCb Callback function called on transfer completion or failure
		(in channel DFC context).  Can be NULL.
		@param aCbArg   Argument passed to callback function.
		@param aMaxTransferSize Maximum fragment size.  If not specified, defaults to the maximum size
		supported by the DMA controller for the type of transfer that is later scheduled.

		@deprecated
    */
	IMPORT_C DDmaRequest(TDmaChannel& aChannel, TCallback aCb=NULL, TAny* aCbArg=NULL,
						 TInt aMaxTransferSize=0);


	/** Constructor.

		Create a new transfer request.

		@param aChannel The channel this request is bound to.
		@param aDmaCb Callback function called on transfer completion or
		failure (in channel DFC or ISR context). Can be NULL.
		@param aCbArg Argument passed to callback function.
		@param aMaxTransferSize Maximum fragment size. If not specified,
		defaults to the maximum size supported by the DMA controller for the
		type of transfer that is later scheduled.

		@released
	*/
	IMPORT_C DDmaRequest(TDmaChannel& aChannel, TDmaCallback aDmaCb,
						 TAny* aCbArg=NULL, TUint aMaxTransferSize=0);


	/** Destructor.

		Assume the request is not being transferred or pending.

		@released
	*/
	IMPORT_C ~DDmaRequest();


	/** Split request into a list of fragments small enough to be fed to the
		DMAC.

		The size of each fragment is smaller than or equal to the maximum
		transfer size supported by the DMAC. If the source and/or destination
		is memory, each fragment points to memory which is physically
		contiguous.

		The kind of transfer to perform is specified via a set of flags used by
		a PIL and a magic cookie passed to the PSL. If the source
		(resp. destination) is a peripheral, aSrc (resp. aDest) is treated as a
		magic cookie by the PIL and passed straight to the PSL.

		The request can be uninitialised or may have been fragmented
		previously. The previous configuration if any is lost whether or not
		the function succeeds.

		The client must ensure that any memory buffers involved in the transfer
		have been suitably prepared for DMA. For memory allocated on the kernel
		side or in a shared chunk this amounts to ensuring cache consistency
		before Queue() is called. However for memory that was allocated on the
		user side the client must also ensure that the memory is protected from
		both data paging and RAM defragmentation before Fragment() is called
		@see Kern::MapAndPinMemory(). Note however, that this function is only
		available if the flexible memory model (FMM) is in use.

		@param aSrc Source memory buffer linear address or peripheral magic
		cookie.
		@param aDest Destination memory buffer linear address or peripheral
		magic cookie.
		@param aCount Number of bytes to transfer.
		@param aFlags Bitmask characterising the transfer.
		@param aPslInfo Hardware-specific information passed to PSL.

		@return KErrNone if success. KErrArgument if aFlags and/or aPslInfo are
		invalid when finding the maximum transfer size. May also fail if
		running out of descriptors.

		@pre The request is not being transferred or pending.
		@pre The various parameters must be valid. The PIL or PSL will fault the
		kernel if not.

		@see TDmaRequestFlags

		@deprecated
    */
	IMPORT_C TInt Fragment(TUint32 aSrc, TUint32 aDest, TInt aCount, TUint aFlags, TUint32 aPslInfo);


	/** New version of the DMA request fragment function, to be used with the
		TDmaTransferArgs structure.

		Split request into a list of fragments small enough to be fed to the
		DMAC.

		The size of each fragment is smaller than or equal to the maximum
		transfer size supported by the DMAC. If the source and/or destination
		is memory, each fragment points to memory which is physically
		contiguous.

		The request can be uninitialised or may have been fragmented
		previously. Any previous configuration is lost whether or not the
		function succeeds.

		The client must ensure that any memory buffers involved in the transfer
		have been suitably prepared for DMA. For memory allocated on the kernel
		side or in a shared chunk this amounts to ensuring cache consistency
		before Queue() is called. However for memory that was allocated on the
		user side the client must also ensure that the memory is protected from
		both data paging and RAM defragmentation before Fragment() is called
		@see Kern::MapAndPinMemory(). Note however, that this function is only
		available if the flexible memory model (FMM) is in use.

		@param aTransferArgs Describes the transfer to be performed.

		@return KErrNone if success. KErrArgument if certain arguments are
		invalid. May also fail if running out of descriptors.

		@pre The request is not being transferred or pending.
		@pre The various parameters must be valid. The PIL or PSL will fault
		the kernel if not.

		@released
	*/
	IMPORT_C TInt Fragment(const TDmaTransferArgs& aTransferArgs);


	/** Transfer asynchronously this request.

		If this request's channel is idle, the request is transferred
		immediately. Otherwise, it is queued and transferred later.

		The client is responsible for ensuring cache consistency before and/or
		after the transfer if necessary.

		@return KErrNone if success, KErrGeneral otherwise.

		@released
	*/
	IMPORT_C TInt Queue();


	/** Append new descriptor(s) to existing list.

		Clients needing to build a custom descriptor list should call this
		function to allocate the list and access the resulting list through
		iFirstHdr and iLastHdr.

		Clients should not change the value of iFirstHdr, iLastHdr and the
		iNext field of the descriptor headers to ensure descriptors can be
		deallocated. Clients are free to change hardware descriptors, including
		chaining, in whatever way suit them.

		Assume the request is not being transferred or pending.

		@param aCount Number of descriptors to append.

		@return KErrNone or standard error code.

		@released
	*/
	IMPORT_C TInt ExpandDesList(TInt aCount=1);


	/** Append new descriptor(s) to existing list. This function variant
		operates on the source port descriptor chain.

		Works like ExpandDesList except that it uses the iSrcFirstHdr and
		iSrcLastHdr fields.

		@see ExpandDesList()

		This function should only be used if SDmacCaps::iAsymHwDescriptors is
		reported as true, as only then the framework will actually use the
		allocated descriptors.

		@param aCount Number of descriptors to append.

		@return KErrNone or standard error code.

		@prototype
	*/
	IMPORT_C TInt ExpandSrcDesList(TInt aCount=1);


	/** Append new descriptor(s) to existing list. This function variant
		operates on the destination port descriptor chain.

		Works like ExpandDesList except that it uses the iDstFirstHdr and
		iDstLastHdr fields.

		@see ExpandDesList()

		This function should only be used if SDmacCaps::iAsymHwDescriptors is
		reported as true, as only then the framework will actually use the
		allocated descriptors.

		@param aCount Number of descriptors to append.

		@return KErrNone or standard error code.

		@prototype
	*/
	IMPORT_C TInt ExpandDstDesList(TInt aCount=1);


	/** Free resources associated with this request.

		Assumes the request is not being transferred or pending.

		@see ExpandDesList()

		@released
	*/
	IMPORT_C void FreeDesList();


	/** Free resources associated with this request. This function variant
		operates on the source port descriptor chain.

		Assumes the request is not being transferred or pending.

		@see ExpandSrcDesList()

		@prototype
	*/
	IMPORT_C void FreeSrcDesList();


	/** Free resources associated with this request. This function variant
		operates on the destination port descriptor chain.

		Assumes the request is not being transferred or pending.

		@see ExpandDstDesList()

		@prototype
	*/
	IMPORT_C void FreeDstDesList();


	/** Enables the functionality for counting the transferred source
		elements.

		This function can be called at any time, but the enabled/disabled
		status is checked by the framework only at two points in time.

		The first one is after a request has been queued, and if it is enabled
		then the counting will commence as soon as the transfer starts.

		The second point is when Resume() is called for a paused transfer, and
		in this case the following applies. If counting was enabled when the
		transfer was paused and it is now disabled then the counting is stopped
		at that point and the count value frozen. If counting was disabled when
		the transfer was paused and it is now enabled then the counting will
		commence when the transfer resumes. (The starting value will depend on
		the argument of the enable function.) Otherwise nothing will change,
		i.e. counting will either continue normally (enabled/enabled) or
		neither stop nor continue (disabled/disabled).

		Once a status has been set, it remains valid for the entire duration of
		the transfer (and beyond, if it is not changed again).

		@param aResetElementCount If ETrue (the default) then the count
		variable will be reset to zero, otherwise it will retain its current
		value.

		@see Queue()
		@see TotalNumSrcElementsTransferred()

		@prototype
	*/
	IMPORT_C void EnableSrcElementCounting(TBool aResetElementCount=ETrue);


	/** Enables the functionality for counting the transferred destination
		elements.

		This function can be called at any time, but the enabled/disabled
		status is checked by the framework only at two points in time.

		The first one is after a request has been queued, and if it is enabled
		then the counting will commence as soon as the transfer starts.

		The second point is when Resume() is called for a paused transfer, and
		in this case the following applies. If counting was enabled when the
		transfer was paused and it is now disabled then the counting is stopped
		at that point and the count value frozen. If counting was disabled when
		the transfer was paused and it is now enabled then the counting will
		commence when the transfer resumes. (The starting value will depend on
		the argument of the enable function.) Otherwise nothing will change,
		i.e. counting will either continue normally (enabled/enabled) or
		neither stop nor continue (disabled/disabled).

		Once a status has been set, it remains valid for the entire duration of
		the transfer (and beyond, if it is not changed again).

		@param aResetElementCount If ETrue (the default) then the count
		variable will be reset to zero, otherwise it will retain its current
		value.

		@see Queue()
		@see TotalNumDstElementsTransferred()

		@prototype
	*/
	IMPORT_C void EnableDstElementCounting(TBool aResetElementCount=ETrue);


	/** Disables the functionality for counting the transferred source
		elements.

		This function can be called at any time, but the enabled/disabled
		status is checked by the framework only at two points in time.

		The first one is after a request has been queued, and if it is enabled
		then the counting will commence as soon as the transfer starts.

		The second point is when Resume() is called for a paused transfer, and
		in this case the following applies. If counting was enabled when the
		transfer was paused and it is now disabled then the counting is stopped
		at that point and the count value frozen. If counting was disabled when
		the transfer was paused and it is now enabled then the counting will
		commence when the transfer resumes. (The starting value will depend on
		the argument of the enable function.) Otherwise nothing will change,
		i.e. counting will either continue normally (enabled/enabled) or
		neither stop nor continue (disabled/disabled).

		Once a status has been set, it remains valid for the entire duration of
		the transfer (and beyond, if it is not changed again).

		@see Queue()
		@see TotalNumSrcElementsTransferred()

		@prototype
	*/
	IMPORT_C void DisableSrcElementCounting();


	/** Disables the functionality for counting the transferred destination
		elements.

		This function can be called at any time, but the enabled/disabled
		status is checked by the framework only at two points in time.

		The first one is after a request has been queued, and if it is enabled
		then the counting will commence as soon as the transfer starts.

		The second point is when Resume() is called for a paused transfer, and
		in this case the following applies. If counting was enabled when the
		transfer was paused and it is now disabled then the counting is stopped
		at that point and the count value frozen. If counting was disabled when
		the transfer was paused and it is now enabled then the counting will
		commence when the transfer resumes. (The starting value will depend on
		the argument of the enable function.) Otherwise nothing will change,
		i.e. counting will either continue normally (enabled/enabled) or
		neither stop nor continue (disabled/disabled).

		Once a status has been set, it remains valid for the entire duration of
		the transfer (and beyond, if it is not changed again).

		@see Queue()
		@see TotalNumDstElementsTransferred()

		@prototype
	*/
	IMPORT_C void DisableDstElementCounting();


	/** Returns the number of elements that have been transferred by this
		transfer request at the source port.

		To use this method, the counting functionality has to be explicitly
		enabled, either before the transfer request is queued or while it is
		paused.

		@see EnableSrcElementCounting()
		@see DisableSrcElementCounting()

		This function should only be called after the transfer has finished
		(completed with or without error, or because it was cancelled) or while
		it is paused. Otherwise it may just return 0.

		@return The number of elements that have been transferred by this
		transfer request at the source port.

		@prototype
	*/
	IMPORT_C TUint32 TotalNumSrcElementsTransferred();


	/** Returns the number of elements that have been transferred by this
		transfer request at the destination port.

		To use this method, the counting functionality has to be explicitly
		enabled, either before the transfer request is queued or while it is
		paused.

		@see EnableDstElementCounting()
		@see DisableDstElementCounting()

		This function should only be called after the transfer has finished
		(completed with or without error, or because it was cancelled) or while
		it is paused. Otherwise it may just return 0.

		@return The number of elements that have been transferred by this
		transfer request at the destination port.

		@prototype
	*/
	IMPORT_C TUint32 TotalNumDstElementsTransferred();


	/** Returns the number of fragments that this transfer request has been
		split into.

		This number will only be different from 0 once Fragment() has been
		called or after descriptors have been manually allocated by the client
		using ExpandDesList().

		If SDmacCaps::iAsymHwDescriptors is true then this function will always
		return 0, and SrcFragmentCount() / DstFragmentCount() should be used
		instead.

		@return The number of fragments (descriptors / pseudo descriptors) that
		this transfer request has been split into.

		@released
	*/
	IMPORT_C TInt FragmentCount();


	/** Returns the number of source port fragments that this transfer request
		has been split into.

		This number will only be different from 0 once Fragment() has been
		called or after descriptors have been manually allocated by the client
		using ExpandSrcDesList().

		This function can only be used if SDmacCaps::iAsymHwDescriptors is
		true, otherwise it will always return 0.

		@return The number of source port fragments (descriptors) that this
		transfer request has been split into.

		@prototype
	*/
	IMPORT_C TInt SrcFragmentCount();


	/** Returns the number of destination port fragments that this transfer
		request has been split into.

		This number will only be different from 0 once Fragment() has been
		called or after descriptors have been manually allocated by the client
		using ExpandDstDesList().

		This function can only be used if SDmacCaps::iAsymHwDescriptors is
		true, otherwise it will always return 0.

		@return The number of destination port fragments (descriptors) that
		this transfer request has been split into.

		@prototype
	*/
	IMPORT_C TInt DstFragmentCount();

private:
	inline void OnDeque();
	TInt CheckTransferConfig(const TDmaTransferConfig& aTarget, TUint aCount) const;
	TInt CheckMemFlags(const TDmaTransferConfig& aTarget) const;
	TInt AdjustFragmentSize(TUint& aFragSize, TUint aElementSize, TUint aFrameSize);
	TUint GetTransferCount(const TDmaTransferArgs& aTransferArgs) const;
	TUint GetMaxTransferlength(const TDmaTransferArgs& aTransferArgs, TUint aCount) const;
	TInt Frag(TDmaTransferArgs& aTransferArgs);
	TInt FragSym(TDmaTransferArgs& aTransferArgs, TUint aCount, TUint aMaxTransferLen);
	TInt FragAsym(TDmaTransferArgs& aTransferArgs, TUint aCount, TUint aMaxTransferLen);
	TInt FragAsymSrc(TDmaTransferArgs& aTransferArgs, TUint aCount, TUint aMaxTransferLen);
	TInt FragAsymDst(TDmaTransferArgs& aTransferArgs, TUint aCount, TUint aMaxTransferLen);
	TInt FragBalancedAsym(TDmaTransferArgs& aTransferArgs, TUint aCount, TUint aMaxTransferLen);
	TInt ExpandDesList(TInt aCount, TInt& aDesCount, SDmaDesHdr*& aFirstHdr,
					   SDmaDesHdr*& aLastHdr);
	void FreeDesList(TInt& aDesCount, SDmaDesHdr*& aFirstHdr, SDmaDesHdr*& aLastHdr);
	TInt FragmentCount(const SDmaDesHdr* aHdr);

public:
	// WARNING: The following attributes are accessed both in client and DFC
	// thread context, so accesses must be protected with the channel lock.
	TDmaChannel& iChannel;		/**< The channel this request is bound to */
	TCallback iCb;			 /**< Called on completion/failure (can be NULL) */
	TAny* iCbArg;			 /**< Callback argument */
	TDmaCallback iDmaCb;		// the new-style callback function
	TAny* iDmaCbArg;			// the new-style callback arg
	TBool iIsrCb;				// client wants callback in ISR context
	TInt iDesCount;			   /**< The number of fragments in list */
	SDmaDesHdr* iFirstHdr;	   /**< The first fragment in the list (or NULL) */
	SDmaDesHdr* iLastHdr;	   /**< The last fragment in the list (or NULL) */
	TInt iSrcDesCount;		   /**< The number of fragments in list */
	SDmaDesHdr* iSrcFirstHdr;  /**< The first fragment in the list (or NULL) */
	SDmaDesHdr* iSrcLastHdr;   /**< The last fragment in the list (or NULL) */
	TInt iDstDesCount;		   /**< The number of fragments in list */
	SDmaDesHdr* iDstFirstHdr;  /**< The first fragment in the list (or NULL) */
	SDmaDesHdr* iDstLastHdr;   /**< The last fragment in the list (or NULL) */
	SDblQueLink iLink;			/**< The link on channel queue of pending requests */
	TBool iQueued;				/**< Indicates whether request is pending or being transferred */
	TUint iMaxTransferSize;		/**< Defaults to DMA controller max. transfer size */

	TUint32 iTotalNumSrcElementsTransferred;
	TUint32 iTotalNumDstElementsTransferred;

	__DMA_DECLARE_INVARIANT
	};


//////////////////////////////////////////////////////////////////////////////

class TDmac;
class DmaChannelMgr;
class TDmaCancelInfo;

/** DMA channel base class.

	Standard derived classes are provided for this channel (see
	TDmaSbChannel, TDmaDbChannel, TDmaSgChannel, and TDmaAsymSgChannel).
	The base-port implementor will only need to write their own derived
	class if one of the standard classes is unsuitable.

	This class has not been designed to be called from several concurrent
	client threads. Multithreaded clients must implement their own locking
	scheme (via DMutex).

	Mutexes are used internally to protect data structures accessed both by the
	client thread and the DFC one. Therefore no fast mutex can be held when
	calling a channel function.
*/
class TDmaChannel
	{
	friend class DDmaRequest;
	friend class TDmac;
	friend class DmaChannelMgr;

public:
	/** Information passed by client when opening a channel. */
	struct SCreateInfo
		{
		/** Default constructor. Initializes all fields with meaningful default
			values.

			Must be inline (for now) because exporting it would break existing
			custom DMA libs as their clients would need the export which would
			be missing from the custom .def files.

			@released
		*/
		SCreateInfo() : iPriority(KDmaPriorityNone), iDynChannel(EFalse) {};

		/** Identifier used by PSL to select channel to open.

			@released
		*/
		TUint32 iCookie;
		/** Number of descriptors this channel can maximally use.

			This value will not be used in the fully implemented new version of
			the DMA framework. Until then it is still required.

			@released
		*/
		TInt iDesCount;
		/** DFC queue used to service DMA interrupts.

			@released
		*/
		TDfcQue* iDfcQ;
		/** DFC priority.

			@released
		*/
		TUint8 iDfcPriority;
		/** Used by PSL to configure a channel priority (if possible).

			The default is KDmaPriorityNone (the don't care value).

			@see TDmaPriority

			@prototype
		*/
		TUint iPriority;
		/** Request a dynamic DMA channel.

			If this is set to ETrue then the Open call is for a 'dynamic' as
			opposed to a static and solely owned DMA channel. A number of
			properties of the opened TDmaChannel object will be different in
			that case.

			The default value is EFalse.

			@prototype
		 */
		TBool iDynChannel;
		};

public:
	/** Opens the DMA channel.

		Channel selection is done by the hardware-specific layer using a cookie
		passed in via aInfo.

		The client should not delete the returned pointer as the framework owns
		channel objects. However, the client should explicitly close the
		channel when finished with it.

		@param aInfo Information passed by caller to select and configure
		channel.

		@param aChannel Points to open channel on successful return. NULL
		otherwise.

		@return KErrNone or standard error code.

		@released
	*/
	IMPORT_C static TInt Open(const SCreateInfo& aInfo, TDmaChannel*& aChannel);


	/** Closes a previously opened DMA channel.

		Assumes the channel is idle and all requests have been deleted.

		The call will cause the resources associated with this channel to be
		released, and the pointer/reference to it mustn't therefore be accessed
		any longer after the function has returned. The channel pointer should
		be set to NULL by the client.

		@released
 	*/
	IMPORT_C void Close();


	/** Logically links this channel to the one specified as an argument, or,
		if the argument is NULL, unlinks this channel.

		The effect of channel linking is that once a transfer on this channel
		has finished, instead of causing the associated client callback to be
		called, 'aChannel' will be enabled by hardware and a preconfigured
		transfer on that channel will start.

		Note that conceptually 'linking' here always refers to the end of a
		channel transfer, not the beginning, i.e. a channel can only be linked
		once and always to a successor, never twice or to a predecessor. (This
		does not preclude the possibility that two channels are linked in a
		circular fashion.)

		This function can only be used if the DMAC supports logical channel
		linking.

		@see SDmacCaps::iChannelLinking

		@param aChannel Points to the channel this one should be linked to, or
		NULL if this channel is to be unlinked from any other one.

		@return KErrNone if the channel has been linked or unlinked
		successfully, KErrCompletion if this channel was already linked to
		aChannel or already unlinked, KErrNotSupported if the DMAC doesn't
		support channel linking, KErrArgument if this channel was already
		linked to a different channel, KErrGeneral if a general error occurred
		preventing a successful outcome.

		@prototype
	*/
	IMPORT_C TInt LinkToChannel(TDmaChannel* aChannel);


	/** Pauses an active transfer on this channel.

		A paused channel transfer can be resumed by calling Resume() or it can
		be stopped altogether by calling CancelAll().

		@see TDmaChannel::Resume()

		Function can only be used if the DMAC supports this functionality.

		@see SDmacCaps::iChannelPauseAndResume

		@return KErrNone if a transfer has been paused successfully,
		KErrCompletion if a transfer was already paused, KErrNotSupported if
		the DMAC doesn't support channel transfer pausing/resuming, KErrGeneral
		if a general error occurred preventing a successful outcome.

		@released
	*/
	IMPORT_C TInt Pause();


	/** Resumes a transfer on this channel that is paused.

		Resume() can be called to resume channel operation when the transfer is
		paused as a result of a previous call to Pause() or because the DMAC
		has encountered a Pause bit in a H/W descriptor.

		@see TDmaChannel::Pause()
		@see TDmaCallbackType::EDmaCallbackLinkedListPaused

		Function can only be used if the DMAC supports this functionality.

		@see SDmacCaps::iChannelPauseAndResume
		@see SDmacCaps::iLinkedListPausedInterrupt

		@return KErrNone if a paused transfer has been resumed successfully,
		KErrCompletion if there was no paused transfer, KErrNotSupported if the
		DMAC doesn't support channel transfer pausing/resuming, KErrGeneral if
		a general error occurred preventing a successful outcome.

		@released
	*/
	IMPORT_C TInt Resume();


	/** Cancels the current request and all the pending ones.

		@released
	*/
	IMPORT_C void CancelAll();


	/** Returns the channel's maximum transfer length based on the passed
		arguments.

		@param aSrcFlags Bitmask characterising transfer source
		@see TDmaTransferArgs::iSrcConfig::iFlags

		@param aDstFlags Bitmask characterising transfer destination
		@see TDmaTransferArgs::iDstConfig::iFlags

		@param aPslInfo Cookie passed to the PSL
		@see TDmaTransferArgs::iPslRequestInfo

		@return 0 if transfer length is not limited, the maximum transfer
		length in bytes otherwise.

		@released
 	*/
	IMPORT_C TUint MaxTransferLength(TUint aSrcFlags, TUint aDstFlags, TUint32 aPslInfo);


	/** Retrieves from the PSL the address / memory alignment mask based on the
		parameters passed. Some DMA controllers impose alignment constraints on
		the base address of memory buffers. This mask is AND'ed against memory
		addresses computed during fragmentation.

		This function needs to be called separately for source and destination.

		@param aTargetFlags Bitmask characterising transfer source or
		destination
		@see TDmaTransferArgs::iSrcConfig::iFlags
		@see TDmaTransferArgs::iDstConfig::iFlags

		@param aElementSize Element size used for the transfer. Can be zero if
		not known or 'don't care'.

		@param aPslInfo Cookie passed to the PSL
		@see TDmaTransferArgs::iPslRequestInfo

		@return A value representing the alignment mask (e.g. 3 if buffer must
		be 4-byte aligned)

		@released
	*/
	IMPORT_C TUint AddressAlignMask(TUint aTargetFlags, TUint aElementSize,
									TUint32 aPslInfo);


	/** Returns a reference to a structure containing the capabilities and
		features of the DMA controller associated with this channel.

		@return A reference to a structure containing the capabilities and
		features of the DMA controller associated with this channel.

		@released
	*/
	IMPORT_C const SDmacCaps& DmacCaps();


	/** Sets up once more the transfer request that has just completed, after
		optionally having adjusted the transfer parameters as specified.

		This function is meant to be called exclusively from a client-supplied
		callback that is executed in ISR context, and only in response to a
		transfer completion notification.

		If this call returns to the caller with KErrNone then the framework's
		ISR handler will subsequently not queue the channel DFC for this
		completed request.

		The parameters specify which changes the framework should apply to the
		descriptors of the transfer request before rescheduling it. Arguments
		for which no change is required should be passed as their default
		values. The parameters correspond to those in the TDmaTransferArgs
		struct as follows.

		@param aSrcAddr @see TDmaTransferArgs::iSrcConfig::iAddr
		@param aDstAddr @see TDmaTransferArgs::iDstConfig::iAddr
		@param aTransferCount @see TDmaTransferArgs::iTransferCount
		@param aPslRequestInfo @see TDmaTransferArgs::iPslRequestInfo
		@param aIsrCb If set to ETrue (the default) then the callback of the
		rescheduled request will again be called in ISR context

		Since Epoc::LinearToPhysical() cannot be called in ISR context the
		addresses passed into this function must be physical ones, i.e.
		TDmaTransferFlags::KDmaPhysAddr is implied.

		If an address refers to a memory target then
		TDmaTransferFlags::KDmaMemIsContiguous is implied as well as no
		fragmentation is possible at this point.

		@pre Must only be called from a 'transfer complete' client callback in
		ISR context.

		@post Framework won't queue the channel DFC for the completed request
		in success case.

		@see DDmaRequest::DDmaRequest(TDmaChannel&, TDmaCallback, TAny*, TUint)
		@see TDmaCallbackType::EDmaCallbackRequestCompletion
		@see TDmaPILFlags::KDmaRequestCallbackFromIsr

		@return KErrGeneral if there was an error, KErrNone otherwise.

		@released
	*/
	IMPORT_C TInt IsrRedoRequest(TUint32 aSrcAddr=KPhysAddrInvalid,
								 TUint32 aDstAddr=KPhysAddrInvalid,
								 TUint aTransferCount=0,
								 TUint32 aPslRequestInfo=0,
								 TBool aIsrCb=ETrue);


	/** Tests whether the channel is currently opened.

		@return ETrue if channel is currently opened, EFalse otherwise.

		NB: This API should not be used any longer.

		After calling TDmaChannel::Open() successfully the channel is
		guaranteed to be open, hence there seems no good reason for this API to
		exist.

		@deprecated
	*/
	inline TBool IsOpened() const;


	/** Tests whether the channel's request queue is currently empty.

		@return ETrue if request queue is currently empty, EFalse otherwise.

		@released
	*/
	inline TBool IsQueueEmpty() const;


	/** Returns a PSL-specific value which uniquely identifies this channel -
		it is used for debug tracing by the PIL.

		@return PSL-specific value which uniquely identifies this channel.

		@released
	*/
	inline TUint32 PslId() const;


	/** Called by a test harness to force an error when the next fragment is
		transferred.

		@param aFragmentCount The number of consecutive fragments to fail

		@released
	*/
	IMPORT_C TInt FailNext(TInt aFragmentCount);


	/** Called by a test harness to force the DMA controller to miss one or
		more interrupts.

		@param aInterruptCount The number of consecutive interrupts to miss

		@released
	*/
	IMPORT_C TInt MissNextInterrupts(TInt aInterruptCount);


	/** Function allowing platform-specific layer to extend channel API with
		new channel-specific operations.

		@param aCmd Command identifier.
		@param aArg PSL-specific argument

		@return KErrNotSupported if aCmd is not supported. PSL-specific value
		otherwise.

		@released
	*/
	IMPORT_C TInt Extension(TInt aCmd, TAny* aArg);


	/** This is a function that allows the Platform Specific Layer (PSL) to
		extend the DMA API with new channel-independent operations.

		@param aCmd Command identifier.
		@param aArg PSL-specific.

		@return KErrNotSupported if aCmd is not supported; a PSL specific value
		otherwise.

		@released
 	*/
	IMPORT_C TInt StaticExtension(TInt aCmd, TAny* aArg);


	/** @see DmacCaps()

		@deprecated
	*/
	inline const TDmac* Controller() const;

	/** @see MaxTransferLength()

		@deprecated
	*/
	inline TInt MaxTransferSize(TUint aFlags, TUint32 aPslInfo);

	/** @see AddressAlignMask()

		@deprecated
	*/
	inline TUint MemAlignMask(TUint aFlags, TUint32 aPslInfo);

protected:
	// Interface with state machines

	/** Constructor.

		@released
	*/
	TDmaChannel();

	/** Called by the PIL when adding a new request to the channel's queue.
		The implementation should update the channel's state as appropriate
		and begin transfer of aReq if possible.

		@param aReq The request which has been added to the queue

		@released
	*/
	virtual void DoQueue(const DDmaRequest& aReq);

	/** Called by the PIL in response to a CancelAll call. It should update
		the channel state appropriately.

		@released
	*/
	virtual void DoCancelAll() = 0;

	/** This is called by the PIL when a DDmaRequest is removed from the
		channel's queue. In general the implementation simply needs to unlink
		the hardware descriptor corresponding to aHdr from the next.

		Since the PIL links the hardware descriptor chains of adjacent queued
		requests (for speed) it is necessary to break the chain when a request
		is completed so that the request may be requeued by the client without
		having called DDmaRequest::Fragment again.

		@param aHdr The header for a descriptor, which must be unlinked
		from its next descriptor (if there is one)

		@released
	*/
	virtual void DoUnlink(SDmaDesHdr& aHdr);

	/** Called by the PIL whenever a transfer associated with aCurReq is
		done. The implementation must advance the channel's state and
		may transfer the next header if necessary (the provided
		scatter-gather channel does not do this). It must also report
		back which header was associated with the last transfer to
		complete.

		@param aCurReq The current request.
		@param aCompletedHdr Must be set by the implementation to the header
		of the last transfer to complete.

		@released
	*/
	virtual void DoDfc(const DDmaRequest& aCurReq, SDmaDesHdr*& aCompletedHdr);

	/** Called by the PIL whenever a transfer associated with aCurReq is
		done. The implementation must advance the channel's state and
		may start the transfer for the next headers if necessary (the
		provided scatter-gather channels do not do this). If one
		header has a successor but the other is the last in the chain it
		is an error.

		@note Must be implemented by PSL if channel uses asymmetric hardware
		descriptors and is not derived from TDmaAsymSgChannel.

		@param aCurReq The current request.

		@param aSrcCompletedHdr Must be set by the implementation to
		the header of the last source descriptor to complete.

		@param aDstCompletedHdr Must be set by the implementation to
		the header of the last destination descriptor to complete.

		@prototype
	*/
	virtual void DoDfc(const DDmaRequest& aCurReq, SDmaDesHdr*& aSrcCompletedHdr,
					   SDmaDesHdr*& aDstCompletedHdr);

	/** This function allows the Platform Specific Layer (PSL) to control the
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

	virtual void SetNullPtr(const DDmaRequest& aReq);
	virtual void ResetNullPtr();

	inline virtual ~TDmaChannel() {}

	inline void Wait();
	inline void Signal();
	inline TBool Flash();

private:
	static void Dfc(TAny*);
	void DoDfc();

protected:
	TDmac* iController;		 // DMAC this channel belongs to (NULL when closed)
	const SDmacCaps* iDmacCaps;	// what is supported by DMAC on this channel
	TUint32 iPslId;			 // unique identifier provided by PSL
	TBool iDynChannel;		 // this is a dynamically allocated channel
	TUint iPriority;		 // hardware priority of this channel
	NFastMutex iLock;		 // for data accessed in both client & DFC context
	SDmaDesHdr* iCurHdr;	 // fragment being transferred or NULL
	SDmaDesHdr** iNullPtr;	 // Pointer to NULL pointer following last fragment
	TDfc iDfc;				 // transfer completion/failure DFC
	TInt iMaxDesCount;		 // maximum number of allocable descriptors
	TInt iAvailDesCount;	 // available number of descriptors
	volatile TUint32 iIsrDfc; // Interface between ISR and DFC:
	enum {KErrorFlagMask = 0x80000000};	   // bit 31 - error flag
	enum {KCancelFlagMask = 0x40000000};   // bit 30 - cancel flag
	enum {KDfcCountMask = 0x3FFFFFFF};	   // bits 0-29 - number of queued DFCs
	SDblQue iReqQ;			// being/about to be transferred request queue
	TInt iReqCount;			// number of requests attached to this channel
	TInt iQueuedRequests; 	// number of requests currently queued on this channel
	TBool iCallQueuedRequestFn;	// call QueuedRequestCountChanged? (default: true)

private:
	TDmaCancelInfo* iCancelInfo; // ...
	TBool iRedoRequest;		// client ISR callback wants a redo of request
	TBool iIsrCbRequest;	// request on queue using ISR callback

	__DMA_DECLARE_VIRTUAL_INVARIANT
	};


//////////////////////////////////////////////////////////////////////////////
// INTERFACE WITH TEST HARNESS
//////////////////////////////////////////////////////////////////////////////

/** Provides access to test information structure stored in the PSL.

	Must be implemented by the PSL (v1).

	@deprecated
*/
IMPORT_C const TDmaTestInfo& DmaTestInfo();


/** Provides access to test information structure stored in the PSL.

	Must be implemented by the PSL (v2).

	@released
*/
IMPORT_C const TDmaV2TestInfo& DmaTestInfoV2();


//////////////////////////////////////////////////////////////////////////////


#include <drivers/dma_compat.inl>
#include <drivers/dma_v2.inl>


#endif	// #ifndef __DMA_V2_H__
