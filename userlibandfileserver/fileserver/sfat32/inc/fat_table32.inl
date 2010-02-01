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
// f32\sfat32\inc\fat_table32.inl
// 
//

/**
 @file
*/

#ifndef FAT_TABLE_32_INL
#define FAT_TABLE_32_INL



//---------------------------------------------------------------------------------------------------------------------------------------

/** @return interface to the FAT drive */
TDriveInterface& CAtaFatTable::DriveInterface() const 
    {
    return iDriveInteface;
    }

/** @return pointer to the owning mount. */
CFatMountCB* CAtaFatTable::OwnerMount() const 
    {
    return iOwner;
    }


/** @return state of this object. */
CAtaFatTable::TState CAtaFatTable::State() const 
    {
    return iState;
    }

/** sets the state of this object. */
void CAtaFatTable::SetState(CAtaFatTable::TState aState)
    {
    //__PRINT3(_L("#=-= CAtaFatTable::SetState() drv:%d, %d->%d\n"), iOwner->DriveNumber(),iState,aState);
    iState = aState;
    }

CAtaFatTable::TFatScanParam::TFatScanParam() 
             :iEntriesScanned(0), iFirstFree(0), iCurrFreeEntries(0), iCurrOccupiedEntries(0) 
    {
    } 

//---------------------------------------------------------------------------------------------------------------------------------------

/** @return object internal state */
CFatHelperThreadBase::TState CFatHelperThreadBase::State() const 
    {
    return iState;
    }

/** sustend the worker thread */
void CFatHelperThreadBase::Suspend() const 
    {
    iThread.Suspend();
    }

/** resumes the worker thread */
void CFatHelperThreadBase::Resume() const 
    {
    iThread.Resume();
    }

/** @return worker thread completion code (logon status) */    
TInt CFatHelperThreadBase::ThreadCompletionCode() const
    {
    return iThreadStatus.Int();
    }

/** @return ETrue if the thread is working, i.e. its logon status is KRequestPending*/
TBool CFatHelperThreadBase::ThreadWorking() const
    {
    return ThreadCompletionCode() == KRequestPending;
    }

/** 
    boost the priority of the worker thread or return it back to normal
    @param  aBoost ETrue to boss the priority, EFalse to "unboost"
*/    
void CFatHelperThreadBase::BoostPriority(TBool aBoost) const
    {
    TThreadPriority priority;
    if(aBoost)
        {
        priority = (TThreadPriority)EHelperPriorityBoosted;
        iPriorityBoosted = ETrue;
        }
    else        
        {
        priority = (TThreadPriority)EHelperPriorityNormal;
        iPriorityBoosted = EFalse;
        }

    iThread.SetPriority(priority);
    
    }    

/** @return ETrue if the thread's priority is boosted. */
TBool CFatHelperThreadBase::IsPriorityBoosted() const
    {
    return iPriorityBoosted;
    }


/** @return worker thread id. */
TThreadId CFatHelperThreadBase::ThreadId() const 
    {
    return iThread.Id();
    }

/** set the state of the object. See CFatHelperThreadBase::TState enum */
void CFatHelperThreadBase::SetState(CFatHelperThreadBase::TState aState) 
    {
    iState = aState;
    }

/** @return  ETrue if the worker thread is allowed to live.*/
TBool CFatHelperThreadBase::AllowedToLive() const 
    {
    return iAllowedToLive;
    }

/**
    Set a flag that indicates if the thread shall be alive or shall finish ASAP
    @param  aAllow controls the thread life and death
*/
void CFatHelperThreadBase::AllowToLive(TBool aAllow) 
    {
    iAllowedToLive = aAllow;
    }

//---------------------------------------------------------------------------------------------------------------------------------------

/** @return object type */
CFatHelperThreadBase::TFatHelperThreadType CFat32FreeSpaceScanner::Type() const 
    {
    return EFreeSpaceScanner;
    }


//---------------------------------------------------------------------------------------------------------------------------------------

/** @return object type */
CFatHelperThreadBase::TFatHelperThreadType CFat32BitCachePopulator::Type() const 
    {
    return EBitCachePopulator;
    }


#endif //FAT_TABLE_32_INL























