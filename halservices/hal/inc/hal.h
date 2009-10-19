// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// hal\inc\hal.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __HAL_H__
#define __HAL_H__

#include <e32def.h>
#include <hal_data.h>
#include <e32property.h>




/**
@publishedPartner
@released

A set of static functions to get and set hardware attributes.

@see HALData
*/
class HAL : public HALData
	{
public:

    /**
    Synonyms for the attribute properties
    HALData::TAttributeProperty, and used in SEntry.
    */
	enum TEntryProperty
		{
		/**		
		When set, means that an attribute is meaningful on this device.
		*/
		EEntryValid=0x1,
		
		
		/**
		When set, means that an attribute is modifiable.
		*/
		EEntryDynamic=0x2,
		};

    
    /**
    Defines an entry in the array that is returned in a call to HAL::GetAll().
    */
	struct SEntry
		{
		/**
		The properties of the attribute.
		
		@see HAL::TEntryProperty
		*/
		TInt iProperties;
		
		/**
		The attribute value.
		
		@see HALData::TAttribute
		*/
		TInt iValue;
		};
public:
    /**
    Gets the value of the specified HAL attribute.

    @param aAttribute The HAL attribute.
    @param aValue	On successful return, contains the attribute value.
					Some attributes may accept aValue as an input as well, to select
					one of several alternate values. See the documentation for the
					individual HAL attributes for details of this.

    @return  KErrNone, if successful;
             KErrNotSupported, if the attribute is not defined in the list
             of attributes, or is not meaningful for this device.
			 KErrArgument, if aValue was invalid (for attributes
			 which take an argument). 
         
    @see HALData::TAttribute
    @see HALData::TAttributeProperty
    */
	IMPORT_C static TInt Get(TAttribute aAttribute, TInt& aValue);

	
	/**
    Sets the specified HAL attribute.

    @param aAttribute The HAL attribute.
    @param aValue      The attribute value.

    @return  KErrNone, if successful;
             KErrNotSupported, if the attribute is not defined in the list
             of attributes, or is not meaningful for this device, or is
             not settable.
         
    @see HALData::TAttribute
    @see HALData::TAttributeProperty

    @capability WriteDeviceData or other capability specified
    for individual attributes in TAttribute
    */
	IMPORT_C static TInt Set(TAttribute aAttribute, TInt aValue);


    /**
    Gets all HAL attributes, and their properties.

    For attributes that are not meaningful on this device (ie. those which have
	not been defined in the config.hcf file),
	the attribute value and its associated property value are set to zero in
	the returned array.

	Attributes for which multiple values can be retrieved
	ie. EDisplayIsPalettized, EDisplayBitsPerPixel, EDisplayOffsetToFirstPixel,
	EDisplayOffsetBetweenLines, and EDisplayPaletteEntry will also be zero in
	the returned array.

    @param aNumEntries On successful return, contains the total number
                       of HAL attributes.
                       If the function returns KErrNoMemory, this value is set
                       to zero.
    @param aData       On successful return, contains a pointer to an array
                       of SEntry objects, each of which contains an attribute value
                       and its property value. Note that the property value is
                       defined by the HAL::TEntry synonym.
                       If the function returns KErrNoMemory, this pointer is set
                       to NULL.

    @return KErrNone, if succesful;
            KErrNoMemory, if there is insufficient memory. 
    */
	IMPORT_C static TInt GetAll(TInt& aNumEntries, SEntry*& aData);

	
    /**
    Gets the value of the specified HAL attribute.

    @param aDeviceNumber The device number. (eg: screen number)
    @param aAttribute The HAL attribute.
    @param aValue	On successful return, contains the attribute value.
					Some attributes may accept aValue as an input as well, to select
					one of several alternate values. See the documentation for the
					individual HAL attributes for details of this.


    @return  KErrNone, if successful;
             KErrNotSupported, if the attribute is not defined in the list
             of attributes, or is not meaningful for this device.
			 KErrArgument, if aValue was invalid (for attributes
			 which take an argument). 
         
    @see HALData::TAttribute
    @see HALData::TAttributeProperty
    */
	IMPORT_C static TInt Get(TInt aDeviceNumber, TAttribute aAttribute, TInt& aValue);
	
	
    /**
    Sets the specified HAL attribute.

    @param aDeviceNumber The device number. (eg: screen number)
    @param aAttribute The HAL attribute.
    @param aValue      The attribute value.

    @return  KErrNone, if successful;
             KErrNotSupported, if the attribute is not defined in the list
             of attributes, or is not meaningful for this device, or is
             not settable.
         
    @see HALData::TAttribute
    @see HALData::TAttributeProperty

    @capability WriteDeviceData or other capability specified
    for individual attributes in TAttribute
    */
	IMPORT_C static TInt Set(TInt aDeviceNumber, TAttribute aAttribute, TInt aValue);
	};


/**
@internalComponent
*/
static const TInt32 KUidHalPropertyKeyBase = 0x1020E306;

__ASSERT_COMPILE(HAL::ENumHalAttributes<256); // only 256 UIDs allocated for HAL property keys



#endif
