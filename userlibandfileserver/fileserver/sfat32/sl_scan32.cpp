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
// f32\sfat32\sl_scan32.cpp
// ScanDrive code, specific for EFAT32.FSY
// 
//

/**
 @file
 @internalTechnology
*/

#include "sl_std.h"
#include "sl_scandrv.h"

const TInt KMaxScanDepth			= 20;   ///< Maximum scan depth of to avoid stack over flow 
const TInt KClusterListGranularity	= 8;    ///< Granularity of cluster list used for storage of clusters when KMaxScanDepth is reached



/**
    CScanDrive factory method
    @param  aMount the owning mount
*/
CScanDrive* CScanDrive::NewL(CFatMountCB* aMount)
	{
	if(!aMount)
		{
        ASSERT(0);
        User::Leave(KErrArgument);
        }

	CScanDrive* self=new (ELeave) CScanDrive();
	CleanupStack::PushL(self);
	self->ConstructL(aMount);
	CleanupStack::Pop();
	
    return self;
	}


CScanDrive::~CScanDrive()
	{
	for(TUint i=0; i<KMaxArrayDepth && iClusterListArray[i]!=NULL; ++i)
		{
		iClusterListArray[i]->Close();
		delete iClusterListArray[i];
		}

    iMediaFatBits.Close();
    iScanFatBits.Close();

	}

/**
    Creates the structure of this class.
    @param aMount The owning mount
*/
void CScanDrive::ConstructL(CFatMountCB* aMount)
    {
    ASSERT(aMount);

    //--- setting up 
	iMount			 = aMount;
	iGenericError	 = ENoErrors;
	iDirError		 = ENoDirError;
	iHangingClusters = 0;
	iMaxClusters	 = iMount->UsableClusters()+KFatFirstSearchCluster; //-- UsableClusters() doesn't count first 2 unused clusers
    //------------------------------
	
    //-- create bit vectors that will represent FAT on media and reconstructed by ScanDrive. Each bit in the vector represents 1 FAT cluster.
    const TUint32 KClustersNum = MaxClusters();

    CleanupClosePushL(iMediaFatBits);
    CleanupClosePushL(iScanFatBits);

    iMediaFatBits.CreateL(KClustersNum);
    iScanFatBits.CreateL(KClustersNum);

    CleanupStack::Pop(&iScanFatBits);
    CleanupStack::Pop(&iMediaFatBits);
    }

//----------------------------------------------------------------------------------------------------
/**
    FAT type-agnostic parser. Reads whole FAT and sets up a bit vector.
    For FAT12/16 it's OK, because the FAT12/16 is fully cached.
*/
void CScanDrive::DoParseFatL()
    {
    const TInt KMaxClusters = MaxClusters();

    iMediaFatBits.Fill(0);

    __PRINT1(_L("CScanDrive::DoParseFatL(), clusters:%d"), KMaxClusters);

    for(TInt i=KFatFirstSearchCluster; i<KMaxClusters; ++i)
	    {
        const TUint32 nFatEntry = ReadFatL(i);
       
        //-- each '1' bit represents a used cluster 
        if(nFatEntry != KSpareCluster)
            iMediaFatBits.SetBit(i);
	    }
    }

//----------------------------------------------------------------------------------------------------
/**
    Parse FAT32 buffer.
    @param  aBuf            buffer, containing FAT32 entries (current portion of FAT)
    @param  aCurrFatEntry   current FAT entry processed
*/
void CScanDrive::DoParseFat32Buf(const TPtrC8& aBuf, TUint32& aCurrFatEntry)
    {
    ASSERT((aBuf.Size() & (sizeof(TFat32Entry)-1)) == 0);
    
    const TInt KNumEntries = aBuf.Size() >> KFat32EntrySzLog2;
    const TFat32Entry* const pFatEntry = (const TFat32Entry*)(aBuf.Ptr());

    for(TInt i=0; i<KNumEntries; ++i)
        {
        if(aCurrFatEntry >= KFatFirstSearchCluster)
            {
            if((pFatEntry[i] & KFat32EntryMask) != KSpareCluster)
                {//-- found a non-free FAT32 entry
                iMediaFatBits.SetBit(aCurrFatEntry);
                }
            }
        ++aCurrFatEntry;    
        }
    }

//----------------------------------------------------------------------------------------------------
/**
    A specialised method to read and parse FAT32 using a larger buffer.
    1. Larger buffer gives better read performance
    2. using dedicated buffer doesn't trash FAT32 LRU cache.
*/
void CScanDrive::DoParseFat32L()
    {
    const TInt    KNumClusters = MaxClusters();
    
    __PRINT1(_L("CScanDrive::DoParseFat32L(), clusters:%d"), KNumClusters);

    ASSERT(iMount->FatType() == EFat32);
    
    const TUint32 KFat1StartPos = iMount->StartOfFatInBytes();
    const TUint32 KFatSize      = KNumClusters * sizeof(TFat32Entry); //-- usable size of one FAT.

    const TUint32 KFatBufSz = 32*K1KiloByte; //-- buffer size for FAT reading. 32K seems to be optimal size

    iMediaFatBits.Fill(0);

    RBuf8 fatParseBuf;
    CleanupClosePushL(fatParseBuf);

    //-- allocate memory for FAT parse buffer
    fatParseBuf.CreateMaxL(KFatBufSz);

    //-- read FAT directly from the media into the large buffer and parse it
    TUint32 rem = KFatSize;
    TUint32 mediaPos = KFat1StartPos;   
    TUint32 currFatEntry = 0;

    while(rem)
        {
        const TUint32 bytesToRead=Min(rem, KFatBufSz);
        TPtrC8 ptrData(fatParseBuf.Ptr(), bytesToRead);

        //-- read portion of the FAT into buffer
        User::LeaveIfError(iMount->LocalDrive()->Read(mediaPos, bytesToRead, fatParseBuf)); 

        //-- parse the buffer and populate bit vector
        DoParseFat32Buf(ptrData, currFatEntry);
        
        mediaPos += bytesToRead;
        rem -= bytesToRead;
        }

    fatParseBuf.Close();
    CleanupStack::PopAndDestroy(&fatParseBuf); 
    }



//----------------------------------------------------------------------------------------------------
/**
    Sets up a bit list representation of the media fat
    Reads whole FAT and sets '1' bits in the bit vector corresponding to the occupied clusters.
*/
void CScanDrive::ReadMediaFatL()
    {
    ASSERT(iMount->ConsistentState());
    
    TInt nRes;

    if(iMount->FatType() == EFat32)
        {//-- for FAT32 try to use specialised method of parsing
        TRAP(nRes, DoParseFat32L())
        if(nRes == KErrNone)
            return;
        }


    //-- use old FAT-agnostic parsing
    DoParseFatL();
    }


/**
    @return True if a directory error has been found
*/
TBool CScanDrive::IsDirError() const
	{
	return(iDirError!=0);
	}

/**
    After StartL() and finishing allows us to know if there were any problems discovered at all.
    The client may wish to remount the filesystem if there were errors.

    @return The code describing the problem.
*/
CScanDrive::TGenericError CScanDrive::ProblemsDiscovered() const
{
    if(IsDirError()) 
        return EScanDriveDirError;

    return iGenericError;
}

/**
    Sets the flag indicating that there are errors in filesystem structure
    See ProblemsDiscovered()

    @param  aError a code describing the error
*/
void CScanDrive::IndicateErrorsFound(TGenericError aError)
{
    ASSERT(aError != ENoErrors);
    iGenericError = aError;
}


//----------------------------------------------------------------------------------------------------
/**
    Starts the scanner.
    
    @param	aMode	Specifies the operational mode.
*/
void CScanDrive::StartL(TScanDriveMode aMode)
	{
	__PRINT2(_L("CScanDrive::StartL(%d), drive:%d"), aMode, iMount->DriveNumber());
    iScanDriveMode = aMode;

    //-- used for measuring time
    TTime   timeStart;
    TTime   timeEnd;
    
    timeStart.UniversalTime(); //-- take start time

	ReadMediaFatL();

        //timeEnd.UniversalTime(); //-- take end time
        //elapsedTime = (TInt)( (timeEnd.MicroSecondsFrom(timeStart)).Int64() / K1mSec);
        //__PRINT1(_L("#@@@ CScanDrive #1:%d ms "), elapsedTime);

	CheckDirStructureL();

    //-- uncomment a line below if you need to compare real and restored FAT tables and print out all differences
    //CompareFatsL(EFalse);

        //timeEnd.UniversalTime(); //-- take end time
        //elapsedTime = (TInt)( (timeEnd.MicroSecondsFrom(timeStart)).Int64() / K1mSec);
        //__PRINT1(_L("#@@@ CScanDrive #2:%d ms "), elapsedTime);

    if(CheckDiskMode())
        {//-- in check disk mode it is nesessarily just to detech FS errors
        CompareFatsL(ETrue); //-- will stop on the first error found
        }
    else
        {//-- In ScanDrive mode we need to find and fix Rugged FAT artefacts.
     
        if(IsDirError())
		    FixupDirErrorL();

	    CompareAndFixFatsL();
        }

	PrintErrors();

    timeEnd.UniversalTime(); //-- take end time
    const TInt elapsedTime = (TInt)( (timeEnd.MicroSecondsFrom(timeStart)).Int64() / K1mSec);
    (void)elapsedTime;

    __PRINT1(_L("CScanDrive: Directories visisted = %d\n"),iDirsChecked);
    __PRINT1(_L("#@@@ CScanDrive time taken:%d ms "), elapsedTime);
    
    

	return;
	}

//----------------------------------------------------------------------------------------------------
/**
    Fix errors detected by the drive scan
 
    @leave System wide error code
*/
void CScanDrive::FixupDirErrorL()
	{
	ASSERT(!CheckDiskMode());
    
    if(!IsDirError())
		return;
	
    if(iDirError==EScanMatchingEntry)
		{
		FindSameStartClusterL();
		FixMatchingEntryL();
		}
	else
		{
        FixPartEntryL();
        }

    IndicateErrorsFound(EScanDriveDirError); //-- indicate that we have found errors
	}

//----------------------------------------------------------------------------------------------------
/**
    Find positions of entries with same start cluster for error correction, searches
    the whole volume. Starts at the root directory. 

    @leave System wide error code
*/
void CScanDrive::FindSameStartClusterL()
	{
	TInt err=FindStartClusterL(iMount->RootIndicator());
	if(err==KErrNone)
		return;
	
    for(TUint i=0;i<KMaxArrayDepth && iClusterListArray[i]!=NULL;++i)
		{
		RArray<TInt>* clusterList=iClusterListArray[i];
		for(TInt j=0;j<clusterList->Count();++j)
			{
			iRecursiveDepth=0;
			err=FindStartClusterL((*clusterList)[j]);
			if(err==KErrNone)
				return;
			}
		}

    if(err != KErrNone)
        {
        __PRINT1(_L("CScanDrive::FindSameStartClusterL() #1 %d"), err);
        User::Leave(KErrNotFound);
        }
	}

//----------------------------------------------------------------------------------------------------
/**
    Scan through directory structure looking for start cluster found in iMatching

    @param aDirCluster Start cluster for scan to start
    @return System wide error value
    @leave 
*/
TInt CScanDrive::FindStartClusterL(TUint32 aDirCluster)
	{
	__PRINT1(_L("CScanDrive::FindStartCluster dirCluster=%d"),aDirCluster);
	
	if(aDirCluster < (TUint)iMount->RootIndicator() || aDirCluster >= MaxClusters())
        {
        __PRINT(_L("CScanDrive::FindStartCluster() #!\n"));
        IndicateErrorsFound(EBadClusterNumber);
        User::Leave(KErrCorrupt);
        }


	if(++iRecursiveDepth==KMaxScanDepth)
		{
		--iRecursiveDepth;
		return(KErrNotFound);
		}

	TEntryPos entryPos(aDirCluster,0);
	TInt dirEntries=0;

	for(;;)
		{
		TFatDirEntry entry;
		ReadDirEntryL(entryPos,entry);

		if(entry.IsParentDirectory()||entry.IsCurrentDirectory()||entry.IsErased())
			{
			if(IsEndOfRootDir(entryPos))
				break;
			MoveToNextEntryL(entryPos);
			continue;
			}

		if(entry.IsEndOfDirectory())
			break;
		
		TEntryPos vfatPos=entryPos;
		const TBool isComplete = MoveToVFatEndL(entryPos,entry,dirEntries);
		
        if(!isComplete)
            {
            __PRINT(_L("CScanDrive::FindStartCluster() #2\n"));
            IndicateErrorsFound(EEntrySetIncomplete);
            User::Leave(KErrBadName);
            }


		TInt err=CheckEntryClusterL(entry,vfatPos);
		if(err==KErrNone)
			{
			--iRecursiveDepth;
			return(err);
			}

		if(IsEndOfRootDir(entryPos))
			break;

		MoveToNextEntryL(entryPos);
		}
	--iRecursiveDepth;
	return(KErrNotFound);
	}

//----------------------------------------------------------------------------------------------------
/**
    Procces aEntry to find matching start cluster

    @param aEntry Directory entry to check
    @param aEntryPos Position of directory to check
    @return System wide error value
    @leave 
*/
TInt CScanDrive::CheckEntryClusterL(const TFatDirEntry& aEntry, const TEntryPos& aEntryPos)
	{
	__PRINT(_L("CScanDrive::CheckEntryClusterL"));
	if((TUint)iMount->StartCluster(aEntry)==iMatching.iStartCluster)
		{
		TBool complete=AddMatchingEntryL(aEntryPos);
		if(complete)
			return(KErrNone);
		}
	else if(aEntry.Attributes()&KEntryAttDir)
		return(FindStartClusterL(iMount->StartCluster(aEntry)));

	return KErrNotFound;
	}

//----------------------------------------------------------------------------------------------------
/**
    Checks directory structure for errors, can be considered the start point of the scan.  
    Handles recursion depth to avoid stack overflow.

    @leave System wide error code
*/
void CScanDrive::CheckDirStructureL()
	{
	CheckDirL(iMount->RootIndicator());
	// Due to recursive nature of CheckDirL when a depth of
	// KMaxScanDepth is reached, clusters are stored in a list
	// and passed into CheckDirL afresh

	for(TUint i=0;i<KMaxArrayDepth && iClusterListArray[i]!=NULL;++i)
		{
		RArray<TInt>* clusterList=iClusterListArray[i];
		++iListArrayIndex;
		for(TInt j=0;j<clusterList->Count();++j)
			{
			iRecursiveDepth=0;
			CheckDirL((*clusterList)[j]);
			}
		}
	}


//----------------------------------------------------------------------------------------------------
/**
    This function is called recursively with Process entry untill the whole volume has been scanned.
    Each directory entry is scanned for errors, these are recorded for later fixing. 

    @param aCluster Directory cluster to start checking
    @leave System wide error codes
*/
void CScanDrive::CheckDirL(TUint32 aCluster)
	{
	__PRINT1(_L("CScanDrive::CheckDirL aCluster=%d"),aCluster);

	// check depth of recursion
	if(++iRecursiveDepth==KMaxScanDepth)
		{
		AddToClusterListL(aCluster);
		--iRecursiveDepth;
		return;
		}

	++iDirsChecked;

	TEntryPos entryPos(aCluster,0);
	TInt dirEntries=0;
	for(;;)
		{
		TFatDirEntry entry;
		ReadDirEntryL(entryPos,entry);
		if(!iMount->IsEndOfClusterCh(entryPos.iCluster))
			++dirEntries;

		if(entry.IsParentDirectory() || entry.IsCurrentDirectory() || entry.IsErased())
			{
			if(IsEndOfRootDir(entryPos))
				break;

			MoveToNextEntryL(entryPos);
			continue;
			}

		if(entry.IsEndOfDirectory())
			{
			if(aCluster)	
				RecordClusterChainL(aCluster,dirEntries<<KSizeOfFatDirEntryLog2);

			break;
			}

		TEntryPos origPos=entryPos;
		TFatDirEntry origEntry=entry;
		TInt origDirEntries=dirEntries;
		
		const TBool isComplete = MoveToVFatEndL(entryPos,entry,dirEntries);
        
        if(!isComplete && CheckDiskMode())
            {//-- broken VFAT entryset; in CheckDisk mode this is the FS error, abort further activity
                __PRINT(_L("CScanDrive::CheckDirL() #1"));
                IndicateErrorsFound(EInvalidEntrySize);
                User::Leave(KErrCorrupt);
            }

        // Only assume that this is a corrupted VFAT entry if the VFAT attributes are set; 
		// assuming a non-VFAT corrupted entry is a VFAT entry is dangerous as we then assume that the 
		// first byte is a count of entries to skip, thus completely invalidating the next <n> directories.
		
        //-- this code seems to deal with one of the Rugged FAT artefacts: partially deleted VFAT entryset, when DOS entry is deleted first
        //-- and delettion of VFAT ones had failed
        if(!isComplete && origEntry.IsVFatEntry())
			{
			AddPartialVFatL(origPos,origEntry);
			if(!IsEofF(entryPos.iCluster))
				{
				TInt toMove=origEntry.NumFollowing()-(dirEntries-origDirEntries);
				if(toMove)
					MovePastEntriesL(entryPos,entry,toMove,dirEntries);
				}
			else
				{
				// we fell off the end of the directory file, so just strip this
				// incomplete long file name entry
				dirEntries = origDirEntries;
				}
			}
		else
			{
            ProcessEntryL(entry);
            }

		if(IsEndOfRootDir(entryPos))
			break;

		MoveToNextEntryL(entryPos);
		}
	--iRecursiveDepth;
	}

//----------------------------------------------------------------------------------------------------
/**
    Process non trivial entries, such as files, if they are correct by filling out their 
    cluster allocation in the bit packed Fat table. If it comes accross a directory 
    CheckDirL will be called.

    @param aEntry Directory entry to check
    @leave System wide error code
*/
void CScanDrive::ProcessEntryL(const TFatDirEntry& aEntry)
	{
	__PRINT(_L("CScanDrive::ProcessEntryL"));
	const TUint entryAtt=aEntry.Attributes();

    if((entryAtt & ~KEntryAttMaskSupported) || aEntry.IsErased())
        {
        __PRINT1(_L("CScanDrive::ProcessEntryL() wrong entry att: 0x%x"), entryAtt);
        IndicateErrorsFound(EEntryBadAtt);
        User::Leave(KErrCorrupt);
        }
	
    if(!(entryAtt&(KEntryAttDir|KEntryAttVolume)) && iMount->StartCluster(aEntry)>0)
		{//-- this is a file with length >0. Check that its cluster chain corresponds to its size
        RecordClusterChainL(iMount->StartCluster(aEntry), aEntry.Size());
        }
	else if(entryAtt&KEntryAttDir)
		{//-- this is the directory, walk into it
        CheckDirL(iMount->StartCluster(aEntry));
        }
	}

//----------------------------------------------------------------------------------------------------
/**
    Walks the cluster chain for a correct file or directory, checks that the cluster 
    has not already been used and that the correct number of clusters are allocated for the 
    size of file. Registers cluster as used, if correct.

    @param aCluster Cluster chain start point
    @param aSizeInBytes Size of the file or directory in bytes
    @leave System wide error values
*/
void CScanDrive::RecordClusterChainL(TUint32 aCluster, TUint aSizeInBytes)
	{
	__PRINT2(_L("CScanDrive::RecordClusterChainL() cl:%d, sz:%d") ,aCluster, aSizeInBytes);

    if(aCluster < KFatFirstSearchCluster || aCluster >= MaxClusters())
	    {
        __PRINT(_L("CScanDrive::RecordClusterChainL() #0"));
        IndicateErrorsFound(EBadClusterNumber);
        User::Leave(KErrCorrupt);
        }
	
    TUint clusterCount;
	
    if(aSizeInBytes==0)
		{
		clusterCount=1;
        }
	else
		{
        const TUint64 tmp = aSizeInBytes + Pow2_64(iMount->ClusterSizeLog2()) - 1;
        clusterCount = (TUint) (tmp >> iMount->ClusterSizeLog2());
        }

	TUint startCluster=aCluster;
	
	while(clusterCount)
		{
        if(IsClusterUsedL(aCluster))
			{//-- this cluster already seems to belong to some other object; crosslinked cluster chain. Can't fix it.
            __PRINT1(_L("CScanDrive::RecordClusterChainL #1 %d"),aCluster); 
            
            if(CheckDiskMode())
                {//-- in check disk mode this is an FS error; Indicate error and abort further scanning
                __PRINT(_L("CScanDrive::RecordClusterChainL #1.1"));
                IndicateErrorsFound(EClusterAlreadyInUse);
                User::Leave(KErrCorrupt);
                }
            
            
            if(IsDirError() || iMatching.iStartCluster > 0 || aCluster != startCluster)
                {//-- secondary entry into this state
                __PRINT(_L("CScanDrive::RecordClusterChainL #1.2")); 
                IndicateErrorsFound(EClusterAlreadyInUse);
                User::Leave(KErrCorrupt);
                }

			iMatching.iStartCluster=aCluster;
			iDirError=EScanMatchingEntry;		//ERROR POINT
            IndicateErrorsFound(EScanDriveDirError); //-- indicate that we have found errors
			return;
			}

		
        if(clusterCount==1)
			{//-- we have reached the end of the cluster chain
			if(!iMount->IsEndOfClusterCh(ReadFatL(aCluster)))
				{
				// According to the directory entry, we have reached the end of the cluster chain,
				// whereas in the media FAT, it is not.
				// This is a rugged FAT artefact; hanging cluster chain:
				// 	A cluster chain which is longer in the FAT table than is recorded in the corresponding directory entry 
				// 	or not terminated by an EOC entry in FAT.
				// This is caused by:
				//  - File truncation failing.
				//	- OR file expanding failing during flushing to the media FAT.
                
                if(CheckDiskMode())
                    {//-- in check disk mode this is an FS error; Indicate error and abort further scanning
                    __PRINT1(_L("CScanDrive::RecordClusterChainL #2 Hanging cluster=%d"),aCluster);
                    IndicateErrorsFound(EInvalidEntrySize);
                    User::Leave(KErrCorrupt);
                    }
				
				// The chain will be truncated to the size recorded in the file's DOS entry and
				// the remaining lost cluster chain will be fixed later in CompareAndFixFatsL().
				FixHangingClusterChainL(aCluster);
                }

            //__PRINT1(_L("#--: %d -> EOC"), aCluster); 
            MarkClusterUsedL(aCluster);
			return;
			}
		else
			{
			const TUint clusterVal=ReadFatL(aCluster);

            //__PRINT2(_L("#--: %d -> %d"), aCluster, clusterVal); 
            if(IsEofF(clusterVal) || clusterVal == KSpareCluster )
                {//-- unexpected end of the cluster chain (it is shorter than recorded in file dir. entry)
                __PRINT1(_L("CScanDrive::RecordClusterChainL #3 %d"),clusterVal); 
                IndicateErrorsFound(EBadClusterValue);
                User::Leave(KErrCorrupt);
                }


			MarkClusterUsedL(aCluster);
			aCluster=clusterVal;
			--clusterCount;
			}
		
        }//while(clusterCount)
	}

//----------------------------------------------------------------------------------------------------
/**
    Move to dos entry, checking all vfat entry ID numbers are in sequence.
    Assumes aEntry is not erased

    @param aPos Position of the entry to move from, returns with new position
    @param aEntry The Dos entry after the Vfat entries on return
    @param aDirLength Running total of the length of the directory in entries
    @leave System wide error codes
    @return EFalse if not valid vfat entries or dos entry, else returns ETrue
*/
TBool CScanDrive::MoveToVFatEndL(TEntryPos& aPos,TFatDirEntry& aEntry,TInt& aDirLength)
	{
	__PRINT2(_L("CScanDrive::MoveToVFatEndL cluster=%d,pos=%d"),aPos.iCluster,aPos.iPos);
	if(!aEntry.IsVFatEntry())
		return IsDosEntry(aEntry);

	TInt toFollow=aEntry.NumFollowing();

    if(toFollow <=0 || aEntry.IsErased())
        {
        __PRINT1(_L("CScanDrive::MoveToVFatEndL #1 %d"),toFollow);
        IndicateErrorsFound(EEntrySetIncomplete);
        User::Leave(KErrCorrupt);
        }


	for(;;)
		{
		MoveToNextEntryL(aPos);
		ReadDirEntryL(aPos,aEntry);
		++aDirLength;
		--toFollow;
		if(!toFollow)
			break;
		
        if(!IsValidVFatEntry(aEntry,toFollow))
			return(EFalse);
		}
	// A sequence of VFat entries must end with a Dos entry to be valid.
	return(IsDosEntry(aEntry));
	}

//----------------------------------------------------------------------------------------------------
/**
    Check if an entry is valid VFat

    @param aEntry Entry to check
    @param aPrevNum Number into VFat entries for a dos entry to ensure in correct position
    @return ETrue if aEntry is a valid vfat entry
*/
TBool CScanDrive::IsValidVFatEntry(const TFatDirEntry& aEntry,TInt aPrevNum)const
	{
	if(aEntry.IsErased()||!aEntry.IsVFatEntry())
		return EFalse;

	return(aEntry.NumFollowing()==aPrevNum);
	}

//----------------------------------------------------------------------------------------------------
/**
    Check if an entry is a Dos entry

    @param aEntry Entry to check
    @return ETrue if aEntry is a dos entry
*/
TBool CScanDrive::IsDosEntry(const TFatDirEntry& aEntry)const
	{
	const TBool res = !(aEntry.Attributes()&~KEntryAttMaskSupported) && !aEntry.IsErased() && !aEntry.IsVFatEntry() && !aEntry.IsEndOfDirectory();
	return res;
	} 

//----------------------------------------------------------------------------------------------------
/**
    Add partial entry to iPartEntry under the error condition of not all Vfat entries being present

    @param aStartPos Position of the Dos entry associated with the VFat entries
    @param aEntry Directory Entry of the Dos entry associated with the VFat entries
    @leave KErrCorrupt Occurs if the entry is not valid
*/
void CScanDrive::AddPartialVFatL(const TEntryPos& aStartPos, const TFatDirEntry& aEntry)
	{
	__PRINT2(_L("CScanDrive::AddPartialVFatL cluster=%d pos=%d"),aStartPos.iCluster,aStartPos.iPos);

    if(IsDirError())
        {
        __PRINT(_L("CScanDrive::AddPartialVFatL #1"));
        User::Leave(KErrCorrupt);
        }

	iPartEntry.iEntryPos=aStartPos;
	iPartEntry.iEntry=aEntry;
	iDirError=EScanPartEntry;
	}

//----------------------------------------------------------------------------------------------------
/**
    Add entry position to iMatching

    @param aEntryPos Position of the entry with the matching entry
    @leave KErrCorrupt if the start cluster is 0 or more that two matching entries occurs
    @return 
*/
TBool CScanDrive::AddMatchingEntryL(const TEntryPos& aEntryPos)
	{
	__PRINT2(_L("CScanDrive::AddMatchingEntryL cluster=%d pos=%d"),aEntryPos.iCluster,aEntryPos.iPos);
	
    if(iMatching.iStartCluster <= 0 || iMatching.iCount >= KMaxMatchingEntries)
        {
        __PRINT(_L("CScanDrive::AddMatchingEntryL #1"));
        User::Leave(KErrCorrupt);
        }


	iMatching.iEntries[iMatching.iCount++]=aEntryPos;
	return iMatching.iCount==KMaxMatchingEntries;
	}


//----------------------------------------------------------------------------------------------------
/**
    Scan for differences in the new and old FAT table writing them to media if discovered
    It is supposed to be called in 'ScanDrive' mode only

    @leave System wide error codes
*/
void CScanDrive::CompareAndFixFatsL()
    {
    __PRINT1(_L("CScanDrive::CompareAndFixFatsL() drv:%d"),iMount->DriveNumber());

    ASSERT(!CheckDiskMode());

    TUint32 nClustersFixed = 0; //-- fixed clusters count
    TUint32 nBadClusters   = 0; //-- bad cluster count
    TUint32 dirtyFatSector = 0; //-- FAT table media sector with not-flushed data

    const TUint32 KSectorSzLog2 = iMount->SectorSizeLog2(); //-- Log2(media Sector Size)
    
    TUint32 diffPos;
    if(iMediaFatBits.Diff(iScanFatBits, diffPos))
    {//-- there is a difference between FATs' bit representation
    
        ASSERT(diffPos >= KFatFirstSearchCluster);

        const TUint32 maxClusters = iScanFatBits.Size();
    
        for(TUint32 i=diffPos; i<maxClusters; ++i)
	        {
            if(BoolXOR(iMediaFatBits[i], iScanFatBits[i]))
                {//-- difference in the cluster "i" between a real FAT and what ScanDrive restored.
          
                //-- indicate that there are some problems in FAT. and we probably wrote something there.
                IndicateErrorsFound(EScanDriveDirError); 
                
                //-- skip BAD cluster, can't mark it as unused.
                if(iMount->IsBadCluster(ReadFatL(i)))
                    {
                    ++nBadClusters;
                    continue;
                    }
         
                //-- Here we found a lost cluster. Its FAT entry will be replaced with KSpareCluster.
                //-- In the case of multiple lost clusters FAT table will be flushed on media sector basis.
                //-- It is much faster than flushing FAT after every write and will guarantee
                //-- that FAT won't be corrupted if the media driver provides atomic sector write. 
                if(nClustersFixed == 0)
                    {//-- this is the first lost cluster entry we found
                    
                    //-- relative FAT media sector for the 'i' entry. The real value doesn't matter, 
                    //-- we will just be flushing FAT before writing to the different FAT media sector.
                    dirtyFatSector = iMount->FAT().PosInBytes(i) >> KSectorSzLog2; 
                    
                    iMount->FAT().WriteL(i, KSpareCluster); //-- fix lost cluster
                    }
                else
                    {
                    const TUint32 fatSec = iMount->FAT().PosInBytes(i) >> KSectorSzLog2; 

                    if(fatSec != dirtyFatSector)
                        {//-- we are going to write to a different media sector
                        iMount->FAT().FlushL();
                        iMount->FAT().WriteL(i, KSpareCluster); //-- fix lost cluster
                        dirtyFatSector = fatSec;
                        }
                    else
                        {//-- write to the same FAT media sector without flushing
                        iMount->FAT().WriteL(i, KSpareCluster); //-- fix lost cluster
                        }
                    
                    }

                ++nClustersFixed;
        
                }//if(BoolXOR(iMediaFatBits[i], iScanFatBits[i])

            }//for(TInt i=KFatFirstSearchCluster; i<maxClusters; ++i)

    }//if(iMediaFatBits.Diff(iScanFatBits, diffPos))


    if(nClustersFixed)
        iMount->FAT().FlushL();
    
    //------

	
	// Add the number of hanging clusters fixed by ScanDrive
	nClustersFixed += iHangingClusters;
    
    __PRINT3(_L("CScanDrive::WriteNewFatsL() fixed clusters=%d,hanging clusters=%d,bad clusters=%d"),nClustersFixed,iHangingClusters,nBadClusters);
    }

//----------------------------------------------------------------------------------------------------
/**
    Read the "Rugged FAT" ID, stored in reserved2 in the Dos entry or associated with the Dos entry of the 
    Entry at the position passed in. This is used to find which version of two matching entries should be kept.

    @param aVFatPos Position of an entry to read ID from
    @leave System wide error codes
    @return The ID found in reserved2 field of dos entry 
*/
TInt CScanDrive::GetReservedidL(TEntryPos aVFatPos)
	{
	__PRINT(_L("CScanDrive::GetReservedidL"));
	TFatDirEntry entry;
	ReadDirEntryL(aVFatPos,entry);
	if(!IsDosEntry(entry))
		{
		TInt toMove=entry.NumFollowing();
		
		while(toMove--)
			MoveToNextEntryL(aVFatPos);
		
		ReadDirEntryL(aVFatPos,entry);
		}
	
	return(entry.RuggedFatEntryId());
	}

//----------------------------------------------------------------------------------------------------
/**
    Erase part entry found in iPartEntry
    @leave System wide error code
*/
void CScanDrive::FixPartEntryL()
	{
	__PRINT2(_L("CScanDrive::FixPartEntryL cluster=%d,pos=%d"),iPartEntry.iEntryPos.iCluster,iPartEntry.iEntryPos.iPos);
	ASSERT(!CheckDiskMode());
    iMount->EraseDirEntryL(iPartEntry.iEntryPos,iPartEntry.iEntry);
    IndicateErrorsFound(EScanDriveDirError); //-- indicate that we have found errors
	}
	
//----------------------------------------------------------------------------------------------------
/**
    Delete entry with largest value in the reserved2 section(bytes 20 and 21) of dos entry

    @leave System wide error code
*/
void CScanDrive::FixMatchingEntryL()
	{
	
    __PRINT1(_L("CScanDrive::FixMatchingEntryL() start cluster=%d"),iMatching.iStartCluster);
	
    if(iMatching.iCount != KMaxMatchingEntries)
        {
        __PRINT1(_L("CScanDrive::FixMatchingEntryL() #1 %d"), iMatching.iCount);            
        User::Leave(KErrCorrupt);
        }

	ASSERT(!CheckDiskMode());

    const TInt idOne=GetReservedidL(iMatching.iEntries[0]);
	const TInt idTwo=GetReservedidL(iMatching.iEntries[1]);
	TFatDirEntry entry;
	
    const TInt num = idOne>idTwo ? 0:1;
	ReadDirEntryL(iMatching.iEntries[num],entry);

	iMount->EraseDirEntryL(iMatching.iEntries[num],entry);
    
    IndicateErrorsFound(EScanDriveDirError); //-- indicate that we have found errors
	}

//----------------------------------------------------------------------------------------------------
/**
	Fix a hanging cluster chain.
	Writes EOF to the corresponding FAT entry, making this cluster chain length correspond to the 
	real file size recorded in the directory entry.
	The remainder of the chain will be cleaned up later in CompareAndFixFatsL().
	
	@leave	System wide error code
*/
void CScanDrive::FixHangingClusterChainL(TUint32 aFatEofIndex)
	{
	__PRINT1(_L("CScanDrive::FixHangingClusterL() Hanging cluster=%d"), aFatEofIndex);
	
	iMount->FAT().WriteFatEntryEofL(aFatEofIndex);
	iMount->FAT().FlushL();
	iHangingClusters++;
	
	// Indicate that we have found an error
	IndicateErrorsFound(EScanDriveDirError);
	}


//----------------------------------------------------------------------------------------------------
/**
    Move past specified number of entries

    @param aEntryPos Start position to move from, updated as move takes place
    @param aEntry Directory entry moved to
    @param aToMove Number of entries to move through
    @param aDirEntries Number of entries moved, updated as move takes place
    @leave System wide error code
*/
void CScanDrive::MovePastEntriesL(TEntryPos& aEntryPos,TFatDirEntry& aEntry,TInt aToMove,TInt& aDirEntries)
	{
	while(aToMove-- && !IsEofF(aEntryPos.iCluster))
		{
		MoveToNextEntryL(aEntryPos);
		++aDirEntries;
		}
	ReadDirEntryL(aEntryPos,aEntry);
	}

//----------------------------------------------------------------------------------------------------
/**
    Adds aCluster to cluster list array so that it may be revisited later, avoids stack 
    over flow

    @param aCluster Directory cluster number to add to the list
    @leave KErrNoMemory If allocation fails
*/
void CScanDrive::AddToClusterListL(TInt aCluster)
	{

	if(iListArrayIndex>=KMaxArrayDepth)
		return;

	if(iClusterListArray[iListArrayIndex]==NULL)
		iClusterListArray[iListArrayIndex]=new(ELeave) RArray<TInt>(KClusterListGranularity);

	iClusterListArray[iListArrayIndex]->Append(aCluster);
	}


//----------------------------------------------------------------------------------------------------
/**
    Used in "CheckDisk" mode mostly. Compares first FAT table on the media with the FAT bitmap restored by walking the directory structure.
    Displays any differences and records an error if found.
    
    @param  aStopOnFirstErrorFound if ETrue will stop after discovering first error (FATs discrepancy)
    
    @leave  System wide error codes
*/
void CScanDrive::CompareFatsL(TBool aStopOnFirstErrorFound)  
    {
	__PRINT1(_L("CScanDrive::CompareFatsL(%d)"), aStopOnFirstErrorFound);
		
   
    TUint32 diffPos;
    if(!iMediaFatBits.Diff(iScanFatBits, diffPos))
        return; //-- FATs are identical
    
    //-- there is a difference between the real FAT and reconstructed one. Find the mismaching bit and fix FAT. 
    const TUint clusters = iMount->UsableClusters();
    ASSERT(diffPos < (TUint32)clusters);
                        
    TUint scanusedcnt=0;
	TUint mediausedcnt=0;
	
    for(TUint i=diffPos; i<clusters; ++i)
	    {
        const TBool bRealFatEntry = iMediaFatBits[i];
        const TBool bNewFatEntry  = iScanFatBits[i];

		if(BoolXOR(bRealFatEntry, bNewFatEntry))
		    {//-- mismatch between FAT on the media and the FAT bitmap restored by walking directory structure

			if(bRealFatEntry)
				{//-- FAT[i] on the media is marked as occupied, but restored FAT bitmap shows that it is free
				if(iMount->IsBadCluster(ReadFatL(i)))
					continue; //-- this is a BAD cluster it can't be occupied by the FS object, OK.

				__PRINT2(_L("FAT[%d] = %d\n"), i, ReadFatL(i));
				
				//-- this is a Rugged FAT artefact; a lost cluster
				__PRINT1(_L("Lost cluster=%d\n"),i);
				
				IndicateErrorsFound(EBadClusterValue);
				}
			else
				{//-- FAT[i] on the media is marked as free, but restored FAT bitmap shows that it is occupied by some object
				IndicateErrorsFound(EClusterAlreadyInUse);
				__PRINT1(_L("Unflushed cluster = %d\n"),i);
				}

		 if(aStopOnFirstErrorFound)
			 break; //-- not asked to check for errors further

            }
		
        if(bRealFatEntry)
			mediausedcnt++;

		if(bNewFatEntry)
			scanusedcnt++;
        }        	

    __PRINT2(_L("Scan Fat Used=%d, Media Fat Used=%d \n"),scanusedcnt,mediausedcnt);
    }	

//----------------------------------------------------------------------------------------------------
/**
    For debug purposes, print errors found as debug output
*/
void CScanDrive::PrintErrors()
    {
#if defined(_DEBUG)    
	__PRINT1(_L("Directories visisted = %d\n"),iDirsChecked);

	if(iDirError==EScanPartEntry)
	    {
    	__PRINT2(_L("Part entry-dir cluster=%d,dir pos=%d,\n"),iPartEntry.iEntryPos.iCluster,iPartEntry.iEntryPos.iPos);
        }
	else if(iDirError==EScanMatchingEntry)
	    {
		__PRINT1(_L("Matching cluster - cluster no=%d\n"),iMatching.iStartCluster);
		__PRINT2(_L("\tcluster 1 - dir cluster=%d,dir pos=%d\n"),iMatching.iEntries[0].iCluster,iMatching.iEntries[0].iPos);
		__PRINT2(_L("\tcluster 2 - dir cluster=%d,dir pos=%d\n"),iMatching.iEntries[1].iCluster,iMatching.iEntries[1].iPos);
	    }
#endif
    }



/**
Read a FAT directory entry from disk, either reads directly from the main cache or
from the cluster buffer if scan drive is running in a seperate thread.

@param aPos Media position of entry to read
@param aDirEntry Contents of directory entry read
@leave System wide error code 
*/
void CScanDrive::ReadDirEntryL(const TEntryPos& aPos,TFatDirEntry& aDirEntry)
    {
	//__PRINT(_L("CScanDrive::ReadDirEntryL"));
	if (iMount->IsEndOfClusterCh(aPos.iCluster))
		{
		Mem::FillZ(&aDirEntry,sizeof(TFatDirEntry));
		return;
		}

	iMount->ReadDirEntryL(aPos, aDirEntry);
    }


/**
    Move to next directory entry, if anEntry is at the end of the cluster, and we are not 
    the root dir, move it to the next cluster in the chain.

    @param aPos Current directory position up dated to position of next entry.
*/
void CScanDrive::MoveToNextEntryL(TEntryPos& aPos)
	{
	//__PRINT(_L("CScanDrive::MoveToNextEntryL"));
	iMount->MoveToNextEntryL(aPos);
	}	

/**
    Read a cluster from the Media Fat if scan run in a separate thread read from scan Fat table
    otherwise read from mount owned Fat table

    @param aClusterNum Cluster to read
    @return Value of cluster read from Fat
*/
TUint32 CScanDrive::ReadFatL(TUint aClusterNum)
	{
	if(aClusterNum < KFatFirstSearchCluster || aClusterNum >= MaxClusters())
        {
        __PRINT1(_L("CScanDrive::ReadFatL() bad cluster:%d\n"),aClusterNum);
        IndicateErrorsFound(EBadClusterNumber);
        User::Leave(KErrCorrupt);
        }

    //-- actually, ReadL() can leave with some error code, that won't be reflected in IndicateErrorsFound().
    //-- it's possible to improve but is it worth it?
    return iMount->FAT().ReadL(aClusterNum);			
    }


/**
    Set a cluster as visited in the bit packed scan Fat
    @param aCluster Cluster number
*/
void CScanDrive::MarkClusterUsedL(TUint aClusterNum)
	{
	if(aClusterNum < KFatFirstSearchCluster || aClusterNum >= MaxClusters())
        {
        __PRINT1(_L("CScanDrive::MarkClusterUsedL() bad cluster:%d\n"),aClusterNum);
        IndicateErrorsFound(EBadClusterNumber);
        User::Leave(KErrCorrupt);
        }

    iScanFatBits.SetBit(aClusterNum);
	}


/**
    Query whether a cluster is already set as used 
    @param aCluster Cluster to query
*/
TBool CScanDrive::IsClusterUsedL(TUint aClusterNum) 
	{
	if(aClusterNum < KFatFirstSearchCluster || aClusterNum >= MaxClusters())
        {
        __PRINT1(_L("CScanDrive::IsClusterUsedL() bad cluster:%d\n"),aClusterNum);
        IndicateErrorsFound(EBadClusterNumber);
        User::Leave(KErrCorrupt);
        }

    return iScanFatBits[aClusterNum];
	}

/**
    @param aPos Position in a directory cluster
    @return  ETrue if aPos is the last entry in the root directory
*/
TBool CScanDrive::IsEndOfRootDir(const TEntryPos& aPos)const
	{
	return(iMount->IsRootDir(aPos)&&(iMount->StartOfRootDirInBytes()+aPos.iPos==(iMount->RootDirEnd()-KSizeOfFatDirEntry)));
	}

/**
    @param aVal Value of the cluster to be tested
    @return ETrue if aVal is the end of cluster marker
*/
TBool CScanDrive::IsEofF(TInt aVal) const 
	{
    return iMount->IsEndOfClusterCh(aVal);
	}

/** @return max. number of clusters on the volume being scanned */
TUint32 CScanDrive::MaxClusters() const
    {
        ASSERT(iMaxClusters);
        return iMaxClusters;
    }

/** @return ETrue in we are operating in "CheckDisk" mode*/
TBool CScanDrive::CheckDiskMode() const 
    {
    return iScanDriveMode == ECheckDisk;
    }









