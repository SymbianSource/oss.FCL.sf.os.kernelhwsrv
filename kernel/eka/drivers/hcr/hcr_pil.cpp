// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Hardware Configuration Respoitory Platform Independent Layer (PIL)
//


// -- INCLUDES ----------------------------------------------------------------


#include "hcr_debug.h"

#include <e32def.h>
#include <e32err.h>
#include <e32des8.h>
#include <nkern/nkern.h>
#include <kernel/kernel.h>

#include <e32rom.h>
#include <plat_priv.h>

#include <kernel/kernboot.h>


#include "hcr_hai.h"
#include "hcr_pil.h"


// -- GLOBALS -----------------------------------------------------------------


GLDEF_C HCR::HCRInternal gHCR;

#ifdef HCR_TRACE
GLDEF_C TBuf<81> gTraceBuffer;
#endif


// -- Local functions prototypes
/**
Retrive Repository file address stored in the iHcrFileAddress field of ROM Image header.
If this filed is zero or it is equal with a special value then it keeps the original vaule of 
aRepos parameter and signals it with the retun value.


@param aRepos     		The reference to a repository variable
    
@return	KErrNone 		if successful, the aRepos parameter references to the file in ROM Image.
        KErrNotFound 	if the ROM Image header contains zero or a special value as the repository file address


*/    
LOCAL_C TInt LocateCoreImgRepository(HCR::TRepository*& aRepos);

/**
This method transfer the value of aFileName to ROM Image conform file name string. 
Retrive the variant dependent ROM Root directory address.
Search the file in \sys\bin directory and if it doesn't exists there it try to find it in \sys\Data.


@param 	aRepos     			The reference to a repository variable.
				aFileName			  The name of the new repository file without path. '\0' terminated c-style string.
    
@return	KErrNone 			if successful
        KErrNotFound 		if file not found in \sys\bin or \sys\Data


*/    
LOCAL_C TInt SearchCoreImgRepository(HCR::TRepository* & aRepos, const TText * aFileName);

/**
Scanning a given directory for the given entry name. The entry name can be sub-directory or file.

@param 	aActDir     		Pointer to curretn directory in the ROM Image directory tree
		aFileName			File to be search
		aEntry				If the file found this referenced to proper directory entry
    
@return	 KErrNone		 			if the entry found
         KErrNotFound					if the entry not found
*/    

LOCAL_C TInt SearchEntryInTRomDir(const TRomDir* aActDir, const TPtrC aFileName, TRomEntry* &aEntry);



// -- FUNCTIONS ---------------------------------------------------------------

/**
 Returns 1 when a1 > a2
 Returns -1 when a1 < a2
 Returns 0 when identical.
 */
TInt CompareSSettingIds(const HCR::TSettingId& a1, const HCR::SSettingId& a2)    
{
    // HCR_FUNC("CompareSSettingIds");
    if (a1.iCat > a2.iCat)
        return (1); // HCR_TRACE_RETURN(1);
    if (a1.iCat < a2.iCat)
        return (-1); // HCR_TRACE_RETURN(-1);
    
    // Categories are the same at this point, check keys.
    if (a1.iKey > a2.iKey)
        return (1); // HCR_TRACE_RETURN(1);
    if (a1.iKey < a2.iKey)
        return (-1); // HCR_TRACE_RETURN(-1);
   
    // Both Categories and jeys are the same here.
    return (0); // HCR_TRACE_RETURN(0);
    }


// -- METHODS -----------------------------------------------------------------
//
// HCRInternal

HCR::HCRInternal::HCRInternal()
   : iStatus(EStatConstructed), iVariant(0), iVariantStore(0), iCoreImgStore(0), iOverrideStore(0)
    {
    HCR_FUNC("HCRInternal(Defualt)");
    }

HCR::HCRInternal::HCRInternal(HCR::MVariant* aVar)
   : iVariant(aVar), iVariantStore(0), iCoreImgStore(0), iOverrideStore(0)
    {
    HCR_FUNC("HCRInternal");
    }
    
HCR::HCRInternal::~HCRInternal()
    {
    HCR_FUNC("~HCRInternal");
    }
   
TUint32 HCR::HCRInternal::GetStatus()
    {
    HCR_FUNC("GetStatus");
    return iStatus;
    }
  
    
TInt HCR::HCRInternal::Initialise()
    {
    HCR_FUNC("HCRInternal::Initialise");
    
    TAny* store = 0; 
    TInt err = 0;
	
	// Variant PSL object must exist before PIL initalised.
	if (iVariant == 0) {
 			err = KErrGeneral; goto failed; }

	// Inform the PSL that we are initialising, give them an opportunity to do
	// initialisation work too.
    err = iVariant->Initialise(); 
    if (err != KErrNone)
    	goto failed;
   
    iStatus = EStatVariantInitialised;
    
    // Ask the PSL for the address of the SRepositoryCompiled object. PSL 
    // can return KErrNotSupported & NULL if compiled repository not 
	// used/support by PSL.
    err = iVariant->GetCompiledRepositoryAddress(store);
    if (err == KErrNone)
        {
        if (store == 0) { // Programming error in PSL, ptr/rc mismatch
 			err = KErrArgument; goto failed; }
        	
        iVariantStore = TRepositoryCompiled::New(reinterpret_cast<const HCR::SRepositoryCompiled *>(store));
        if (iVariantStore == 0) { 
			err = KErrNoMemory; goto failed; }

        }
    else if (err != KErrNotSupported)
    	goto failed;       
  
        
    // Ask the PSL if it wants the PIL not to search for the Core Image 
	// SRepositoryFile settings.
    iCoreImgStore = 0;
    if (!iVariant->IgnoreCoreImgRepository())
    	{
    	err = LocateCoreImgRepository(iCoreImgStore);
    	if (err == KErrNone)
     	   {
        	if (iCoreImgStore == 0) {
				err = KErrNoMemory; goto failed; }
        	}
    	else if (err != KErrNotFound)
    		goto failed;
		}       
  
        
    // Ask the PSL for the address of the SRepositoryFile object. PSL 
    // can return KErrNotSupported & NULL if a local media based file 
	// repository is not used/support by PSL.  
    store = 0;
    err = iVariant->GetOverrideRepositoryAddress(store);
    if (err == KErrNone)
        {
        if (store == 0) { // Programming error in PSL, ptr/rc mismatch
 			err = KErrArgument; goto failed; }       
        
        iOverrideStore = TRepositoryFile::New(reinterpret_cast<const HCR::SRepositoryFile *>(store));
        if (iOverrideStore == 0) {
			err = KErrNoMemory; goto failed; }
			
        }
    else if (err != KErrNotSupported)
    	goto failed;       

	iStatus = EStatInitialised;
	
    // Sanity check here to ensure we have atleast one repository to use and run
    // sanity check on their contents to look for ordering issues and duplicates.
	HCR_TRACE3("=== HCR Ready: compiled:%x, coreimg:%x, override:%x", iVariantStore, iCoreImgStore, iOverrideStore);
    if ((iVariantStore == 0) && (iCoreImgStore == 0) && (iOverrideStore == 0)) {
 		err = KErrArgument; goto failed; }


#ifdef _DEBUG
	err = CheckIntegrity();
	if (err != KErrNone)
		goto failed;	
#endif

	iStatus = EStatReady;
	return KErrNone;

failed:
    iStatus = (iStatus & EStatMinorMask) | EStatFailed;
	HCR_LOG_RETURN(err);
    }


TInt HCR::HCRInternal::SwitchRepository(const TText * aFileName, const TReposId aId)
	{
	HCR_FUNC("HCRInternal::SwitchRepository");
	
	TInt retVal = KErrNone;
	TRepository* store = NULL;

	if( aFileName != NULL)
		{
		retVal = SearchCoreImgRepository(store, aFileName);
		HCR_TRACE2("--- SearchCoreImgRepository()->%d (0x%08x)", retVal, retVal);
		}
		
	if( retVal == KErrNone )
		{
		switch(aId)
			{
			case ECoreRepos:
			    HCR_TRACE0("--- ECoreRepos");
				if( iCoreImgStore )
					{
					delete iCoreImgStore;
					}
				iCoreImgStore = store;
				break;
				
			case EOverrideRepos:
			    HCR_TRACE0("--- EOverrideRepos");
				if( iCoreImgStore )
    				{
					delete iOverrideStore;
					}
				iOverrideStore = store;
				break;
		
			default:
			    HCR_TRACE0("--- default:");
				retVal = KErrNotSupported;
				break;		
			}
		}

	HCR_LOG_RETURN(retVal);
    }

TInt HCR::HCRInternal::CheckIntegrity()
	{
	HCR_FUNC("HCRInternal::CheckIntegrity");
	
	TInt err = KErrNone;
	if (iVariantStore)
		{
		err = iVariantStore->CheckIntegrity();
		if (err != KErrNone)
			HCR_LOGMSG_RETURN("HCR iVariantStore failed integrity check", err);
		}

	if (iCoreImgStore)
		{
		err = iCoreImgStore->CheckIntegrity();
		if (err != KErrNone)
			HCR_LOGMSG_RETURN("HCR iCoreImgStore failed integrity check", err);
		}	
	
	if (iOverrideStore)
		{
		err = iOverrideStore->CheckIntegrity();
		if (err != KErrNone)
			HCR_LOGMSG_RETURN("HCR iOverrideStore failed integrity check", err);
		}

	HCR_TRACE0("=== HCR Repository integrity checks PASSED!");
	return KErrNone;	
	}

TInt HCR::HCRInternal::FindSetting(const TSettingId& aId, TSettingType aType, TSettingRef& aSetting)
    {
    HCR_FUNC("HCRInternal::FindSetting");
    TInt err = 0;
    TBool found = EFalse;
    
    HCR_TRACE3("--- Repository state: %x, %x, %x", iOverrideStore, iCoreImgStore, iVariantStore);
    
    if (iOverrideStore && 
        ((err = iOverrideStore->FindSetting(aId, aSetting)) == KErrNone))
        found = ETrue;
    if ((err != KErrNone) && (err != KErrNotFound))
        HCR_LOG_RETURN(err);
        
    if (!found &&
        iCoreImgStore &&
        ((err = iCoreImgStore->FindSetting(aId, aSetting)) == KErrNone))
        found = ETrue;
    if ((err != KErrNone) && (err != KErrNotFound))
        HCR_LOG_RETURN(err);

    if (!found &&
        iVariantStore &&
        ((err = iVariantStore->FindSetting(aId, aSetting)) == KErrNone))
        found = ETrue;
        
    if ((err != KErrNone) && (err != KErrNotFound))
        HCR_LOG_RETURN(err);

    HCR_TRACE3("--- Search results: %d, %d, %x", found, err, aSetting.iSet);
    
    if (!found || (aSetting.iSet == 0))
        HCR_LOG_RETURN(KErrNotFound);

    // Setting found at this point in the function
    //
    
    TSettingType type=static_cast<TSettingType>(aSetting.iRep->GetType(aSetting)); 
    if (type & ~aType)
        HCR_LOG_RETURN(KErrArgument); // Wrong setting type
    
    HCR_TRACE3("--- Setting found! ID: (%d,%d) Type: %d", aId.iCat, aId.iKey, type);
    
    return KErrNone;
    }
    
TInt HCR::HCRInternal::FindWordSettings(TInt /*aNum*/, const TSettingId* /*aIds*/, 
                        TInt32* /*aValues*/, TSettingType* /*aTypes*/, TInt* /*aErrors*/)
    {
    HCR_FUNC("HCRInternal::FindWordSettings");

    return KErrNotSupported;
    }



// -- METHODS -----------------------------------------------------------------
//
// TRepository


TBool HCR::TRepository::IsWordValue(const TSettingRef& aRef)
    {
    HCR_FUNC("TRepository::IsWordValue");
    return ((aRef.iSet->iType & KMaskWordTypes) != 0);
    }

TBool HCR::TRepository::IsLargeValue(const TSettingRef& aRef)
    {
    HCR_FUNC("TRepository::IsLargeValue");
    return ((aRef.iSet->iType & KMaskLargeTypes) != 0);
    }

void HCR::TRepository::GetId(const TSettingRef& aRef, TCategoryUid& aCat, TElementId& aKey)
    {
    HCR_FUNC("TRepository::GetId1");
    aCat = aRef.iSet->iId.iCat;
    aKey = aRef.iSet->iId.iKey;
    }

void HCR::TRepository::GetId(const TSettingRef& aRef, SSettingId& aId)
    {
    HCR_FUNC("TRepository::GetId2");
    aId = aRef.iSet->iId;
    }

TInt32 HCR::TRepository::GetType(const TSettingRef& aRef)
    {
    HCR_FUNC("TRepository::GetType");
    return (aRef.iSet->iType);
    }

TUint16 HCR::TRepository::GetLength(const TSettingRef& aRef)
    {
    HCR_FUNC("TRepository::GetLength");
    
	// Assume large value, will be caught when value retreived if not correct.
	// Saves some CPU cycles...
	// if (IsLargeValue(aRef))
    return (aRef.iSet->iLen);
    }


// -- METHODS -----------------------------------------------------------------
//
// TRepositoryCompiled


HCR::TRepository* HCR::TRepositoryCompiled::New(const SRepositoryCompiled* aRepos)
    {
    HCR_FUNC("TRepositoryCompiled::New");
    return new TRepositoryCompiled(aRepos);
    }

HCR::TRepositoryCompiled::TRepositoryCompiled(const SRepositoryCompiled* aRepos)
 : iRepos(aRepos)
    {
    HCR_FUNC("TRepositoryCompiled");
    }
    
HCR::TRepositoryCompiled::~TRepositoryCompiled()
    {
    HCR_FUNC("~TRepositoryCompiled");
    }
        
TInt HCR::TRepositoryCompiled::Initialise()
    {
    HCR_FUNC("TRepositoryCompiled::Initialise");
    return KErrNone;
    }
    
TInt HCR::TRepositoryCompiled::CheckIntegrity()
    {
    HCR_FUNC("TRepositoryCompiled::CheckIntegrity");
    
	if (iRepos->iOrderedSettingList == 0)
        HCR_LOGMSG_RETURN("Compiled Repository header missing setting array list", KErrNotFound);
    
	HCR_TRACE2("Compiled repository 0x%x contains %05d entries", iRepos, iRepos->iHdr->iNumSettings);

    SSettingC* arr = iRepos->iOrderedSettingList;
    TSettingId prev(0,0);
    TInt rc=0;
    for (int i=0; i < iRepos->iHdr->iNumSettings; i++, arr++)
    	{
    	HCR_TRACE3("Checking entry %05d - (0x%x,%d)", i, arr->iName.iId.iCat,  arr->iName.iId.iKey);
    	rc = CompareSSettingIds(prev, arr->iName.iId);
		// Check for duplicates that reside next to each other
    	if ((i > 0) && (rc == 0))
    		HCR_TRACE_RETURN (KErrAlreadyExists);
    	// Check that the entries are in ascending order	
    	if (rc != -1)
    		HCR_TRACE_RETURN (KErrCorrupt);
    	prev = arr->iName.iId;
		}
    return KErrNone; 
    }
    
TInt HCR::TRepositoryCompiled::FindSetting(const TSettingId& aId, TSettingRef& aSetting)
    {
    HCR_FUNC("TRepositoryCompiled::FindSetting");
    
    if ((iRepos->iHdr->iNumSettings == 0) || 
        (iRepos->iOrderedSettingList == 0))
        HCR_LOG_RETURN(KErrNotFound);
    
    SSettingC* arr = iRepos->iOrderedSettingList;
    int low = 0;
    int high = iRepos->iHdr->iNumSettings-1;
    int mid;
    int com;
    
    while (low<=high)
        {
        mid = (low+high) >> 1;
        com = CompareSSettingIds(aId, arr[mid].iName.iId);
        if (com < 0)
            high = mid-1;
        else if (com > 0)
            low = mid+1;
        else
            {
            aSetting.iRep = this;
            aSetting.iSet = &((arr[mid]).iName);
            return KErrNone;
            }    
        } 
        
    aSetting.iRep = 0;
	aSetting.iSet = 0; 
    return KErrNotFound;
    }
    
TInt HCR::TRepositoryCompiled::GetValue(const TSettingRef& aRef, UValueWord& aValue)
    {
    HCR_FUNC("TRepositoryCompiled::GetValue");
    if (!IsWordValue(aRef))
        HCR_LOG_RETURN(KErrArgument);
        
	SSettingC* sptr = (SSettingC*)(aRef.iSet);
    aValue = sptr->iValue.iLit;
	return KErrNone;
    }

TInt HCR::TRepositoryCompiled::GetLargeValue(const TSettingRef& aRef, UValueLarge& aValue)
    {
    HCR_FUNC("TRepositoryCompiled::GetLargeValue");
    if (!IsLargeValue(aRef))
        HCR_LOG_RETURN(KErrArgument);

	SSettingC* sptr = (SSettingC*)(aRef.iSet);
    aValue = sptr->iValue.iPtr;
    return KErrNone;
    }


// -- METHODS -----------------------------------------------------------------
//
// TRepositoryFile


HCR::TRepository* HCR::TRepositoryFile::New(const SRepositoryFile* aRepos)
    {
    HCR_FUNC("TRepositoryFile::New");
    return new TRepositoryFile(aRepos);
    }

HCR::TRepositoryFile::TRepositoryFile(const SRepositoryFile* aRepos)
 : iRepos(aRepos)
    {
    HCR_FUNC("TRepositoryFile");
    }

HCR::TRepositoryFile::~TRepositoryFile()
    {
    HCR_FUNC("~TRepositoryFile");
    
    }
        
TInt HCR::TRepositoryFile::Initialise()
    {
    HCR_FUNC("TRepositoryFile::Initialise");
    
	
    return KErrNone;
    }
    
TInt HCR::TRepositoryFile::CheckIntegrity()
    {
    HCR_FUNC("TRepositoryFile::CheckIntegrity");
    
    
	if ((*((TUint32*)&(iRepos->iHdr)) != 0x66524348) || 
		(iRepos->iHdr.iFormatVersion != 0x0001))
        HCR_LOGMSG_RETURN("File Repository header describes an unsupported repository type", KErrCorrupt);
    
    HCR_TRACE2("File repository 0x%x contains %05d entries", iRepos, iRepos->iHdr.iNumSettings);
    
    SSettingF* arr = (SSettingF*) (iRepos+1);
    TSettingId prev(0,0);
    TInt rc=0;
    for (int i=0; i < iRepos->iHdr.iNumSettings; i++, arr++)
    	{
    	HCR_TRACE3("Checking entry %05d - (0x%x,%d)", i, arr->iName.iId.iCat,  arr->iName.iId.iKey);
    	rc = CompareSSettingIds(prev, arr->iName.iId);
	   	// Check for duplicates that reside next to each other
    	if ((i > 0) && (rc == 0))
    		HCR_TRACE_RETURN (KErrAlreadyExists);
    	// Check that the entries are in ascending order	
    	if (rc != -1)
    		HCR_TRACE_RETURN (KErrCorrupt);
    	prev = arr->iName.iId;
		}
    return KErrNone; 
    }

TInt HCR::TRepositoryFile::FindSetting(const TSettingId& aId, TSettingRef& aSetting)
    {
    HCR_FUNC("TRepositoryFile::FindSetting");
    
    if (iRepos->iHdr.iNumSettings == 0)
        HCR_LOG_RETURN(KErrNotFound);
    
    SSettingF* arr = (SSettingF*) (iRepos+1);
    int low = 0;
    int high = iRepos->iHdr.iNumSettings-1;
    int mid;
    int com;
    
    while (low<=high)
        {
        mid = (low+high) >> 1;
        com = CompareSSettingIds(aId, arr[mid].iName.iId);
        if (com < 0)
            high = mid-1;
        else if (com > 0)
            low = mid+1;
        else
            {
            aSetting.iRep = this;
            aSetting.iSet = &((arr[mid]).iName);
            return KErrNone;
            }    
        } 
        
    aSetting.iRep = 0;
	aSetting.iSet = 0; 
    return KErrNotFound;
    }

TInt HCR::TRepositoryFile::GetValue(const TSettingRef& aRef, UValueWord& aValue)
    {
    HCR_FUNC("TRepositoryFile::GetValue");

    if (!IsWordValue(aRef))
        HCR_LOG_RETURN(KErrArgument);
        
	SSettingF* sptr = (SSettingF*)(aRef.iSet);
    aValue = sptr->iValue.iLit;
	return KErrNone;
    }

TInt HCR::TRepositoryFile::GetLargeValue(const TSettingRef& aRef, UValueLarge& aValue)
    {
    HCR_FUNC("TRepositoryFile::GetLargeValue");

    if (!IsLargeValue(aRef))
        HCR_LOG_RETURN(KErrArgument);

	SSettingF* sptr = (SSettingF*)(aRef.iSet);
	TRepositoryFile *rptr = (TRepositoryFile *)(aRef.iRep);
	
    aValue.iData = (TUint8*) rptr->iRepos;
	aValue.iData += rptr->iRepos->iLSDfirstByteOffset+sptr->iValue.iOffset;
    return KErrNone;
    }



// -- FUNCTIONS ---------------------------------------------------------------

#ifndef HCRTEST_NO_KEXT_ENTRY_POINT
#ifndef __WINS__
DECLARE_EXTENSION_WITH_PRIORITY(KExtensionMaximumPriority)
#else
DECLARE_STANDARD_EXTENSION()
#endif // __WINS__
	{
	HCR_FUNC("InitExtension");

	HCR::MVariant* varPtr = CreateHCRVariant();
	if (varPtr==0)
		HCR_LOG_RETURN(KErrNoMemory);

	new(&gHCR) HCR::HCRInternal(varPtr);

	TInt err = HCRSingleton->Initialise();

	if (err != KErrNone)
		HCR_LOG_RETURN(err);

	return err;
	}
#endif // HCRTEST_NO_KEXT_ENTRY_POINT

// -- Implementation of local functions

TInt SearchEntryInTRomDir(const TRomDir* aActDir, const TPtrC aFileName, TRomEntry* &aEntry)
	{
	HCR_FUNC("SearchEntryInTRomDir");
	TInt retVal = KErrNotFound;
	HCR_TRACE2("--- aFileName: %S (%d)", &aFileName, aFileName.Length());
	
	if( aActDir == 0)
		{
		HCR_TRACE_RETURN(retVal);
		}
	
	TInt dirSize = aActDir->iSize;
	aEntry = (TRomEntry*)&aActDir->iEntry;
	HCR_TRACE3("--- dirSize: 0x%08x (%d), aEntry: 0x%08x", dirSize, dirSize, aEntry);
	
	TBool found = EFalse;
	while( !found )
		{
		HCR_TRACE0("Begin of loop...");
		TInt nameLength = (aEntry->iNameLength)<<1;
		
		HCR_HEX_DUMP_ABS((TUint8 *)aEntry, sizeof(TRomEntry)+(nameLength - 2) );
		const TText* entryName = &aEntry->iName[0];
		HCR_TRACE1("--- entryName length: %d", nameLength);
		TBuf<512> newEntryName( nameLength);
		for( TInt i = 0; i != nameLength; ++i)
			{
			newEntryName[i] = (unsigned char)('A' <= entryName[i] && 'Z' >= entryName[i]? entryName[i]+('a'-'A'): entryName[i]);
			}		
				
		HCR_TRACE6("--- aFileName: %S (%d/%d), newEntryName: %S (%d/%d)", &aFileName, aFileName.Length(), aFileName.Size(), &newEntryName, newEntryName.Length(), newEntryName.Size());
		TInt r = aFileName.Compare(newEntryName);
		HCR_TRACE1("--- result of CompareFileNames: 0x%08x", r);
		
		if ( r == 0)
			{
			found = ETrue;
			HCR_TRACE1("--- aEntry: 0x%08x", aEntry);
			}
		else
			{
		
			TInt entrySize = sizeof(TRomEntry) + (nameLength - 2);
			HCR_TRACE2("--- entrySize: 0x%08x, (%d)", entrySize, entrySize);
			
			// The entrySize must be aligned to 4 bytes boundary
			entrySize = ((entrySize&0x03) == 0 ? entrySize : ((entrySize&0xfffffffc) + 4));
			HCR_TRACE2("--- entrySize: 0x%08x, (%d)", entrySize, entrySize);
			
			aEntry = (TRomEntry*)((char *)aEntry + entrySize);
			dirSize -= entrySize;
			HCR_TRACE2("--- aEntry: 0x%08x, dirSize:%d", aEntry, dirSize);
			if( dirSize <= 0)
				{
				break;
				}
			}
		}
		
	if( found)
		{
		retVal = KErrNone;
		}
		
	HCR_TRACE_RETURN(retVal);		
	}
	
TInt SearchCoreImgRepository(HCR::TRepository*& aRepos, const TText * aFileName)
    {
    HCR_FUNC("SearchCoreImgRepository(TRepository*& aRepos, TText & aFileName)");
    
    TInt retVal = KErrNotFound;

	// Convert aFileName to directory entry style Unicode 
	const TText* p = aFileName;
	
	if( *p == 0)
		{
		// Empty file name -> return with KErrNotFound!
		HCR_TRACE_RETURN(retVal);
		}
		
	while( *(++p)) {};					// Search the end of file name string.
	TInt nameLen=(TInt)(p-aFileName);	
	
	HCR_TRACE2("--- aFileName: %s (%d)", aFileName, nameLen );
	
	TBuf<256> origFileName;
	origFileName.Append((const TText*)aFileName, nameLen);
	HCR_TRACE2("--- origFileName: %S (%d)", &origFileName, origFileName.Length());
	
	
	TBuf<512> fileNameBuf;
	for( TInt i = 0; i != nameLen; ++i)
		{
		fileNameBuf.Append( 'A' <= origFileName[i] && 'Z' >= origFileName[i]? origFileName[i]+('a'-'A'): origFileName[i]);
		fileNameBuf.Append(TChar(0));
		}

	TPtrC fileName(fileNameBuf);
	HCR_TRACE3("--- fileName: %S (%d/%d)", &fileName, fileName.Length(), fileName.Size());

    // Locate ROM Root directory
    TSuperPage& superpage = Kern::SuperPage();
   	TRomRootDirectoryList* romRootDirAddress = (TRomRootDirectoryList*)superpage.iRootDirList;
 
    HCR_TRACE3("--- Superpage: 0x%08x, ROM root dir list: 0x%08x (Num of root dirs:%d)", &superpage, romRootDirAddress, romRootDirAddress->iNumRootDirs );

	// Search the root directory which is match to the current hardware variant
    TUint hardwareVariant 	   = superpage.iActiveVariant;
    TInt variantIndex;
    TRootDirInfo* rootDirInfo = 0;
    
    for(variantIndex = 0; variantIndex < romRootDirAddress->iNumRootDirs; ++variantIndex )
	    {
	    HCR_TRACE3("--- variantIndex:%d, current hardware variant: 0x%08x, root dir hardware variant:0x%08x", variantIndex, hardwareVariant, romRootDirAddress->iRootDir[variantIndex].iHardwareVariant);
	    
    	if( romRootDirAddress->iRootDir[variantIndex].iHardwareVariant == hardwareVariant)
	    	{
	    	rootDirInfo = &romRootDirAddress->iRootDir[variantIndex]; 
	    	break;
	    	}
	    }
    
    if( rootDirInfo == 0 )
	    {
	    // Not found root directory for this hardware variant
	    HCR_TRACE_RETURN(retVal);
	    }
	    
	TRomDir* romDir = (TRomDir*)rootDirInfo->iAddressLin;

	HCR_TRACE3("--- romDir: 0x%08x (files:0x%08x, entries:0x%08x)", romDir, romDir->FileCount(), romDir->EntryCount() );
	TRomEntry* entry = (TRomEntry*)&romDir->iEntry;
	
	// We are searching in \sys\bin\ and \sys\Data\ directory only	
	TPtrC level1DirName((const TText*)"s\0y\0s\0", 6);		// Unicode, because the entry names are unicode too.
	TPtrC level2Dir1Name((const TText*)"b\0i\0n\0", 6);
	TPtrC level2Dir2Name((const TText*)"d\0a\0t\0a\0", 8);		// Originally \sys\Data however we search all entry in lower case
	
	TInt r = SearchEntryInTRomDir(romDir, level1DirName, entry);
	HCR_TRACE1("--- result of SearchEntryInTRomDir: 0x%08x", r);

	if( r == KErrNone)
		{
		// \sys directory found.
		romDir = (TRomDir*)entry->iAddressLin;
		HCR_TRACE1("--- romDir: 0x%08x ", romDir);
		
		TRomDir* parentDir = romDir;
		// Search in \sys\bin directory
		r = SearchEntryInTRomDir(romDir, level2Dir1Name, entry);
	
		HCR_TRACE1("--- result of SearchEntryInTRomDir: 0x%08x", r);
		if( r == KErrNone)
			{
			// \sys\bin directory found
			romDir = (TRomDir*)entry->iAddressLin;
			HCR_TRACE1("--- romDir: 0x%08x ", romDir);
			// Search the repository file
			r = SearchEntryInTRomDir(romDir, fileName, entry);
			
			HCR_TRACE1("--- result of SearchEntryInTRomDir: 0x%08x", r);
			if( r == KErrNone)
				{
				// Repository file found
				retVal = KErrNone;				
				HCR_TRACE1("--- Repository address: 0x%08x ", entry->iAddressLin);
				aRepos = HCR::TRepositoryFile::New(reinterpret_cast<const HCR::SRepositoryFile *>(entry->iAddressLin));	
				if (aRepos == NULL)
                        retVal = KErrNoMemory;					
                        
                HCR_TRACE_RETURN(retVal);
				}
			}

		// \sys\bin directory or repository file in \sys\bin directory not found.    
		// Search \sys\Data directory
		romDir = parentDir;
		r = SearchEntryInTRomDir(romDir, level2Dir2Name, entry);
		HCR_TRACE1("--- result of SearchEntryInTRomDir: 0x%08x", r);
		if( r == KErrNone)
			{
			// \sys\Data directory found
			romDir = (TRomDir*)entry->iAddressLin;
			HCR_TRACE1("--- romDir: 0x%08x ", romDir);
			
			// Search repository file
			r = SearchEntryInTRomDir(romDir, fileName, entry);
			
			HCR_TRACE1("--- result of SearchEntryInTRomDir: 0x%08x", r);
			if( r == KErrNone)
				{
				// Repository file found    
				retVal = KErrNone;				
				HCR_TRACE1("--- Repository address: 0x%08x ", entry->iAddressLin);
				aRepos = HCR::TRepositoryFile::New(reinterpret_cast<const HCR::SRepositoryFile *>(entry->iAddressLin));
				if (aRepos == NULL)
                    retVal = KErrNoMemory;					
				}
			}
		}
	
    HCR_TRACE_RETURN(retVal);
    }

TInt LocateCoreImgRepository(HCR::TRepository*& aRepos)
    {
    HCR_FUNC("LocateCoreImgRepository");

#ifdef __WINS__
    aRepos = 0;                     // To avoid warning on WINS
    return KErrNotFound;
#else

#ifdef HCRTEST_COREIMG_DONTUSE_ROMHDR

	const TText8* hcrfile = (const TText8*) "hcr.dat";
	TInt retVal = SearchCoreImgRepository(aRepos, hcrfile);
	if (retVal != KErrNone)
		return retVal;
	
#else

	const TRomHeader& romHeader = Epoc::RomHeader(); 	// 0x80000000;
	HCR_TRACE2("--- ROM Header: 0x%08x, HCR file address: 0x%08x", &romHeader, romHeader.iHcrFileAddress);
	
	if(romHeader.iHcrFileAddress != 0)
			{
			aRepos = HCR::TRepositoryFile::New(reinterpret_cast<const HCR::SRepositoryFile *>(romHeader.iHcrFileAddress));
			if (aRepos == 0)
				return KErrNoMemory;
			}
	else
		return KErrNotFound;
		
#endif // HCRTEST_COREIMG_DONTUSE_ROMHDR
	return KErrNone;
#endif // __WINS__
    }
	
