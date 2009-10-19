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
// Global configuration for the tests
// 
//

#ifndef UTL_H
#define UTL_H

#include <e32test.h>

GLREF_D RTest test;

#define TEST_FOR_ERROR( r )	__testForError( r, __LINE__, __FILE__ )
#define __testForError(x,l,f) testForError(x,l,_S(f))
_LIT( KErrorFailString, "Test failed with error %d");
inline void testForError( TInt aCode, TInt aLine, const TText* aFileName )
	{
	if( aCode != KErrNone )
		{
		test.Printf( KErrorFailString, aCode );
		test.operator()( EFalse, aLine, aFileName );
		}
	}

#define TEST_FOR_MATCH( r, m ) __testForMatch( (TUint)(r), (TUint)(m), __LINE__, __FILE__ )
#define __testForMatch(r,m,l,f) testForMatch(r,m,l,_S(f))
_LIT( KMatchFailString, "Test failed, expected %d; got %d");
inline void testForMatch( TUint aValue, TUint aExpected, TInt aLine, const TText* aFileName )
	{
	if( aValue != aExpected )
		{
		test.Printf( KMatchFailString, aExpected, aValue );
		test.operator()( EFalse, aLine, aFileName );
		}
	}



#endif
