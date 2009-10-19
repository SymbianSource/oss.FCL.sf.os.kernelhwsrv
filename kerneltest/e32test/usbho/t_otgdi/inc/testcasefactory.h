// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// @internalComponent
// 
//

#ifndef TESTFACTORY_H
#define TESTFACTORY_H

#include <e32base.h>
#include <e32hashtab.h>
#include <e32test.h>     // RTest headder
#include "testengine.h"

// Test object
extern RTest test;



const TInt KTestCaseIdLength(35); // current longest is 30

// Forward declarations
class CTestCaseRoot;

/**
This class describes an identity that consists of a string and will
be used for the key for the association map in the test factory
*/
class TStringIdentity
	{
public:

	/**
	Constructor, build the identity from a string
	@param anIdentity the string that is used for a unique identity
	*/
	explicit TStringIdentity(const TDesC& anIdentity)
		{
		iIdentity.Copy(anIdentity);
		iIdentity.UpperCase();
		}

	/**
	Generate a unique hash value from the key
	@param aKey the key i.e. string identity
	@return the 32bit unique hash value
	*/
	static TUint32 Hash(const TStringIdentity& aKey)
		{
		return DefaultHash::Des16(aKey.iIdentity);
		}

	/**
	Compare the string identity of a target key with the element key
	@param aKeyTarget the target string identity 
	@param aKeyElement the current element key from the association map
	@return the boolean result
	*/
	static TBool Id(const TStringIdentity& aKeyTarget,const TStringIdentity& aKeyElement)
		{
		return aKeyElement.iIdentity == aKeyTarget.iIdentity;
		}
	
private:

	// The identity as a string symbian descriptor
	TBuf<64> iIdentity;
	
	};

/**
This class represents the test case factory
Design pattern used: Singleton,Factory
*/
class RTestFactory
	{
public:

	/**
	The signature of the method to create the test case.  All test cases that have a receipt will have a method of being 
	created by this factory.
	*/
	typedef CTestCaseRoot* (*TCreationMethod)(TBool);
	typedef RHashMap<TStringIdentity,TCreationMethod> RFactoryMap;


	/**
	Destructor, Destroy the test factory
	*/
	~RTestFactory();

	/**
	Register a test case with this factory.  If the test case could not be registered, the resultant
	error will be logged and when requested to be created the factory should state that the test case could
	not be supported.
	@param aTestCaseId the identity of the test case
	@param aCreationMethod the method used to create the test case
	*/
	static void RegisterTestCase(const TDesC& aTestCaseId,
	  TCreationMethod aTestCreationMethod);	
	
	/**
	Obtain a test case object that is for the given test case identity
	@param aTestCaseId the identity of the test case
	@return the test case object for the given identity
	@leave KErrNotSupported if the test case object was not found
	*/
	static CTestCaseRoot* CreateTestCaseL(const TDesC& aTestCaseId);
	
	/**
	Display through the use of the debug port, a list of all the test cases that 
	have registered themselves with the factory
	*/
	static void ListRegisteredTestCases(RPointerArray<HBufC> & testCaseNames);
	static void ListRegisteredTestCases() { RPointerArray<HBufC> test;ListRegisteredTestCases(test);test.ResetAndDestroy();};
	
	static TInt TestCaseCount() {	return(Instance().iTestCases.Count());	}
	
	static void GetTestID(TInt aIndex, TBuf<KTestCaseIdLength> &aTestID);
	
	static TBool TestCaseExists(const TDesC& aTestCaseId);

private:

	/**
	Constructor
	*/
	RTestFactory();
	
	/**
	Disable copy constructor
	*/
	RTestFactory(const RTestFactory& aRef);
		
	/**
	Disable assignment operator
	*/
	RTestFactory& operator=(const RTestFactory& aRhs);
		
	/**
	Retrieve the factory singleton instance
	@return the only instance of this class
	*/
	static RTestFactory& Instance();	
	
private:

	/**
	The association between the test cases identities and the test case objects
	that have registered themselves with the factory (i.e. that are available)	
	*/
	RFactoryMap iTestCases;
	
	};
	
/**
This class represents the receipt object that when instantiated registers a test case class
in the Test case factory under its test case identity.
*/
template<typename T> // T: The test case class	
class TTestCaseFactoryReceipt
	{
public:
	/**
	Called by the factory when the _TestCaseClass needs to be instantiated
	@param aAutomation whether to bypass manual actions
	@return a pointer to the required test case object
	*/
	static CTestCaseRoot* CreateTestCaseL(TBool aAutomation)
		{
		// Use the symbian factory two phase constructor
		return T::NewL(aAutomation);
		}
	
	/**
	Constructor, builds on instantiation a factory receipt object for a test case
	@param aTestCaseId the identity of the test case for which this is a receipt for
	*/
	explicit TTestCaseFactoryReceipt(const TDesC& aTestCaseId)
		{
		RTestFactory::RegisterTestCase(aTestCaseId, CreateTestCaseL);		
		}
	};



#endif // TESTFACTORY_H

