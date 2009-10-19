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
@file hcr_pil.h
Kernel side definitions for the HCR Platform Indepent Layer. 

@internalTechnology
*/

#ifndef HCR_PIL_H
#define HCR_PIL_H


// -- INCLUDES ----------------------------------------------------------------


#include <e32def.h>
#include <e32err.h> 

#include "hcr_hai.h"


// -- CLASSES -----------------------------------------------------------------

namespace HCR
{
	class TRepository;


    //!< Mask for testing for word size settings
    static const TInt KMaskWordTypes = 0x0000FFFF;      

    //!< Mask for testing for large settings  
    static const TInt KMaskLargeTypes = 0xFFFF0000;


	/**
	*/
	class TSettingRef
		{
	public:
	    TSettingRef(TRepository* aRepos, SSettingBase* aSetting)
			{ iRep = aRepos; iSet = aSetting; }
        ~TSettingRef()
        	{ }
        
    public:
    	TRepository*  iRep;
    	SSettingBase* iSet;
		};

	
    /**
    */
    class HCRInternal
        {
    public:
        HCRInternal();
		HCRInternal(HCR::MVariant* aVar);
        ~HCRInternal();
     
        TInt Initialise();
        
        TInt FindSetting(const TSettingId& aId, TSettingType aType, TSettingRef& aSetting);
        TInt FindWordSettings(TInt aNum, const TSettingId* aIds, TInt32* aValues,
                                TSettingType* aTypes, TInt* aErrors);
        TInt CheckIntegrity();
        
        enum States 
			{
			EStatUndef          	= 0x00000000,
			EStatNotReady       	= 0x00010000,
			EStatConstructed    	= EStatNotReady + 0x0001,
			EStatVariantInitialised = EStatNotReady + 0x0002,
			EStatInitialised    	= EStatNotReady + 0x0004,

			EStatReady				= 0x00020000,
			
			EStatFailed				= 0x00800000,
			
			EStatMajornMask			= 0xFFFF0000,
			EStatMinorMask			= 0x0000FFFF
			};
			
		TUint32 GetStatus();
			
    public:		// For Test
    	enum TReposId
        	{
			ECoreRepos = 1,
	    	EOverrideRepos
	    	};
	    	
	    /**
	    Based on the input parameter aId it switches the selected repository to the given
	    name. It is searching the new repository file in \sys\bin and \sys\Data respectively.
	    It keeps the original value of the repository if the file not found.

	    @param aFileName 	The zero terminated, c-style ASCII string of the new repository file without path.
	    					If the name is an empty string (NULL) the it deletes the repository object
	    @param aId     		The internal repository identifier (see TReposId)

	        
		@return	KErrNone 			if successful, the selected internal repository variables point to the new HCR 
													or the referenced repository object deleted.
	            KErrNotFound 		if the new repository file not found.
	            KErrNotSupported 	if repository identifier not supported

	    */    	
    	TInt SwitchRepository(const TText * aFileName, const TReposId aId=ECoreRepos);
        
        
    private:    
    	/** Member holding the status of the HCR service */
    	TUint32 iStatus; 	
    
        /** Handle on the variant code in the PSL component part */    
        HCR::MVariant* iVariant;    
        
        /** Compiled settings in the PSL code */
        TRepository* iVariantStore;
        
        /** File settings in the core ROM image */
        TRepository* iCoreImgStore;
        
        /** File settings shadowed in RAM from NAND */
        TRepository* iOverrideStore;
        
        friend class HCRInternalTestObserver;

        };
    
    
    /**
    */
    class TRepository
        {
    public: 
    	// Repository methods
        virtual TInt Initialise ()=0;
        virtual TInt CheckIntegrity ()=0;
        virtual TInt FindSetting (const TSettingId& aId, TSettingRef& aSetting)=0;
        
        // Setting accessor methods
        virtual TBool IsWordValue(const TSettingRef& aRef);
        virtual TBool IsLargeValue(const TSettingRef& aRef);
        virtual void GetId(const TSettingRef& aRef, TCategoryUid& aCat, TElementId& aKey);
        virtual void GetId(const TSettingRef& aRef, SSettingId& aId);
        virtual TInt32 GetType(const TSettingRef& aRef);
        virtual TUint16 GetLength(const TSettingRef& aRef);

        virtual TInt GetValue(const TSettingRef& aRef, UValueWord& aValue)=0;
        virtual TInt GetLargeValue(const TSettingRef& aRef, UValueLarge& aValue)=0;   
        };
    
    
    /**
    */
    class TRepositoryCompiled : public TRepository
        {
    public: 
        static TRepository* New(const SRepositoryCompiled* aRepos);
        virtual ~TRepositoryCompiled();
        
        virtual TInt Initialise();
        virtual TInt CheckIntegrity();
        virtual TInt FindSetting(const TSettingId& aId, TSettingRef& aSetting);

        virtual TInt GetValue(const TSettingRef& aRef, UValueWord& aValue);
        virtual TInt GetLargeValue(const TSettingRef& aRef, UValueLarge& aValue);   
        
    private:
        TRepositoryCompiled(const SRepositoryCompiled* aRepos);
        
    private:   
        const SRepositoryCompiled* iRepos;
        };
    
    /**
    */
    class TRepositoryFile : public TRepository
        {
    public: 
        static TRepository* New(const SRepositoryFile* aRepos);
        virtual ~TRepositoryFile();
        
        virtual TInt Initialise();
        virtual TInt CheckIntegrity();
        virtual TInt FindSetting(const TSettingId& aId, TSettingRef& aSetting);

        virtual TInt GetValue(const TSettingRef& aRef, UValueWord& aValue);
        virtual TInt GetLargeValue(const TSettingRef& aRef, UValueLarge& aValue);   
       
    private:
        TRepositoryFile(const SRepositoryFile* aRepos);
        
    private:
        const SRepositoryFile* iRepos;
        };
        
     
    } // namespace HCR



// -- GLOBALS -----------------------------------------------------------------


GLREF_C HCR::HCRInternal gHCR;

#define HCRSingleton (&gHCR)

#define HCRReady    ((gHCR.GetStatus() & HCRInternal::EStatReady) == HCRInternal::EStatReady)
#define HCRNotReady ((gHCR.GetStatus() & HCRInternal::EStatReady) == 0)



#endif // HCR_PIL_H

