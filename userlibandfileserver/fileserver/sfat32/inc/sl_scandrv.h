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

const TInt KMaxMatchingEntries		= 2;        ///< Maximum number of matching directory entries scan drive can fix. Any more indicates a fault in the file system
const TInt KMaxArrayDepth			= 6;        ///< Maximum array depth for cluster storage when KMaxScanDepth is reached

/**
Data structure used to store the location of a partial VFat entry
*/
struct TPartVFatEntry
	{
	TEntryPos    iEntryPos; ///< The position of the partial VFat entry
	TFatDirEntry iEntry;    ///< The Dos entry The VFat entries belong with
	};


/**
Data structure used to store the locations of entries with matching
start cluster numbers. 
*/
struct TMatchingStartCluster
	{
	TEntryPos   iEntries[KMaxMatchingEntries]; ///< The positions of the matching entries
	TInt        iCount;         ///< Count of matching entries
	TInt        iStartCluster;  ///< The matching cluster number found in more than one entry
	};

/**
Scan drive class performs scan drive functionality on all types
of fat volume.
*/
class CScanDrive : public CBase
	{
public:
	/**
	Error type found by scan drive, only a single error should occur in 
	any scan of the volume
	*/
	enum TDirError{EScanMatchingEntry=1,EScanPartEntry, ETruncation};

public:

	~CScanDrive();
	static CScanDrive* NewL(CFatMountCB* aMount);
	void ConstructL(CFatMountCB* aMount);

	TInt  StartL();
    TBool ProblemsDiscovered() const;  


private:
#if defined(DEBUG_SCANDRIVE)
	void PrintErrors();
	void CompareFatsL() const;
#endif
	void FixupDirErrorL();

	void ReadMediaFatL();
    void DoParseFatL();
    void DoParseFat32L();
    void DoParseFat32Buf(const TPtrC8& aBuf, TUint32& aCurrFatEntry);

	TBool AlreadyUsedL(TUint aCluster) const;
	void SetUsedL(TUint aCluster);

	TUint32 ReadFatL(TInt aClusterNum) const;
	void FindSameStartClusterL();
	TInt FindStartClusterL(TInt aDirCluster);
	void CheckDirStructureL();
	void CheckDirL(TInt aCluster);
	void ProcessEntryL(const TFatDirEntry& aEntry);
	TInt CheckEntryClusterL(const TFatDirEntry& aEntry, const TEntryPos& aEntryPos);
	void WriteClusterChainL(TInt aCluster,TUint aSizeInBytes);
	TBool MoveToVFatEndL(TEntryPos& aPos,TFatDirEntry& aEntry,TInt& aDirLength);
	TBool IsValidVFatEntry(const TFatDirEntry& aEntry,TInt prevNum)const;
	TBool IsDosEntry(const TFatDirEntry& aEntry)const;
	void AddPartialVFatL(const TEntryPos& aStartPos, const TFatDirEntry& aEntry);
	TBool AddMatchingEntryL(const TEntryPos& aEntryPos);
	TInt GetReservedidL(const TEntryPos aVFatPos);
	void WriteNewFatsL();
	void FixPartEntryL();
	void FixMatchingEntryL();
	void MovePastEntriesL(TEntryPos& aEntryPos,TFatDirEntry& aEntry,TInt aToMove,TInt& aDirEntries);
	void AddToClusterListL(TInt aCluster);
	inline TBool AlreadyExistsL(TInt aCluster)const;
	inline TBool IsEndOfRootDir(const TEntryPos& aPos)const;
	inline TBool IsEofF(TInt aVal)const;
	inline TBool IsDirError()const;
	void MoveToNextEntryL(TEntryPos& aPos);
	void ReadDirEntryL(const TEntryPos& aPos,TFatDirEntry& aDirEntry);

    void IndicateErrorsFound();

private:
	CFatMountCB*            iMount;             ///< The owning Fat mount
	TPartVFatEntry          iPartEntry;         ///< Storage for a partial VFat entry set error
	TMatchingStartCluster   iMatching;          ///< Storage for Matching start cluster error
	TDirError               iDirError;          ///< Indicates the error tpye found also used to indicate if an error has occured
	TInt                    iDirsChecked;       ///< Count of the number of directories checked
	TInt                    iRecursiveDepth;    ///< Depth of recursion the scan has reached
	RArray<TInt>*           iClusterListArray[KMaxArrayDepth]; ///< Size in bytes of the bit packed Fat	Cluster list array used when maximum depth has been reached so that directory may be re-visited. Avoid stack overflow
	TInt                    iListArrayIndex;    ///< Current position into cluster list array
	TInt                    iTruncationCluster; ///< Cluster at which cluster chain truncation should take place, used for truncation errors
	
    TBool                   iFoundProblems; ///< if ETrue after finish, it means that there where some problems FS structure and they were probably fixed;
    RBitVector              iMediaFatBits;  ///< Storage for bit packed Fat read from media 
    RBitVector              iScanFatBits;   ///< Storage for bit packed Fat built up by the scan

	};



#endif //SL_SCANDRV_H


