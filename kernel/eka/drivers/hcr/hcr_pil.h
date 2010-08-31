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
// Contains internal definitions for the PIL software of the HCR component
// which includes the singleton class that contains the algorithms and the
// TRepository hierachy that encapsulated the repository data in all its forms
// hiding the specifics from the algoritms in the singleton HCRInternal object.
//

/**
@file hcr_pil.h
Kernel side definitions for the HCR Platform Independent Layer. 

@internalTechnology
*/

#ifndef HCR_PIL_H
#define HCR_PIL_H


// -- INCLUDES ----------------------------------------------------------------


#include <e32def.h>
#include <e32err.h> 

#include "hcr_hai.h"


// -- INTERNAL/HELPER FUNCTIONS PROTOTYPE --------------------------------------
TInt CompareSSettingIds(const HCR::TSettingId& a1, const HCR::SSettingId& a2);


// -- CLASSES -----------------------------------------------------------------

namespace HCR
{

    class TRepository;


    /**< Mask for testing for word size settings */
    static const TInt KMaskWordTypes = 0x0000FFFF;      

    /**< Mask for testing for large settings */  
    static const TInt KMaskLargeTypes = 0xFFFF0000;

    
	/**
	 *  Class implements the reference to the setting, it consists of two
	 * pointers to the repository where the setting is set and to the setting
	 * data itself.   
	 */
	class TSettingRef
		{
	public:

	    /**
	     *  Default C++ constructor. It initiates the reference class 
	     * object with the reference structure data.
	     * @param aSetRef          Reference Setting data 
	     */
	    TSettingRef()
	        {iRep = NULL; iSet = NULL;}
	    
	    /**
	     *  C++ constructor. It initiates the the reference class object 
	     * @param  aRepos          Pointer to the settings repository
	     * @param  aSetting        Pointer to the setting
	     */
	    TSettingRef(TRepository* aRepos, SSettingBase* aSetting)
			{ iRep = aRepos; iSet = aSetting; }
	    
	   
	        
	    /**
	     *   C++ destructor.
	     */
        ~TSettingRef()
        	{ }
        
	public:
	    /**< Pointer to the repository*/
	    TRepository*  iRep;
	    /**< Pointer to the setting*/
	    SSettingBase* iSet;
		};
	

	//Disable WINS (old Visual C++) warning
	#pragma warning(disable:4284)
	/**
	 * Internal HCR, SafeArray (TSa) class. 
	 * Safe Array implementation is based on a smart pointer
	 * pattern which wraps the pointer by the template class and give it a new
	 * flavour. In this case it guarantees that the heap allocated array 
	 * associated with the class instance variable pointer will be deallocated 
	 * during stack unwinding.
	 * IMPORTANT! 
	 * Please don't instantiate this class on the heap as this will break the 
	 * functionality of this class. Operator [] does not check the boundary of
	 * the array, consider safe array implementation as a simple replacement of
	 * standard pointer with all its drawbacks.
	 */

	template <typename T>
	    class TSa
	        {
	        public:
	            /** 
	             *  Default constructor.
	             * During initialization it sets the member variable pointer iSa
	             * to NULL. 
	             */
	            inline TSa() :iSa(NULL){}
	            
	           
	            /**
	             *  operator()() returns an address to the array  
	             * maintained by this SafeArray object.
	             * It can be usefull when it's necessary to get the pointer  
	             * value, for instance passing as function parameter.
	             * @return         Pointer to the first element of the 
	             *                 maintained array of elements of type T. 
	             *  
	             */
	            inline T* operator ()(){return iSa;}
	         
	            /**
	             * operator=() changes the memory ownership by   
	             * reinitiazing SafeArray class object with the address to   
	             * already allocated array. The original heap allocation  
	             * associated with this SafeArray object is deallocated before
	             * reassignment. It's implemented in hcr_pil.cpp.
	             * @param  aP      Pointer to the already allocated array of
	             *                 elements of the type T.
	             * @return         Reference to (*this) object.
	             */
	             TSa<T>& operator=(T* aP);
	                
	            
	            /**
	             * operator[]() returns the reference to the element of 
	             * array maintained by this SafeArray object at position defined
	             * by aIndex function parameter. 
	             * @param  aIndex      Position of the element within SafeArray
	             * @return             Reference to the element from the array
	             */
	            inline T& operator[](TInt aIndex){return *(iSa + aIndex);}
	            
	           	             
	            /**
	             *  Destructor
	             */
	            ~TSa();
	                
	                        
	        private:
	            /**
	             *  Copy constructor must not be called explicitly by the
	             * code
	             */
	            inline TSa(TSa& aSa);
	            
	        protected:
	            /**< Pointer to the allocated heap array*/
	            T*     iSa;
	        };
#pragma warning(default:4284)
	 
	                
    /**
     *  Internal HCR class, object of this class is created by the kernel 
     * when the kernel extensions are loaded and initialized.
     */
    class HCRInternal
        {
    public:       

        /**
         * Internal HCR states
         */
        enum States 
            {
            EStatUndef              = 0x00000000,
            EStatNotReady           = 0x00010000,
            EStatConstructed        = EStatNotReady + 0x0001,
            EStatVariantInitialised = EStatNotReady + 0x0002,
            EStatInitialised        = EStatNotReady + 0x0004,

            EStatReady              = 0x00020000,

            EStatFailed             = 0x00800000,

            EStatMajornMask         = 0xFFFF0000,
            EStatMinorMask          = 0x0000FFFF
            };

        // For Test
        enum TReposId
            {
            ECoreRepos = 1,
            EOverrideRepos
            };

    public:
        /**
         *  Default C++ constructor.
         */
        HCRInternal();
        
        /**
         *  C++ constructor with passing MVariant object for further  
         * instance variable initialization.
         */
		HCRInternal(HCR::MVariant* aVar);
		
		/**
		 *  C++ destructor.
		 */
        ~HCRInternal();
     
        /**
         *  The method initializes  internal instance variable pointers
         * to the addresses of repositories by getting them via call to Variant 
         * object functional API.
         * @return          
         *  - KErrNone        No errors reported
         *  - KErrGeneral     Internal HCR fault
         *  - KErrArgument    Programming error in PSL, ptr/rc 
         *                    mismatch
         *  - KErrNoMemory    Memory allocation failure
         */
        TInt Initialise();

        /**
         *  Based on the input parameter aId it switches the selected repository 
         * to the given name. It is searching the new repository file in 
         * \sys\bin and \sys\Data respectively. It keeps the original value of 
         * the repository if the file not found.
         * @param aFileName     The zero terminated, c-style ASCII string of the 
         *                      new repository file without path. If the name is
         *                      an empty string (NULL) the it deletes the 
         *                      repository object
         * @param aId         The internal repository identifier (see TReposId)
         * @return 
         *  - KErrNone          if successful, the selected internal repository  
         *                      variables point to the new HCR or the referenced 
         *                      repository object deleted.
         *  - KErrNotFound      if the new repository file not found.
         *  - KErrNotSupported  if repository identifier not supported
         */      
        TInt SwitchRepository(const TText * aFileName, const TReposId aId=ECoreRepos);

        
        /**
         *  Internal HCR method checks all repositories integrity.
         * @return
         *  - KErrNone          Successful, no errors reported
         *  - KErrAlreadyExist  Check for the setting duplicates fails
         *  - KErrCorrupt       One of the repositories was found to be corrupt 
         *                      e.g. repository header incorrect, settings not 
         *                      ordered etc
         */
        TInt CheckIntegrity();

        /**
         * Internal HCR method returns a current HCR state.
         * @return Current HCR composite status flag data member, @see States 
         *         for more details
         */
        TUint32 GetStatus();
        
        /**
         *  The method searches the given setting defined by aId parameter
         * and with the type defined by aType parameter. Reference setting data
         * is returned in aSetting output parameter. The search procedure is 
         * performed through all enabled repositories. It starts looking in 
         * Override first, then if setting is not found it goes to CoreImg and
         * in the end in Variant repository.
         * @param   aId         in: setting to find
         * @param   aType       in: required type
         * @param   aSetting    out: found setting reference data
         * @return              The following errors are returned:
         *     - KErrNone         It successfuly ends, no errors are reported
         *     - KErrNotFound     The setting was not found
         *     - KErrArgument     The found setting type does not match the aType
         */
        TInt FindSetting(const TSettingId& aId, TSettingType aType,
                                                        TSettingRef& aSetting);
        
        /**
         *  Internal HCR helper method finds setting and its type.
         * @param   aId         in:  setting id to find
         * @param   aType       out: found setting type. If the setting is  
         *                      not found, the returned value is set to 
         *                      ETypeUndefined
         * @param   aSetting    out: found setting data
         * @return               The following errors can be returned:
         *     - KErrNone       It successfuly ends, no errors are reported
         *     - KErrNotFound   The setting was not found
         */
        TInt FindSettingWithType(const TSettingId& aId, TSettingType& aType,
                                 TSettingRef& aSetting);

       
        /**
         *  Internal helper method search all the word settings provided
         * in aIds[] settings array. The search procedure starts from Override
         * store, if the setting is not found there, it goes through the CoreImg
         * and finaly ends up in the Variant data.
         * @param   aNum        in: number of settings to find
         * @param   aIds        in: array of settings to find
         * @param   aValues     out: all found settings values are written  
         *                      back to this array. If the setting is not found
         *                      the returned setting value is set to 0
         * @param   aTypes      out: If this array is provided by upper user,
         *                      the setting types are written back to this array.
         *                      If the element is not found, its type is set to
         *                      ETypeUndefined. 
         * @param   aErrors     out: user must always provide this array, 
         *                      where the method will report the search result 
         *                      for each individual setting. There are three 
         *                      possible values:
         *                      - KErrNone  Setting is found, no errors reported
         *                      - KErrNotFound Setting is not found
         *                      - KErrErrArgument Found setting has larger than
         *                        four bytes size
         * @return  The following errors can be returned:
         *  - Zero or positive number of settings found in category, -ve on error
         *  - KErrArgument if some parameters are wrong(i.e. aErrors is a null
         *                   pointer, aNum is negative and so on) 
         *  - KErrNotReady if the HCR is used before it has been initialised
         *  - KErrCorrupt  if HCR finds a repository to be corrupt
         *  - KErrGeneral  if an internal failure occurs, see trace
         *  
         * @pre Caller must invoke this function inside the thread critical 
         *      section to let the method finish its job. It avoids memory leak 
         *      in case of possible client thread termination. 
         */
        TInt GetWordSettings(TInt aNum, const SSettingId aIds[], TInt32 aValues[],
                                TSettingType aTypes[], TInt aErrors[]);
        
        /**
         *  Internal HCR method returns the number of settings in the specified
         * category.
         * @param aCatUid   in: The setting identifier category to use in the 
         *                      search
         * @return 
         *  - Zero or positive number of settings found in category, -ve on error
         *  - KErrNotReady if the HCR is used before it has been initialised
         *  - KErrCorrupt if HCR finds a repository to be corrupt
         *  - KErrGeneral if an internal failure occurs, see trace
         */
        TInt FindNumSettingsInCategory (TCategoryUid aCatUid);
        
        
        /**
         * Internal HCR method searches all elements within the specified 
         * category aCatUid.
         * @param aCatUid   in: The setting identifier category to use in the search
         * @param aMaxNum   in: The maximum number of settings to return. It is also 
         *                  the size of the arrays in the following arguments 
         * @param aElIds    out: Client supplied array populated on exit. Large
         *                  enough to hold all elements in category.
         * @param aTypes    out: Client supplied array populated with setting types 
         *                  enumerations on exit. May be 0 if client is 
         *                  not interested.
         * @param aLens     out: Client supplied array populated with setting lengths
         *                  on exit. May be 0 if client is not interested.
         *
         * @return Zero or positive number of settings found in category, -ve on error
         *  - KErrArgument if some parameters are wrong(i.e. aErrors is a null
         *                   pointer, aNum is negative and so on)
         *  - KErrNotReady if the HCR is used before it has been initialised
         *  - KErrCorrupt  if HCR finds a repository to be corrupt
         *  - KErrGeneral  if an internal failure occurs, see trace
         */
        TInt FindSettings(TCategoryUid aCatUid, 
                TInt aMaxNum,  TElementId aIds[],  
                TSettingType aTypes[], TUint16 aLens[]);


        /**
         *  Internal HCR method finds all the settings within the specified 
         * category and which matches aMask and aPattern.
         * @param aCat      in: The category to retrieve settings for
         * @param aMaxNum   in: The maximum number of settings to retrieve. It  
         *                  is also the size of the arrays in the following 
         *                  arguments   
         * @param aElemMask in: The bits in the Element ID to be checked against 
         *                  aPattern
         * @param aPattern  in: Identified the bits that must be set for a 
         *                  setting to be returned in the search
         * @param aIds      out: Client supplied array populated on exit. Large
         *                  enough to hold aMaxNum element ids.
         * @param aTypes    out: Client supplied array populated with setting types 
         *                  enumerations on exit. May be 0 if client is 
         *                  not interested.
         * @param aLen      out: Client supplied array populated with setting 
         *                  lengths on exit. May be 0 if client is not interested.
         * @return 
         *  - Zero or positive number of settings found in category, -ve on error
         *  - KErrArgument if some parameters are wrong(i.e. aErrors is a null
         *                   pointer, aNum is negative and so on) 
         *  - KErrNotReady if the HCR is used before it has been initialised
         *  - KErrCorrupt  if HCR finds a repository to be corrupt
         *  - KErrGeneral  if an internal failure occurs, see trace
         */
        TInt FindSettings(TCategoryUid aCat, TInt aMaxNum, 
                TUint32 aMask, TUint32 aPattern,  
                TElementId aIds[], TSettingType aTypes[], TUint16 aLens[]);
 
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
     *  Base Repository class. This class defines API needed to be 
     * implemented in the derived classes.
     */
    class TRepository
        {
    public: 
    	// Repository methods		
		virtual ~TRepository();
        virtual TInt CheckIntegrity ()=0;
        virtual TInt FindSetting (const TSettingId& aId, TSettingRef& aSetting)=0;
        
        /**
         *  Pure virtual function, must implement the search procedure for the
         * setting in the repository within the bounds defined by aLow and aHigh
         * parameters. It returns found setting reference data and its position.
         * @param   aId         in:  Setting to find
         * @param   aSetting    out: Found setting reference data
         * @param   aPosition   out: Position the found setting in the repository
         * @param   aLow        in:  Low index where to start search
         * @param   aHigh       in:  High index where to end search
         * @return
         *  - KErrNone          Successful, no errors were reported 
         *  - KErrNotFound      Either the repository does not have any settings,
         *                      and its length is zero or the setting was not
         *                      found, all output parameters are set to zeros in
         *                      this case. 
         */
        virtual TInt FindSetting (const TSettingId& aId, TSettingRef& aSetting,
                TInt32& aPosition, TInt32 aLow, TInt32 aHigh) = 0;
        
        /**
         *  Pure virtual function, must implement the word setting search 
         * procedure.
         * @param   aNum        in: Number of settings to be found
         * @param   aIds        in: An array of setting ids pointers to be found
         * @param   aValues     out: An array of pointers to the values 
         *                      populated during search procedure.
         * @param   aTypes      out: An array of pointers to the types populated
         *                      during search procedure.
         * @param   aErrors     out: An array of pointers to the errors populated
         *                      during search procedure. This can be the following
         *                      errors:
         *                          - KErrNone      Successfuly done, no errors 
         *                            reported
         *                          - KErrNotFound  The setting was not found
         *                          - KErrArgument  The found setting type is large
         *                            than 4 bytes.
         * @return
         *  - KErrNone      Successfuly done, no errors reported
         *  - KErrNotReady  Repository is not ready
         *  - system wider error
         */
        virtual TInt GetWordSettings(TInt aNum, SSettingId* aIds[], 
                       TInt32* aValues[], TSettingType* aTypes[], 
                       TInt* aErrors[])=0;

        /**
         * Pure virtual function, must return a reference to TSettingRef
         * structure at specified position within the repository.
         * @param   aIndex      in: Setting position(index) in the repository
         * @param   aRef        out: Reference data storage
         */
        virtual void GetSettingRef(TInt32 aIndex, TSettingRef& aRef) = 0;
        
        /**
         *  Pure virtual function, must implement the search all elements within 
         * the defined category.
         * @param   aCatUid     in: Category id where to search the elements
         * @param   aFirst      out: Repository index where the first element is
         *                           situated
         * @param   aLast       out: Repository index where the last element is
         *                           situated
         * @return
         *  - KErrNone      Successfuly done, no errors were reported
         *  - KErrNotFound  No any elements were found in this category or repo-
         *                  sitory is empty
         */
        virtual TInt FindNumSettingsInCategory(TCategoryUid aCatUid, 
                TInt32& aFirst, TInt32& aLast) = 0;
        
       
        // Setting accessor methods
        virtual TBool IsWordValue(const TSettingRef& aRef);
        virtual TBool IsLargeValue(const TSettingRef& aRef);
        virtual void GetId(const TSettingRef& aRef, SSettingId& aId);
        virtual TInt32 GetType(const TSettingRef& aRef);
        virtual TUint16 GetLength(const TSettingRef& aRef);
        
        virtual void GetSettingInfo(const TSettingRef& aRef, 
                TElementId& aId, TSettingType& aType, TUint16& aLen);

        
        virtual TInt GetValue(const TSettingRef& aRef, UValueWord& aValue)=0;
        virtual TInt GetLargeValue(const TSettingRef& aRef, UValueLarge& aValue)=0;

        };
    
    
    /**
     * Compoiled repository class
     */
    class TRepositoryCompiled : public TRepository
        {
    public: 
        static TRepository* New(const SRepositoryCompiled* aRepos);
        virtual ~TRepositoryCompiled();
        
        virtual TInt CheckIntegrity();
        
        virtual TInt FindSetting(const TSettingId& aId, TSettingRef& aSetting);
        
        /**
         *  Pure virtual function defined in the base class TRepository, 
         * it implements the search procedure for the setting in the repository 
         * within the bounds defined by aLow and aHigh parameters. It returns 
         * found setting reference data and its position. Also @see TRepository
         * for more details. 
         */
        virtual TInt FindSetting (const TSettingId& aId, TSettingRef& aSetting,
                 TInt32& aPosition,TInt32 aLow, TInt32 aHigh);
        
                
        /** 
         *  Pure virtual function defined in the base TRepository class,
         * it implement the word setting search procedure. Also @see TRepository
         * for more details.
         */
        virtual TInt GetWordSettings(TInt aNum, SSettingId* aIds[], 
                    TInt32* aValues[], TSettingType* aTypes[], TInt* aErrors[]);

        
        /**
         *  This method implements returning a reference to TSettingRef
         * structure at specified position within the repository. 
         */
        virtual  void GetSettingRef(TInt32 aIndex, TSettingRef& aRef);
        
        /**
         *  Pure virtual function defined in the base TRepository class, 
         *  implements the search for all elements procedure withinthe defined
         *  category. Also @see TRepository for more details.
         */
        virtual TInt FindNumSettingsInCategory(TCategoryUid aCatUid,
                TInt32& aFirst, TInt32& aLast);
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
        
        virtual TInt CheckIntegrity();
        virtual TInt FindSetting(const TSettingId& aId, TSettingRef& aSetting);
        
        /**
         *  Pure virtual function defined in the base class TRepository, 
         * it implements the search procedure for the setting in the repository 
         * within the bounds defined by aLow and aHigh parameters. It returns 
         * found setting reference data and its position. Also @see TRepository
         * for more details. 
         */
        virtual TInt FindSetting (const TSettingId& aId, TSettingRef& aSetting,
                          TInt32& aPosition, TInt32 aLow, TInt32 aHigh);

        /** 
         *  Pure virtual function defined in the base TRepository class,
         * it implement the word setting search procedure. Also @see TRepository
         * for more details.
         */
        virtual TInt GetWordSettings(TInt aNum, SSettingId* aIds[], 
                         TInt32* aValues[], TSettingType* aTypes[],
                         TInt* aErrors[]);

        /**
         *  This method implements returning a reference to TSettingRef
         * structure at specified position within the repository. 
         */
        virtual  void GetSettingRef(TInt32 aIndex, TSettingRef& aRef);

        /**
         *  Pure virtual function defined in the base TRepository class, 
         *  implements the search for all elements procedure withinthe defined
         *  category. Also @see TRepository for more details.
         */ 
        virtual TInt FindNumSettingsInCategory(TCategoryUid aCatUid,
                TInt32& aFirst, TInt32& aLast);
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

