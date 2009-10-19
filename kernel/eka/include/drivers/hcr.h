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


/** 
@file hcr.h
Kernel side API for Hardware Configuration Repository (HCR).


===============================================================
 ____            _        _                    
|  _ \ _ __ ___ | |_ ___ | |_ _   _ _ __   ___ 
| |_) | '__/ _ \| __/ _ \| __| | | | '_ \ / _ \
|  __/| | | (_) | || (_) | |_| |_| | |_) |  __/
|_|   |_|  \___/ \__\___/ \__|\__, | .__/ \___|
                              |___/|_|         

This API and component are in an early release form. As such
this component, it's API/HAI interfaces and internal design 
are not fixed and may be updated/changed at any time before 
final release.

===============================================================


@publishedPartner
@prototype
*/




#ifndef HCR_H
#define HCR_H


// -- INCLUDES ----------------------------------------------------------------


#include <e32err.h>
#include <e32def.h> 
#include <e32cmn.h> 
#include <e32des8.h>


// -- CLASSES -----------------------------------------------------------------

/**
The HCR namespace contains all the types and APIs that make up the
Kernel side Hardware Configuration Repository (HCR).
It provides accessor functions to settings held by the HCR and may be used by 
kernel side clients such as physical device drivers and other services from
thread contexts.
The published Setting IDs available for use with this API can be found
in the BSP exported header 'hcrconfig.h'. This provides the top-level header
that clients can include to gain access to all IDs for the BSP.
*/
namespace HCR
    {

    /** Maximum length of a large setting type, in bytes */     
    static const TInt KMaxSettingLength = 512;


    /** Setting category identifier type */
    typedef TUint32 TCategoryUid;
    
    /** Setting element identifier type */
    typedef TUint32 TElementId;
        
    /** The setting Identifier structure. Used to create static initialised
    arrys for use with multiple setting retrieval calls.	
	*/    
    struct SSettingId
	    {
        TCategoryUid iCat;		//!< Allocated UID for setting category
        TElementId iKey;		//!< Element indetifier for setting in category
		};
		
    /** The setting Identifier type. A class used to uniquely identify a 
    setting in the HCR. Used in calls to HCR API. 
	*/
    class TSettingId
    	{
    public:
    	TSettingId ()
		 { iCat = iKey = 0; };
    	TSettingId (TCategoryUid aCat, TElementId aKey)
    	 { iCat = aCat; iKey = aKey; };
    	TSettingId (const SSettingId& aId)
    	 { iCat = aId.iCat; iKey = aId.iKey; };
		TSettingId& operator= (const SSettingId& rhs)
		 { iCat = rhs.iCat; iKey = rhs.iKey; return *this; }  
		   
        /** The allocated UID identifying the category the setting belongs too */
        TCategoryUid iCat;
        
        /** The integer key identifying the setting element in the category */
        TElementId iKey;
    	};
    
    /** The setting types supported. The types are shown in two groups: Word 
    size - a maximum of 4 bytes; and ii) Large size - types exceeding 4 bytes 
    in size.
	*/
    enum TSettingType
    	{
        ETypeUndefined  = 0,            //!< Type unknown/not set
        
        // Word size settings
        ETypeInt32      = 0x00000001,   //!< 32bit signed integer
        ETypeInt16      = 0x00000002,   //!< 16bit signed integer
        ETypeInt8       = 0x00000004,   //!< 8bit signed integer
        ETypeBool       = 0x00000008,   //!< 32bit boolean value
        ETypeUInt32     = 0x00000010,   //!< 32bit unsigned integer
        ETypeUInt16     = 0x00000020,   //!< 16bit unsigned integer
        ETypeUInt8      = 0x00000040,   //!< 8bit unsigned integer
        ETypeLinAddr    = 0x00000100,   //!< 32bit virtual address
        
        // Large settings
        ETypeBinData   		= 0x00010000,   //!< Raw binary data (TUint8 array)
        ETypeText8     		= 0x00020000,   //!< String data (TText8 array) 
		ETypeArrayInt32 	= 0x00040000,	//!< 32bit signed integer array   
		ETypeArrayUInt32	= 0x00080000,	//!< 32bit unsigned integer array
        ETypeInt64     		= 0x01000000,   //!< 64bit signed integer
        ETypeUInt64    		= 0x02000000,   //!< 64bit unsigned integer    
     	};


	/**
    Retrieve settings of built in types from the HCR.
    
    @param aId     The setting identifier
    @param aValue  The retrieved setting data value  
    
	@return	KErrNone if successful, output parameters valid
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace.

	@pre    Call from thread context, during Init1 or later
	*/
	IMPORT_C TInt GetInt(const TSettingId& aId, TInt8& aValue);
	IMPORT_C TInt GetInt(const TSettingId& aId, TInt16& aValue);
	IMPORT_C TInt GetInt(const TSettingId& aId, TInt32& aValue);
    IMPORT_C TInt GetInt(const TSettingId& aId, TInt64& aValue);
	
	IMPORT_C TInt GetBool(const TSettingId& aId, TBool& aValue);
	
	IMPORT_C TInt GetUInt(const TSettingId& aId, TUint8& aValue);
    IMPORT_C TInt GetUInt(const TSettingId& aId, TUint16& aValue);
    IMPORT_C TInt GetUInt(const TSettingId& aId, TUint32& aValue);
    IMPORT_C TInt GetUInt(const TSettingId& aId, TUint64& aValue);

    IMPORT_C TInt GetLinAddr(const TSettingId& aId, TLinAddr& aValue);
    
	/**
    Retrieve a binary data (ETypeBinData) setting from the HCR.

    @param aId     The setting identifier
    @param aMaxLen The maximum value length that can be stored in the buffer
    @param aValue  A pointer to the buffer or a descriptor to hold the value
    @param aLen    Contains the length of the setting value written

    
	@return	KErrNone if successful and aValue has been set
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrTooBig if the setting is larger than the supplied buffer
            KErrGeneral if an internal failure occurs, see trace

	@pre    Call from thread context, during Init1 or later
	*/
    IMPORT_C TInt GetData(const TSettingId& aId, TDes8& aValue);
    IMPORT_C TInt GetData(const TSettingId& aId, TUint16 aMaxLen, 
                                    TUint8* aValue, TUint16& aLen);        
	/**
    Retrieve a character string (ETypeText8) setting from the HCR.
    
    @param aId     The setting identifier
    @param aMaxLen The maximum value length that can be stored in the buffer
    @param aValue  A pointer to the buffer or a descriptor to hold the value
	@param aLen    Contains the length of the setting value written    

	@return	KErrNone if successful and aValue has been set
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrTooBig if the setting is larger than the supplied buffer
            KErrGeneral if an internal failure occurs, see trace

	@pre    Call from thread context, during Init1 or later
	*/
    IMPORT_C TInt GetString(const TSettingId& aId, TDes8& aValue);
    IMPORT_C TInt GetString(const TSettingId& aId, TUint16 aMaxLen, 
                                    TText8* aValue, TUint16& aLen);
                                                                        
	/**
    Retrieve an array setting from the HCR. All value length paramters are 
	measured in bytes.

    @param aId     The setting identifier
    @param aMaxLen The maximum value length that can be stored in the buffer
    @param aValue  A pointer to the buffer to hold the value
    @param aLen    Contains the length of the setting value written
    
	@return	KErrNone if successful and aValue has been set
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrTooBig if the setting is larger than the supplied buffer
            KErrGeneral if an internal failure occurs, see trace

	@pre    Call from thread context, during Init1 or later
	*/
    IMPORT_C TInt GetArray(const TSettingId& aId, TUint16 aMaxLen, 
                                    TInt32* aValue, TUint16& aLen);        
    IMPORT_C TInt GetArray(const TSettingId& aId, TUint16 aMaxLen, 
                                    TUint32* aValue, TUint16& aLen);   
									     
    /**
    Retrieve multiple simple settings from the Hardware Configuration 
    Repository in one call. This method can be used for all settings of size 4 
    byes or less (i.e those with a type in 0x0000ffff).
    
    @param aNum     in: The number of settings to retrieve. It is also the 
                    size of the arrays in the following arguments
    @param aIds     in:  An ordered array of setting identifiers to retrieve, lowest first
    @param aValues  out: An array of values, populated on exit
    @param aTypes   out: An optional array of type enumerations describing 
                    the type of each setting found. May be 0 if client is 
                    not interested
    @param aErrors  out: An optional array of return codes to describe the 
                    result of the lookup for each setting. May be 0 if 
                    client is not interested
    
	@return	KErrNone if successful and all values have been retrieved
			KErrArgument if one of the arguments is incorrect.
            KErrNotFound if one or more setting IDs is not known 
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace
            KErrNotSupported if method is not supported
                
	@pre    Call from thread context, during Init1 or later
	*/    
    IMPORT_C TInt GetWordSettings(TInt aNum, const SSettingId aIds[], 
            TInt32 aValues[], TSettingType aTypes[], 
                                     TInt aErrors[]);

    /**
    Retrieve the type and size of a HCR setting. Can be used by clients to 
    obtain the setting size if a dynamic buffer is to be used.
    
    @param aId     The setting identifier
    @param aType   The type enumeration of found setting
    @param aLen    The length in bytes of found setting
        
	@return	KErrNone if successful, output parameters valid
            KErrNotFound if aId is not a known setting ID
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace
            KErrNotSupported if method is not supported

	@pre    Call from thread context, during Init1 or later
    */    
    IMPORT_C TInt GetTypeAndSize(const TSettingId& aId, 
                                        TSettingType& aType, TUint16& aLen);
                                        
    /**
    Retrieve the number of settings held in the HCR for one particular category.
	It allows a client to perpare buffers for other calls to the HCR to 
	retrieve these settings.
	This search method will return the total number of setting records found 
	across all HCR repositories for a given category. It does not apply the 
	override rules of other routines meaning that it counts duplicates to
	maintain performance.
    
    @param aCatUid	in: The setting identifier category to use in the search
        
	@return	Zero or positive number of settings found in category, -ve on error
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace
            KErrNotSupported if method is not supported

	@pre    Call from thread context, during Init1 or later
    */ 
	IMPORT_C TInt FindNumSettingsInCategory (TCategoryUid aCatUid);
	 
    /**
    Retrieve all the setting ids, types and sizes in one particular
	category. Can be used by clients to obtain the number, size and types of 
	all the settings in a category. It allows a client to alloc buffers for 
	other calls to the HCR to retrieve these settings.
    
    @param aCatUid	 in: The setting identifier category to use in the search
    @param aMaxNum   in: The maximum number of settings to return. It is also 
                         the size of the arrays in the following arguments   
    
    @param aNumFound out: The number of settings found 
    @param aElIds    inout: Client supplied array populated on exit. Large
						    enough to hold all elements in category.
    @param aTypes	 inout: Client supplied array populated with setting types 
						    enumerations on exit. May be 0 if client is 
                            not interested.
    @param aLen  	 inout: Client supplied array populated with setting lengths
						    on exit. May be 0 if client is not interested.
        
	@return	Zero or positive number of settings found in category, -ve on error
            KErrOverflow if ok but with more settings than aMaxNum were found
			KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace
            KErrNotSupported if method is not supported

	@pre    Call from thread context, during Init1 or later
    */ 
    IMPORT_C TInt FindSettings(TCategoryUid aCatUid, 
					TInt aMaxNum, TUint32& aNumFound, 
					TElementId* aElIds, TSettingType* aTypes, TUint16* aLens);
                                       
    /** 
    Finds multiple settings in the Hardware Configuration Repository who's
    setting ID matches the supplied search bit pattern. This method is useful 
    for categories that contain structured settings i.e. row/column structured 
    or record based categories as might be the case with hardware service
    providers.
    
    The caller supplies the category to search, an element ID mask and the 
    pattern to match. SettingIDs that satisfy this logic are returned:
    ((elementID & aElementMask) == (aPattern & aElementMask))
    
    For example, a set of driver capability structures might be encoded into
    an element ID where the 24 MSb are the row/record number and the 8 LSb 
    are the column/field index. Thus to retrieve all fields in row 2 supply: 
        aElemMask = 0xffffff00, aPattern = 0x000002** 
    to retrieve key fields of all records supply:
        aElemMask = 0x000000ff, aPattern = 0x******01
    (* = dont care)
   
    @param aCat      in: The category to retrieve settings for
    @param aMaxNum   in: The maximum number of settings to retrieve. It is also 
                         the size of the arrays in the following arguments   
    @param aAtId     in: The Minimum element ID to commence the search at. 
                         Used when retrieving settings in batches.
    @param aElemMask in: Element ID mask.
    @param aPattern  in: Identifies the set of fieldy to return in the search.
						                   
    @param aNumFound out: The number of settings found 
    @param aElIds    inout: Client supplied array populated on exit. Large
						    enough to hold aMaxNum element ids.
    @param aTypes    inout: Client supplied array populated with setting types 
						    enumerations on exit. May be 0 if client is 
                            not interested.
    @param aLen  	 inout: Client supplied array populated with setting lengths
						    on exit. May be 0 if client is not interested.
    
	@return	Zero or positive number of settings found in category, -ve on error
            KErrOverflow if ok but with more settings than aMaxNum were found 
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace
            KErrNotSupported if method is not supported
         
	@pre    Call from thread context, during Init1 or later
	*/    
    IMPORT_C TInt FindSettings(TCategoryUid aCat, 
					TInt aMaxNum, TUint32 aAtId,
                    TUint32 aMask, TUint32 aPattern, TUint32& aNumFound,
                    TElementId* aElIds, TSettingType* aTypes, TUint16* aLens);
     
}

#endif // HCR_H

