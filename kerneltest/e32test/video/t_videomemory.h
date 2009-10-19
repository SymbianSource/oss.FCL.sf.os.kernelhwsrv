/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#ifndef T_VIDEOMEMORY_H
#define T_VIDEOMEMORY_H

// Constants for testing of videomory. 
// There is no science behind the values - they are just different numbers 
// that are expected to read back the same as they were written. The purpose
// is not to validate that the video memory as SUCH works, so no need for covering
// all sorts of different bit patterns, walking ones/zeros. We just need to have a 
// way to see that we can write something, and that it changes if a differnet value
// written. 

const TUint32 KTestValue1 = 0x12345678;  
const TUint32 KTestValue2 = 0x87654321;
const TUint32 KTestValue3 = 0x11223344;
const TUint32 KTestValue4 = 0x44332211;

#endif
