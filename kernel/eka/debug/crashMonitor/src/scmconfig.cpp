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
// e32\debug\crashMonitor\src\scmconfig.cpp
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32err.h>
#include <e32const.h>
#include <e32const_private.h>

#include <scmconfig.h>
#include <scmconfigitem.h>
#include <scmdatatypes.h>

namespace Debug
	{
	/**
	 * SCMConfiguration constructor
	 * Initialises the configuration object to default values
	 */
	SCMConfiguration::SCMConfiguration() 
	: iConfigList(NULL)
		{
		}
	
	/**
	 * SCMConfiguration destructor
	 */
	SCMConfiguration::~SCMConfiguration()
		{	
		ClearList();	
		}

	/**
	 * Goes to the flash and reads the configuration block and populates the object state
	 * accordingly
	 * @return one of the system wide error codes
	 */
	TInt SCMConfiguration::Deserialize(TByteStreamReader& aReader)
		{		
		if( !iConfigList)
			{
			// we need to set up a default configuration to load the data into
			TInt err = SetDefaultConfig();
			if(err != KErrNone)
				{
				CLTRACE1("SCMConfiguration::Deserialize SetDefaultConfig failed err = %d", err);
				return err;
				}
			}
		
		TInt startPos = aReader.CurrentPosition();		
		
		TBuf8<10> magicNumbers;
		// try and read the magic numbers - if they dont exist then 
		// there is not an scm config there
		const TInt KLen = KScmConfigHeaderString().Length();
		for(TInt i=0;i<KLen;i++)
			{
			TUint8 b = aReader.ReadByte();			
			magicNumbers.Append(TChar(b));	
			}
		
		if(magicNumbers.Compare(KScmConfigHeaderString()) != 0)
			{
			CLTRACE("No scm, config to read !");
			return KErrNotReady;
			}
				
		TConfigItem* item = iConfigList;
		while(item)
			{
			item->Deserialize(aReader);
			item = item->iNext;
			}
		
		TInt endPos = aReader.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("SCMConfiguration::Deserialize size error");	
			return KErrCorrupt;
			}			
		return KErrNone;			
		}
	
	/**
	 * This writes the current configuration object state to flash. This configuration will be used on the next crash
	 * @return one of the system wide error codes
	 */
	TInt SCMConfiguration::Serialize(TByteStreamWriter& aWriter)	
		{		
		if( !iConfigList)
			{
			CLTRACE("SCMConfiguration::Serialize ERROR - NO LIST!!");
			return KErrNotReady;
			}
		
		TInt startPos = aWriter.CurrentPosition();
		
		// write the number of crashes and magic numbers
		
		// try and read the magic numbers - if they dont exist then 
		// there is not an scm config there
		const TInt KLen = KScmConfigHeaderString().Length();
		const TDesC8& des = KScmConfigHeaderString();
		for(TInt i=0;i<KLen;i++)
			{
			aWriter.WriteByte(des[i]);
			}				
		
		TConfigItem* item = iConfigList;
		while(item)
			{
			item->Serialize(aWriter);
			item = item->iNext;
			}

		TInt endPos = aWriter.CurrentPosition();
		if( endPos - startPos != GetSize())
			{
			// error between actual size & real size in data
			CLTRACE("SCMConfiguration::Serialize size error");	
			return KErrCorrupt;
			}						
		return KErrNone;
		}
	
	/**
	 * Returns entire size of the SCMConfiguration block
	 * @return Size
	 */
	TInt SCMConfiguration::GetSize() const
		{
		// returns the size of all the config items when serialized to disk / flash
		return (TConfigItem::ELast * iConfigList->GetSize()) + KScmConfigHeaderString().Length();
		}
	
	/** 
	 * This will return, one at a time, the highest priority. 
	 * @see ResetToHighestPriority()
	 * @param aSizeToDump this will contain the size in bytes of data to dump for this type - 0 means dump all
	 * @return Data type to dump 
	 */
	TConfigItem* SCMConfiguration::GetNextItem()
		{
		if(!iNextItem)
			{
			return NULL;
			}
			
		//get the values we need
		TConfigItem* item  = iNextItem;	
		
		//Now move iNextItem to be next in the list
		iNextItem = iNextItem->iNext;			
		return item;
		}	
	
	/**
	 * Deletes the linked list
	 * @return system wide OS code
	 */
	void SCMConfiguration::ClearList()
		{
		if(!iConfigList)
			{
			return;
			}
		
		//all we need to do in here is delete the members of our linked list
		TConfigItem* item = iConfigList;
		
		do{			
			TConfigItem* tmp = item->iNext;
			delete item;		
			item = tmp;		
		}
		while(item != NULL);
		
		iConfigList = NULL;
		}
	
	/**
	 * Rather than reading the configuration from the flash, this method sets up the configuration object
	 * to a default configuration type
	 * @return one of the system wide error codes
	 */ 
	TInt SCMConfiguration::SetDefaultConfig()
		{
		//flush the object first
		ClearList();
	
		//This is a predefined default config - in the future we may have multiple defaults based on use case
		// currently however we use a fixed size list of config items of size TConfigItem::ELast		
		// also the TSCMDataType of each item must be unique				
		
		for(TInt cnt = TConfigItem::ELast - 1; cnt >= 0; cnt --)
			{			
			TInt priority = cnt + 1; 
			
			//Lets not do these by default
			if((TConfigItem::TSCMDataType)cnt == TConfigItem::EThreadsUsrStack || (TConfigItem::TSCMDataType)cnt == TConfigItem::EThreadsSvrStack)
				{
				priority = 0;
				}
			
			//set it with the priority of its enum (ie. assume that the enum is listed in its priority - it is)
			//by default dump everything until we run out of space
			TInt err = CreateConfigItem((TConfigItem::TSCMDataType)cnt, priority, 0);
			if(KErrNone != err)
				{
				return err;
				}
			}
		
		return KErrNone;		
		}
		
	/**
	 * This configures the required data for a given configuration item
	 * Note that aSizeToDump is only used in the case of memory dumps such as stacks
	 * @param aDataType - Type of data to dump
	 * @param aPriority - its priority 0-256. 0 Means do not dump and 256 is highest priority
	 * @param aSizeToDump - amount in bytes to dump. Only relevant for memory dumps and ignored when aPriority is 0
	 * @return one of the OS wide return codes
	 */
	TInt SCMConfiguration::CreateConfigItem(const TConfigItem::TSCMDataType aDataType, const TUint8 aPriority, const TInt32 aSizeToDump)
		{
		//create the config item
		TConfigItem* item = new TConfigItem(aDataType, aPriority, aSizeToDump);
		
		//insert to priority list
		return InsertToList(item);		
		}
		
	
	/**
	 * ModifyConfigItemPriority - modifies prioity for a given configuration item
	 * @param aDataType - The unique type of the config item
	 * @param aPriority - its priority 0-256. 0 Means do not dump and 256 is highest priority
	 * @return one of the OS wide return codes
	 */
	TInt  SCMConfiguration::ModifyConfigItemPriority(const TConfigItem::TSCMDataType aDataType, const TUint8 aPriority)
		{

		// find the item with the matching data type
		TConfigItem* item = iConfigList;		
		while(item)
			{
			if(item->iDataType == aDataType)
				{
				break;
				}
			item = item->iNext;			
			}
	
		if(!item)
			{
			return KErrNotFound;
			}
		
		item->iPriority = aPriority;
		
		// now reorder the list according to new priority
		TInt err = RemoveFromList(item);
		if(err != KErrNone)
			{
			return err;
			}
		
		err = InsertToList(item);

		if(err != KErrNone)
			{
			return err;
			}
		
		return KErrNone;
		}

/**
 * Removes item from the linked list
 * @param aItem - item to remove
 * @return OS code
 */
TInt SCMConfiguration::RemoveFromList(TConfigItem* aItem)
		{
		if(!aItem)
			{
			return KErrArgument;
			}
		
		if(!iConfigList)
			{
			return KErrCorrupt;  // oops no list to remove
			}
		
		
		if(aItem == iConfigList)
			{
			// special case remove from beginning of list
			iConfigList = iConfigList->iNext;
			return KErrNone;
			}
		
		TConfigItem* item = iConfigList; 
		while(item)
			{
			// is the next item the match ?
			if(item->iNext == aItem)
				{
				item->iNext = aItem->iNext;
				return KErrNone;
				}		
			item = item->iNext;	
			}
		
		return KErrNotFound;	
		}

/**
 * Inserts a priority item into the linked list in its correct location
 * @param aItem - item to insert
 * @return OS code
 */
TInt SCMConfiguration::InsertToList(TConfigItem* aItem)
	{ 	
	
	//if the list is empty, then this is the only item
	if(!iConfigList)
		{
		iConfigList = aItem;			
		return KErrNone;
		}
	
	//should it go at the start? special case not covered by while loop
	TConfigItem* temp;
	
	if(aItem->iPriority >= iConfigList->iPriority)
		{
		temp = iConfigList;
		iConfigList = aItem;
		aItem->iNext = temp;
		return KErrNone;
		}
	
	TConfigItem* item = iConfigList;
	
	do{		
		//if we get to the end of the list and the item still hasnt been assigned then it must be lowest priority
		if(item->iNext == NULL)
			{
			item->iNext = aItem;
			return KErrNone;
			}
		
		//check if its priority is between these
		if(aItem->iPriority < item->iPriority && aItem->iPriority >= item->iNext->iPriority)
			{
			//insert between these nodes
			temp = item->iNext;
			item->iNext = aItem;
			aItem->iNext = temp;
			return KErrNone;
			}
	
		item = item->iNext;	
	}
	while(item != NULL);	
	
	//should never get here
	return KErrUnknown;
	}
	
/**
 * This resets the next item counter back to the highest priority item in the list
 */
void SCMConfiguration::ResetToHighestPriority()
	{
	//set the next item counter back to the head of the list
	iNextItem = iConfigList;
	}

/**
 * Overloaded == operator
 * @param aOther Item to compare
 * @return
 */
TBool SCMConfiguration::operator == (const SCMConfiguration& aOther) const
	{
	
	if(!iConfigList && !aOther.iConfigList)
		{
		return ETrue;
		}
	 		
	if((!iConfigList && aOther.iConfigList) || (iConfigList && !aOther.iConfigList))
		{
		return EFalse;
		}
	
	
	TConfigItem* item1 = iConfigList;
	TConfigItem* item2 = aOther.iConfigList;
	
	while(item1 && item2)
		{
		if(!(*item1 == *item2))
			{
			return EFalse;
			}
		
		item1 = item1->iNext;			
		item2 = item2->iNext;			
		}

	if( item1 != item2)  // both should now be null - if not then lists were different lengths
		{
		return EFalse;
		}
		
	return ETrue;
	
	}
 	
/**
 * Getter for the head of the SCMConfig list
 * @return Head of List
 */	
TConfigItem* SCMConfiguration::ConfigList() const
	{
	// returns the head of the list	
	return iConfigList;
	} 
} //End of debug namespace

//eof




