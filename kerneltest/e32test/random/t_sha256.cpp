// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Description: e32test/random/tsha256.cpp
//

//---------------------------------------------------------------------------------------------------------------------
//! @SYMTestCaseID				KBASE-sha256-2701
//! @SYMTestType				UT
//! @SYMTestCaseDesc			Verifies the implementation of SHA256 used by the Secure RNG
//! @SYMPREQ					PREQ211
//! @SYMTestPriority			High
//! @SYMTestActions				
//! 	1. 	Tests the correct operation of the SHA256 implementation used by the Secure RNG using publically published
//!         test vectors and corresponding outputs.
//! 
//! @SYMTestExpectedResults
//! 	1.	The implementation should always return the expected outputs for the given test vectors.
//---------------------------------------------------------------------------------------------------------------------

//epoc include
#include <e32test.h>
//user include
#include "sha256.h"


//RTest for testing SHA256
RTest test(_L("Unit Test For SHA256"));

//Test data input for SHA256 taken from FIPS 180-2 and openssl
_LIT8(KTestData1, "\x61\x62\x63");
_LIT8(KTestData2, "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10");
_LIT8(KTestData3, "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x01\x02\x03\x04");

//Expected output for the above test data input for SHA256 taken from FIPS 180-2
_LIT8(KTestVector1, "\xba\x78\x16\xbf\x8f\x01\xcf\xea\x41\x41\x40\xde\x5d\xae\x22\x23\xb0\x03\x61\xa3\x96\x17\x7a\x9c\xb4\x10\xff\x61\xf2\x00\x15\xad");
_LIT8(KTestVector2, "\x91\x1B\x64\x76\x69\x49\xA2\xE8\x56\xF1\xB6\xC3\x50\x1D\x5A\x6B\xF1\x7D\xD5\x0B\x6A\x78\xD6\x09\x3A\xFC\x42\x52\xD2\xF7\x1A\x18");
_LIT8(KTestVector3, "\x27\x53\x57\xF9\x38\x73\xAF\xFF\xF0\x0C\x4A\x83\x04\x33\xCA\x51\x37\xCC\x32\x7D\xDF\xB1\x5C\x46\xD6\xCD\x8A\x0A\x8A\x6E\x48\x3C");

/*
Functionality test for SHA256 algorithm
*/
void Sha256FunctionalityTest(const TDesC8& aMessageData, const TDesC8& aHashOfMessageData)
	{
	TBuf8<KSHA256HashSize> hash; // temp buffer
	SHA256 sha256; 
	sha256.Update(aMessageData.Ptr(), aMessageData.Length());
	hash.Copy(sha256.Final().Ptr(),KSHA256HashSize);
	TInt compareVal = aHashOfMessageData.Compare(hash);
	test(compareVal == 0);
	}

/*
Basic functionality test for Sha256
*/
void SHA2Tests()
	{
	//functionality test for Hash Algorithm using short message data (3 bytes in length)
	Sha256FunctionalityTest(KTestData1(), KTestVector1());
	
	//functionality test for Hash Algorithm using sha256 block size message data (64 bytes)
	Sha256FunctionalityTest(KTestData2(), KTestVector2());
	
	//functionality test for Hash Algorithm using long message data (68 bytes)
	Sha256FunctionalityTest(KTestData3(), KTestVector3());
	}

/*
Main function for sha256 algorithm testing
*/
GLDEF_C TInt E32Main(void)
	{
	test.Title();
	test.Start(_L(" SHA256 Algorithm Test \n"));		 
		
	CTrapCleanup* cleanup=CTrapCleanup::New();
	test(cleanup != NULL);
	        
	__UHEAP_MARK;
	SHA2Tests();		
	__UHEAP_MARKEND;

	test.End();
	delete cleanup;
	return KErrNone;
	}



