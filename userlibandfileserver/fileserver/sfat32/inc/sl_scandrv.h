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
// f32\sfat\inc\sl_scandrv.h
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef SL_SCANDRV_H
#define SL_SCANDRV_H

#include "sl_std.h"

//---------------------------------------------------------------------------------------------------------------------------------

const TUint KMaxMatchingEntries		= 2;        ///< Maximum number of matching directory entries scan drive can fix. Any more indicates a fault in the file system
const TUint KMaxArrayDepth			= 6;        ///< Maximum array depth for cluster storage when KMaxScanDepth is reached

/** Data structure used to store the location of a partial VFat entry */
struct TPartVFatEntry
	{
	TEntryPos    iEntryPos; ///< The position of the partial VFat entry
	TFatDirEntry iEntry;    ///< The Dos entry The VFat entries belong with
	};


/** Data structure used to store the locations of entries with matching start cluster numbers. */
struct TMatchingStartCluster
	{
	TEntryPos   iEntries[KMaxMatchingEntries]; ///< The positions of the matching entries
	TUint       iCount;         ///< Count of matching entries
	TUint       iStartCluster;  ///< The matching cluster number found in more than one entry
	};


//---------------------------------------------------------------------------------------------------------------------------------
/**
    This class is used for checking volume for FS errors and fixing a limited set of FS artefacts introduced by Rugged FAT on write failures.
    It can operate in 2 modes:
    
    1. "ScanDrive" mode, scan whole volume for possible Rugged FAT artefacts and fix them if possible. 
        1.1 If there was no problem at all, then StartL() finishes normally and ProblemsDiscovered() returns ENoErrors.
        1.2 If there was Rugged FAT artefact and it had been successfully fixed, StartL() finishes normally and ProblemsDiscovered() returns EScanDriveDirError.
            In this case the client may perform volum remounting, because FAT is very likely to have been changed.
        1.3 If there was a fatal error, like media failure or unfixable FS problem, StartL() will leave with some generic error code.

    2.  "CheckDisk" mode. check file system for known artefacts and return an error if _any_ problem discovered.
        In this case StartL() _may_ leave with something like KErrCorrupt if there was a media failure or scan has stumbled across unknown FS error, 
        ProblemsDiscovered() _may_ return some code describing the problem. If StartL() did not leave, but ProblemsDiscovered() returns a code different 
        from ENoErrors, this means that there is FS corruption. 
*/
class CScanDrive : public CBase
	{

public:

	~CScanDrive();
	static CScanDrive* NewL(CFatMountCB* aMount);
	void ConstructL(CFatMountCB* aMount);

public:

    /** description of known problems that this scanned can deal with. Mostly used in "CheckDisk " mode */
    enum TGenericError
        {
        ENoErrors = 0,          ///< 0  no errors discovered
        EBadClusterNumber,      ///< 1  cluster number that doesn't correspond to the max. amount of clusters on the volume
        EClusterAlreadyInUse,   ///< 2  cross-linked cluster chain
        EBadClusterValue,       ///< 3  also means "lost cluster"
        EInvalidEntrySize,      ///< 4  size of file/directory does not correspond to the cluster chain length
        EEntrySetIncomplete,    ///< 5  incomplete VFAT entryset
        EEntryBadAtt,           ///< 6  incorrect entry attributes

        
        EUnknownError = 95,     ///< unknown error

        EScanDriveDirError=100  ///< 100 ScanDrive error
        };

    TGenericError ProblemsDiscovered() const;

	/** CScanDrive mode of operation */
    enum TScanDriveMode
        {
        EScanAndFix, ///< "ScanDrive" mode, scan whole volume for possible Rugged FAT artefacts and fix them
        ECheckDisk,  ///< "CheckDisk" mode. check file system for known artefacts and return an error if _any_ problem discovered
        };
    
    void StartL(TScanDriveMode aMode);

private:
	void PrintErrors();
	void CompareFatsL(TBool aStopOnFirstErrorFound) ;
	void CompareAndFixFatsL();

    void FixupDirErrorL();

	void ReadMediaFatL();
    void DoParseFatL();
    void DoParseFat32L();
    void DoParseFat32Buf(const TPtrC8& aBuf, TUint32& aCurrFatEntry);

	TBool IsClusterUsedL(TUint aCluster);
	void MarkClusterUsedL(TUint aCluster);

	TUint32 ReadFatL(TUint aClusterNum) ;
	void FindSameStartClusterL();
	TInt FindStartClusterL(TUint32 aDirCluster);
	void CheckDirStructureL();
	void CheckDirL(TUint32 aCluster);
	void ProcessEntryL(const TFatDirEntry& aEntry);
	TInt CheckEntryClusterL(const TFatDirEntry& aEntry, const TEntryPos& aEntryPos);
	void RecordClusterChainL(TUint32 aCluster,TUint aSizeInBytes);
	TBool MoveToVFatEndL(TEntryPos& aPos,TFatDirEntry& aEntry,TInt& aDirLength);
	TBool IsValidVFatEntry(const TFatDirEntry& aEntry,TInt prevNum)const;
	TBool IsDosEntry(const TFatDirEntry& aEntry)const;
	void AddPartialVFatL(const TEntryPos& aStartPos, const TFatDirEntry& aEntry);
	TBool AddMatchingEntryL(const TEntryPos& aEntryPos);
	TInt GetReservedidL(const TEntryPos aVFatPos);
	
	void FixPartEntryL();
	void FixMatchingEntryL();
	void FixHangingClusterChainL(TUint32 aFatEocIndex);
	void MovePastEntriesL(TEntryPos& aEntryPos,TFatDirEntry& aEntry,TInt aToMove,TInt& aDirEntries);
	void AddToClusterListL(TInt aCluster);
	inline TBool AlreadyExistsL(TInt aCluster)const;
	inline TBool IsEndOfRootDir(const TEntryPos& aPos)const;
	inline TBool IsEofF(TInt aVal)const;
	inline TBool IsDirError()const;
	void MoveToNextEntryL(TEntryPos& aPos);
	void ReadDirEntryL(const TEntryPos& aPos,TFatDirEntry& aDirEntry);

    inline void IndicateErrorsFound(TGenericError aError);
    inline TUint32 MaxClusters() const;
    inline TBool CheckDiskMode() const;

protected:
	
    /**
    Internal ScanDrive mode specific errors.
    In Rugged FAT mode (current implementation) any type of error of this kind can occur only once and it will be fixed.
    Otherwise the FS is considered to be corrupted.
    */
    enum TDirError 
        {
        ENoDirError= 0,         ///< No errors found
        EScanMatchingEntry=1,   ///< Two entries pointing to the same cluster chain; Rugged FAT rename/replace artefact
        EScanPartEntry,         ///< Deleted DOS entry and orphaned VFAT ones from the same entryset; Rugged FAT 'file/dir delete' artefact
        };


private:
	CFatMountCB*            iMount;             ///< The owning FAT mount
	
    TPartVFatEntry          iPartEntry;         ///< Storage for a partial VFAT entry set error, see EScanPartEntry
	TMatchingStartCluster   iMatching;          ///< Storage for Matching start cluster error, see EScanMatchingEntry
	
    TDirError               iDirError;          ///< Indicates the error type found also used to indicate if an error has occured
	TUint32                 iHangingClusters;	///< Number of hanging clusters found (and marked as EOF by ScanDrive), at which cluster chain
												///< truncation should take place; Rugged FAT 'file shrinking/expanding' artefact
    TInt                    iDirsChecked;       ///< Count of the number of directories checked
	TInt                    iRecursiveDepth;    ///< Depth of recursion the scan has reached
	RArray<TInt>*           iClusterListArray[KMaxArrayDepth]; ///< Size in bytes of the bit packed FAT	cluster list array used when maximum depth
															   ///< has been reached so that directory may be re-visited. Avoid stack overflow.
    TUint                   iListArrayIndex;    ///< Current position into cluster list array
	TUint32                 iMaxClusters;       ///< Max. amount of clusters on the volume

    RBitVector              iMediaFatBits;      ///< Storage for bit packed FAT read from media 
    RBitVector              iScanFatBits;       ///< Storage for bit packed FAT built up by the scan

    TGenericError           iGenericError;      ///< FS error that is discovered by scanning in any mode  
    TScanDriveMode          iScanDriveMode;     ///< Mode of operation
	};



#endif //SL_SCANDRV_H


