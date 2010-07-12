// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// TOC Partition Management for Embedded MMC devices
// 
//

#include "toc.h"


/*
 * Search entry in TOC with aName as part of ItemName.
 */
TInt Toc::GetItemByName(const TText8* aName, STocItem& aItem)
    {
    if ( aName == NULL )
        {
        return KErrNotFound;
        }
    
	// calculate length for name to be searched
    TUint nameLen = 0;
    for (; nameLen < KMaxItemNameLen && aName[nameLen] != 0; nameLen++) {};
        
	if ( !nameLen ) 
	    return KErrGeneral; // zero length or Blank Name

	// check all items in TOC	 
	for (TUint i=0; i < KMaxNbrOfTocItems && iTOC[i].iStart != KEndOfToc; i++)
		{	
		// calculate length of current item 
        TUint fileNameLen = 0;
		for (; fileNameLen < KMaxItemNameLen && iTOC[i].iFileName[fileNameLen] != 0; fileNameLen++) {};
		    
		if ( fileNameLen < nameLen ) 
		    continue;  // file name too short
	
		// compare Item with aName
		for (TUint k = 0; k <= (fileNameLen - nameLen); k++ )				
			{
            TUint l=0;
            for (; l < nameLen; l++ )
                {
                if ( aName[l] != iTOC[i].iFileName[k+l] ) 
                    break;
                }

            if ( l == nameLen )
                {
                // item found
                aItem = iTOC[i];
                aItem.iStart += (iTocStartSector << KSectorShift);
                return KErrNone;
                }
			}		
		}
	
	return KErrNotFound;
    }

//  End of File
