// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\twintnotifier.h
// Text Window Server text notifiers.
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __TWINTNOTIFIER_H__
#define __TWINTNOTIFIER_H__

#include <f32file.h>


//  Notifier Plugin architecture copied from UIKON



/**
@internalComponent
*/
_LIT(KNotifierPlugInExt,"*.*");



/**
@publishedPartner
@released

Defines the path which is searched for notifier plugin DLLs.
*/
_LIT(KNotifierPlugInSearchPath,"\\sys\\bin\\tnotifiers\\");



/**
@publishedPartner
@deprecated
*/
const TUid KUidNotifierPlugIn={0x10005522}; 



/**
@publishedPartner
@released
*/
const TUid KUidTextNotifierPlugInV2={0x101fe38b}; 



/**
@internalComponent
*/
_LIT8(KNotifierPaused,"Eik_Notifier_Paused");



/**
@internalComponent
*/
_LIT8(KNotifierResumed,"Eik_Notifier_Resumed");



enum TNotExtStatus
/**
@internalComponent
*/
	{
	ENotExtRequestCompleted	=0,
	ENotExtRequestQueued	=1,
	};




class MNotifierManager
/**
@publishedPartner
@released

An interface that allows notifiers to manage their own startup and shutdown.

This class is likely to be of interest to notifiers that observe engines
using publically available APIs rather than those that are run via RNotifier

@see RNotifier
*/
	{
public:
    /**
    Starts the specified notifier.

	@param aNotifierUid The Uid that identifies the notifier.
	@param aBuffer      Data that can be passed from the client-side.
	                    The format and meaning of any data
	                    is implementation dependent.
	@param aResponse    Data that can be returned to the client-side.
						The format and meaning of any data is implementation dependent.
	*/
	virtual void StartNotifierL(TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse)=0;
	
	
	
    /**
    Cancels the specified notifier.
 	
 	@param aNotifierUid The Uid that identifies the notifier.
    */
	virtual void CancelNotifier(TUid aNotifierUid)=0;
	
	
    /**
    Updates a currently active notifier with new data.
    
    @param aNotifierUid The Uid that identifies the notifier.
    @param aBuffer      New data that can be passed from the client-side.
                        The format and meaning of any data is implementation dependent.
    @param aResponse    Data that can be returned to the client-side.
                        The format and meaning of any data is implementation dependent.
    */
	virtual void UpdateNotifierL(TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse)=0;
	};




class MNotifierBase2
/**
@publishedPartner
@released

Interface to a plug-in server side notifier for the text window server.

Any number of MNotifierBase2 objects can be included in a single DLL.
All notifiers are loaded during device startup and are not destroyed until
the notifier server closes down.

Note that the text window server is not used in production ROMs.
It is used when testing low level code.
*/
	{
public:
    /**
    Defines a set of notifier priorities. The use and application of these
    values is implementation dependent. ENotifierPriorityAbsolute defines
    the highest priority value; subsequent enum values define decreasing
    priority values.
    */
	enum TNotifierPriority
		{
		ENotifierPriorityAbsolute=500,
		ENotifierPriorityVHigh=400,	
		ENotifierPriorityHigh=300,	
		ENotifierPriorityLow=200,	
		ENotifierPriorityVLow=100,	
		ENotifierPriorityLowest=0	
		};
public:
    /**
    Encapsulates the notifier parameters.
    */
	class TNotifierInfo
		{
	public:
	    /**
	    The Uid that identifies the notifier.
        */
		TUid iUid;
		
		/**
		The Uid that identifies the channel to be used by
		the notifier (e.g. the screen, an LED etc).
	    */
		TUid iChannel;
		
	    /**
	    The notifier priority, typically chosen from the standard set.
	    
	    @see MNotifierBase2::TNotifierPriority
	    */
		TInt iPriority;
		};
public:



	/**
	Frees all resources owned by this notifier.

    This function is called by the notifier framework when all resources
    allocated by notifiers should be freed. As a minimum, the implementation
    should delete this object (i.e. delete this;).

    Note that it is important to implement this function correctly
    to avoid memory leaks.
	*/
	virtual void Release()=0;


		
    /**
    Performs any initialisation that this notifier may require.

    The function is called when the notifier is first loaded,
    i.e. when the plug-in DLL is loaded. It is called only once.

    As a minimum, the implementation should return a TNotifierInfo instance
    describing the notifier parameters. A good implementation would be to set
    up a TNotifierInfo as a data member, and then to return it. This is because
    the same information is returned by Info().

    The function is safe to leave from, so it is possible,
    although rarely necessary, to allocate objects as you would normally do
    in a ConstructL() function as part of two-phase construction.

	@return The parameters of the notifier.

	@see MNotifierBase2::Info
    */	 
	virtual TNotifierInfo RegisterL()=0;



	/**
	Gets the notifier parameters.

    This is the same information as returned by RegisterL(), although
    the returned values may be varied at run-time.
	*/
	virtual TNotifierInfo Info() const=0;



	/**
	Starts the notifier.

    This is called as a result of a client-side call
    to RNotifier::StartNotifier(), which the client uses to start a notifier
    from which it does not expect a response.

    The function is synchronous, but it should be implemented so that
    it completes as soon as possible, allowing the notifier framework
    to enforce its priority mechanism.

    It is not possible to wait for a notifier to complete before returning
    from this function unless the notifier is likely to finish implementing
    its functionality immediately.

	@param	aBuffer Data that can be passed from the client-side.
	        The format and meaning of any data is implementation dependent.
	        
	@return A pointer descriptor representing data for the initial response

	@see RNotifier::StartNotifier
	*/ 
	virtual TPtrC8 StartL(const TDesC8& aBuffer)=0;



	/**
	Starts the notifier.

    This is called as a result of a client-side call to
    the asynchronous function RNotifier::StartNotifierAndGetResponse().
    This means that the client is waiting, asynchronously, for the notifier
    to tell the client that it has finished its work.

    It is important to return from this function as soon as possible,
    
    The implementation of a derived class must make sure that Complete() is
    called on the RMessage2 object when the notifier is deactivated.

    This function may be called multiple times if more than one client starts
    the notifier.

    @param aBuffer    Data that can be passed from the client-side. The format
                      and meaning of any data is implementation dependent.
    @param aReplySlot The offset within the message arguments for the reply.
                      This message argument will refer to a modifiable
                      descriptor, a TDes8 type, into which data
                      can be returned. The format and meaning
                      of any returned data is implementation dependent.
    @param aMessage   Encapsulates a client request.
    
    @see RNotifier::StartNotifierAndGetResponse
    */
	virtual void StartL(const TDesC8& aBuffer, TInt aReplySlot, const RMessagePtr2& aMessage)=0;
	
	
	
	/**
	Cancels an active notifier.

    This is called as a result of a client-side call
    to RNotifier::CancelNotifier().

    An implementation should free any relevant resources, and complete
    any outstanding messages, if relevant.
    
    @see RNotifier::CancelNotifier
	*/ 
	virtual void Cancel()=0;



	/**
	Updates a currently active notifier with new data.

	This is called as a result of a client-side call
	to RNotifier::UpdateNotifier().
	
	@param aBuffer Data that can be passed from the client-side.
	               The format and meaning of any data is
	               implementation dependent.
	
	@return A pointer descriptor representing data that may be returned.
	        The format and meaning of any data is implementation dependent.

	@see RNotifier::UpdateNotifier
	*/ 
	virtual TPtrC8 UpdateL(const TDesC8& aBuffer)=0;
public:



	/**
	Sets the notifier manager.

	@param aManager A pointer to the notifier manager.
	*/
	void SetManager(MNotifierManager* aManager);
protected:
	MNotifierManager* iManager;
private:
	TInt iNotBSpare;
	};






// Remaining classes are internal to the text window server



class CNotifierManager;

/**
@internalComponent
*/
class CNotifierServer : public CServer2
	{
public:
	static CNotifierServer* NewL();
	~CNotifierServer();
public: // from CServer2
	CSession2* NewSessionL(const TVersion &aVersion,const RMessage2&) const;
public:
	CNotifierServer(TInt aPriority);
	inline CNotifierManager* Manager() const;
public:
	void SetIsExiting();
	TBool IsExiting() const;
private:
	void ConstructL();
private:
	CNotifierManager* iManager;
	TBool iExiting;
	};




/**
@internalComponent
*/
class CNotifierSession : public CSession2
	{
public:
	CNotifierSession(const CNotifierServer& aServer);
	~CNotifierSession();
public: // from CSession2
	void ServiceL(const RMessage2& aMessage);
private:
	enum TNotifierPanic
		{
		ENotifierPanicInconsistentDescriptorLengths=0,
		ENotifierPanicPasswordWindow,
		};
private:
	void DisplayAlertL(const RMessage2& aMessage);
	void DisplayInfoMsgL(const RMessage2& aMessage);
	void DoStartNotifierL(const RMessage2& aMessage);
	void DoUpdateNotifierL(const RMessage2& aMessage);
	void StartNotifierAndGetResponseL(const RMessage2& aMessage,TBool& aCleanupComplete);
	void PanicClient(const RMessage2& aMessage,TNotifierPanic aCode);
	static TInt InfoPrintThread(TAny* aMessage);
	void RunPasswordWindowL(const RMessage2& aMessage);
public:
	static RSemaphore NotifierSemaphore;
private:
	const CNotifierServer* iServer;
	TInt iClientId;
	};




class CQueueItem;
class CChannelMonitor;
class CActivityMonitor;
class CNotifierQueue;

/**
@internalComponent
*/
class CNotifierManager : public CBase, public MNotifierManager
	{
public:
	static CNotifierManager* NewL();
	~CNotifierManager();
	void RegisterL(RFs& aFs);
	void NotifierStartL(TUid aNotifierUid,const TDesC8& aBuffer,TPtrC8* aResponse,TInt aClientId);
	TInt NotifierUpdateL(TUid aNotifierUid,const TDesC8& aBuffer,TDes8* aResponse,TInt aClientId);
	TInt NotifierCancel(TUid aNotifierUid);
	void NotifierStartAndGetResponseL(TUid aNotifierUid,const TDesC8& aBuffer,TInt aReplySlot,
										const RMessage2& aMessage,TInt aClientId,TBool& aCleanupComplete);
	void HandleClientExit(TInt aClientId);
	void NotifierStartAndGetResponseL(TUid aNotifierUid,TUid aChannelUid,const TDesC8& aBuffer,TInt aReplySlot,
										const RMessage2& aMessage,TInt aClientId,TBool& aCleanupComplete);
public: // from MNotifierManager
	void StartNotifierL(TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse);
	void CancelNotifier(TUid aNotifierUid);
	void UpdateNotifierL(TUid aNotifierUid,const TDesC8& aBuffer,TDes8& aResponse);
private:
	void DoAddPlugInL(const TDesC& aPath,const TDesC& aFileName,const TUidType& aUidType);
	CNotifierManager();
	void ConstructL();
	void StartFromQueueL(CQueueItem* aItem);
private:
	CArrayPtr<MNotifierBase2>* iObservedList;
	CArrayFix<RLibrary>* iLibraries;
	CChannelMonitor* iChannelMonitor;
	CActivityMonitor* iActivityMonitor;
	CNotifierQueue* iQueue;
	};




/**
@internalComponent
*/
class TChannelActivity
	{
public:
	inline TChannelActivity(TUid aChannel,TInt aHighestPriorityRunning);
public:
	TUid iChannel;
	TInt iHighestPriorityRunning;
	};



	
/**
@internalComponent
*/
class CChannelMonitor : public CBase
	{
public:
	static CChannelMonitor* NewL();
	inline void AddNewChannelL(TUid aChannel);
	TBool AlreadyHasChannel(TUid aChannel) const;
	TInt ActivityLevel(TUid aChannel) const;
	void UpdateChannel(TUid aChannel,TInt aLevel);
private:
	CChannelMonitor();
private:
	CArrayFixFlat<TChannelActivity> iMonitor;
	};




/**
@internalComponent
*/
class CNotifierActivity : public CBase
	{
public:
	static CNotifierActivity* NewLC(const MNotifierBase2::TNotifierInfo& aInfo,TInt aClientId);
	~CNotifierActivity();
	TInt Find(TInt aClientId) const;
private:
	CNotifierActivity(const MNotifierBase2::TNotifierInfo& aInfo);
	void ConstructL(TInt aClientId);
public:
	const MNotifierBase2::TNotifierInfo iInfo;
	CArrayFixFlat<TInt> iClientArray;
	};




/**
@internalComponent
*/
class CActivityMonitor : public CBase
	{
public:
	static CActivityMonitor* NewL();
	~CActivityMonitor();
	void AddL(const MNotifierBase2::TNotifierInfo& aInfo,TInt aClientId);
	void Remove(TUid aNotifierUid,TInt aClientId);
	void RemoveNotifier(TUid aNotifierUid,TUid aChannel);
	void RemoveClient(TInt aClientId);
	TBool IsNotifierActive(TUid aNotifierUid,TUid aChannel) const;
	TBool IsChannelActive(TUid aChannel,TUid& aNotifier,MNotifierBase2::TNotifierPriority& aHighestPriority) const;
	TBool IsClientPresent(TUid aNotifierUid,TUid aChannel,TInt aClientId) const;
	TBool NotifierForClient(TUid& aNotifier,TInt aClientId) const;
private:
	CActivityMonitor();
	TInt Find(TUid aNotifierUid) const;
	TInt Find(TUid aNotifierUid,TUid aChannel) const;
private:
	CArrayPtrFlat<CNotifierActivity> iMonitor;
	};




/**
@internalComponent
*/
class CQueueItem : public CBase
	{
public:
	static CQueueItem* NewL(const MNotifierBase2::TNotifierInfo& aInfo,const TDesC8& aBuffer,TInt aReplySlot,
										const RMessage2& aMessage,TInt aClientId); //Asynchronous
	static CQueueItem* NewL(const MNotifierBase2::TNotifierInfo& aInfo,const TDesC8& aBuffer,
										TInt aClientId); //synchronous
	~CQueueItem();
private:
	CQueueItem(const MNotifierBase2::TNotifierInfo& aInfo);
	void ConstructL(const TDesC8& aBuffer,TInt aClientId);
	void ConstructL(const TDesC8& aBuffer,const RMessage2& aMessage,TInt aClientId,TInt aReplySlot);
public:
	const MNotifierBase2::TNotifierInfo iInfo;
	HBufC8* iBuffer;
	TBool iAsynchronous;
	RMessage2 iMessage;  // IMPORTANT, we need to keep a full RMessage object until suport for V1 notifiers is removed
	TInt iClientId;
	TInt iReplySlot;
	};




/**
@internalComponent
*/
class CNotifierQueue : public CBase
	{
public:
	static CNotifierQueue* NewL();
	inline void QueueItemL(CQueueItem* aItem);
	CQueueItem* FetchItem(TUid aChannel);
	TBool IsAlreadyQueued(TUid aNotifier,TUid aChannel) const;
	void RemoveClient(TInt aClientId);
	TInt GetHighestQueuePriority(TUid aChannel);
private:
	inline CNotifierQueue();
private:
	CArrayPtrFlat<CQueueItem> iQueue;
	};

inline TChannelActivity::TChannelActivity(TUid aChannel, TInt aHighestPriorityRunning)
	:iChannel(aChannel),iHighestPriorityRunning(aHighestPriorityRunning)
	{}

inline void CChannelMonitor::AddNewChannelL(TUid aChannel)
	{iMonitor.AppendL(TChannelActivity(aChannel,0));}

inline CNotifierManager* CNotifierServer::Manager() const
	{return iManager;}

inline void CNotifierQueue::QueueItemL(CQueueItem* aItem)
	{iQueue.AppendL(aItem);}
inline CNotifierQueue::CNotifierQueue()
	:iQueue(3)
	{}

#endif	// __TWINTNOTIFIER_H__
