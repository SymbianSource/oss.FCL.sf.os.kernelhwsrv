
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
//
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

/**
 @file
 @internalTechnology
*/
#ifndef __SCMCONFIG_H_INCLUDED__
#define __SCMCONFIG_H_INCLUDED__


#include <e32def.h>

#include <scmbytestreamutil.h>
#include <scmconfigitem.h>

namespace Debug{
	
	//Note -- Changing the size of this requires a change in KScmConfigMaxSize
    _LIT8(KScmConfigHeaderString, "SCMCONFIG");

    /**
     * This class handles the configuration section of the crash partition.
     * It is responsible for reading and writing it
     */
	class SCMConfiguration : public MByteStreamSerializable
		{
		
		public:
			
			//The 9 here refers to the size of KScmConfigHeaderString which is serialised
			static const TInt KScmConfigMaxSize = TConfigItem::ELast * sizeof(TConfigItem) + 9;
			
			SCMConfiguration();		
			virtual ~SCMConfiguration();
			
			// from MByteStreamSerializable
			virtual TInt Serialize(TByteStreamWriter& aWriter);
			virtual TInt Deserialize(TByteStreamReader& aReader);
		 	virtual TInt GetSize() const;
			
		 	TConfigItem* GetNextItem();
			TInt SetDefaultConfig();
						
			TInt CreateConfigItem(const TConfigItem::TSCMDataType aDataType, const TUint8 aPriority, const TInt32 aSizeToDump);
			TInt ModifyConfigItemPriority(const TConfigItem::TSCMDataType aDataType, const TUint8 aPriority);
			
			void ResetToHighestPriority();
			
		 	TBool operator == (const SCMConfiguration& aOther) const;
			
		 	TConfigItem* ConfigList() const;
			TInt InsertToList(TConfigItem* aItem);
			void ClearList(); 		
		 	
		private:	
			
			/**
			 * This is an ordered linked list of TConfigItems. The first is the highest priority and so on until the lowest priority 
			 */			
			TConfigItem* iConfigList;
			
			/**
			 * Everytime GetNextItemToDump is called this moves down along the list 
			 */
			TConfigItem* iNextItem;   
			
		private:	
			TInt RemoveFromList(TConfigItem* aItem);
			
							
		};
	}

#endif /*__SCMCONFIG_H_INCLUDED__*/
