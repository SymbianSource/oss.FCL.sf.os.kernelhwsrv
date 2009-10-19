// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Utility functions for the Mass Storage file system.
// 
//

/**
 @file
 @internalTechnology
 
 Indicates if a number passed in is a power of two
 
 @param aNum number to be tested
 @return Flag to indicate the result of the test
*/
GLREF_C TBool IsPowerOfTwo(TInt aNum);

/**
Calculates the log2 of a number

@param aNum Number to calulate the log two of
@return The log two of the number passed in
*/
GLREF_C TInt Log2(TInt aNum);

