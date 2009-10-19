// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Header file for Kernel Performance Logger test
// 
//

/**
 @file
*/

#ifndef T_PERFLOGGER_H__
#define T_PERFLOGGER_H__

#include <d32btrace.h>

//-------------------------------------------------------------------------------------

/** Unrolled trace record layout for simplier access to the record fields */
struct TTraceLayout
	{
    TInt        iSize;
    TUint       iFlags;
    TUint       iCategory;
    TUint       iSubCategory;
	
    TUint32     iHeader2;
    TUint32     iTimestamp;
    TUint32     iTimestamp2;
    TUint32     iContext;
    TUint32     iPC;
    TUint32     iExtra;
	
    TInt        iDataWords;    ///< number of 32 bit words in the trace record "data" section. 0 means that tere is no data.
    const TUint32*    ipData;  ///< pointer to the data.
	
    TTraceLayout()
		{
        iSize=0; iFlags=0; iCategory=0; iSubCategory=0; iHeader2 =0;        
        iTimestamp=0; iTimestamp2=0; iContext=0; iPC=0; iExtra=0; iDataWords=0;            
        ipData=NULL;
		}
	
	};


//-------------------------------------------------------------------------------------

void  Initialise();
void  Finalise();
TUint ParseTraceRecord(const TUint8* apRecord, TTraceLayout& aTraceLayout);
void  PrintTraceRecord(const TTraceLayout& aTraceLayout);
TUint URnd(TUint aMin, TUint aMax);
TUint URnd(TUint aMax);


#endif //T_PERFLOGGER_H__
