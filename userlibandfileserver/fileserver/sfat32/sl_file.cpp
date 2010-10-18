// Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfat32\sl_file.cpp
// 
//

#include "sl_std.h"
#include "sl_cache.h"
#include <e32math.h>

const TInt KSeekIndexSize=128; // Cache 128 clusters
const TInt KSeekIndexSizeLog2=7;
const TInt KFirstClusterNum=2;

CFatFileCB::CFatFileCB()
    {
    __PRINT1(_L("CFatFileCB created 0x%x"),this);
    }

CFatFileCB::~CFatFileCB()
    {
    __PRINT1(_L("~CFatFileCB deleted 0x%x"),this);

    //-- a nasty trick to find out if the CFatFileCB is in consistent state on the moment of destruction.
    //-- Because of OOM conditions CFatFileCB might not be fully constructed and to be deleted, while FlushAll()
    //-- implies valid iMount.
    const CMountCB* pMount  = &Mount();
    if(pMount)
        {//-- do some finalisation work if CMountCB is valid
        if(FileAttModified())
            {
            IndicateFileTimeModified(ETrue); //-- this will force writing file modification time to the media on Flush
            TRAP_IGNORE(FlushAllL());
            }
        }

    delete[] iSeekIndex;
    }


void CFatFileCB::CreateSeekIndex()
//
// Create a seek index
//
    {

    iSeekIndex = new TUint32[KSeekIndexSize];
    if (iSeekIndex == NULL)
        return;

    Mem::FillZ(iSeekIndex, sizeof(TUint32) * KSeekIndexSize);

    iSeekIndexSize=CalcSeekIndexSize(FCB_FileSize());
    }

TInt CFatFileCB::SeekToPosition(TUint aNewRelCluster, TUint aClusterOffset)
//
// Use the seek index to set iCurrentPos.iCluster as close as possible to aNewRelCluster
// Return aNewRelCluster-aCurrentPos.iCluster
//
    {
    TInt clusterOffset=aClusterOffset;
    TInt seekPos=(aNewRelCluster>>iSeekIndexSize)-1;
    __ASSERT_DEBUG(seekPos<KSeekIndexSize,Fault(EFatFileSeekIndexTooSmall));

    while(seekPos>=0 && iSeekIndex[seekPos]==0 && clusterOffset!=0)
        {
        seekPos--;
        clusterOffset--;
        }
    if (clusterOffset==0) // Counted back to the current cluster
        return(aClusterOffset);
    if (seekPos<0)
        {
        iCurrentPos.iCluster=FCB_StartCluster();
        return(aNewRelCluster);
        }

    iCurrentPos.iCluster=iSeekIndex[seekPos];
    return(aNewRelCluster-((seekPos+1)<<iSeekIndexSize));
    }

void CFatFileCB::SetSeekIndexValueL(TUint aRelCluster, TUint aStoredCluster)
//
// Sets a value in the seekindex
//
    {

    TInt seekPos=(aRelCluster>>iSeekIndexSize)-1;
    __ASSERT_DEBUG(seekPos<KSeekIndexSize,Fault(EFatFileSeekIndexTooSmall));
    __ASSERT_DEBUG(seekPos>=0,Fault(EFatFileSeekIndexTooSmall2));
    iSeekIndex[seekPos] = aStoredCluster;
    }

void CFatFileCB::CheckPosL(TUint aPos)
//
// Check that the file is positioned correctly.
// If aPos<currentPos attempt to guess the new position.
//
    {
    __PRINT1(_L("CFatFileCB::CheckPosL(%d)"), aPos);
    if (aPos==iCurrentPos.iPos)
        return;
    __ASSERT_DEBUG(aPos <= FCB_FileSize(), Fault(EFatFilePosBeyondEnd));

    TUint newRelCluster=aPos>>ClusterSizeLog2();
    if ( aPos && (aPos==(newRelCluster<<ClusterSizeLog2())) )
        newRelCluster--;
    TUint oldRelCluster=iCurrentPos.iPos>>ClusterSizeLog2();
    
    if ( iCurrentPos.iPos && (iCurrentPos.iPos==(oldRelCluster<<ClusterSizeLog2())) )
        oldRelCluster--;    
    
    TInt clusterOffset=newRelCluster-oldRelCluster;
    TUint32 oldCluster=iCurrentPos.iCluster;

    iCurrentPos.iPos=aPos;
    if (clusterOffset==0)
        return;
    TInt seekOffset=clusterOffset;
    if (iSeekIndex!=NULL)
        { // Can alter iCurrentPos.iCluster
        seekOffset=SeekToPosition(newRelCluster,seekOffset);
        if (seekOffset==0)
            return;
        }
    if (clusterOffset==-1 && seekOffset!=1)
        { // Check previous cluster
        TUint32 cluster=oldCluster-1;
        if (FAT().GetNextClusterL(cluster) && cluster==oldCluster)
            {
            iCurrentPos.iCluster=oldCluster-1;
            return;
            }
        }
    if (seekOffset<0)
        {
        seekOffset=newRelCluster;
        iCurrentPos.iCluster=FCB_StartCluster();
        }
    while (seekOffset--)
        {
        if (!FAT().GetNextClusterL(iCurrentPos.iCluster))
            {
            __PRINT(_L("CFatFileCB::CheckPosL() corrupt#1"));
            User::Leave(KErrCorrupt);
            }
        TInt cluster=newRelCluster-seekOffset;
        if (iSeekIndex!=NULL && cluster && (cluster>>iSeekIndexSize)<<iSeekIndexSize==cluster)
            SetSeekIndexValueL(cluster,iCurrentPos.iCluster);
        }
    }

//-----------------------------------------------------------------------------
/** 
    Initialize FileCB from file's entry data.
    
    @param  aFatDirEntry        this file DOS dir entry.
    @param  aFileDosEntryPos    this file DOS entry dir. iterator in the parent directory.
*/
void CFatFileCB::SetupL(const TFatDirEntry& aFatDirEntry, const TEntryPos& aFileDosEntryPos)
    {
    __PRINT1(_L("CFatFileCB::SetupL[0x%x]"), this);
    

    //-- set up a file control block
    iCurrentPos.iCluster= FatMount().StartCluster(aFatDirEntry);
    iCurrentPos.iPos=0;
    
    SetAtt(aFatDirEntry.Attributes());
    SetModified(aFatDirEntry.Time(FatMount().TimeOffset()));
    
    FCB_SetStartCluster(iCurrentPos.iCluster);
    FCB_SetFileSize(aFatDirEntry.Size()); 

    iFileDosEntryPos = aFileDosEntryPos;

    SetMaxSupportedSize(KMaxSupportedFatFileSize);

    //-- create seek index
    ASSERT(!iSeekIndex);
    CreateSeekIndex();
    if(!iSeekIndex)
        User::Leave(KErrNoMemory);

    
    IndicateFileAttModified(EFalse);
    IndicateFileSizeModified(EFalse);
    IndicateFileTimeModified(EFalse);
    }

//-----------------------------------------------------------------------------
/**
    Read data from the file.
    
    @param  aFilePos    start read position within a file
    @param  aLength     how many bytes to read; on return will be how many bytes actually read
    @param  aDes        local buffer desctriptor
    @param  aMessage    from file server, used to write data to the buffer in different address space.
    @param  aDesOffset  offset within data descriptor where the data will be copied

    @leave on media read error

*/
void CFatFileCB::ReadL(TInt64 aPos,TInt& aLength, TDes8* aDes, const RMessagePtr2& aMessage, TInt aOffset)
    {
    __PRINT3(_L("CFatFileCB::ReadL[0x%x] pos=%LU len=%d"), this, aPos, aLength);
    
    if((TUint64)aPos > KMaxSupportedFatFileSize-1)
        User::Leave(KErrNotSupported);  //-- max. position in the file is 0xFFFFFFFE

    FatMount().CheckStateConsistentL();
    
    CheckPosL(I64LOW(aPos));
    
    const TUint startPos = iCurrentPos.iPos;
    const TUint curSize  = FCB_FileSize();
    const TUint length   = (TUint)aLength;
    
    if((startPos + length > curSize) || (startPos > startPos + length) )
        aLength=curSize-startPos;
		
	TUint flag = DirectIOMode(aMessage) ? RLocalDrive::ELocDrvDirectIO : 0;
	
    FatMount().ReadFromClusterListL(iCurrentPos,aLength,aDes,aMessage,aOffset, flag);
	aLength=iCurrentPos.iPos-startPos;
	}


void CFatFileCB::ReadL(TInt aFilePos,TInt& aLength,const TAny* aTrg,const RMessagePtr2& aMessage)
    {
    ReadL(TInt64(aFilePos),aLength,(TDes8*) aTrg,aMessage, 0);
    }

//-----------------------------------------------------------------------------
/**
    Write data to the file.
    
    @param  aFilePos    start write position within a file
    @param  aLength     how many bytes to write; on return contain amount of data actually written
    @param  aDes        local buffer desctriptor
    @param  aMessage    from file server, used to write data to the media from different address space.
    @param  aDesOffset  offset within data descriptor 

    @leave on media read error

*/
void CFatFileCB::WriteL(TInt64 aPos,TInt& aLength,const TDesC8* aSrc,const RMessagePtr2& aMessage, TInt aOffset)
    {
    __PRINT3(_L("CFatFileCB::WriteL[0x%x] pos=%LU len=%d"), this, aPos, aLength);

    // FAT supports 32 bits only for file size
    TUint64 endPos = aPos + aLength;
    if(endPos > KMaxSupportedFatFileSize)
        User::Leave(KErrNotSupported);
    
    FatMount().CheckStateConsistentL();
    FatMount().CheckWritableL();
    const TUint pos = I64LOW(aPos);
    CheckPosL(pos);
    
    const TUint startCluster = FCB_StartCluster();
    const TUint length       = (TUint)aLength;
    
    endPos = iCurrentPos.iPos + length; 
    if ((endPos           > FCB_FileSize()) ||
        (iCurrentPos.iPos > endPos)         ) // Overflow condition 
        DoSetSizeL(iCurrentPos.iPos+length,EFalse);
    
    TUint startPos=iCurrentPos.iPos;
    TUint badcluster=0;
    TUint goodcluster=0;
   	
	TUint flag = DirectIOMode(aMessage) ? RLocalDrive::ELocDrvDirectIO : 0;
	
	TRAPD(ret, FatMount().WriteToClusterListL(iCurrentPos,aLength,aSrc,aMessage,aOffset,badcluster, goodcluster, flag));
   	
    if (ret == KErrCorrupt || ret == KErrDied)
        {
        if(startCluster == 0)
            { //Empty File, revert all the clusters allocated.
            const TUint32 cluster = FCB_StartCluster();
            FCB_SetStartCluster(0);
            FCB_SetFileSize(0);
            IndicateFileSizeModified(ETrue);
            
            FlushAllL();

            iCurrentPos.iCluster = 0;
            iCurrentPos.iPos = 0;

            FAT().FreeClusterListL(cluster);
            FAT().FlushL();
            }
        else
            { //Calculate the clusters required based on file size, revert extra clusters if allocated.
            const TUint curSize = FCB_FileSize();
            TUint ClustersNeeded = curSize >> ClusterSizeLog2();
            if(curSize > (ClustersNeeded << ClusterSizeLog2()))
                {
                ClustersNeeded++;
                }

            TUint32 cluster = FCB_StartCluster();
            while(--ClustersNeeded)
                {
                FAT().GetNextClusterL(cluster);
                }
                
            iCurrentPos.iCluster = cluster;

            if (FAT().GetNextClusterL(cluster))
                {
                FAT().FreeClusterListL(cluster);
                }

            FAT().WriteFatEntryEofL(iCurrentPos.iCluster);
            FAT().FlushL();
            }
        }

    User::LeaveIfError(ret);

    if(badcluster != 0)
        {
        if(FCB_StartCluster() == badcluster)
            {
            FCB_SetStartCluster(goodcluster);
            FlushStartClusterL();
            }
        else
            {
            TUint32 aCluster = FCB_StartCluster();
            do
                {
                if((TUint)badcluster == FAT().ReadL(aCluster))
                    {
                    FAT().WriteL(aCluster, goodcluster);
                    FAT().FlushL();
                    break;
                    }
                }
            while(FAT().GetNextClusterL(aCluster));
            }
        }
    aLength=iCurrentPos.iPos-startPos;

    if(!IsSequentialMode() && FatMount().IsRuggedFSys() && pos+(TUint)aLength > FCB_FileSize())
        {
        WriteFileSizeL(pos+aLength);
        }

    }


void CFatFileCB::WriteL(TInt aFilePos,TInt& aLength,const TAny* aSrc,const RMessagePtr2& aMessage)
    {
    WriteL(TInt64(aFilePos),aLength,(TDesC8*) aSrc,aMessage, 0);
    }


//-----------------------------------------------------------------------------

void CFatFileCB::ResizeIndex(TInt aNewMult,TUint aNewSize)
//
// Resize the seek index to accomodate a larger or smaller filesize
// Assumes KSeekIndexSize is a power of 2.
//
    {

    TInt maxNewIndex=aNewSize>>(ClusterSizeLog2()+aNewMult);


    TInt    index=0;
    TInt    indexEnd=KSeekIndexSize;
    TInt    newValEnd=maxNewIndex;

    if (iSeekIndexSize<aNewMult)
        {
        TInt newVal=index;
        TInt step=1<<(aNewMult-iSeekIndexSize);
        index+=step-1;
        while(index<indexEnd && newVal<newValEnd)
            {
            iSeekIndex[newVal] =  iSeekIndex[index];
            newVal++;
            index+=step;
            }
        while(newVal<indexEnd)
            iSeekIndex[newVal++] =  0;
        }
    else
        {
        TInt diffSize = iSeekIndexSize-aNewMult;
        TInt oldVal=(KSeekIndexSize>>diffSize) - 1;
        TInt newVal=indexEnd-1;
        TInt skip=(1<<diffSize)-1;

        if ((iSeekIndexSize - aNewMult) > KSeekIndexSizeLog2)
            {
            ClearIndex(0); //-- Invalidate every entry.
            }
        else
            {
            while(newVal>=index)
                {

                iSeekIndex[newVal--] =  iSeekIndex[oldVal--];


                for(TInt i=skip;i>0;i--)
                    {   
                    iSeekIndex[newVal--] = 0;

                    }
                }
            }
        }
    iSeekIndexSize=aNewMult;
    }


/**
    Zero freed clusters in the index

    @param  aNewSize new size of the file that the index corresponds to.
            if = 0  all existing index will be zero filled
*/ 
void CFatFileCB::ClearIndex(TUint aNewSize)
    {

    if (!iSeekIndex)
        return;

    if(aNewSize==0)
        {
        //-- zero fill all the array
        Mem::FillZ(iSeekIndex, KSeekIndexSize*sizeof(TUint32));
        return;
        }

    // Files that fill up a cluster exactly do not have a trailing empty
    // cluster. So the entry for that position must also be invalidated
    aNewSize--;
    TInt firstInvalidIndex=aNewSize>>(iSeekIndexSize+ClusterSizeLog2());
        
    TInt indexLen=KSeekIndexSize-firstInvalidIndex;

    Mem::FillZ(iSeekIndex+firstInvalidIndex, indexLen * sizeof(TUint32));
    }

TInt CFatFileCB::CalcSeekIndexSize(TUint aSize)
//
// Find the nearest power of 2 > aSize
//
    {
    TInt count = 0;
    const TUint indexSize=KSeekIndexSize<<ClusterSizeLog2();//KSeekIndexSize=128
    if (aSize<=indexSize)
      return(count);
    
    while((aSize>>=1)>0)
        {
        count++;
        }
    return (count - (KSeekIndexSizeLog2 + ClusterSizeLog2()) + 1);
    }

//-----------------------------------------------------------------------------
/**
    Set file size.
    @param aSize new file size.
*/
void CFatFileCB::SetSizeL(TInt64 aSize)
    {
    __PRINT2(_L("CFatFileCB::SetSizeL[0x%x] sz=%LU"), this, aSize);
    
    //-- max. file size for FAT is 4GB-1
    if (I64HIGH(aSize))
        User::Leave(KErrNotSupported);

    DoSetSizeL(I64LOW(aSize), FatMount().IsRuggedFSys());
    }


void CFatFileCB::SetSizeL(TInt aSize)
    {
    SetSizeL(TInt64(aSize));
    }

//-----------------------------------------------------------------------------
/**
    Shrink file to zero size.
*/
void CFatFileCB::DoShrinkFileToZeroSizeL()
    {
        ASSERT(FCB_FileSize());
        ASSERT(FileSizeModified());
        
        ClearIndex(0); // Clear seek index array
        
        //-- update file dir. entry
        const TUint32 cluster = FCB_StartCluster();
        FCB_SetStartCluster(0);
        FCB_SetFileSize(0);
            FlushAllL();
        
        //-- free cluster list. 
            CheckPosL(0);
            FAT().FreeClusterListL(cluster);
            FAT().FlushL();
            }

//-----------------------------------------------------------------------------
/*
    Shrink file to smaller size, but > 0

    @param aNewSize new file size
    @param aForceCachesFlush if ETrue, all file/FAT caches will be flushed 
*/
void CFatFileCB::DoShrinkFileL(TUint32 aNewSize, TBool aForceCachesFlush)
    {
    ASSERT(FileSizeModified());
    ASSERT(FCB_FileSize() > aNewSize && aNewSize);
    
    if(aForceCachesFlush)       
        WriteFileSizeL(aNewSize); //-- write file size directly to its dir. entry

    CheckPosL(aNewSize);

    TUint32 cluster=iCurrentPos.iCluster;
    
    if (FAT().GetNextClusterL(cluster))
        {//-- truncate the cluster chain
        FAT().WriteFatEntryEofL(iCurrentPos.iCluster);
        FAT().FreeClusterListL(cluster);
        }
        
    ClearIndex(aNewSize);
    FAT().FlushL();
    }
    
//-----------------------------------------------------------------------------
/**
    Expand a file.
    
    @param aNewSize new file size.
    @param aForceCachesFlush if ETrue, all file/FAT caches will be flushed
*/
void CFatFileCB::DoExpandFileL(TUint32 aNewSize, TBool aForceCachesFlush)
    {
    ASSERT(FCB_FileSize() < aNewSize);
    ASSERT(FileSizeModified());

    const TUint32 KClusterSzLog2  = ClusterSizeLog2();
    const TUint32 newSizeClusters = (TUint32)(((TUint64)aNewSize + Pow2(KClusterSzLog2) - 1) >> KClusterSzLog2);


    //-- expanding a file
    if (FCB_StartCluster() == 0)
        {//-- the initial file size is 0 (no cluster chain)
         
        ClearIndex(0); //-- clear seek index array
        //-- FAT().FreeClusterHint() will give us a hint of the last free cluster
        const TUint32 tempStartCluster=FAT().AllocateClusterListL(newSizeClusters, FAT().FreeClusterHint()); 
        FAT().FlushL();

        iCurrentPos.iCluster=tempStartCluster;
        FCB_SetStartCluster(tempStartCluster);
        FCB_SetFileSize(aNewSize);
        FlushAllL();
        }
    else
        {
        const TUint curSize = FCB_FileSize(); 
        const TUint32 oldSizeClusters = ((curSize + Pow2(KClusterSzLog2) - 1) >> KClusterSzLog2);
        ASSERT(newSizeClusters >= oldSizeClusters);
        const TUint newClusters = newSizeClusters-oldSizeClusters;  //-- Number of clusters we need to append to the existing cluster chain
        if (newClusters)
            {
            TEntryPos currentPos=iCurrentPos;
            CheckPosL(FCB_FileSize());
            FAT().ExtendClusterListL(newClusters,iCurrentPos.iCluster);
            iCurrentPos=currentPos;
            }
    
        FAT().FlushL();
        
        if(!IsSequentialMode() && aForceCachesFlush)    // Write file size directly to its dir. entry if a cache flush
            WriteFileSizeL(aNewSize);               // is needed and rugged FAT is not ignored by client
        }

    }

//-----------------------------------------------------------------------------
/**
    Set file size. This can involve extending/truncating file's cluster chain.
    @param  aSize               new file size
    @param  aForceCachesFlush   if ETrue, all changes in metadata will go to the media immediately. 
                                it is used in Rugged FAT mode.
*/
void CFatFileCB::DoSetSizeL(TUint aSize, TBool aForceCachesFlush)
    {
    __PRINT4(_L("CFatFileCB::DoSetSizeL[0x%x] sz:%d, oldSz:%d, flush:%d"), this, aSize, FCB_FileSize(), aForceCachesFlush);

    FatMount().CheckStateConsistentL();
    FatMount().CheckWritableL();

    
    // Can not change the file size if it is clamped
    if(Mount().IsFileClamped(MAKE_TINT64(0,FCB_StartCluster())) > 0)
        User::Leave(KErrInUse);
    
    if(aSize == FCB_FileSize())
        return;

    IndicateFileSizeModified(ETrue);
	IndicateFileAttModified(ETrue);		// ensure file size is flushed

	TInt newIndexMult=CalcSeekIndexSize(aSize);
	if (iSeekIndex!=NULL && newIndexMult!=iSeekIndexSize)
		ResizeIndex(newIndexMult,aSize);

	//-------------------------------------------
    //-- shrinking file to 0 size
    if(aSize == 0)
        {
        DoShrinkFileToZeroSizeL();
        return;
        }

    //-------------------------------------------
    //-- shrinking file to non-zero size
    if (aSize < FCB_FileSize())
        {
        DoShrinkFileL(aSize, aForceCachesFlush);
        return;
        }
    
    //-------------------------------------------
    //-- expanding a file
    DoExpandFileL(aSize, aForceCachesFlush);

    }

//-----------------------------------------------------------------------------
/**
    Set file entry details, like file attributes and modified time
    This method doesn't write data to the media immediately, instead, all modified data are cached and can be flushed later 
    in FlushAllL()

    @param  aTime           file modification time (and last access as well)
    @param  aSetAttMask     file attributes OR mask
    @param  aClearAttMask   file attributes AND mask

*/
void CFatFileCB::SetEntryL(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask)
    {
    __PRINT1(_L("CFatFileCB::SetEntryL[0x%x]"), this);
    
    FatMount().CheckStateConsistentL();
    FatMount().CheckWritableL();

    //-- change file attributes
    const TUint setAttMask = (aSetAttMask & KEntryAttMaskSupported); //-- supported attributes to set
	TUint oldAtt = Att();
	TUint newAtt = oldAtt;

	if (setAttMask|aClearAttMask)
		{
        newAtt |= setAttMask;
        newAtt &= ~aClearAttMask;
		if (newAtt != oldAtt)
			{
	        SetAtt(newAtt);
		    IndicateFileAttModified(ETrue); //-- indicate that file attributes have changed
			}
		}
    
    //-- set file entry modification time if required
	if (aSetAttMask&KEntryAttModified)
		{
        SetModified(aTime);        //-- set file modified time
        IndicateFileAttModified(ETrue); //-- indicate that file attributes have changed
        IndicateFileTimeModified(ETrue); //-- this will force writing file mod. time to the media on Flush
        }

    }


//-----------------------------------------------------------------------------
/** 
    The same as FlushAllL(). This method is called from RFile::Flush()
*/
void CFatFileCB::FlushDataL()
    {
    __PRINT1(_L("CFatFileCB::FlushDataL[0x%x]"), this);
    FlushAllL();
    }

//-----------------------------------------------------------------------------
/** 
    Flush the fide directory entry data: files size, attributes, time etc. 
*/
void CFatFileCB::FlushAllL()
    {

    //-- define this symbol in order to enable legacy behaviour, i.e. compulsory updating file dir. entry on flush.
    //-- otherwise the FlushAllL() will update the file dir. entry only if it differs from what is on the media, i.e.
    //-- file size, start cluster, attributes and modification timestamp
    #define ALWAYS_UPDATE_ENTRY_ON_FLUSH

    __PRINT1(_L("CFatFileCB::FlushAllL[0x%x]"), this);

    if (Mount().IsCurrentMount()==EFalse)
        User::Leave(KErrDisMounted);

    FatMount().CheckStateConsistentL();
    FatMount().CheckWritableL();

    if(!FileSizeModified() && !FileAttModified() && !FileTimeModified())
        return; //-- nothing has changed in the file entry at all


    //-- read file dir. entry
    TFatDirEntry entry;
    FatMount().ReadDirEntryL(iFileDosEntryPos,entry);
    __ASSERT_ALWAYS(entry.IsEndOfDirectory()==EFalse,User::Leave(KErrCorrupt));

    //-- the problem with KEntryAttModified here is that the file server uses this flag to 
    //-- deal with dirty file data. This means that this flag can be set even if there were no changes
    //-- in file time and attributes. Just check if any of the entry field has changed at all
    
    TBool bUpdateDirEntry = ETrue;
    const TTimeIntervalSeconds  timeOffset = FatMount().TimeOffset();

#ifndef ALWAYS_UPDATE_ENTRY_ON_FLUSH
    
    TBool bTimeModified = FileTimeModified();  //-- check if file modifiication time has been changed explicitly
    if(bTimeModified)
        {//-- additional check; for FAT entry modification time has 2 sec. granularity.
        bTimeModified = !entry.IsTimeTheSame(iModified, timeOffset);
        }

    if(!bTimeModified)
      if(//-- TS is the same as on the media, check other entry fields
        (entry.Attributes() == (Att() & KEntryAttMaskSupported)) && //-- file attributes have not changed
        (entry.Size() == FCB_FileSize()) &&                         //-- file size hasn't changed
        (entry.StartCluster() == FCB_StartCluster())                //-- file start cluster hasn't changed 
        )               
        {
        bUpdateDirEntry = EFalse; //-- no need to update file dir. entry
        }

#endif //#ifndef ALWAYS_UPDATE_ENTRY_TS_ON_FLUSH

    if(bUpdateDirEntry)
        {//-- write entry to the media
        __PRINT(_L("  CFatFileCB::FlushAllL #1"));
        entry.SetAttributes(Att() & KEntryAttMaskSupported);
        entry.SetSize(FCB_FileSize());
        entry.SetTime(iModified, timeOffset);
        
        entry.SetStartCluster(FCB_StartCluster());

	    const TBool setNotify = FatMount().GetNotifyUser();
		if(setNotify)
			{
			FatMount().SetNotifyOff();	// do not launch a notifier
			}

			TRAPD(ret, FatMount().WriteDirEntryL(iFileDosEntryPos,entry));
		
		if(setNotify)
			{
			FatMount().SetNotifyOn();
			}

		User::LeaveIfError(ret);

		IndicateFileSizeModified(EFalse);
		IndicateFileTimeModified(EFalse);
	    }


    //-- KEntryAttModified must be reset anyway
    IndicateFileAttModified(EFalse); 
	}

//-----------------------------------------------------------------------------

/**
    Rename already opened file.
    @param  aNewName new file name; all trailing dots from the name will be removed
*/
void CFatFileCB::RenameL(const TDesC& aNewName)
    {
    __PRINT2(_L("CFatFileCB::RenameL[0x%x], name:%S"),this, &aNewName);

    FatMount().CheckStateConsistentL();
    FatMount().CheckWritableL();

    const TPtrC fileName = RemoveTrailingDots(aNewName); //-- remove trailing dots from the name


    FatMount().DoRenameOrReplaceL(*iFileName, fileName, CFatMountCB::EModeRename, iFileDosEntryPos);
    
    AllocBufferL(iFileName, fileName);
    
    if(!FatMount().IsRuggedFSys())
        FAT().FlushL();
    }


//***********************************************************
//* BlockMap interface
//***********************************************************
    
TInt CFatFileCB::BlockMap(SBlockMapInfo& aInfo, TInt64& aStartPos, TInt64 aEndPos)
//
// Retrieves the block map of a given section of the file, in the FAT file system.
//  
    {
    __PRINT2(_L("CFatFileCB::BlockMap aStartPos=%ld aEndPos=%ld"), aStartPos, aEndPos);
    
    if ( I64HIGH(aStartPos) || I64HIGH(aEndPos) )
        return KErrNotSupported;

    TUint startPos = I64LOW(aStartPos);
    TUint endPos = I64LOW(aEndPos);

    // aEndPos will always be >=0 at this point
    const TUint length = endPos - startPos;
    
    // Store the position of cluster zero in aInfo
    CFatMountCB& fatMount = FatMount();

    TInt drvNo=-1;
    TBusLocalDrive* locDrv;
    if((fatMount.LocalDrive()->GetLocalDrive(locDrv)==KErrNone) && ((drvNo=GetLocalDriveNumber(locDrv))>=0) && (drvNo<KMaxLocalDrives))
        aInfo.iLocalDriveNumber=drvNo;
    else
        return KErrNotSupported;

    // Fetch the address of cluster 0
	TInt r;
    TRAP(r, aInfo.iStartBlockAddress = fatMount.FAT().DataPositionInBytesL(KFirstClusterNum));
	if (r != KErrNone)
		return r;


	TRAP(r, CheckPosL(startPos));
    if (r != KErrNone)
        return r;

    aInfo.iBlockStartOffset = fatMount.ClusterRelativePos(iCurrentPos.iPos);
    aInfo.iBlockGranularity = 1 << FatMount().ClusterSizeLog2();
    const TUint myStartPos = iCurrentPos.iPos;
    if ( myStartPos + length > FCB_FileSize())
        return KErrArgument;

    TRAP(r, FatMount().BlockMapReadFromClusterListL(iCurrentPos, length, aInfo));
    if (r != KErrNone)
        return r;

    aStartPos = iCurrentPos.iPos;
    if ((I64LOW(aStartPos) == FCB_FileSize()) || ( I64LOW(aStartPos) == (myStartPos + length)))
        return KErrCompletion;
    
    
    return KErrNone;
    }


TInt CFatFileCB::GetInterface(TInt aInterfaceId,TAny*& aInterface,TAny* aInput)
    {
    switch(aInterfaceId)
        {
        case EExtendedFileInterface:
            ((CFileCB::MExtendedFileInterface*&) aInterface) = this;
            return KErrNone;

        case EBlockMapInterface:
            aInterface = (CFileCB::MBlockMapInterface*) this;
            return KErrNone;

        case EGetLocalDrive:
            return FatMount().LocalDrive()->GetLocalDrive((TBusLocalDrive*&) aInterface);

        default:
            break;
        }
    
    return CFileCB::GetInterface(aInterfaceId,aInterface,aInput);
    }


/**
    Overwrites file's start cluster (iStartCluster) in its directory entry.
*/
void CFatFileCB::FlushStartClusterL()
    {
    __PRINT1(_L("CFatFileCB::FlushStartClusterL[0x%x]"), this);

    CFatMountCB& mount = FatMount();
    TFatDirEntry dirEntry;
    
    mount.ReadDirEntryL(iFileDosEntryPos, dirEntry); //-- read this file's dir. entry
    dirEntry.SetStartCluster(FCB_StartCluster());    //-- set new start cluster
    mount.WriteDirEntryL(iFileDosEntryPos, dirEntry);//-- write the entry back
    }


/**
    This is a RuggedFAT - specific method. Writes file size to the corresponding field of its file directory entry.
*/
void CFatFileCB::WriteFileSizeL(TUint aSize)
    {
    __PRINT2(_L("CFatFileCB::WriteFileSizeL[0x%x], sz:%d"), this, aSize);

    CFatMountCB& mount = FatMount();
    TFatDirEntry dirEntry;

    mount.ReadDirEntryL(iFileDosEntryPos, dirEntry); //-- read this file's dir. entry
    dirEntry.SetSize(aSize);                         //-- set new size

	// As we're updating the directory entry anyway, we might as well update the attributes & time 
	// if these have been modified to save having to update them later...
	if (FileAttModified())
		{
		dirEntry.SetAttributes(Att() & KEntryAttMaskSupported);
        IndicateFileAttModified(EFalse); 
		IndicateFileTimeModified(ETrue);	//-- this mirrors the behaviour of CFatFileCB::~CFatFileCB()
		}
	if (FileTimeModified())
		{
		dirEntry.SetTime(iModified, FatMount().TimeOffset());
        IndicateFileTimeModified(EFalse);
		}

    mount.WriteDirEntryL(iFileDosEntryPos, dirEntry);//-- write the entry back

    IndicateFileSizeModified(EFalse);
    }







