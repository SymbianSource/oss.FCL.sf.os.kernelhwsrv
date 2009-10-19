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
@file hcr_hai.h
Kernel side definitions for the HCR Hardware Abstraction Interface for variants
to implement when creating a HCR.dll binary.


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

#ifndef HCR_HAI_H
#define HCR_HAI_H


// -- INCLUDES ----------------------------------------------------------------


#include <e32def.h>
#include <e32err.h> 

#include <drivers/hcr.h>

/** Macro used in PSL source as the value for the finger print field in a
compiled repository.
@see SRepositoryBase::iFingerPrint
*/
#define HCR_FINGER_PRINT {'H', 'C', 'R'}


// -- CLASSES -----------------------------------------------------------------


/**
The HCR namespace contains all the types and classes that make up the
Kernel side Hardware Configuration Repository (HCR).
*/
namespace HCR
    {
	/** Constant defined for the first HCR repository format. Used for both 
	compiled and file repositories. 	
	@see SRepositoryBase::iFormatVersion	
	*/	    
    static const TInt KRepositoryFirstVersion = 0x0001;
    
    /** Interface class defining the methods variants must implement to provide
    a complete HCR component for the targeted variant.  
    The HCR supports three repositories and it is recommended that as few of 
    these are employed for a variant to minimise lookup overheads as setting 
    override flexibility is provided at the expense of lookup performance.       
    */
    class MVariant
        {
    public:
    
        /** 
        Perform platform specific initialisation of variant HCR object. Invoked 
        during HCR kernel extension module initialisation. 
        Note: an error code from this method will prevent device startup.

    	@return	KErrNone if successful, or any other system wide error code.
        */    
        virtual TInt Initialise() = 0;
        
        
    	/**
        This method returns the address of the compile time setting repository 
        built into the variant HCR.dll project/binary. This repository is 
        optional and may be absent in which case 0 should be returned in aAddr. 
         
        @param aAddr out: a pointer to a HCR::SRepositoryCompiled 
    	@return	KErrNone if successful, output parameters valid,
    	        KErrNotSupported if a compile time repository is not supported,
    	        Any other system wide error code.
        @see HCR::SRepositoryCompiled
       	*/
        virtual TInt GetCompiledRepositoryAddress(TAny* & aAddr) = 0;
        
   	    /**
        This method is called at kernel initialisation and allows the PSL to 
		disable the initial lookup of the built-in Core Image file repository. 
		The PSL should return ETrue if the device/BSP is not going to support 
		this repository. 	
         
    	@return	ETrue if the PIL should not find the repository in the ROM
    	        EFalse if the core image repository is to be used/supported
       	*/
        virtual TBool IgnoreCoreImgRepository () = 0;
        
    	/**
        This method returns the address of the override repository that 
        provides override values for the variant. Typically this repository
        is held in NAND flash and shadowed in RAM by the OS loader. It is
        a read-only settings repository. This repository is optional and may 
        be absent in which case 0 should be returned in aAddr.
         
        @param aAddr out: a pointer to a HCR::SRepositoryFile
    	@return	KErrNone if successful, output parameters valid,
    	        KErrNotSupported if a compile time repository is not supported,
    	        Any other system wide error code.
        @see HCR::SRepositoryFile
       	*/
        virtual TInt GetOverrideRepositoryAddress(TAny* & aAddr) = 0;
        
        };       
        
        
    /** Union that holds one of the supported word-sized setting values 
    */
    union UValueWord
        {
        TInt32      iInt32;
        TInt16      iInt16;
        TInt8       iInt8;
        TBool       iBool;
        TUint32     iUInt32;
        TUint16     iUInt16;
        TUint8      iUInt8;
        TLinAddr    iAddress;
        };

    /** Union that holds a pointer to one of the supported large value types 
    */
    union UValueLarge
        {
        TUint8*     iData;          //!< Array of TUint8 values
        TText8*     iString8;       //!< Array of TText8 values
		TInt32*		iArrayInt32;	//!< Array of TInt32 values
		TUint32*	iArrayUInt32;	//!< Array of TUInt32 values
        TInt64*     iInt64;         //!< Single TInt64 value
        TUint64*    iUInt64;        //!< Single TUint64 value
        };
    
    /** Type used to hold the offset to a large setting value */
    typedef TInt TValueLargeOffset;
      
    /** Union type used to hold either the literal value or a C++ pointer to 
    the value. Used in compile time settings.   
    */
    union USettingValueC
        {
        UValueWord  iLit;
        UValueLarge iPtr;
        };

    /** Union type used to hold either the literal value or an offset from the 
    start if the setting repository to the setting value. Used in file and RAM
    mapped settings.   
    */
    union USettingValueF
        {
        UValueWord          iLit;
        TValueLargeOffset   iOffset;
        };
           
    /** Metadata flags to describe properties of the settings.
    */
    enum TSettingProperties
        {
        EPropUndefined     = 0x0000,   //!< Unknown/not set
        
   
        // Following properties are not yet supported:
        EPropUnintiailised = 0x0001,   //!< Setting has no initial value
        EPropModifiable    = 0x0002,   //!< Setting is set/writable
        EPropPersistent    = 0x0004,   //!< Setting is non-volatile
        
        // Following properties are not yet supported but are envisaged to be
		// implemented to support setting breaks/migration where settings 
		// evolve and/or replace each other.
        EPropDeprecated	   = 0x1000,   //!< Setting supported but deprecate, accessed logged
		EPropReplaced      = 0x2000,   //!< HCR redirected to retrieve value from replacement setting, access logged
		EPropWithdrawn	   = 0x4000    //!< Setting withdrawn, log & panic if accessed 
        };        
        
    /** Provides base class for setting records. All setting structures start
    with this structure providing common setting attributes such as the
    identifier, type etc.    
    */
    struct SSettingBase
        {
        SSettingId  iId;        // Always the first member!
        TInt32      iType;      //!< @see TSettingType
        TUint16     iFlags;     //!< @See TSettingProperties
        TUint16     iLen;       //!< Only valid if setting is a large type
        };
            
    /** This structure holds a setting defined at compile time within a compile
    time defined repository.     
    @see SRepositoryCompiled
    */
    struct SSettingC            // : public SSettingBase
        {
        SSettingBase    iName;  // Always the first member!
        USettingValueC  iValue;
        };

    /** This structure holds a setting define in a file or memory within a file
    based repository.     
    @see SRepositoryFile
    */
    struct SSettingF             // : public SSettingBase
        {
        SSettingBase    iName;   // Always the first member!
        USettingValueF  iValue;
        };


    /** Metadata flags to describe the repository type/layout.
    */
    enum TRepositoryType
        {
        EReposUndefined = 0x00,   //!< Unknown
        
        EReposCompiled  = 'c',   //!< Repository is in Compiled layout
        EReposFile      = 'f'    //!< Repository is in File layout
        };        

    /** Metadata flags to describe repository properties.
    */
    enum TRepositoryProperties
        {
        EReposClear       = 0x0000,   //!< Unknown
        EReposReadOnly    = 0x0001,   //!< Repository is read-only
        EReposNonVolatile = 0x0002    //!< Repository is writable, saved to flash
        };        

    /** Provides base class for setting repositories. All repository structures 
    start with this structure providing common repository attributes such as the
    type , size etc.   
    */
    struct SRepositoryBase
        {
        TUint8      iFingerPrint[3];    //!< Fixed value {'H', 'C', 'R'}
        TUint8      iType;              //!< @See TRepositoryType
        TInt16      iFormatVersion;     //!< Format/layout version number
        TUint16     iFlags;             //!< @see TRepositoryProperties
        TInt32      iNumSettings;       //!< Number of settings in repository
        };    
 
 
    /** This class is the root object for a compile time defined settings 
    repository and is used in the PSL HCR variant object to hold read-only
    compile time settings. This type of repository makes use of pointers to 
    structures and arrays as it is compiled.
    */
    struct SRepositoryCompiled
        {
        SRepositoryBase*    iHdr;        // Always the first member!
        SSettingC*          iOrderedSettingList;
        };    
 
 
    /** Byte type for large setting value data 
    */
    typedef TUint8 TSettingData;
 
    /** This class is the root object for a file or memory based settings 
    repository. It assumes the repository has a flat contiguous layout and
    employees offsets to data rather then C++ pointers as in compiled 
    setting repositories.                 
    All offsets are relative to the address of &iHdr member.
    The last two members are expected to be present in the file/memory as shown
    although there is no way at type definition time to know the size of these
    members, hence they are commented out and will be accessed in the code
    using memory/file address arithmetic.
    */
    struct SRepositoryFile 
        {
        SRepositoryBase     iHdr;        // Always the first member!
        TUint32             iLSDfirstByteOffset;
		TUint32             iLSDataSize;
		TUint32				iReserved[3];
     // SSettingF           iOrderedSettingList[iNumSettings];
     // TSettingData        iLargeSettingsData[iLSDataSize];
        };    
  
    }


// -- GLOBALS -----------------------------------------------------------------


/**
Global entry point used by PIL to create the variant HCR object in the PSL
code.

@return Pointer to variant is successfully, 0 otherwise.
@see HCR::MVariant
*/
GLREF_C HCR::MVariant* CreateHCRVariant(); 



#endif // HCR_HAI_H
