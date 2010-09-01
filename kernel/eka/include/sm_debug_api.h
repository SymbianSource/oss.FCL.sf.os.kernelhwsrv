// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Definition of the stop-mode debug interface.
// 

/**
@file
@publishedPartner
@prototype
*/

#ifndef D_STOP_MODE_API_H
#define D_STOP_MODE_API_H

#include <rm_debug_api.h>
#include <plat_priv.h>

namespace Debug
	{
	/**
	 * The stop-mode debug interface is a stateless interface which may be called at any point
	 * except user mode, provided the rest of the OS is not going to run or pre-empt it.
	 * For example, for stop-mode debugging, the ICE may run the stop_mode_api routine to
	 * collect information about the system so long as no exceptions are enabled, and all 
	 * registers/stack are preserved and restored after the call completes. Note that in an SMP environment
	 * it is expected that ALL the CPU's have been stopped
	 */

	/** Stop Mode Debug API Version Numbers */
	const TUint KStopModeMajorVersionNumber=0;
	const TUint KStopModeMinorVersionNumber=0;
	const TUint KStopModePatchVersionNumber=2;

	/**
	 * Enumerators used to identify the buffers created for use with the Stop-Mode Debug API.
	 */
	enum TBufferType
		{
		EBuffersFunctionality = 0,   /**< Enumerator corresponding to the buffer created to store the debug functionality block. */
		EBuffersRequest = 1,         /**< Enumerator corresponding to the request buffer. */
		EBuffersResponse = 2,        /**< Enumerator corresponding to the response buffer. */
		
		/**
		 * @internalTechnology
		 * A user should find the number of buffer tags from the DFBlock rather than this enumerator.
		 */
		EBuffersLast
		};

	/**
	 * Enumerators used to identify the functions for use with the Stop-Mode Debug API.
	 */
	enum TFunctionalityStopModeFunctions
		{
		EStopModeFunctionsExitPoint = 0,  /**< Enumerator corresponding to the Debug::GetList() function. */
		EStopModeFunctionsGetList = 1,    /**< Enumerator corresponding to the Debug::ExitPoint() function. */
		EStopModeFunctionsTestAPI = 2,	  /**< Enumerator corresponding to the Debug::TestAPI() function. */
		
		/**
		 * @internalTechnology
		 * A user should find the number of supported functions from the DFBlock rather than this enumerator.
		 */
		EStopModeFunctionsLast
		};

	/**
	 * This structure defines the start elements of a stop-mode debug functionality block.
	 * It is assumed that the rest of the functionality block will extend past the end of
	 * the structure and will be accessed according to the documented format of the
	 * stop-mode functionality block.
	 */
	struct DFunctionalityBlock
		{
		TUint32 iSize;				/**< Size of the functionality block in bytes. */
		TVersion iStopModeVersion;	/** Version of the stop-mode debug API. */
		TTagHeader iFirstHeader;	/** The header for the first sub-block in the functionality block. */
		};

	/**
	 * This structure used for extracting static data using the Stop Mode Extension API
	 * StopModeDebug::GetList using TListId::EStaticInfo
	 * as the first argument.
	 */
	class TStaticListEntry
		{
	public:
    
		/** Build time of ROM in microseconds */
	    TUint64 iTime;    
    
		/** Number of CPUs */
	    TInt iCpuNumbers;
    
		/** ROM build number */
	    TUint16 iBuildNumber;    
        
		/** Major Version of ROM build */
	    TUint8 iMajorVersion;                   

		/** Minor Version of ROM build */
	    TUint8 iMinorVersion;
    
		/** Currently unused element. May be used in future to aid maintaining compatibility. */
	    TUint32 iSpare[10];    
		};

	/**
	 * This structure represents a request to return a list via the SM API
	 */
	struct TListItem
		{
		/** Size of this TListItem */
		TUint32 iSize;
	
		/** The type of list to return */
		TListId iListId;
	
		/** The scope of the list to return  */
		TListScope iListScope;
	
		/**
		 * Data corresponding to the list scope, for example if iListScope specifies a thread
		 * specific listing then iScopeData should contain the thread ID to return the list for.
		 * If iListScope = EGlobalScope then iScopeData is ignored.
		 */
		TUint64 iScopeData;
	
		/**
		 * The first element in the target list to return data for. For example if a thread list is being
		 * requested then specifying iStartElement = 100 indicates that the first thread to be returned should
		 * be the first thread with thread ID >= 100.
		 */
		TUint64 iStartElement;
	
		/** Memory address of where the data should be written */
		TAny* iBufferAddress;
	
		/** Size of the buffer available for writing the data into */
		TUint32 iBufferSize;
		};

	/**
	 * Structure that describes a list being returned
	 */
	struct TListReturn
		{
		/** List that is being returned */
		TUint32 iReqNo;

		/** Number of items in the returned list */
		TUint32 iNumberItems;

		/** Size occupied by data */
		TUint32 iDataSize;
		};

	/**
	 * Class used to add extended functionality to DDebuggerInfo class.
	 * 
	 * @publishedPartner
	 * @prototype
	 */
	class DStopModeExtension
		{
		public:
			DStopModeExtension()
				:iFunctionalityBlock(NULL),
				iSpare1(0),
				iSpare2(0),
				iSpare3(0),
				iSpare4(0),
				iSpare5(0),
				iSpare6(0),
				iSpare7(0)
				{};        
		   
			static void Install(DStopModeExtension* aExt);
			
		public:
			Debug::DFunctionalityBlock* iFunctionalityBlock;
			TUint32 iSpare1;
			TUint32 iSpare2;
			TUint32 iSpare3;
			TUint32 iSpare4;
			TUint32 iSpare5;
			TUint32 iSpare6;
			TUint32 iSpare7;
		};

	/**
	 * This is the interface to the stop mode debug API. The ROM resident address of these functions can be found 
	 * from the Debug Functionality block via the superpage. It may be assumed that all of these functions
	 * will exit via the function ExitPoint and thus setting a breakpoint there will capture the end of execution.
	 * For more detailed information, see the stop mode guide.
	 */
	class StopModeDebug
			{
			public:
				/**
				 * Stop mode debug API. Call this to action any request for information, or to manipulate
				 * debug data.
				 * 
				 * This is a stateless interface - it does not record information about previous invocations. It
				 * does not take any OS locks, wait on any synchronisation objects, allocate/deallocate heap memory or call
				 * ANY OS routines (unless documented as doing so). It will not cause any exceptions due to page faults,
				 * but will report that it encountered such problems where appropriate.
				 *
				 * @pre This must be called with a valid stack in supervisor mode. There are no exceptions/interrupts
				 * enabled which will cause the OS state to change during the execution of this routine.
				 * @args aItem Structure describing the list we want to retrieve
				 * @args aCheckConsistent If true, this will honour any locks the system holds and return KErrNotReady
				 * @return KErrNone on success or one of the other system wide error codes.
				 */
				IMPORT_C static TInt GetList(const TListItem* aItem, TBool aCheckConsistent);

				/**
				 * Stop mode debug API
				 * 
				 * This is a test function that allows us to test our communications with the hardware debugger
				 * 
				 * @pre This must be called with a valid stack in supervisor mode. There are no exceptions/interrupts
				 * enabled which will cause the OS state to change during the execution of this routine.
				 * @args aItem Structure describing the list we want to retrieve
				 * @return KErrNone on success or one of the other system wide error codes.
				 */
				IMPORT_C static TInt TestAPI(const TListItem* aItem);

			public:	
				static TInt ExitPoint(const TInt aReturnValue);

			private:
				/** Code segment list routines */
				static TInt ProcessCodeSeg(TUint8*& aBuffer, TUint32& aBufferSize, DEpocCodeSeg* aCodeSeg);
			
				//TODO: Horrible signature. Structify it
				static TInt AppendCodeSegData(TUint8*& aBuffer, TUint32& aBufferSize, const TModuleMemoryInfo& aMemoryInfo, const TBool aIsXip, const TCodeSegType aCodeSegType, const TDesC8& aFileName, DEpocCodeSeg* aCodeSeg);
				static TInt GetCodeSegList(const TListItem* aItem, bool aCheckConsistent);
				static DEpocCodeSeg* GetNextCodeSeg(const TUint32 aStart, const Debug::TListScope aListScope, const TUint64 aScopeData);
				static DEpocCodeSeg* GetNextGlobalCodeSeg(const TUint32 aStart);
				static DEpocCodeSeg* GetNextThreadSpecificCodeSeg(const TUint32 aStart, const TUint64 aThreadId);

				/** Process list routines */
				static TInt GetProcessList(const TListItem* aItem, bool aCheckConsistent);	
				static TInt AppendProcessToBuffer(DProcess* aProc, TUint8* aBuffer, TUint8* aBufferEnd, TUint32& aProcSize);
				
				/** Static Info Retrieval routines */
				static TInt GetStaticInfo(const TListItem* aItem, bool aCheckConsistent);
				
				static void GetObjectFullName(const DObject* aObj, TFullName& aName);
		
				/** Utility functions */
				static TInt CopyAndExpandDes(const TDesC& aSrc, TDes& aDest);
				
			};

	};
#endif // D_STOP_MODE_API_H

// End of file sm_debug_api.h
