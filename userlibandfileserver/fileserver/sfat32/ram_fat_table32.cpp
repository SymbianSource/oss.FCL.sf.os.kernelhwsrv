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
// f32\sfat32\ram_fat_table32.cpp
// FAT16/32 File Allocation Table classes implementation for the RAM media
// 
//

/**
 @file
 @internalTechnology
*/



#include "sl_std.h"
#include "sl_fatcache32.h"
#include "fat_table32.h"


//---------------------------------------------------------------------------------------------------------------------------------------

//#######################################################################################################################################
//#     CRamFatTable class implementation 
//#######################################################################################################################################

/**
    Constructor, the RamFatTable allows disk compression by redirecting the FAT

    @param aOwner Owning mount.
*/
CRamFatTable::CRamFatTable(CFatMountCB& aOwner)
             :CFatTable(aOwner)
{
    iFatTablePos=aOwner.FirstFatSector()<<aOwner.SectorSizeLog2();
    iIndirectionTablePos=iFatTablePos+aOwner.FatSizeInBytes();
}

/** factory method */
CRamFatTable* CRamFatTable::NewL(CFatMountCB& aOwner)
{
    __PRINT1(_L("CRamFatTable::NewL() drv:%d"),aOwner.DriveNumber());

    CRamFatTable* pSelf = new (ELeave) CRamFatTable(aOwner);

    CleanupStack::PushL(pSelf);
    pSelf->InitializeL();
    CleanupStack::Pop();

    return pSelf;
}


void CRamFatTable::InitializeL()
{
    CFatTable::InitializeL();

    ASSERT(iMediaAtt & KMediaAttVariableSize);
    ASSERT(FatType() == EFat16 || FatType()== EFat32);

    iFatTablePos=iOwner->FirstFatSector()<<iOwner->SectorSizeLog2();
    iIndirectionTablePos=iFatTablePos+iOwner->FatSizeInBytes();

    //-- set RAM disk base
    TLocalDriveCapsV2 caps;
    TPckg<TLocalDriveCapsV2> capsPckg(caps);
    User::LeaveIfError(iOwner->LocalDrive()->Caps(capsPckg));
  
    iRamDiskBase = caps.iBaseAddress; 
}

/**
    Just Count free clusters in the FAT
*/
void CRamFatTable::MountL(const TMountParams& /*aMountParam*/)
{
    CountFreeClustersL();
}


/**
    Return the start address of the Ram Drive
    @return start address of the Ram Drive 
*/
TUint8 *CRamFatTable::RamDiskBase() const
    {
    return(iRamDiskBase);
    }


/**
    Allocate a new cluster number

    @return New cluster number
*/
TInt CRamFatTable::AllocateClusterNumber()
    {
    return(iOwner->MaxClusterNumber()-NumberOfFreeClusters());
    }

/**
    Write a value to the FAT (indirection table) 

    @param aFatIndex Cluster to write to
    @param aValue value to write to Fat
*/
void CRamFatTable::WriteL(TUint32 aFatIndex, TUint32 aValue)
    {
    //__PRINT(_L("CRamFatTable::WriteL"));

//  __ASSERT_ALWAYS(aFatIndex>=2 && (aValue>=2 || aValue==0) && aValue<=0xFFFF,User::Leave(KErrCorrupt));
    TUint32 indirectCluster=aFatIndex;
    TUint32 indirectClusterNewVal=0;
    ReadIndirectionTable(indirectCluster);
//  If value in indirection table!=0 we assume we have already written to the indirection table
//  So just update the FAT table
    if (indirectCluster!=0 && aValue!=0)
        {
        WriteFatTable(aFatIndex,aValue);
        return;
        }
//  If value in indirection table is 0, we haven't written to it yet, though the memory has
//  already been allocated by the EnlargeL() function
    if (indirectCluster==0 && aValue!=0) // Assumes memory has already been allocated
        indirectClusterNewVal=AllocateClusterNumber();
//  Write aValue into aFaxIndex and indirectClusterNewVal into the corresponding position
//  in the indirection table    
    WriteFatTable(aFatIndex,aValue,indirectClusterNewVal);
    }   

/**
    Read the value of a cluster in the Fat

    @param aFatIndex A cluster to read
    @return The cluster value read
*/
TUint32 CRamFatTable::ReadL(TUint32 aFatIndex) const
    {
    __ASSERT_ALWAYS(aFatIndex>=KFatFirstSearchCluster,User::Leave(KErrCorrupt));

    TUint32 clusterVal;

    switch(FatType())
        {
        case EFat16:
            clusterVal=*(TUint16*)(RamDiskBase()+PosInBytes(aFatIndex)+iFatTablePos);
        break;

        case EFat32:
            clusterVal=*(TUint32*)(RamDiskBase()+PosInBytes(aFatIndex)+iFatTablePos);
        break;
    
        default:
            ASSERT(0);
        return 0;
        }
    
    return clusterVal;
    }

/**
    Write a value to the FAT and indirection table

    @param aFatIndex Cluster number to write to
    @param aFatValue Cluster value for Fat
    @param anIndirectionValue Value for indirection table
*/
void CRamFatTable::WriteFatTable(TInt aFatIndex,TInt aFatValue,TInt anIndirectionValue)
    {
    const TUint8* pos=RamDiskBase()+PosInBytes(aFatIndex);

    switch(FatType())
        {
        case EFat16:
            *(TUint16*)(pos+iFatTablePos)=(TUint16)aFatValue;
            *(TUint16*)(pos+iIndirectionTablePos)=(TUint16)anIndirectionValue;
        break;

        case EFat32:
            *(TUint32*)(pos+iFatTablePos)=(TUint32)aFatValue;
            *(TUint32*)(pos+iIndirectionTablePos)=(TUint32)anIndirectionValue;
        break;
    
        default:
            ASSERT(0);
        return;
        }
    
    }

/**
    Write to just the fat table

    @param aFatIndex Cluster number to write to
    @param aFatValue Cluster value for Fat
*/
void CRamFatTable::WriteFatTable(TInt aFatIndex,TInt aFatValue)
    {

    switch(FatType())
        {
        case EFat16:
            *(TUint16*)(RamDiskBase()+PosInBytes(aFatIndex)+iFatTablePos)=(TUint16)aFatValue;
        break;

        case EFat32:
            *(TUint32*)(RamDiskBase()+PosInBytes(aFatIndex)+iFatTablePos)=(TUint32)aFatValue;
        break;
    
        default:
            ASSERT(0);
        return;
        }

    }

/**
    Write to just the fat table

    @param aFatIndex Cluster number to write to
    @param aFatValue Value for indirection table
*/
void CRamFatTable::WriteIndirectionTable(TInt aFatIndex,TInt aFatValue)
    {
    switch(FatType())
        {
        case EFat16:
            *(TUint16*)(RamDiskBase()+PosInBytes(aFatIndex)+iIndirectionTablePos)=(TUint16)aFatValue;
        break;

        case EFat32:
            *(TUint32*)(RamDiskBase()+PosInBytes(aFatIndex)+iIndirectionTablePos)=(TUint32)aFatValue;
        break;
    
        default:
            ASSERT(0);
        return;
        }
    }

/**
    Find the real location of aCluster
    @param aCluster Cluster to read, contians cluster value upon return
*/
void CRamFatTable::ReadIndirectionTable(TUint32& aCluster) const
    {
    switch(FatType())
        {
        case EFat16:
            aCluster=*(TUint16*)(RamDiskBase()+PosInBytes(aCluster)+iIndirectionTablePos);    
        break;

        case EFat32:
            aCluster=*(TUint32*)(RamDiskBase()+PosInBytes(aCluster)+iIndirectionTablePos);
        break;
    
        default:
            ASSERT(0);
        return;
        }
    
    }

/**
    Copy memory in RAM drive area, unlocking required

    @param aTrg Pointer to destination location
    @param aSrc Pointer to source location
    @param aLength Length of data to copy
    @return Pointer to end of data copied
*/
TUint8* CRamFatTable::MemCopy(TAny* aTrg,const TAny* aSrc,TInt aLength)
    {
    TUint8* p=Mem::Copy(aTrg,aSrc,aLength);
    return(p);
    }

/**
    Copy memory with filling the source buffer with zeroes. Target and source buffers can overlap.
    Used on RAMDrive shrinking in order to wipe data from the file that is being deleted.
    
    @param   aTrg       pointer to the target address
    @param   aSrc       pointer to the destination address
    @param   aLength    how many bytes to copy
    @return  A pointer to a location aLength bytes beyond aTrg (i.e. the location aTrg+aLength).
*/
TUint8* CRamFatTable::MemCopyFillZ(TAny* aTrg, TAny* aSrc,TInt aLength)
{
    //-- just copy src to the trg, the memory areas can overlap.
    TUint8* p=Mem::Copy(aTrg, aSrc, aLength);
    
    //-- now zero-fill the source memory area taking into account possible overlap.
    TUint8* pSrc = static_cast<TUint8*>(aSrc);
    TUint8* pTrg = static_cast<TUint8*>(aTrg);
    
    TUint8* pZFill = NULL; //-- pointer to the beginning of zerofilled area
    TInt    zFillLen = 0;  //-- a number of bytes to zero-fill
    
    if(aTrg < aSrc)
    {
        if(pTrg+aLength < pSrc)
        {//-- target and source areas do not overlap
         pZFill = pSrc;
         zFillLen = aLength;
        }
        else
        {//-- target and source areas overlap, try not to corrupt the target area
         zFillLen = pSrc-pTrg;
         pZFill = pTrg+aLength;
        }
    }
    else
    {
        if(pSrc+aLength < pTrg)
        {//-- target and source areas do not overlap
         pZFill = pSrc;
         zFillLen = aLength;
        }
        else
        {//-- target and source areas overlap, try not to corrupt the target area
         zFillLen = pSrc+aLength-pTrg;
         pZFill = pSrc;
        }
    }

    Mem::FillZ(pZFill, zFillLen);

    return(p);
}


/**
    Zero fill RAM area corresponding to the cluster number aCluster
    @param  aCluster a cluster number to be zero-filled
*/
void CRamFatTable::ZeroFillClusterL(TInt aCluster)
    {
    TLinAddr clusterPos= I64LOW(DataPositionInBytesL(aCluster));
    Mem::FillZ(iRamDiskBase+clusterPos, 1<< iOwner->ClusterSizeLog2());     
    }


/**
Return the location of a Cluster in the data section of the media

@param aCluster to find location of
@return Byte offset of the cluster data 
*/
TInt64 CRamFatTable::DataPositionInBytesL(TUint32 aCluster) const
    {
    //__PRINT(_L("CRamFatTable::DataPositionInBytes"));
    if(!ClusterNumberValid(aCluster))
        {
        __ASSERT_DEBUG(0, Fault(EFatTable_InvalidIndex));
        User::Leave(KErrCorrupt);
        }

    ReadIndirectionTable(aCluster);
    return(aCluster<<iOwner->ClusterSizeLog2());
    }

//-----------------------------------------------------------------------------

/**
    Allocate and link a cluster chain, leaves if there are not enough free clusters.
    Chain starts as close as possible to aNearestCluster, last cluster will be marked as EOF.

    @param aNumber Number of clusters to allocate
    @param aNearestCluster Cluster the new chain should be nearest to
    @leave System wide error codes
    @return The first cluster number allocated
*/
TUint32 CRamFatTable::AllocateClusterListL(TUint32 aNumber, TUint32 aNearestCluster)
	{
    __PRINT2(_L("CRamFatTable::AllocateClusterList() N:%d,NearestCL:%d"),aNumber,aNearestCluster);
	__ASSERT_DEBUG(aNumber>0, Fault(EFatBadParameter));

	if(!RequestFreeClusters(aNumber))
    	{
		__PRINT(_L("CRamFatTable::AllocateClusterListL - leaving KErrDirFull"));
		User::Leave(KErrDiskFull);
		}

	//-- if this leaves for some reason, there will be no lost clusters
    TInt firstCluster = aNearestCluster = AllocateSingleClusterL(aNearestCluster);
	
    
    if (aNumber>1)
	    {//-- if this part leaves (e.g. fail to expand the RAM drive), we will need to handle the first allocated EOC
    	TRAPD(nRes, ExtendClusterListL(aNumber-1, aNearestCluster));
        if(nRes != KErrNone)
            {
            __PRINT1(_L("CRamFatTable::AllocateClusterListL:ExtendClusterListL() failed with %d") ,nRes);
            FreeClusterListL(firstCluster); //-- clean up EOC in firstCluster
            User::Leave(nRes);
            }
        }


    return firstCluster;
	}	

/**
Allocate and mark as EOF a single cluster as close as possible to aNearestCluster,
calls base class implementation but must Enlarge the RAM drive first. Allocated cluster RAM area will be zero-filled.

@param aNearestCluster Cluster the new cluster should be nearest to
@leave System wide error codes
@return The cluster number allocated
*/
TUint32 CRamFatTable::AllocateSingleClusterL(TUint32 aNearestCluster)
    {
    __PRINT(_L("CRamFatTable::AllocateSingleClusterL"));
    iOwner->EnlargeL(1<<iOwner->ClusterSizeLog2()); //  First enlarge the RAM drive
    TInt fileAllocated=CFatTable::AllocateSingleClusterL(aNearestCluster); //   Now update the free cluster and fat/fit
    ZeroFillClusterL(fileAllocated);  //-- zero-fill allocated cluster 
    return(fileAllocated);
    }   


/**
    Extend a file or directory cluster chain, enlarging RAM drive first. Allocated clusters are zero-filled.
    Leaves if there are no free clusters (the disk is full).
    Note that method now doesn't call CFatTable::ExtendClusterListL() from its base class, be careful making changes there.

    @param aNumber      number of clusters to allocate
    @param aCluster     starting cluster number / ending cluster number after
    @leave KErrDiskFull + system wide error codes
*/
void CRamFatTable::ExtendClusterListL(TUint32 aNumber, TUint32& aCluster)
    {
    __PRINT2(_L("CRamFatTable::ExtendClusterListL(%d, %d)"), aNumber, aCluster);
    __ASSERT_DEBUG(aNumber>0,Fault(EFatBadParameter));

    iOwner->EnlargeL(aNumber<<iOwner->ClusterSizeLog2());

    while(aNumber && GetNextClusterL(aCluster))
        aNumber--;

    if(!aNumber)
        return;

    if (NumberOfFreeClusters() < aNumber)
        {
        __PRINT(_L("CRamFatTable::ExtendClusterListL - leaving KErrDirFull"));
        User::Leave(KErrDiskFull);
        }

    while(aNumber--)
        {
        const TInt freeCluster=FindClosestFreeClusterL(aCluster);

        WriteFatEntryEofL(freeCluster); //  Must write EOF for FindClosestFreeCluster to work again
        DecrementFreeClusterCount(1);
        WriteL(aCluster,freeCluster);
        aCluster=freeCluster;
        ZeroFillClusterL(freeCluster); //-- zero fill just allocated cluster (RAM area)
        }

    SetFreeClusterHint(aCluster); 
  
    }

/**
Mark a chain of clusters as free in the FAT. Shrinks the RAM drive once the
clusters are free 

@param aCluster Start cluster of cluster chain to free
@leave System wide error codes
*/
void CRamFatTable::FreeClusterListL(TUint32 aCluster)
    {
    __PRINT1(_L("CRamFatTable::FreeClusterListL aCluster=%d"),aCluster);
    if (aCluster==0)
        return; // File has no cluster allocated

    const TInt clusterShift=iOwner->ClusterSizeLog2();
    TUint32 startCluster=aCluster;
    TUint32 endCluster=0;
    TInt totalFreed=0;
    TLinAddr srcEnd=0;

    if(IsFat32())
        {
        while(endCluster!=EOF_32Bit)
            {
            TInt num=CountContiguousClustersL(startCluster,endCluster,KMaxTInt);
            if (GetNextClusterL(endCluster)==EFalse || endCluster==0)
                endCluster=EOF_32Bit;   // endCluster==0 -> file contained FAT loop

        //  Real position in bytes of the start cluster in the data area
            TLinAddr startClusterPos=I64LOW(DataPositionInBytesL(startCluster));
        //  Sliding value when more than one block is freed
            TLinAddr trg=startClusterPos-(totalFreed<<clusterShift);
            __PRINT1(_L("trg=0x%x"),trg);

        //  Beginning of data area to move
            TLinAddr srcStart=startClusterPos+(num<<clusterShift);
            __PRINT1(_L("srcStart=0x%x"),srcStart);
        //  Position of next part of cluster chain or position of end of ram drive
            if (endCluster==EOF_32Bit)  //  Last cluster is the end of the chain
                {
            
        
            //  Fixed to use the genuine RAM drive size rather than the number
            //  of free clusters - though they *should* be the same
            //  It avoids the problem of iFreeClusters getting out of sync with 
            //  the RAM drive size but doesn't solve the issue of why it can happen...
                
                srcEnd=I64LOW(iOwner->Size());
                __PRINT1(_L("srcEnd=0x%x"),srcEnd);
                }
            else                        //  Just move up to the next part of the chain
                srcEnd=I64LOW(DataPositionInBytesL(endCluster));

        //-- Copy (srcEnd-srcStart) bytes from iRamDiskBase+srcStart onto iRamDiskBase+trg
        //-- zero-filling free space to avoid leaving something important there
        ASSERT(srcEnd >= srcStart);
        if(srcEnd-srcStart > 0)
            { 
            MemCopyFillZ(iRamDiskBase+trg,iRamDiskBase+srcStart,srcEnd-srcStart);
            }
        else
            {//-- we are freeing the cluster chain at the end of the RAM drive; Nothing to copy to the drive space that has become free,
             //-- but nevertheless zero fill this space.
            Mem::FillZ(iRamDiskBase+trg, num<<clusterShift);
            }


            totalFreed+=num;
            startCluster=endCluster;
            UpdateIndirectionTable(srcStart>>clusterShift,srcEnd>>clusterShift,totalFreed);
            }
        }
    else
        {
        while(endCluster!=EOF_16Bit)
            {
            TInt num=CountContiguousClustersL(startCluster,endCluster,KMaxTInt);
            if (GetNextClusterL(endCluster)==EFalse || endCluster==0)
                endCluster=EOF_16Bit;   // endCluster==0 -> file contained FAT loop

        //  Real position in bytes of the start cluster in the data area
            TLinAddr startClusterPos=I64LOW(DataPositionInBytesL(startCluster));
        //  Sliding value when more than one block is freed
            TLinAddr trg=startClusterPos-(totalFreed<<clusterShift);
            __PRINT1(_L("trg=0x%x"),trg);

        //  Beginning of data area to move
            TLinAddr srcStart=startClusterPos+(num<<clusterShift);
            __PRINT1(_L("srcStart=0x%x"),srcStart);
        //  Position of next part of cluster chain or position of end of ram drive
            if (endCluster==EOF_16Bit)  //  Last cluster is the end of the chain
                {
            
        
            //  Fixed to use the genuine RAM drive size rather than the number
            //  of free clusters - though they *should* be the same
            //  It avoids the problem of iFreeClusters getting out of sync with 
            //  the RAM drive size but doesn't solve the issue of why it can happen...
                
                srcEnd=I64LOW(iOwner->Size());
                __PRINT1(_L("srcEnd=0x%x"),srcEnd);
                }
            else                        //  Just move up to the next part of the chain
                srcEnd=I64LOW(DataPositionInBytesL(endCluster));

        //-- Copy (srcEnd-srcStart) bytes from iRamDiskBase+srcStart onto iRamDiskBase+trg
        //-- zero-filling free space to avoid leaving something important there
        ASSERT(srcEnd >= srcStart);
        if(srcEnd-srcStart > 0)
            { 
            MemCopyFillZ(iRamDiskBase+trg,iRamDiskBase+srcStart,srcEnd-srcStart);
            }    
        else
            {//-- we are freeing the cluster chain at the end of the RAMdrive; Nothing to copy to the drive space that has become free,
             //-- but nevertheless zero fill this space.
            Mem::FillZ(iRamDiskBase+trg, num<<clusterShift);
            }    
        
            totalFreed+=num;
            startCluster=endCluster;
            UpdateIndirectionTable(srcStart>>clusterShift,srcEnd>>clusterShift,totalFreed);
            }
        }
    TInt bytesFreed=totalFreed<<clusterShift;
    
//  First free the cluster list
    CFatTable::FreeClusterListL(aCluster);
//  Now reduce the size of the RAM drive
    iOwner->ReduceSizeL(srcEnd-bytesFreed,bytesFreed);
    }

/**
Shift any clusters between aStart and anEnd backwards by aClusterShift

@param aStart Start of shift region
@param anEnd End of shift region
@param aClusterShift amount to shift cluster by
*/
void CRamFatTable::UpdateIndirectionTable(TUint32 aStart,TUint32 anEnd,TInt aClusterShift)
    {
    __PRINT(_L("CRamFatTable::UpdateIndirectionTable"));
#if defined(__WINS__)
    TUint32 count=iOwner->MaxClusterNumber();
    while (count--)
        {
        TUint32 cluster=count;
        ReadIndirectionTable(cluster);
        if (cluster>=aStart && cluster<anEnd)
            WriteIndirectionTable(count,cluster-aClusterShift);
        }
#else
    TUint16* table=(TUint16*)(RamDiskBase()+iIndirectionTablePos);
    TUint16* entry=table+iOwner->MaxClusterNumber();
    while (entry>table)
        {
        TUint32 cluster=*--entry;
        if (cluster<aStart)
            continue;
        if (cluster<anEnd)
            *entry=TUint16(cluster-aClusterShift);
        }
#endif
    }

TUint32 CRamFatTable::CountContiguousClustersL(TUint32 aStartCluster, TUint32& aEndCluster, TUint32 aMaxCount) const
	{
	__PRINT2(_L("CRamFatTable::CountContiguousClustersL() start:%d, max:%d"),aStartCluster, aMaxCount);
	TUint32 clusterListLen=1;
	TUint32 endCluster=aStartCluster;
	TInt64 endClusterPos=DataPositionInBytesL(endCluster);
	while (clusterListLen<aMaxCount)
		{
		TInt oldCluster=endCluster;
		TInt64 oldClusterPos=endClusterPos;
		if (GetNextClusterL(endCluster)==EFalse || (endClusterPos=DataPositionInBytesL(endCluster))!=(oldClusterPos+(1<<iOwner->ClusterSizeLog2())))
			{
			endCluster=oldCluster;
			break;
			}
		clusterListLen++;
		}
	aEndCluster=endCluster;
	
    return(clusterListLen);
    }



