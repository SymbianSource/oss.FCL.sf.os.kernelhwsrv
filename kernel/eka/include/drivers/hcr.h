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
Kernel side client API for Hardware Configuration Repository (HCR). 
The HCR service provides access to hardware settings defined for the base port. 
This API is used by kernel side components such as PDDs, hardware service 
providers and other kernel extensions.

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
client API of the Kernel Hardware Configuration Repository (HCR).
It provides accessor functions to retrieve settings held by the HCR and may be 
called by kernel components from with in thread context. 

The _published_ Setting IDs available for use with this API can be found
in the BSP exported header 'hcrconfig.h'. This provides the top-level header
that clients can include to gain access to all IDs for the BSP. IDs for settings
that are internal to a component and not used by others are defined in a file
private to that component.

The HCR supports a number of setting repositories and searches them in a defined
order, always returns the first setting found matching the ID or criteria.
This allows setting values to be overriden by more recent repositories created
during platform development and product creation.
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
    setting in the HCR. Used in calls to HCR API to retrieve one setting. 
	*/
    class TSettingId : public SSettingId
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
    Retrieve a word size integer setting value from the HCR.
    On error aValue is undefined.
        
    @param aId     in: The setting identifier
    @param aValue  out: The retrieved setting data value  
    
	@return	KErrNone if successful, output parameters valid
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace.
            Otherwise one of the other system-wide error codes. 

	@pre    Call from thread context, during Init1 or later
	*/
	IMPORT_C TInt GetInt(const TSettingId& aId, TInt8& aValue);
	IMPORT_C TInt GetInt(const TSettingId& aId, TInt16& aValue);
	IMPORT_C TInt GetInt(const TSettingId& aId, TInt32& aValue);
    IMPORT_C TInt GetInt(const TSettingId& aId, TInt64& aValue);
	
	/**
    Retrieve a boolean setting value from the HCR.
    On error aValue is undefined.
    
    @param aId     in: The setting identifier
    @param aValue  out: The retrieved setting data value  
    
	@return	KErrNone if successful, output parameters valid
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace.
            Otherwise one of the other system-wide error codes. 

	@pre    Call from thread context, during Init1 or later
	*/
	IMPORT_C TInt GetBool(const TSettingId& aId, TBool& aValue);
	
	/**
    Retrieve an word size unsigned integer setting value from the HCR.
    On error aValue is undefined.
        
    @param aId     in: The setting identifier
    @param aValue  out: The retrieved setting data value  
    
	@return	KErrNone if successful, output parameters valid
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace.
            Otherwise one of the other system-wide error codes. 

	@pre    Call from thread context, during Init1 or later
	*/
	IMPORT_C TInt GetUInt(const TSettingId& aId, TUint8& aValue);
    IMPORT_C TInt GetUInt(const TSettingId& aId, TUint16& aValue);
    IMPORT_C TInt GetUInt(const TSettingId& aId, TUint32& aValue);
    IMPORT_C TInt GetUInt(const TSettingId& aId, TUint64& aValue);

	/**
    Retrieve a word size linear address setting value from the HCR.
    On error aValue is undefined.
        
    @param aId     in: The setting identifier
    @param aValue  out: The retrieved setting data value  
    
	@return	KErrNone if successful, output parameters valid
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace.
            Otherwise one of the other system-wide error codes. 

	@pre    Call from thread context, during Init1 or later
	*/
    IMPORT_C TInt GetLinAddr(const TSettingId& aId, TLinAddr& aValue);
    
    
	/**
    Retrieve a large binary data setting value from the HCR. The value
	is copied into the supplied descriptor buffer. 
	On error the descriptor and output arguments have undefined values.

    @param aId     in: The setting identifier
    @param aValue  inout: A pre-allocated descriptor to hold the value
    
	@return	KErrNone if successful and aValue has been set
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrTooBig if the setting is larger than the supplied buffer
            KErrGeneral if an internal failure occurs, see trace
            Otherwise one of the other system-wide error codes.

	@pre    Call from thread context, during Init1 or later
	*/
    IMPORT_C TInt GetData(const TSettingId& aId, TDes8& aValue);
    
	/**
    Retrieve a large binary data setting value from the HCR. The value is copied
	into the supplied byte array buffer. 
	On error the buffer and output arguments have undefined values.

    @param aId     in: The setting identifier
    @param aMaxLen in: The maximum value length that can be stored in the buffer
    @param aValue  inout: The address of a pre-allocated buffer to hold the value
    @param aLen    out: Contains the length of the setting value written
    
	@return	KErrNone if successful and aValue has been set
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrTooBig if the setting is larger than the supplied buffer
            KErrGeneral if an internal failure occurs, see trace
            Otherwise one of the other system-wide error codes.

	@pre    Call from thread context, during Init1 or later
	*/
    IMPORT_C TInt GetData(const TSettingId& aId, TUint16 aMaxLen, 
                                    TUint8* aValue, TUint16& aLen);   
									     
	/**	
    Retrieve an 8 bit character string setting from the HCR.  The value
	is copied into the supplied descriptor buffer. Note the string is not zero
	terminated. 
	On error the descriptor and output arguments have undefined values.
    
    @param aId     in: The setting identifier
    @param aValue  inout: A pre-allocated descriptor to hold the value

	@return	KErrNone if successful and aValue has been set
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrTooBig if the setting is larger than the supplied buffer
            KErrGeneral if an internal failure occurs, see trace
            Otherwise one of the other system-wide error codes.

	@pre    Call from thread context, during Init1 or later
	*/
    IMPORT_C TInt GetString(const TSettingId& aId, TDes8& aValue);
									     
	/**	
    Retrieve an 8 bit character string setting from the HCR.  The value
	is copied into the byte array buffer. Note the string is not zero
	terminated. 
	On error the descriptor and output arguments have undefined values.
    
    @param aId     in: The setting identifier
    @param aMaxLen in: The maximum value length that can be stored in the buffer
    @param aValue  inout: The address of a pre-allocated buffer to hold the value
	@param aLen    out: Contains the length of the setting value written    

	@return	KErrNone if successful and aValue has been set
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrTooBig if the setting is larger than the supplied buffer
            KErrGeneral if an internal failure occurs, see trace
            Otherwise one of the other system-wide error codes.

	@pre    Call from thread context, during Init1 or later
	*/
    IMPORT_C TInt GetString(const TSettingId& aId, TUint16 aMaxLen, 
                                    TText8* aValue, TUint16& aLen);
                                                                        
	/**
    Retrieve an array of signed integers from the HCR. The value
	is copied into the byte array buffer. 
	On error the descriptor and output arguments have undefined values.

    @param aId     in: The setting identifier
    @param aMaxLen in: The maximum value length that can be stored in the buffer
    @param aValue  inout: The address of a pre-allocated word array to hold the value
    @param aLen    out: Contains the length, in bytes of the setting value written
    
	@return	KErrNone if successful and aValue has been set
            KErrNotFound if aId is not a known setting ID
            KErrArgument if the setting identified is not the correct type
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrTooBig if the setting is larger than the supplied buffer
            KErrGeneral if an internal failure occurs, see trace
            Otherwise one of the other system-wide error codes.

	@pre    Call from thread context, during Init1 or later
	*/
    IMPORT_C TInt GetArray(const TSettingId& aId, TUint16 aMaxLen, 
                                    TInt32* aValue, TUint16& aLen);        
    IMPORT_C TInt GetArray(const TSettingId& aId, TUint16 aMaxLen, 
                                    TUint32* aValue, TUint16& aLen);   
									     
    /**
    Retrieve multiple word sized settings from the Hardware Configuration 
    Repository in one call. This method can be used for all settings of size 4 
    bytes or less (i.e those with a type less than 0x0000ffff).
    The caller is responsible for pre-allocating the arrays supplied. Note the
    array of setting IDs (aIds) supplied by the client must be ordered with 
	aIds[0] containing the lowest and aIds[aNum-1] the highest. Undefined 
	behaviour will result if this pre-condition is not met.
	
	On successful return the client will need to check the number found (return
	value) matches their needs and cast each value in the aValues
	array to the correct type before use. The correct type is either known at 
	compile time by the caller or determined from aTypes, if supplied.
	
   	When an overall error is returned from the function the output arrays have 
	undefined values.

    @param aNum     in: The number of settings to retrieve. It is also the 
                    size of the arrays in the following arguments
    @param aIds     in:  An ordered array of setting identifiers to retrieve
    @param aValues  inout: An array of values, populated on exit
    @param aTypes   inout: An optional array of type enumerations, populated on
					exit describing the type of each setting found. 
					May be 0 if client is not interested
    @param aErrors  inout: An array of search errors for each setting populated 
					on exit. If no error found for the setting then KErrNone
					is written. Possible error codes:
                    KErrArgument     the setting is not of a suitable type
                    KErrNotFound     the setting is not found
                    KErrNone         when setting found
                       
    
	@return	Zero or positive number of settings found, -ve on error
            KErrArgument    if some parameters are wrong(i.e. aErrors is a null
                            pointer, aNum is negative and so on)
            KErrCorrupt     if HCR finds a repository to be corrupt
            KErrGeneral     if an internal failure occurs, see trace
            KErrNotReady    if the HCR is used before it has been initialised
            KErrNoMemory    if the memory allocation within this function failed
            Otherwise one of the other system-wide error codes.
                
	@pre    Call from thread context, during Init1 or later
	*/   
    IMPORT_C TInt GetWordSettings(TInt aNum, const SSettingId aIds[], 
            TInt32 aValues[], TSettingType aTypes[], TInt aErrors[]);
    

    /**
    Retrieve the type and size of a HCR setting. Can be used by clients to 
    obtain the setting size should a buffer need to be allocated.
    On error the output arguments are undefined.    
    
    @param aId     in: The setting identifier
    @param aType   out: The type enumeration of the found setting
    @param aLen    out: The length in bytes of the found setting
        
	@return	KErrNone if successful, output parameters valid
            KErrNotFound if aId is not a known setting ID
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace
            Otherwise one of the other system-wide error codes.

	@pre    Call from thread context, during Init1 or later
    */    
    IMPORT_C TInt GetTypeAndSize(const TSettingId& aId, 
                                        TSettingType& aType, TUint16& aLen);
                                        
    /**
    Retrieve the number of unique ettings held in the HCR for one particular 
	category. It allows a client to perpare buffers for other calls to the HCR 
	to retrieve these settings. 
	The method carries out a search to return the total number of unique setting
	records found across all HCR repositories for a given category. It does not 
	count settings that are duplicate from being redefined in different 
	repositories.
	The function is particularly useful for open-ended categories were the 
	run-time client can not predict the number of settings prvisioned. 
	
    @param aCatUid	in: The setting identifier category to use in the search
        
	@return	Zero or positive number of settings found in category, -ve on error
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt if HCR finds a repository to be corrupt
            KErrGeneral if an internal failure occurs, see trace
            Otherwise one of the other system-wide error codes.

	@pre    Call from thread context, during Init1 or later
    */ 
	IMPORT_C TInt FindNumSettingsInCategory (TCategoryUid aCatUid);
	
	/**
    Retrieve details of all the settings (ids, types and sizes) in one 
	particular category. This function can be used by clients to obtain the 
	number of, ids, sizes and types of all the settings in a category. 
	It allows a client to alloc buffers for other calls to the HCR to retrieve 
	the values of these settings.
	
   	On successful return the client will need to check the number found (return
	value) matches the expected number. When there are more defined in
	the category than was able to be returned, i.e. when number found 
	exceeded aMaxNum then aMaxNum is returned.
	
   	When an overall error is returned from the function the output arrays have 
	undefined values.

    @param aCat  	 in: The setting category to search for
    @param aMaxNum   in: The maximum number of settings to return. It is also 
                         the size of the arrays in the following arguments 
    @param aKeyIds   inout: Client supplied array populated on exit. Large
						    enough to hold all elements in category.
    @param aTypes	 inout: Client supplied array populated with setting types 
						    enumerations on exit. Array address may be 0 if 
                            client is not interested.
    @param aLens  	 inout: Client supplied array populated with setting lengths
						    on exit for those settings with a type > 0x0000ffff. 
							When less than this 0 is set in the aLens array element. 
							Array address may be 0 if client is not interested.
        
	@return	Zero or positive number of settings found in category, -ve on error
			KErrArgument if some parameters are wrong(i.e. aErrors is a null
                            pointer, aNum is negative and so on)
			KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt  if HCR finds a repository to be corrupt
            KErrGeneral  if an internal failure occurs, see trace
            Otherwise one of the other system-wide error codes.

	@pre    Call from thread context, during Init1 or later
    */ 
    IMPORT_C TInt FindSettings(TCategoryUid aCatUid, TInt aMaxNum,
					TElementId aKeyIds[], TSettingType aTypes[], TUint16 aLens[]);
                                       
    /** 
    Retrieve details of all the settings (ids, types and sizes) in one 
	particular category who's key ID matches the supplied bit pattern/mask.
	This function can be used by clients to obtain the number of, ids, sizes 
	and types of all the settings in a category. It allows a client to alloc 
	buffers for other calls to the HCR to retrieve the values of these settings. 
	
	This search method allows categories to contain structured settings 
	i.e. row/column structured or record based categories as might be used
	for configuration data of a hardware service provider.
        
    The caller supplies the category to search, a setting key ID mask and the 
    pattern to match. Setting keys that satisfy this logic are returned:
    ((elementID & aElementMask) == (aPattern & aElementMask))
    
    For example, a set of driver capability structures might be encoded into
    an element ID where the 24 MSb are the row/record number and the 8 LSb 
    are the column/field index. Thus to retrieve all fields in row 2 supply: 
        aElemMask = 0xffffff00, aPattern = 0x000002** 
    to retrieve key fields of all records supply:
        aElemMask = 0x000000ff, aPattern = 0x******01
    (* = dont care)
    
   	On successful return the client will need to check the number found (return
	value) matches the expected number. When there are more defined in
	the category than was able to be returned, i.e. when number found 
	exceeded aMaxNum then aMaxNum is returned.
	
   	When an overall error is returned from the function the output arrays have 
	undefined values.
   
    @param aCat      in: The category to retrieve settings for
    @param aMaxNum   in: The maximum number of settings to retrieve. It is also 
                         the size of the arrays in the following arguments   
    @param aMask     in: The bits in the Element ID to be checked against 
                         aPattern
    @param aPattern  in: Identified the bits that must be set for a 
                         setting to be returned in the search
    @param aKeyIds   inout: Client supplied array populated on exit. Large
						    enough to hold aMaxNum element ids.
    @param aTypes	 inout: Client supplied array populated with setting types 
						    enumerations on exit. Array address may be 0 if 
                            client is not interested.
    @param aLens  	 inout: Client supplied array populated with setting lengths
						    on exit for those settings with a type > 0x0000ffff. 
							When less than this 0 is set in the aLens array element. 
							Array address may be 0 if client is not interested.
    
	@return	Zero or positive number of settings found in category, -ve on error
            KErrArgument if some parameters are wrong(i.e. aErrors is a null
                            pointer, aNum is negative and so on) 
            KErrNotReady if the HCR is used before it has been initialised
            KErrCorrupt  if HCR finds a repository to be corrupt
            KErrGeneral  if an internal failure occurs, see trace
            KErrNoMemory if the memory allocation within this function failed
            Otherwise one of the other system-wide error codes.
            
         
	@pre    Call from thread context, during Init1 or later
	*/    
    IMPORT_C TInt FindSettings(TCategoryUid aCat, TInt aMaxNum, 
					TUint32 aMask, TUint32 aPattern, TElementId aKeyIds[], 
					TSettingType aTypes[], TUint16 aLens[]);
     
}

#endif // HCR_H

