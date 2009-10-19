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
// e32\include\drivers\soundsc.inl
// The kernel side inline header file for the shared chunk sound driver.
// 
//

/**
 @file
 @internalTechnology
 @prototype
*/

// class TSndScTransfer	

inline TInt TSndScTransfer::GetNotStartedLen()
	{return(iEndOffset-iStartedOffset);}

inline TInt TSndScTransfer::GetStartOffset()
	{return(iStartedOffset);}

inline TInt TSndScTransfer::GetLengthTransferred()
	{return(iLengthTransferred);}
	
// class TSoundScRequest	
	
inline TSoundScRequest::TSoundScRequest()
	{iNext=NULL; iClientRequest=0;}	
	
// class TSoundScPlayRequest
	
inline void TSoundScPlayRequest::SetFail(TInt aCompletionReason)
	{iCompletionReason=aCompletionReason; iTf.iTfState=TSndScTransfer::ETfDone;}
	
inline void TSoundScPlayRequest::UpdateProgress(TInt aLength)
	{if (iTf.SetCompleted(aLength)) iCompletionReason=KErrNone;}	
			
// class TSoundScRequestQueue	
		
inline TBool TSoundScRequestQueue::IsEmpty()
	{return(iPendRequestQ.IsEmpty());}
	
inline TBool TSoundScRequestQueue::IsAnchor(TSoundScRequest* aReq)
	{return(aReq==&iPendRequestQ.iA);}	
	
// class DRecordBufferManager
	
inline TAudioBuffer* DRecordBufferManager::GetCurrentRecordBuffer()
	{return(iCurrentBuffer);}
	
inline TAudioBuffer* DRecordBufferManager::GetNextRecordBuffer()
	{return(iNextBuffer);}
	
// class DSoundScLdd

inline DSoundScPdd* DSoundScLdd::Pdd()
	{return((DSoundScPdd*)iPdd);}
	
inline void DSoundScLdd::CompletePlayRequest(TSoundScPlayRequest* aReq,TInt aResult)
	{aReq->iCompletionReason=aResult; DoCompletePlayRequest(aReq);}	
		
inline DSoundScLdd* DSoundScPdd::Ldd()
	{return(iLdd);}

// class DSoundScPdd

inline TInt DSoundScPdd::TimeTransferred(TInt64& /*aTime*/, TInt /*aState*/)
	{return(KErrNotSupported);}
