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
#include <e32cmn.h>

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


// -- WINS Specific ----------------------------------------------------------

#ifdef __WINS__

// Set to ensure Rom Hdr dependency does not break compilation in 
// LocateCoreImgRepository() at the end of this file.
// Undef incase it is set in MMP file, avoids compiler warning.
//
#undef HCRTEST_COREIMG_DONTUSE_ROMHDR
#define HCRTEST_COREIMG_DONTUSE_ROMHDR

#endif

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

#ifdef __EPOC32__
TBool ROMAddressIsInUnpagedSection(const TLinAddr address)
	{
    HCR_FUNC("ROMAddressIsInUnpagedSection");
	
	const TRomHeader& romHdr = Epoc::RomHeader();
	TLinAddr romBase = romHdr.iRomBase;

	HCR_TRACE1("--- address to check if in unpaged ROM section = 0x%8x", address);
	HCR_TRACE2("--- iRomSize (0x%8x), iPageableRomStart (0x%8x), ", romHdr.iRomSize, romHdr.iPageableRomStart);

	if ((address < romBase) || (romBase > romBase+romHdr.iRomSize))
		return EFalse;
	if (romHdr.iPageableRomStart == 0)
		return ETrue;
	if (address < romBase+romHdr.iPageableRomStart)
		return ETrue;
	return EFalse;
	}
#endif


TInt CompareByCategory(const HCR::TCategoryUid aCatId, const HCR::SSettingId& aSetId)    
    {
    //HCR_FUNC("CompareByCategory");
    if (aCatId > aSetId.iCat)
        return (1); // HCR_TRACE_RETURN(1);
    if (aCatId < aSetId.iCat)
        return (-1); // HCR_TRACE_RETURN(-1);
    
    // Both Categories and jeys are the same here.
    return (0); 
    }

/*
 * SafeArray TSa class object destructor. It delets the allocated in the heap
 * memory and set the instance pointer to NULL. See also TSa class definition
 * in hcr_pil.h.
 */
template<typename T>
    HCR::TSa<T>::~TSa()
    {
    delete[] iSa;
    iSa = NULL;
    }

/**
 * operator=() changes the memory ownership by   
 * reinitiazing SafeArray class object with the address to   
 * already allocated array.
 */
template<typename T>
   HCR::TSa<T>& HCR::TSa<T>::operator=(T* aP)
    {
    delete[] iSa;
    iSa = aP; 
    return (*this);
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
    
    if (iVariant)
		{
		delete iVariant;
    	iVariant =0;
    	}
    if (iVariantStore)
		{
		delete iVariantStore;
    	iVariantStore =0;
    	}
    if (iCoreImgStore)
		{
		delete iCoreImgStore;
    	iCoreImgStore =0;
    	}
    if (iOverrideStore)
		{
		delete iOverrideStore;
    	iOverrideStore =0;
    	}
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
	HCR_TRACE_RETURN(err);
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
					NKern::ThreadEnterCS();
					delete iCoreImgStore;
					NKern::ThreadLeaveCS();
					}
				iCoreImgStore = store;
				break;
				
			case EOverrideRepos:
			    HCR_TRACE0("--- EOverrideRepos");
				if( iOverrideStore )
    				{
	    			NKern::ThreadEnterCS();
					delete iOverrideStore;
					NKern::ThreadLeaveCS();
					}
				iOverrideStore = store;
				break;
		
			default:
			    HCR_TRACE0("--- default:");
				retVal = KErrNotSupported;
				break;		
			}
		}

	HCR_TRACE_RETURN(retVal);
    }

TInt HCR::HCRInternal::CheckIntegrity()
	{
	HCR_FUNC("HCRInternal::CheckIntegrity");
	
	TInt err = KErrNone;
	if (iVariantStore)
		{
		err = iVariantStore->CheckIntegrity();
		if (err != KErrNone)
			HCR_TRACEMSG_RETURN("HCR iVariantStore failed integrity check", err);
		}

	if (iCoreImgStore)
		{
		err = iCoreImgStore->CheckIntegrity();
		if (err != KErrNone)
			HCR_TRACEMSG_RETURN("HCR iCoreImgStore failed integrity check", err);
		}	
	
	if (iOverrideStore)
		{
		err = iOverrideStore->CheckIntegrity();
		if (err != KErrNone)
			HCR_TRACEMSG_RETURN("HCR iOverrideStore failed integrity check", err);
		}

	HCR_TRACE0("=== HCR Repository integrity checks PASSED!");
	return KErrNone;	
	}


TInt HCR::HCRInternal::FindSetting(const TSettingId& aId, TSettingType aType, 
        TSettingRef& aSetting)
    {
    HCR_FUNC("HCRInternal::FindSetting");
    TInt err = KErrNone;
    TBool found = EFalse;
    
    HCR_TRACE3("--- Repository state: %x, %x, %x", iOverrideStore, iCoreImgStore, iVariantStore);
    
    if (iOverrideStore && 
        ((err = iOverrideStore->FindSetting(aId, aSetting)) == KErrNone))
        found = ETrue;
    __NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);
        
    if (!found &&
        iCoreImgStore &&
        ((err = iCoreImgStore->FindSetting(aId, aSetting)) == KErrNone))
        found = ETrue;
    __NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);

    if (!found &&
        iVariantStore &&
        ((err = iVariantStore->FindSetting(aId, aSetting)) == KErrNone))
        found = ETrue;
    __NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);

    HCR_TRACE3("--- Search results: %d, %d, %x", found, err, aSetting.iSet);
    
    if (!found)
        HCR_TRACE_RETURN(KErrNotFound);

    // aSetting should now point to the found setting
    __NK_ASSERT_DEBUG(aSetting.iSet != 0);

    // Setting found at this point in the function
    //
    
    TSettingType type=static_cast<TSettingType>(aSetting.iRep->GetType(aSetting)); 
    if (type & ~aType)
        HCR_TRACE_RETURN(KErrArgument); // Wrong setting type
    
    HCR_TRACE3("--- Setting found! ID: (%d,%d) Type: %d", aId.iCat, aId.iKey, type);
    
    return err;
    }


TInt HCR::HCRInternal::FindSettingWithType(const TSettingId& aId, TSettingType& aType, 
      TSettingRef& aSetting)
    {
    HCR_FUNC("HCRInternal::FindSettingWithType");
    TInt err = KErrNone;
    TBool found = EFalse;
    
    HCR_TRACE3("--- Repository state: %x, %x, %x", iOverrideStore, iCoreImgStore, iVariantStore);
    
    if (iOverrideStore && 
        ((err = iOverrideStore->FindSetting(aId, aSetting)) == KErrNone))
        found = ETrue;
    __NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);

    if (!found &&
        iCoreImgStore &&
        ((err = iCoreImgStore->FindSetting(aId, aSetting)) == KErrNone))
        found = ETrue;
    __NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);

    if (!found &&
        iVariantStore &&
        ((err = iVariantStore->FindSetting(aId, aSetting)) == KErrNone))
        found = ETrue;
    __NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);

    HCR_TRACE3("--- Search results: %d, %d, %x", found, err, aSetting.iSet);
    
    if (!found)
        {
        aType = ETypeUndefined;
        HCR_TRACE_RETURN(KErrNotFound);
        }

    // aSetting should now point to the found setting
    __NK_ASSERT_DEBUG(aSetting.iSet != 0);

    // Setting found at this point in the function
    //
    
    aType=static_cast<TSettingType>(aSetting.iRep->GetType(aSetting)); 
    
    HCR_TRACE3("--- Setting found! ID: (%d,%d) Type: %d", aId.iCat, aId.iKey, aType);
    
    return err;
    }


TInt HCR::HCRInternal::GetWordSettings(TInt aNum, const SSettingId aIds[], 
        TInt32 aValues[], TSettingType aTypes[], TInt aErrors[])
    {
    HCR_FUNC("++ HCRInternal::GetWordSettings");
    HCR_TRACE3("--- Repository state: %x, %x, %x", iOverrideStore, iCoreImgStore, iVariantStore);
    
    if(aNum <= 0 || aIds == NULL || aErrors == NULL)
        HCR_TRACE_RETURN(KErrArgument);
    
    TInt err = 0;
    //If the user only supplies a single setting then there is no reasons to 
    //continue with multiple searach and it should be limited by internal 
    //invocation of FindSettingWithType.
    if(aNum == 1)
        {
        TSettingRef sref(0,0);
		TSettingType types[1];
        TSettingType* pTypes;
                
        //aTypes array is optional and user may not provided it for us. So we
        //need to be sure it's not a null pointer
        if(aTypes == NULL)
            {
            //If this is a null pointer then just create our own element and 
            //assign it to the pTypes pointer
            pTypes = types;
            }
        else
            {
            //else we use the user supplied array
            pTypes = aTypes;
            }
                
        //Let's find this setting
        err = HCRSingleton->FindSettingWithType(aIds[0], *pTypes, sref);
        
        //and analyse the result of operation
        
        //If setting is not found or it's larger than 4 bytes then store this
        //error cause in the user error array 
        if(err == KErrNotFound || err == KErrArgument)
            {
            //Indicate the error for the element and set the value to 0
            aErrors[0] = err;
            aValues[0] = 0;
            return 0;
            }
        //fatal error here, nothing to do, just exit and return the error code
        else if(err == KErrNotReady || err != KErrNone)
            {
            HCR_TRACE_RETURN(err);
            }
        else //err == KErrNone
            {
            //Get the value of the setting
            err = sref.iRep->GetValue(sref, reinterpret_cast<UValueWord&>(aValues[0]));

            //The GetValue can only return either KErrArgument or KErrNone
            if(err == KErrArgument)
                {
                aErrors[0] = KErrArgument;
                aValues[0] = 0;
                return 0;
                }
            else //err == KErrNone
                {
                aErrors[0] = KErrNone;
                }
            
            }
        
        //This single setting was found so indicate it to the user
        return (1);
        }

    
    //Introducing a SafeArray of pointers to the settings, which is passed to ver- 
    //sion of GetWordSettings() method declared in TRepository, and implemented 
    //in TRepositoryCompiled and TRepositoryFile
    TSa<SSettingId*> ids;

    //SafeArray of pointers to the aValues user array elements 
    TSa<TInt32*> values;
    
    //SafeArray of pointers to the aErrors user array elements 
    TSa<TInt*> errors;
    
    //SafeArray of pointers to the aTypes user array elements
    TSa<TSettingType*> types;
    
    
    //Local replacement for the aTypes[] array if it's not provided by user
    TSa<TSettingType> typesHolder;
    
    //Allocate the arrays of pointers in the  heap
    ids = new SSettingId*[aNum];
    values = new TInt32*[aNum];
    errors = new TInt*[aNum];
    types  = new TSettingType*[aNum];


    //Check all arrays allocations
    if(!ids() || !values() || !errors() || !types())
        {
        //One of the allocation was unsuccessful 
        HCR_TRACE_RETURN(KErrNoMemory);
        }
    
    //If the user did not supply the aTypes array for us we need to create one 
    //for ourself
    if(aTypes == NULL)
        {
        typesHolder = new TSettingType[aNum];
        if(!typesHolder())
            HCR_TRACE_RETURN(KErrNoMemory);
        }
    
       
    //Ininialize newly created array of pointers to the user supplied settings 
    for (TInt index = 0; index < aNum; index++)
        {
        ids[index] = const_cast<SSettingId*>(&aIds[index]);
        values[index] = const_cast<TInt32*>(&aValues[index]);
        errors[index] = &aErrors[index];
       
        if(aTypes == NULL)
            types[index] = &typesHolder[index];
        else
            types[index] = &aTypes[index];
        }
    
    
    //nfCount represents a total number of settings which were not found in all
    //repositories
    TInt nfCount = aNum;
    
    //nfReposCount represents a number of settings "not found - nf" in the searched
    //repository
    TInt nfReposCount   = 0;

    //It represents a number of setting found in the repository
    TInt reposCount   = 0;
    
    
    //First step through the Override store and gather all settings we need.
    //In the end of this procedure we'll have number of settings not found here
    //and found settings data are copied to the user arrays.
    if (iOverrideStore)
        {

        //Call the sibling method from the TRepositoryFile object
        err = iOverrideStore->GetWordSettings(aNum, ids(),
                values(), types(), errors());

        //Analyse the err we've got 
        if(err != KErrNone && err != KErrNotFound)
            {
            HCR_TRACE_RETURN(err);
            }
        else if(err == KErrNone)
            {
            //Search for number of not found parameters
            for(TInt index = 0; index < aNum; index ++)
                {
                switch(*(errors[index]))
                    {
                    //The setting was found or it's found but the type is larger
                    //than 4 bytes then we just increase a counter of the found
                    //settings in the repository
                    case KErrNone:
                    case KErrArgument:
                        reposCount ++;
                        break;


                    //The setting was not found, then re-initialize all the 
                    //arrays of pointers with the pointer to this element. 
                    //nfReposCount depict the counter of not found element and
                    //index shows the intial element position.
                    //As nfReposCount is always less or equal to index then we
                    //can easily make reassignment as nfReposCoun element was
                    //already analysed. In the end the nfReposCount is increased.
                    case KErrNotFound:
                        ids[nfReposCount]        = ids[index];
                        values[nfReposCount]     = values[index];
                        types[nfReposCount]      = types[index];
                        errors[nfReposCount]     = errors[index];                  
                        nfReposCount ++;
                        break;


                    default:
                        //No any action is needed
                        break;
                    }
                }

            }
        else //err == KErrNotFound
            {
            //No settings were found in the repository
            //reposCount is zero intialized, so nothing to do here
            }
        
        //Update the global counter only if there are some settings were found,
        //otherwise it can be situation when we overwrite the nfCount with zero
        //when either no any setting presents or no settings were found in the
        //repository
        if(reposCount > 0)
            nfCount = nfReposCount;
        }
    
    //Go through core image and search for the rest of settings
    nfReposCount = 0;
    reposCount = 0;
    
    if (iCoreImgStore && nfCount > 0)
        {

        err = iCoreImgStore->GetWordSettings(nfCount, ids(),
                                                values(), types(), errors());

        if (err != KErrNone && err != KErrNotFound)
            {
            HCR_TRACE_RETURN(err);
            }
        else if(err == KErrNone)
            {
            //Search for number of errors
            for(TInt index = 0; index < nfCount; index ++)
                {
                switch(*(errors[index]))
                    {
                    //The setting was found or it's found but the type is larger
                    //than 4 bytes then we just increase a counter of the found
                    //settings in the repository
                    case KErrNone:
                    case KErrArgument:
                        reposCount ++;
                        break;

                    //The setting was not found, then re-initialize all the 
                    //arrays of pointers with the pointer to this element. 
                    //nfReposCount depict the counter of not found element and
                    //index shows the intial element position.
                    //As nfReposCount is always less or equal to index then we
                    //can easily make reassignment as nfReposCoun element was
                    //already analysed. In the end the nfReposCount is increased.
                    case KErrNotFound:
                        ids[nfReposCount]        = ids[index];
                        values[nfReposCount]     = values[index];
                        types[nfReposCount]      = types[index];
                        errors[nfReposCount]     = errors[index];                  
                        nfReposCount ++;
                        break;


                    default:
                        //No any action is needed
                        break;

                    }

                }

            }
        else //err == KErrNotFound 
            {
            //No settings were found in the repository
            //reposCount is zero intialized, so nothing to do here
            }


        //Update the global counter only if there are some settings were found,
        //otherwise it can be situation when we overwrite the nfCount with zero
        //when either no any setting presents or no settings were found in the
        //repository 
        if(reposCount > 0)
            nfCount = nfReposCount;
        }
    
    //let's go through the last Variant store
    nfReposCount = 0;
    reposCount = 0;
    if(iVariantStore && nfCount > 0)
        {
        err = iVariantStore->GetWordSettings(nfCount, ids(), values(), 
                types(), errors());

        if (err != KErrNone && err != KErrNotFound)
            {
            HCR_TRACE_RETURN(err);
            }
        else if(err == KErrNone)
            {
            //Search for number of errors
            for(TInt index = 0; index < nfCount; index ++)
                {
                switch(*(errors[index]))
                    {
                    //The setting was found or it's found but the type is larger
                    //than 4 bytes then we just increase a counter of the found
                    //settings in the repository
                    case KErrNone:
                    case KErrArgument:
                        reposCount ++;
                        break;

                    //The setting was not found, then re-initialize all the 
                    //arrays of pointers with the pointer to this element. 
                    //nfReposCount depict the counter of not found element and
                    //index shows the intial element position.
                    //As nfReposCount is always less or equal to index then we
                    //can easily make reassignment as nfReposCoun element was
                    //already analysed. In the end the nfReposCount is increased.
                    case KErrNotFound:
                        *values[nfReposCount]     = 0;
                        *types[nfReposCount]      = ETypeUndefined;
                        *errors[nfReposCount]     = KErrNotFound;
                        nfReposCount ++;
                        break;


                    default:
                        //No any action is needed
                        break;

                    }
                }
            
            }
        else //err == KErrNotFound
            {
            //No settings were found in the repository
            //reposCount is zero intialized, so nothing to do here
            }
        
        //Update the global counter only if there are some settings were found,
        //otherwise it can be situation when we overwrite the nfCount with zero
        //when either no any setting presents or no settings were found in the
        //repository
        if(reposCount > 0)
            nfCount = nfReposCount;
        }
    //Return the number of found elements
    return (aNum - nfCount);
    }





TInt HCR::HCRInternal::FindNumSettingsInCategory (TCategoryUid aCatUid)
    {
    HCR_FUNC("++ HCRInternal::FindNumSettingsInCategory");
    TInt err = 0;

    HCR_TRACE3("--- Repository state: %x, %x, %x", iOverrideStore, iCoreImgStore, iVariantStore);

    //First and last element index within category in the Override store
    TInt32 oLowIndex = 0;
    TInt32 oHighIndex = 0;
    TInt oCount = 0;

    //Find numOverride number of settings within the category in the OverrideStore 
    //repository
    if(iOverrideStore)
        {
        err = iOverrideStore->FindNumSettingsInCategory(aCatUid, 
                oLowIndex, oHighIndex);
       
        if(err == KErrNotFound)
            oCount = 0;
        else
            oCount = oHighIndex - oLowIndex + 1;

        //If CoreImg and Variant store are not activated so just return the
        //number of elements found in the Override store
        if(!iCoreImgStore && !iVariantStore)
            return oCount;
        }


    //First and last element index within category in the CoreImg store
    TInt32 cLowIndex = 0;
    TInt32 cHighIndex = 0;
    TInt32 cLength = 0;
    TInt   cCount = 0;

        
    
    //Temproary holder for the found element position
    TInt32 elementPos;
    //Temproary holder for the low index, which is used to decrease the scope
    //of search
    TInt32 lowIndex = oLowIndex;
    
    //Setting data holders
    SSettingId setId;
    TSettingRef setRef;
    
    if(iCoreImgStore)
        {
        //Find numCoreImg number of settings within the category in the CoreImg re-
        //pository
        err = iCoreImgStore->FindNumSettingsInCategory(aCatUid, 
                cLowIndex, cHighIndex);

        if(err == KErrNotFound)
            cLength = 0;
        else
            //Calculate the number of elements within category, in CoreImg store
            cLength = cHighIndex - cLowIndex + 1;

        if(oCount > 0)
            {
            //Find all elemnts from CoreImg store which are not redefined in the 
            //Override store. When element is not found in the Override store 
            //then cCount is increased.
            for(TInt element = 0; element < cLength; element ++)
                {
                //Find element in the repository by its index
                iCoreImgStore->GetSettingRef(cLowIndex + element, setRef);
                //and get its id
                iCoreImgStore->GetId(setRef, setId);
                
                //Check either this element is already redefined in the Override
                //store
                err = iOverrideStore->FindSetting( setId, setRef, 
                        elementPos, lowIndex, oHighIndex);

                if(err == KErrNone)
                    {
                    //if the element is found in the Override store, then store the posi-
                    //tion of this element in lowIndex, to narrow next search procedure
                    lowIndex = elementPos;
                    }
                else //err == KErrNotFound
                    {
                    //if element is not found then it means it's not redefined in the 
                    //Override store and this element must be counted in the total number
                    //of elemnts in all stores
                    cCount ++;
                    
                    //FindSetting can only return KErrNotFound, let's assert 
                    //we've only got KErrNotFound
                    __NK_ASSERT_DEBUG(err == KErrNotFound);
                    
                    }
                
                }
            }
        else
            {
            cCount = cLength;
            }

        }

    //First and last element index within giving category in the Variant store
    TInt32 vLowIndex  = 0;
    TInt32 vHighIndex = 0;
    TInt32 vLength = 0;
    TInt vCount = 0;

    if(iVariantStore)
        {
        //Find numVariant number of settings within the category in the VariantStore
        //repository
        err = iVariantStore->FindNumSettingsInCategory(aCatUid, vLowIndex, 
                vHighIndex);

        //Analyze returned error code

        if(err == KErrNotFound)
            vLength = 0;
        else
            //Calculate the number of elements within category, in CoreImg store
            vLength = vHighIndex - vLowIndex + 1;


        if(oCount > 0 || cCount >0)
            {
            //Find all elemnts from Variant store which are not redefined either in the 
            //Override or CoreImg store. These elements are added to the total 
            //count.
            
            // Some additional containers. They are needed because we  
            // must check two stores Override and Variant in this iteration. Making a 
            // decision of uniqueness of the element is made from the analyse of both  
            // result. The element is only unique defined in the Variant store if it's  
            // not redefined either in the Override or Variant store
            TSettingRef tmpRef;
            //Temproary holder for the found element position
            TInt32 elementPos2 = 0;
            //Temproary holder for the low index, which is used to decrease the scope
            //of search
            TInt32 lowIndex2 = cLowIndex;
            // This index contains Override low index and will be changed by the position
            // of a new found element 
            lowIndex= oLowIndex;

            TBool isRedefined = EFalse;
            
            for(TInt element = 0; element < vLength; element ++)
                {
                //Find the setting in the repository by its index and
                iVariantStore->GetSettingRef(vLowIndex + element, setRef);
                //get its id
                iVariantStore->GetId(setRef, setId);

                if(oCount > 0)
                    {
                    //Check either this element is already redefined in the Override store 
                    err = iOverrideStore->FindSetting(setId, tmpRef, 
                            elementPos, lowIndex, oHighIndex);

                    if(err == KErrNone)
                        {
                        //if the element is found in the Override store, then store the posi-
                        //tion of this element in lowIndex, to narrow next search procedure
                        lowIndex = elementPos;
                        isRedefined = ETrue;
                        }
                    else //err == KErrNotFound
                        {
                        //the element is not presented in the Override store
                        //nothing to do here

                        //FindSetting can only return KErrNotFound, let's assert 
                        //we've only got KErrNotFound
                        __NK_ASSERT_DEBUG(err == KErrNotFound);
                        }
                    

                    }


                if(cCount > 0 && !isRedefined)
                    {
                    //Check either this element is already redefined in the CoreImg store
                    err = iCoreImgStore->FindSetting(setId, tmpRef, 
                            elementPos2, lowIndex2, cHighIndex);


                    if(err == KErrNone)
                        {
                        //if the element is found in the Override store, then store the posi-
                        //tion of this element in lowIndex, to narrow next search procedure
                        lowIndex2 = elementPos2;
                        isRedefined = ETrue;
                        }
                    else //err == KErrNotFound
                        {
                        //the element is not presented in the Override store
                        //nothing to do here

                        //FindSetting can only return KErrNotFound, let's assert 
                        //we've only got KErrNotFound
                        __NK_ASSERT_DEBUG(err == KErrNotFound);
                        }
                    
                    }
                

                if(!isRedefined)
                    vCount ++;
                else
                    isRedefined = EFalse;

                }//for(TInt element = 0; element < vLength; element ++)

            }
        else
            {
            vCount = vLength;
            }
        }

    //Return the total number of elements found in the category
    return (oCount + cCount + vCount);
    }




TInt HCR::HCRInternal::FindSettings(TCategoryUid aCatUid, 
        TInt aMaxNum, TElementId aIds[],  
        TSettingType aTypes[], TUint16 aLens[])
    {
    HCR_FUNC("++ HCRInternal::FindSettings w/o patterns");
   
    HCR_TRACE3("--- Repository state: %x, %x, %x", iOverrideStore, 
            iCoreImgStore, iVariantStore);
   
    //Error container
    TInt err = KErrNone;
    
    //Number of found elements in the Override store
    TInt oNumFound = 0;

    //Low and High indexes in the Override store
    TInt32 oLoIndex = 0;
    TInt32 oHiIndex = 0;

    //Temproary holder for the found element position
    TInt32 elementPos = 0;
    TInt32 lowIndex = 0;
    

    //Tempoary type and length value holders if the
    //user does not provide these arrays for us
    TSettingType tmpType;
    TUint16 tmpLen;

        
    //Setting datat holders
    TSettingRef setRef;
    TSettingId  setId;

    
    //Find number of elements, low and hingh index in the Override store
    if(iOverrideStore)
        {
        err = iOverrideStore->FindNumSettingsInCategory(aCatUid, oLoIndex,
                oHiIndex);
        if(err == KErrNone)
            {
            //If number of elements in the Override Store is larger than aMaxNum or 
            //CoreImage/Variant stores are not present then write all found 
            //settings into the user array, return the number of found elements and
            //exit
            oNumFound = (oHiIndex - oLoIndex + 1);
            lowIndex = oLoIndex;
       
            if(oNumFound < aMaxNum)
                {
                for(TInt index = 0; index < oNumFound; index ++)
                    {
                    //Get setting reference data from the repository
                    iOverrideStore->GetSettingRef(oLoIndex + index, setRef);

                    //Copy the settings data into the user arrays
                    iOverrideStore->GetSettingInfo(setRef, 
                            aIds[index], 
                            aTypes ? aTypes[index]:tmpType,
                            aLens ? aLens[index]:tmpLen);

                    
                    }
                }
            else //oNumFound >= aMaxNum
                {
                //Copy data to the user array
                for(TInt index = 0; index < aMaxNum; index++)
                    {
                    //Get setting reference data from the repository
                    iOverrideStore->GetSettingRef(oLoIndex + index, setRef);
                    //Copy the settings data into the user arrays
                    iOverrideStore->GetSettingInfo(setRef, 
                            aIds[index], 
                            aTypes ? aTypes[index]:tmpType,
                            aLens  ? aLens[index]:tmpLen);

                    }
                return aMaxNum;
                }
            }
        else // err == KErrNotFound
            {
            //Nothing to do here, oNumFound is set to zero already

            //FindNumSettingsInCategory can only return KErrNotFound, let's  
            //assert we've only got KErrNotFound
            __NK_ASSERT_DEBUG(err == KErrNotFound);
            }

        }

   
    //Low/High index in the CoreImg
    TInt32 cLoIndex = 0;
    TInt32 cHiIndex = 0;
    TInt cNumFound = 0;
    
    //Temproary setting reference holder
    TSettingRef tmpRef;

    //Temproary holder for the found element position
    elementPos = 0;
    lowIndex = oLoIndex;

    //Redefined status flag, it's used to flag that the element is found in the 
    //upper stores
    TBool isRedefined = EFalse;
    
    //User array index
    TInt usrArrIndx = 0;

    //If the count is still less than aMaxNum then continue with searching 
    //settings in the CoreImage store
    if(iCoreImgStore)
        {

        //Find number of elements and low/high indexes
        err = iCoreImgStore->FindNumSettingsInCategory(aCatUid, cLoIndex,
                cHiIndex);

        if(err == KErrNone)
            {
            for(TInt index = 0; index < (cHiIndex - cLoIndex + 1); index ++)
                {
                //Get the setting data by its index in the repository
                iCoreImgStore->GetSettingRef(cLoIndex + index, setRef);
                //get setting id
                iCoreImgStore->GetId(setRef, setId);
                
                if(oNumFound > 0)
                    {
                    //Check either this element is already redefined in the 
                    err = iOverrideStore->FindSetting(setId, tmpRef, 
                            elementPos, lowIndex, oHiIndex);

                    
                    if(err == KErrNone)
                        {
                        lowIndex = elementPos + 1;
                        isRedefined = ETrue;
                        }
                    else //err == KErrNotFound
                        {
                        //Nothing to do hear, isRedefined flag is EFalse
                        //all analysis is done later in the code

                        //FindSetting can only return KErrNotFound, let's assert 
                        //we've only got KErrNotFound
                        __NK_ASSERT_DEBUG(err == KErrNotFound);
                        }
                    
                    }

                //Examine the redefined status flag
                if(!isRedefined)
                    {
                    // If the element was not found then we need to copy to 
                    // the pA array and increase the counter of setting data 
                    // only if we did not reach the aMaxNum of found elements
                    
                    usrArrIndx = oNumFound + cNumFound;
                    if(usrArrIndx < aMaxNum)
                        {
                        //Copy the settings data into the user arrays
                        iCoreImgStore->GetSettingInfo(setRef, 
                                 aIds[usrArrIndx], 
                                 aTypes ? aTypes[usrArrIndx]:tmpType,
                                 aLens ? aLens[usrArrIndx]:tmpLen);
                        cNumFound ++;
                        }
                    else
                        {
                        //It reaches the goal, all required elements are found
                        //stop here and return the result
                        break;
                        }
                    }
                else
                    //Element is found in other repositories, just reset a flag
                    isRedefined = EFalse;
                }
            }
        else //err == KErrNotFound
            {
            //cNumFound is already set to zero during the initialization
            //Nothing to do here

            //FindNumSettingsInCategory can only return KErrNotFound, let's  
            //assert we've only got KErrNotFound
            __NK_ASSERT_DEBUG(err == KErrNotFound);
            }
        
        }

    
    //Low/High index in the CoreImg
    TInt32 vLoIndex = 0;
    TInt32 vHiIndex = 0;
    TInt vNumFound = 0;

    //Temproary holder for the found element position
    TInt32 elementPos2 = 0;
    
    TInt32 lowIndex2 = cLoIndex;
    lowIndex  = oLoIndex;

    isRedefined = EFalse;
    

    //If the count is still less than aMaxNum then continue with searching 
    //settings in the CoreImage store
    if(iVariantStore)
        {

        //Find number of elements and low/high indexes
        err = iVariantStore->FindNumSettingsInCategory(aCatUid, vLoIndex,
                vHiIndex);
        if(err == KErrNone)
            {

            for(TInt index = 0; index < (vHiIndex - vLoIndex + 1); index ++)
                {
                //Get setting reference data by its index in the repository
                iVariantStore->GetSettingRef(vLoIndex + index, setRef);
                
                //and get setting id
                iVariantStore->GetId(setRef, setId);
                
                if(oNumFound > 0)
                    {
                    //Check either this element is already redefined in the 
                    err = iOverrideStore->FindSetting(setId, tmpRef, elementPos,  
                            lowIndex, oHiIndex);
                    
                
                    //Also suppress the error checking due the reason described 
                    //above
                    if(err == KErrNone)
                        {
                        lowIndex = elementPos + 1;
                        isRedefined = ETrue;
                        }
                    else //err == KErrNotFound
                        {
                        //Element is not found, nothing to proceed here

                        //FindSetting can only return KErrNotFound, let's assert 
                        //we've only got KErrNotFound
                        __NK_ASSERT_DEBUG(err == KErrNotFound);
                        }
                    
                    }

                if(cNumFound > 0 && !isRedefined)
                    {
                    //Check either this element is already redefined in the 
                    err = iCoreImgStore->FindSetting(setId, tmpRef, elementPos2,  
                            lowIndex2, cHiIndex);

                    if(err == KErrNone)
                        {
                        lowIndex2 = elementPos2 + 1;
                        isRedefined = ETrue;
                        }
                    else //err == KErrNotFound
                        {
                        //Element is not found, nothing to proceed here

                        //FindSetting can only return KErrNotFound, let's assert 
                        //we've only got KErrNotFound
                        __NK_ASSERT_DEBUG(err == KErrNotFound);
                        }
                    
                    }
               
                if(!isRedefined)
                    {
                    usrArrIndx = oNumFound + cNumFound + vNumFound;
                    if(usrArrIndx < aMaxNum)
                        {
                        //Copy the settings data into the user arrays
                        iVariantStore->GetSettingInfo(setRef, 
                                 aIds[usrArrIndx], 
                                 aTypes ? aTypes[usrArrIndx]:tmpType,
                                 aLens ? aLens[usrArrIndx]:tmpLen);

                        vNumFound ++;
                        }
                    else
                        {
                        //It reaches the goal, all required elements are found
                        //stop here and return the result
                        break;
                        }
                    }
                else
                    {
                    isRedefined = EFalse;
                    }
                }
            }
        else //err == KErrNotFound
            {
            //oNumFound is already set to zero during the initialization
            //Nothing to do here

            //FindNumSettingsInCategory can only return KErrNotFound, let's  
            //assert we've only got KErrNotFound
            __NK_ASSERT_DEBUG(err == KErrNotFound);
            }
                
        }
    
    //Let's prepare the final data
    return (oNumFound + cNumFound + vNumFound);
    }







TInt HCR::HCRInternal::FindSettings(TCategoryUid aCat, TInt aMaxNum,
                TUint32 aMask, TUint32 aPattern, 
                TElementId aIds[], TSettingType aTypes[], TUint16 aLens[])
    {
    //Holder for errors and number of elements
    TInt r = KErrNone;
    //Total number of elements within the given category
    TInt allInCatFound = 0;
    //Number of elements which corresponds to the aMask and aPattern
    TInt numFound = 0;
    
    //Find the number of elements within the category
    r = FindNumSettingsInCategory(aCat);
    
    //We don't expect any errors here
    __NK_ASSERT_DEBUG(r >= 0);
    
    if (r == 0)
        //No any elements found for this category 
        return 0;
    else
        allInCatFound = r;
    
    //Result data array holder
    TSa<TElementId> pIds; 
    TSa<TSettingType> pTypes;
    TSa<TUint16> pLens;
    
    pIds = new TElementId[allInCatFound];
    pTypes = new TSettingType[allInCatFound];
    pLens = new TUint16[allInCatFound];

    if(pIds() == NULL || pTypes() == NULL || pLens() == NULL)
        //One of the allocation was unsuccessful 
        HCR_TRACE_RETURN(KErrNoMemory);
    
    r = FindSettings(aCat, allInCatFound, pIds(), pTypes(), pLens());
    
    //We don't expect any errors here
    __NK_ASSERT_DEBUG(r >= 0);
    
    //Check either we've got less elements than it must be
    __NK_ASSERT_DEBUG(r == allInCatFound);
    
    //Choose the elements which satisfy this condition
    //((elementID & aElementMask) == (aPattern & aElementMask)). The total num-
    //ber of returned elements should not exceed the aMaxNum
    for(TInt index = 0; index < allInCatFound; index++)
        {
            if(((pIds[index] & aMask) == (aPattern & aMask)))
                {
                aIds[numFound] = pIds[index];

                if(aTypes)
                    aTypes[numFound] = pTypes[index];

                if(aLens)
                    aLens[numFound] = pLens[index];

                numFound ++;
                }
            else
                continue;
            
            //Check either we already found  or not enough elements
            //If we did then break the loop
            if(numFound == aMaxNum)
                break;
        }
    
    return numFound;
    }


// -- METHODS -----------------------------------------------------------------
//
// TRepository

HCR::TRepository::~TRepository()
	{
    HCR_FUNC("~TRepository");
	}

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

void HCR::TRepository::GetSettingInfo(const HCR::TSettingRef& aSetRef, 
               HCR::TElementId& aId, HCR::TSettingType& aType, TUint16& aLen)
    {
    HCR_FUNC("TRepository::GetSettingInfo");

    aId = aSetRef.iSet->iId.iKey;
   
    aType = static_cast<TSettingType>(aSetRef.iSet->iType);

    aLen = aSetRef.iSet->iLen;
    }

// -- METHODS -----------------------------------------------------------------
//
// TRepositoryCompiled


HCR::TRepository* HCR::TRepositoryCompiled::New(const SRepositoryCompiled* aRepos)
    {
    HCR_FUNC("TRepositoryCompiled::New");
    
    __NK_ASSERT_ALWAYS(aRepos != 0);
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
        
TInt HCR::TRepositoryCompiled::CheckIntegrity()
    {
    HCR_FUNC("TRepositoryCompiled::CheckIntegrity");
    
    __NK_ASSERT_ALWAYS(this != 0);   
    __NK_ASSERT_ALWAYS(iRepos != 0);

	if (iRepos->iOrderedSettingList == 0)
        HCR_TRACEMSG_RETURN("Compiled Repository header missing setting array list", KErrNotFound);
    
	HCR_TRACE2("Compiled repository 0x%x contains %05d entries", iRepos, iRepos->iHdr->iNumSettings);

    SSettingC* arr = iRepos->iOrderedSettingList;
    TSettingId prev(0,0);
    TInt rc=0;
    for (int i=0; i < iRepos->iHdr->iNumSettings; i++, arr++)
    	{
    	__NK_ASSERT_ALWAYS(arr != 0);
    	HCR_TRACE3("Checking entry %05d - (0x%x,0x%x)", i, arr->iName.iId.iCat,  arr->iName.iId.iKey);
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
    
    __NK_ASSERT_DEBUG(iRepos != 0);
    __NK_ASSERT_DEBUG(iRepos->iHdr != 0);
    
    if ((iRepos->iHdr->iNumSettings == 0) || 
        (iRepos->iOrderedSettingList == 0))
        HCR_TRACE_RETURN(KErrNotFound);
    
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




TInt HCR::TRepositoryCompiled::FindSetting(const TSettingId& aId, 
       TSettingRef& aSetting,  TInt32& aPosition,  TInt32 aLow, TInt32 aHigh)
    {
    HCR_FUNC("TRepositoryCompiled::FindSetting within the given range");
    
    
    __NK_ASSERT_DEBUG(iRepos != 0);
    __NK_ASSERT_DEBUG(iRepos->iHdr != 0);
	__NK_ASSERT_DEBUG(iRepos->iOrderedSettingList != 0);
	__NK_ASSERT_DEBUG(iRepos->iHdr->iNumSettings != 0);
    
    SSettingC* arr = iRepos->iOrderedSettingList;
    TInt32 low = aLow;
    TInt32 high = aHigh;
    TInt32 mid;
    TInt32 com;
    
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
            aPosition = mid;
            return KErrNone;
            }    
        } 
        
    aSetting.iRep = 0;
    aSetting.iSet = 0;
    aPosition = 0;
    return KErrNotFound;
    }
    


TInt HCR::TRepositoryCompiled::GetWordSettings(TInt aNum,   
       SSettingId* aIds[], TInt32* aValues[], TSettingType* aTypes[],
        TInt* aErrors[])
    {
    HCR_FUNC("TRepositoryCompiled::GetWordSettings");
    
    __NK_ASSERT_DEBUG(iRepos != 0);
    __NK_ASSERT_DEBUG(iRepos->iHdr != 0);
	__NK_ASSERT_DEBUG(aIds != NULL);
	__NK_ASSERT_DEBUG(aValues != NULL);
	__NK_ASSERT_DEBUG(aTypes != NULL);
	__NK_ASSERT_DEBUG(aErrors != NULL);
    
    if ((iRepos->iHdr->iNumSettings == 0) || 
        (iRepos->iOrderedSettingList == 0))
        HCR_TRACE_RETURN(KErrNotFound);
 
    TInt err = KErrNone;
        
    TInt32 rMaxIndex = 0;
    TInt32 rMinIndex = 0;
    TInt32 uFirstIndex = 0;
    TInt32 uLastIndex = 0;
    TInt32 rIndex = 0;
    TInt32 uIndex = 0;
    
    TSettingRef settingRef(NULL, NULL);
    SSettingC* pSetting = NULL;


    //Find position index within the repository for the first and last setting
    //from user supplied array aIds[]
    uIndex = 0;
    TBool isRedefined = EFalse;
    err = KErrNotFound;
    uFirstIndex = 0;
    while(!isRedefined && uIndex < aNum)
        {
        //Find first setting from user array. The importance here is that we   
        //should get value of first setting index in the repository in rMinIndex.
        //This time the  scope of search is whole repository.
        err = this->FindSetting(*aIds[uIndex],settingRef, rMinIndex, 
                0, iRepos->iHdr->iNumSettings);

		__NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);

        if(err == KErrNotFound)
            {
            *aErrors[uIndex] = err;
            *aValues[uIndex] = 0;
            *aTypes[uIndex] = ETypeUndefined;
            
            //As FindSetting did not find the element, let's challenge with 
            //the next one from aIds[] array
            uIndex ++;
            continue;
            }
        else // err == KErrNone
            {
            //Get the value and type
            pSetting = (SSettingC*) settingRef.iSet;
            
			*aTypes[uIndex] = static_cast<TSettingType>(settingRef.iSet->iType); 

			//Check for the found type is this word size? If it's not then 
			//indicate error for this setting
			if(*aTypes[uIndex] > ETypeLinAddr)
				{
				*aErrors[uIndex] = KErrArgument;
				*aValues[uIndex] = 0;
				}
			else
				{
				*aErrors[uIndex] = KErrNone;
				*aValues[uIndex] = pSetting->iValue.iLit.iInt32;
				}
				
            //Break the loop by setting the redefined status
            isRedefined = ETrue;
            }
        }
    
    //At this point we should find at least one element from the user array,   
    //store this index in the local variable, it is used later in the code.   
    //Please be noticed we've also got rMinIndex - first setting index in the
    //repository.
    if(err == KErrNone)
        uFirstIndex = uIndex;
    //if we did not find any elements at all just return KErrNotFound
    else
        return KErrNotFound;

    
    
    //Now lets find the last setting
    uIndex = aNum - 1;
    isRedefined = EFalse;
    err = KErrNotFound;
    while(!isRedefined && uIndex > uFirstIndex)
        {
        //Find the last setting from user array. The importance here is that we   
        //should get value of first setting index in the repository in 
        //rMinIndex. This time the  scope of search is whole repository.
        err = this->FindSetting(*aIds[uIndex],settingRef, rMaxIndex, 
                rMinIndex, iRepos->iHdr->iNumSettings);

		__NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);

        if(err == KErrNotFound)
            {
            *aErrors[uIndex] = err;
            *aValues[uIndex] = 0;
            *aTypes[uIndex] = ETypeUndefined;
            
            //As FindSetting did not find the element, let's challenge with 
            //previous one, as we are moving in reverse direction
            uIndex --;
            continue;
            }
        else //err == KErrNone
            {
            pSetting = (SSettingC*) settingRef.iSet;
            *aTypes[uIndex] = static_cast<TSettingType>(settingRef.iSet->iType); 
            
			//Check for the found type is this word size? If it's not then indicate
			//error for this setting
			if(*aTypes[uIndex] > ETypeLinAddr)
				{
				*aErrors[uIndex] = KErrArgument;
				*aValues[uIndex] = 0;
				}
			else
				{
				*aErrors[uIndex] = KErrNone;
				*aValues[uIndex] = pSetting->iValue.iLit.iInt32;
				}
				
            isRedefined = ETrue;
            }
        }

    //At this point we found the last setting, store it's user array index in   
    //the local variable, it is used later in the code. Please be noticed   
    //we've also got rMaxIndex - last setting index in the repository.
    if(err == KErrNone)
        uLastIndex = uIndex;
    else
        //if we are here we did not find any other elements than was found
        //in previous iteration then just stop here
        return KErrNotFound;
    
    //The scope of user array settings in the repository is found. 
    //Let's find all other settings from user array. Bare in mind the low
    //bound for the repository index is increased each iteration to optimize the
    //search time.
    for(uIndex = uFirstIndex + 1; uIndex < uLastIndex; uIndex ++)
        {
        err = this->FindSetting(*aIds[uIndex],settingRef, rIndex, 
                rMinIndex, rMaxIndex);

		__NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);

        if(err == KErrNotFound)
            {
            *aErrors[uIndex] = err;
            *aValues[uIndex] = 0;
            *aTypes[uIndex] = ETypeUndefined;

            //As FindSetting did not find the element, let's challenge with 
            //another one
            continue;
            }
        else //err == KErrNone
            {

            pSetting = (SSettingC*) settingRef.iSet;
            *aTypes[uIndex] = static_cast<TSettingType>(settingRef.iSet->iType); 

			//Check for the found type is this word size? If it's not then indicate
			//error for this setting
			if(*aTypes[uIndex] > ETypeLinAddr)
				{
				*aErrors[uIndex] = KErrArgument;
				*aValues[uIndex] = 0;
				}
			else
				{
				*aErrors[uIndex] = KErrNone;
				*aValues[uIndex] = pSetting->iValue.iLit.iInt32;
				}
				
            rMinIndex = rIndex + 1;

            }

        }

    return KErrNone;
    }





TInt HCR::TRepositoryCompiled::FindNumSettingsInCategory(TCategoryUid aCatUid,
        TInt32& aFirst, TInt32& aLast)
    {
    HCR_FUNC("TRepositoryCompiled::FindNumSettingsInCategory");

    __NK_ASSERT_DEBUG(iRepos != 0);
    __NK_ASSERT_DEBUG(iRepos->iHdr != 0);
	__NK_ASSERT_DEBUG(iRepos->iOrderedSettingList != 0);
    
    if(iRepos->iHdr->iNumSettings == 0)
        {
        aFirst = 0;
        aLast = 0;
        HCR_TRACE_RETURN(KErrNotFound);
        }

    SSettingC* arr = iRepos->iOrderedSettingList;
    int low = 0;
    int high = iRepos->iHdr->iNumSettings-1;
    int mid = 0;
    int com = 0;

    //Let's find any setting within the category, mid will store the setting 
    //index in the repository
    while (low<=high)
        {
        mid = (low+high) >> 1;
        com = CompareByCategory(aCatUid, arr[mid].iName.iId);
        if (com < 0)
            high = mid-1;
        else if (com > 0)
            low = mid+1;
        else
            {
            break;
            }    
        } 

    // If no one setting with the given category was found the return error  
    // to the user
    if(low > high)
        {
        aFirst = 0;
        aLast  = 0;
        return KErrNotFound;
        }

    //Search the first element within the category
    low = mid;
    while(low >= 0 && arr[low].iName.iId.iCat == aCatUid)
        {
        if(low > 0)
            low --;
        else
            break;
        }
    //Check the boundary conditions, there are two cases when we exit the loop
    //either we found an element which category is not one we are looking for or
    //we reach the beggining of the repository. If we reach the beggining of the
    //repository we don't really know is it because this is last elment or it
    //has required aCatUid, so we check these two conditions below
    if(arr[low].iName.iId.iCat == aCatUid)
        aFirst = low;

    //We finish the loop either reaching the setting which category id is not
    //what we need or this is first setting in the repository again with another
    //category, so in both case we throw this element from the account.
    else
        aFirst = low + 1;

    //Search the last element within the category
    high = mid;
    while(high <= iRepos->iHdr->iNumSettings - 1 && arr[high].iName.iId.iCat == aCatUid)
        {
        if(high < iRepos->iHdr->iNumSettings - 1)
            high ++;
        else
            break;
        }

    //Same situation as above, boundary conditions
    if(arr[high].iName.iId.iCat == aCatUid)
        aLast = high;
    else
        aLast = high -1;


    return KErrNone;
    }



void HCR::TRepositoryCompiled::GetSettingRef(TInt32 aIndex, 
        HCR::TSettingRef& aRef)
    {
    __NK_ASSERT_DEBUG(iRepos != 0);
    __NK_ASSERT_DEBUG(iRepos->iHdr != 0);
    __NK_ASSERT_DEBUG(iRepos->iHdr->iNumSettings != 0 && iRepos->iOrderedSettingList != 0);
    __NK_ASSERT_DEBUG(aIndex >=0 && aIndex < iRepos->iHdr->iNumSettings);
    

    //Get the pointer to the repository data
    SSettingC* arr = iRepos->iOrderedSettingList;
        
    aRef.iRep = this;
    aRef.iSet = &(arr[aIndex].iName);
    }


TInt HCR::TRepositoryCompiled::GetValue(const TSettingRef& aRef, UValueWord& aValue)
    {
    HCR_FUNC("TRepositoryCompiled::GetValue");
    if (!IsWordValue(aRef))
        HCR_TRACE_RETURN(KErrArgument);
        
	SSettingC* sptr = (SSettingC*)(aRef.iSet);
    aValue = sptr->iValue.iLit;
	return KErrNone;
    }

TInt HCR::TRepositoryCompiled::GetLargeValue(const TSettingRef& aRef, UValueLarge& aValue)
    {
    HCR_FUNC("TRepositoryCompiled::GetLargeValue");
    if (!IsLargeValue(aRef))
        HCR_TRACE_RETURN(KErrArgument);

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

    __NK_ASSERT_ALWAYS(aRepos != 0);
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
	
#ifdef __WINS__
	// On target hardware the iRepos pointer always points to a file in the Core
	// rom image and hence is not memory allocated on kernel heap. Hence it does
	// not need to be freeded. 
	// When running under the emulator the file repositories are loaded into
	// allocated memory which needs to be freed here.

	delete const_cast<SRepositoryFile*>(iRepos);
	iRepos = 0;
#endif // __WINS__

    }
    
TInt HCR::TRepositoryFile::CheckIntegrity()
    {
    HCR_FUNC("TRepositoryFile::CheckIntegrity");
    
    __NK_ASSERT_ALWAYS(this != 0);   
    __NK_ASSERT_ALWAYS(iRepos != 0);
    
	if ((*((TUint32*)&(iRepos->iHdr)) != 0x66524348) || 
		(iRepos->iHdr.iFormatVersion != 0x0001))
        HCR_TRACEMSG_RETURN("File Repository header describes an unsupported repository type", KErrCorrupt); 
	
    HCR_TRACE2("File repository 0x%x contains %05d entries", iRepos, iRepos->iHdr.iNumSettings);
    
    SSettingF* arr = (SSettingF*) (iRepos+1);
    TSettingId prev(0,0);
    TInt rc=0;
    for (int i=0; i < iRepos->iHdr.iNumSettings; i++, arr++)
    	{
    	__NK_ASSERT_ALWAYS(arr != 0);
    	HCR_TRACE3("Checking entry %05d - (0x%x,0x%x)", i, arr->iName.iId.iCat,  arr->iName.iId.iKey);
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
 
    __NK_ASSERT_DEBUG(iRepos != 0);
   
    if (iRepos->iHdr.iNumSettings == 0)
        HCR_TRACE_RETURN(KErrNotFound);
    
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


TInt HCR::TRepositoryFile::FindSetting (const TSettingId& aId,
        TSettingRef& aSetting, TInt32& aPosition, TInt32 aLow, TInt32 aHigh)
    {
    HCR_FUNC("TRepositoryFile::FindSetting within the given range");


    __NK_ASSERT_DEBUG(iRepos != 0);
    __NK_ASSERT_DEBUG(iRepos->iHdr.iNumSettings != 0);

    SSettingF* arr = (SSettingF*) (iRepos+1);
    TInt32 low = aLow;
    TInt32 high = aHigh;
    TInt32 mid;
    TInt32 com;

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
            aPosition = mid;
            return KErrNone;
            }    
        } 

    aSetting.iRep = 0;
    aSetting.iSet = 0; 
    aPosition = 0;
    return KErrNotFound;
    }




TInt HCR::TRepositoryFile::GetWordSettings(TInt aNum,   
        SSettingId* aIds[], TInt32* aValues[], TSettingType* aTypes[],
        TInt* aErrors[])
    {
    HCR_FUNC("TRepositoryFile::GetWordSettings");


    __NK_ASSERT_DEBUG(iRepos != 0);
	__NK_ASSERT_DEBUG(aIds != NULL);
	__NK_ASSERT_DEBUG(aValues != NULL);
	__NK_ASSERT_DEBUG(aTypes != NULL);
	__NK_ASSERT_DEBUG(aErrors != NULL);
   
    if (iRepos->iHdr.iNumSettings == 0)
        return KErrNotFound;

    TInt err = KErrNone;

    TInt32 rMaxIndex = 0;
    TInt32 rMinIndex = 0;
    TInt32 uFirstIndex = 0;
    TInt32 uLastIndex = 0;
    TInt32 rIndex = 0;
    TInt32 uIndex = 0;

    TSettingRef settingRef(NULL, NULL);
    SSettingF* pSetting = NULL;

    //Find position index within the repository for the first and last setting
    //from user supplied array aIds[]
        uIndex = 0;
        TBool isRedefined = EFalse;
        err = KErrNotFound;
        uFirstIndex = 0;
        while(!isRedefined && uIndex < aNum)
            {
            //Find first setting from user array. The importance here is that we   
            //should get value of first setting index in the repository in rMinIndex.
            //This time the  scope of search is whole repository.
            err = this->FindSetting(*aIds[uIndex],settingRef, rMinIndex, 
                    0, iRepos->iHdr.iNumSettings);

			__NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);

            if(err == KErrNotFound)
                {
                *aErrors[uIndex] = err;
                *aValues[uIndex] = 0;
                *aTypes[uIndex] = ETypeUndefined;
                
                //As FindSetting did not find the element, let's challenge with 
                //the next one from aIds[] array
                uIndex ++;
                continue;
                }
            else // err == KErrNone
                {
                //Get the value and type
                pSetting = (SSettingF*) settingRef.iSet;
                //again copy the type value into the user array if it's provided
                *aTypes[uIndex] = static_cast<TSettingType>(settingRef.iSet->iType); 
            
				//Check for the found type is this word size? If it's not then 
				//indicate error for this setting
				if(*aTypes[uIndex] > ETypeLinAddr)
					{
					*aErrors[uIndex] = KErrArgument;
					*aValues[uIndex] = 0;
					}
				else
					{
					*aErrors[uIndex] = KErrNone;
					*aValues[uIndex] = pSetting->iValue.iLit.iInt32;
					}
					
                //Break the loop by setting the redefined status
                isRedefined = ETrue;
                }
            }
        
        //At this point we should find at least one element, store this index in the  
        //local variable, this is used later in the code. Please be noticed we've  
        //also got rMinIndex - first setting index in the repository. 
        if(err == KErrNone)
            uFirstIndex = uIndex;
        else
            //if we are hear it means we did not find any user settings at all
            //we can't do any thing and just return KErrNotFound to indicate
            //this fact
            return KErrNotFound;

        
        
        //Now lets find the last setting
        uIndex = aNum - 1;
        isRedefined = EFalse;
        err = KErrNotFound;
        
        while(!isRedefined && uIndex > uFirstIndex)
            {
            //Find the last setting from user array. The importance here is that we   
            //should get value of first setting index in the repository in 
            //rMinIndex. This time the  scope of search is whole repository.
            err = this->FindSetting(*aIds[uIndex],settingRef, rMaxIndex, 
                    rMinIndex, iRepos->iHdr.iNumSettings);

			__NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);
			
            if(err == KErrNotFound)
                {
                *aErrors[uIndex] = err;
                *aValues[uIndex] = 0;
                *aTypes[uIndex] = ETypeUndefined;
                
                //As FindSetting did not find the element, let's challenge with 
                //previous one
                uIndex --;
                continue;
                }
            else //err == KErrNone
                {
                pSetting = (SSettingF*) settingRef.iSet;
                *aTypes[uIndex] = static_cast<TSettingType>(settingRef.iSet->iType); 

				//Check for the found type is this word size? If it's not then indicate
				//error for this setting
				if(*aTypes[uIndex] > ETypeLinAddr)
					{
					*aErrors[uIndex] = KErrArgument;
					*aValues[uIndex] = 0;
					}
				else
					{
					*aErrors[uIndex] = KErrNone;
					*aValues[uIndex] = pSetting->iValue.iLit.iInt32;
					}
					
                isRedefined = ETrue;
                }
            }

        //At this point we found the last setting, store it's user array index in   
        //the local variable, this is used later in the code. Please be noticed   
        //we've also got rMaxIndex - last setting index in the repository.
        if(err == KErrNone)
            uLastIndex = uIndex;
        else
            //if we are here we did not find any other elements than was found
            //in previous iteration then just stop here
            return KErrNotFound;  
        
        //The scope of user array settings in the repository is found. 
        //Let's find all other settings from user array. Bare in mind the low
        //bound for the repository index is increased each iteration to optimize the
        //search time.
        for(uIndex = uFirstIndex + 1; uIndex < uLastIndex; uIndex ++)
            {
            err = this->FindSetting(*aIds[uIndex],settingRef, rIndex, 
                    rMinIndex, rMaxIndex);

			__NK_ASSERT_DEBUG(err == KErrNotFound || err == KErrNone);

            if(err == KErrNotFound)
                {
                *aErrors[uIndex] = err;
                *aValues[uIndex] = 0;
                *aTypes[uIndex] = ETypeUndefined;

                //As FindSetting did not find the element, let's challenge with 
                //another one
                continue;
                }
            else //err == KErrNone
                {

                pSetting = (SSettingF*) settingRef.iSet;
                
                TSettingType type = static_cast<TSettingType>(settingRef.iSet->iType); 
                *aTypes[uIndex] = type; 

                //Check for the found type is this word size? If it's not then indicate
                //error for this setting
                if(type > ETypeLinAddr)
                    {
                    *aErrors[uIndex] = KErrArgument;
                    *aValues[uIndex] = 0;
                    }
                else
                    {
                    *aErrors[uIndex] = KErrNone;
                    *aValues[uIndex] = pSetting->iValue.iLit.iInt32;
                    }

                rMinIndex = rIndex + 1;
                }

            }

        return KErrNone;
    }



void HCR::TRepositoryFile::GetSettingRef(TInt32 aIndex, 
         HCR::TSettingRef& aSetRef)
    {
    __NK_ASSERT_DEBUG(iRepos != 0);
    __NK_ASSERT_DEBUG(iRepos->iHdr.iNumSettings != 0);
    __NK_ASSERT_DEBUG(aIndex >= 0 && aIndex < iRepos->iHdr.iNumSettings);

    SSettingF* arr = (SSettingF*)(iRepos + 1);
    
    aSetRef.iRep = this;
    aSetRef.iSet = &(arr[aIndex].iName);
    }




TInt HCR::TRepositoryFile::FindNumSettingsInCategory(TCategoryUid aCatUid,
        TInt32& aFirst, TInt32& aLast)
    {
    HCR_FUNC("TRepositoryFile::FindNumSettingsInCategory");

    __NK_ASSERT_DEBUG(iRepos != 0);
    
    if(iRepos->iHdr.iNumSettings == 0)
        {
        aFirst = 0;
        aLast = 0;
        HCR_TRACE_RETURN(KErrNotFound);
        }
    
    SSettingF* arr = (SSettingF*) (iRepos+1);
    TInt32 low = 0;
    TInt32 high = iRepos->iHdr.iNumSettings-1;
    TInt32 mid = 0;
    TInt32 com = 0;


    //Let's find any setting within the category, mid will store the setting 
    //index in the repository
    while (low<=high)
        {
        mid = (low+high) >> 1;
        com = CompareByCategory(aCatUid, arr[mid].iName.iId);
        if (com < 0)
            high = mid-1;
        else if (com > 0)
            low = mid+1;
        else
            {
            break;
            }    
        } 

    // If no one setting with the given category was found the return error  
    // to the user
    if(low > high)
        {
        aFirst = 0;
        aLast  = 0;
        return KErrNotFound;
        }

    //Search the first element within the category
    low = mid;
    while(low >= 0 && arr[low].iName.iId.iCat == aCatUid)
        {
        if(low > 0)
            low --;
        else
            break;
        }

    //Check the boundary conditions, there are two cases when we exit the loop
    //either we found an element which category is not one we are looking for or
    //we reach the beggining of the repository. If we reach the beggining of the
    //repository we don't really know is it because this is last elment or it
    //has required aCatUid, so we check these two conditions below
    if(arr[low].iName.iId.iCat == aCatUid)
        aFirst = low;
        
    //We finish the loop either reaching the setting which category id is not
    //what we need or this is first setting in the repository again with another
    //category, so in both case we throw this element from the account.
    else
        aFirst = low + 1;


    //Search the last element within the category
    high = mid;
    while(high <= iRepos->iHdr.iNumSettings - 1 && arr[high].iName.iId.iCat == aCatUid)
        {
        if(high < iRepos->iHdr.iNumSettings - 1)
            high ++;
        else
            break;
        }

    //Same situation as above, boundary conditions
    if(arr[high].iName.iId.iCat == aCatUid)
        aLast = high;
    else
        aLast = high - 1;

    return KErrNone;
    }




TInt HCR::TRepositoryFile::GetValue(const TSettingRef& aRef, UValueWord& aValue)
    {
    HCR_FUNC("TRepositoryFile::GetValue");

    if (!IsWordValue(aRef))
        HCR_TRACE_RETURN(KErrArgument);
        
	SSettingF* sptr = (SSettingF*)(aRef.iSet);
    aValue = sptr->iValue.iLit;
	return KErrNone;
    }


TInt HCR::TRepositoryFile::GetLargeValue(const TSettingRef& aRef, UValueLarge& aValue)
    {
    HCR_FUNC("TRepositoryFile::GetLargeValue");

    if (!IsLargeValue(aRef))
        HCR_TRACE_RETURN(KErrArgument);

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
		HCR_TRACE_RETURN(KErrNoMemory);

	//Call of the "placement" new operator, which constructs the HCR object on 
	//the global memory address defined by gHCR and initialized with the same
	//data given by constructor below
	new(&gHCR) HCR::HCRInternal(varPtr);

	TInt err = HCRSingleton->Initialise();

	if (err != KErrNone)
		HCR_TRACE_RETURN(err);

	return err;
	}
#endif // HCRTEST_NO_KEXT_ENTRY_POINT

// -- Implementation of local functions
#ifndef __WINS__
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
		TInt nameLength = (aEntry->iNameLength)<<1;
		
		// Uncommnet to get dump of ROM data when debugging....
		// HCR_TRACE0("Begin of loop...");
		// HCR_HEX_DUMP_ABS((TUint8 *)aEntry, sizeof(TRomEntry)+(nameLength - 2) );
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
	
#endif // !__WINS__


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
	

#ifdef __WINS__
    TBuf<KMaxFileName> wholeFilePath; 
	void* reposBuf = 0;
    
#ifdef __VC32__
	
#ifdef _DEBUG
	// - wins udeb version
    wholeFilePath.Copy((const TText*)"\\EPOC32\\RELEASE\\WINS\\UDEB\\");
#else
    // - wins urel version
    wholeFilePath.Copy((const TText*)"\\EPOC32\\RELEASE\\WINS\\UREL\\");
#endif

#else

#ifdef _DEBUG
    // - winscw udeb version
    wholeFilePath.Copy((const TText*)"\\EPOC32\\RELEASE\\WINSCW\\UDEB\\");
#else
    // - winscw urel version
    wholeFilePath.Copy((const TText*)"\\EPOC32\\RELEASE\\WINSCW\\UREL\\");
#endif
    
#endif    
    
    for( TInt j = 0; j < nameLen; ++j)
          {
          wholeFilePath.Append( origFileName[j] );
          }
    
    HCR_TRACE3("--- epoc emulator file path: %S (%d/%d)", &wholeFilePath, wholeFilePath.Length(), wholeFilePath.Size());
    
    TInt length = wholeFilePath.Length();
    
    NKern::ThreadEnterCS();
    TCHAR* chFilePath = new TCHAR[length+1];
    NKern::ThreadLeaveCS();
    
    for(int loop=0;loop<length;++loop) 
        {
        chFilePath[loop] = wholeFilePath[loop];
        }
    chFilePath[length] = '\0';
    
    //try to locate file
    WIN32_FIND_DATAW wfd;
    HANDLE hFile = FindFirstFile(chFilePath, &wfd);
    TBool foundFile = EFalse;
    if (hFile == INVALID_HANDLE_VALUE)
        {
        HCR_TRACE0("--- file not found in \\sys\\bin; try \\sys\\data");
        
#ifdef __VC32__
    
#ifdef _DEBUG
        // - wins udeb version
        wholeFilePath.Copy((const TText*)"\\EPOC32\\RELEASE\\WINS\\UDEB\\Z\\sys\\data\\");
#else
        // - wins urel version
        wholeFilePath.Copy((const TText*)"\\EPOC32\\RELEASE\\WINS\\UREL\\Z\\sys\\data\\");
#endif

#else

#ifdef _DEBUG
        // - winscw udeb version
        wholeFilePath.Copy((const TText*)"\\EPOC32\\RELEASE\\WINSCW\\UDEB\\Z\\sys\\data\\");
#else
        // - winscw urel version
        wholeFilePath.Copy((const TText*)"\\EPOC32\\RELEASE\\WINSCW\\UREL\\Z\\sys\\data\\");
#endif
    
#endif  
        
        for( TInt i = 0; i < nameLen; ++i)
            {
            wholeFilePath.Append( origFileName[i] );
            }
            
        HCR_TRACE3("--- epoc emulator file path: %S (%d/%d)", &wholeFilePath, wholeFilePath.Length(), wholeFilePath.Size());
            
        length = wholeFilePath.Length();
        
        NKern::ThreadEnterCS();
        delete[] chFilePath;
        chFilePath = new TCHAR[length+1];
        NKern::ThreadLeaveCS();
        
        for(int loop=0;loop<length;++loop) 
            {
            chFilePath[loop] = wholeFilePath[loop];
            }
        chFilePath[length] = '\0';
        
        hFile = FindFirstFile(chFilePath, &wfd);
        
        if (hFile == INVALID_HANDLE_VALUE)
            {
            HCR_TRACE0("--- file not found in \\sys\\data");
            }
        else
            {
            HCR_TRACE0("--- file found in \\sys\\data");
            foundFile = ETrue;        
            }
        }
    else
        {
        HCR_TRACE0("--- file found in \\sys\\bin");
        foundFile = ETrue;
        }
    
    if(!foundFile)
        {
        // No file found; release memory and return
        NKern::ThreadEnterCS();
        delete[] chFilePath;
        NKern::ThreadLeaveCS();
        
        HCR_TRACE_RETURN(KErrNotFound);
        }
        

    __NK_ASSERT_ALWAYS(wfd.nFileSizeHigh==0);
            
    DWORD num_read = 0;    
    retVal = KErrNone;
    
    NKern::ThreadEnterCS();
    reposBuf = new BYTE[wfd.nFileSizeLow];
    NKern::ThreadLeaveCS();
    
    if(reposBuf == NULL)
		{
        HCR_TRACEMSG_RETURN("--- Error allocating memory for reading file", KErrNoMemory);
		}
    else
        {
        hFile = CreateFile(chFilePath, GENERIC_READ,          // open for reading
                FILE_SHARE_READ,       // share for reading
                NULL,                  // default security
                OPEN_EXISTING,         // existing file only
                FILE_ATTRIBUTE_NORMAL, // normal file
                NULL); 
        
        BOOL read = ReadFile(hFile, reposBuf, wfd.nFileSizeLow, &num_read, NULL);
        if(!read) 
            {
            retVal = GetLastError();  
            HCR_TRACE1("--- Error reading file %d", GetLastError());
            }
        }

    CloseHandle(hFile);
    NKern::ThreadEnterCS();
    delete[] chFilePath;
    NKern::ThreadLeaveCS();
    
    NKern::ThreadEnterCS();
    aRepos = HCR::TRepositoryFile::New(reinterpret_cast<const HCR::SRepositoryFile *>(reposBuf)); 
    NKern::ThreadLeaveCS();
    
    if (aRepos == NULL)
        {
        retVal = KErrNoMemory;
        }
    
    HCR_TRACE_RETURN(retVal);
    
#else
	
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
#ifdef __EPOC32__
			// HCR design requires the core image file repository to be in the
			// unpaged portion of the core ROM image. This check will Fault the
			// kernel startup if this is not found to be the case, perhaps due 
			// to mis-configured obey files.
			// Skipped on emulator builds as Epoc class in platform.h not
			// defined. Hence support for core images not supported. 
			__NK_ASSERT_ALWAYS(ROMAddressIsInUnpagedSection((TLinAddr)entry->iAddressLin));   
#endif
				NKern::ThreadEnterCS();
				aRepos = HCR::TRepositoryFile::New(reinterpret_cast<const HCR::SRepositoryFile *>(entry->iAddressLin));	
				NKern::ThreadLeaveCS();
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
#ifdef __EPOC32__
			// HCR design requires the core image file repository to be in the
			// unpaged portion of the core ROM image. This check will Fault the
			// kernel startup if this is not found to be the case, perhaps due 
			// to mis-configured obey files.
			// Skipped on emulator builds as Epoc class in platform.h not
			// defined. Hence support for core images not supported. 
			__NK_ASSERT_ALWAYS(ROMAddressIsInUnpagedSection((TLinAddr)entry->iAddressLin));   
#endif
				NKern::ThreadEnterCS();
				aRepos = HCR::TRepositoryFile::New(reinterpret_cast<const HCR::SRepositoryFile *>(entry->iAddressLin));
				NKern::ThreadLeaveCS();
				if (aRepos == NULL)
                    retVal = KErrNoMemory;					
				}
			}
		}
	
    HCR_TRACE_RETURN(retVal);
#endif //ifdef __WINS__
    }

TInt LocateCoreImgRepository(HCR::TRepository*& aRepos)
    {
    HCR_FUNC("LocateCoreImgRepository");

#ifdef HCRTEST_COREIMG_DONTUSE_ROMHDR
    
    // Use this testing more on Emulator platform
    // and on hardware when ROM Header is not to be used or not implemented
    
	const TText8* hcrfile = (const TText8*) "hcr.dat";
	TInt retVal = SearchCoreImgRepository(aRepos, hcrfile);
	if (retVal != KErrNone)
		return retVal;
	
#else

	const TRomHeader& romHeader = Epoc::RomHeader(); 	// 0x80000000;
	HCR_TRACE2("--- ROM Header: 0x%08x, HCR file address: 0x%08x", &romHeader, romHeader.iHcrFileAddress);
	
	if(romHeader.iHcrFileAddress != 0)
			{
#ifdef __EPOC32__
			// HCR design requires the core image file repository to be in the
			// unpaged portion of the core ROM image. This check will Fault the
			// kernel startup if this is not found to be the case, perhaps due 
			// to mis-configured obey files.
			// Skipped on emulator builds as Epoc class in platform.h not
			// defined. Hence support for core images not supported. 
			__NK_ASSERT_ALWAYS(ROMAddressIsInUnpagedSection((TLinAddr)romHeader.iHcrFileAddress));   
#endif
			NKern::ThreadEnterCS();
			aRepos = HCR::TRepositoryFile::New(reinterpret_cast<const HCR::SRepositoryFile *>(romHeader.iHcrFileAddress));
			NKern::ThreadLeaveCS();
			if (aRepos == 0)
				return KErrNoMemory;
			}
	else
		return KErrNotFound;
		
#endif // HCRTEST_COREIMG_DONTUSE_ROMHDR


	return KErrNone;
    }
	
