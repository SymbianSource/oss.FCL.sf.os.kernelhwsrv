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
// some utility classes for writing data to flash buffer
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/

#ifndef __SCMCONFIGITEM_H_INCLUDED__
#define __SCMCONFIGITEM_H_INCLUDED__


#include <e32def.h>
#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif // ! __KERNEL_MODE__

#include <scmbytestreamutil.h>


namespace Debug
	{ 	
	/**
	 * This object represents a data type to dump
	 */
	class TConfigItem : public MByteStreamSerializable
		{
		public:
			enum TSCMDataType
				{				
				EExceptionStacks,			/**< Dumps exception stacks */
				EVariantSpecificData,		/**< Dumps Variant Specific data */
				ERomInfo,					/**< Dumps ROM Build Info */
				ELocks,						/**< Dumps Kernel Lock Info */
				EKernelHeap,				/**< Dumps the Kernel Heap */
				ETraceData,					/**< Dumps any trace data we find */
				EProcessCodeSegs,			/**< Dumps System wide Code Segments for each Process */
				EThreadsUsrStack,			/**< Dumps System wide User Stacks for each thread */
				EThreadsSvrStack,			/**< Dumps System wide Supervisor Stacks for each thread */
				EThreadsUsrRegisters,		/**< Dumps User Registers available for every thread in the System */
				EThreadsSvrRegisters,		/**< Dumps Supervisor Registers available for every thread in the System */ 
				EProcessMetaData,			/**< Dumps the Process List */
				EThreadsMetaData,			/**< Dumps the Thread List */
				ECrashedProcessCodeSegs,    /**< Dumps the Code Segments for the process that has crashed */
				ECrashedProcessUsrStacks,	/**< Dumps the User stacks for each thread in the process that has crashed */
				ECrashedProcessSvrStacks,	/**< Dumps the Supervisor stacks for each thread in the process that has crashed */
				ECrashedProcessMetaData,	/**< Dumps Info about the process that has crashed */
				ECrashedThreadMetaData,		/**< Dumps Info about the Thread that has crashed */				
				ELast						/**< End Marker */
				};
			
			TConfigItem();
			TConfigItem(TSCMDataType aDataType,  TUint8 aPriority, TInt32 aSizeToDump);
	
			TSCMDataType GetDataType() const;
			TInt GetPriority() const;
			TInt GetSizeToDump() const;
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
		 	virtual TInt GetSize() const;
		 	
		 	void SetSpaceRequired(TUint aSpaceReq);
		 	TUint GetSpaceRequired();
		 	
		 	void Print() const;
		 	TBool operator == (const TConfigItem& aOther) const;
		 	TConfigItem* Next() const;

#ifndef __KERNEL_MODE__		 	
		 	 static const TDesC& GetSCMConfigOptionText(TConfigItem::TSCMDataType aType);
#endif // ! __KERNEL_MODE__
		 	 
		private:
			TSCMDataType iDataType;		/** The type this data represents */
			TUint iSizeToDump;			/** the size of the data to dump */
			TUint iSpaceRequiredToDump;	/** If known, this will contain the size of the data we need to dump */
			TUint8  iPriority;			/** Priority of this data (0 is not required) */
			TConfigItem* iNext;	 		/** Next config item in list */
			
			friend class SCMConfiguration;
			
		};			
	}


#endif //  __SCMCONFIGITEM_H_INCLUDED__
