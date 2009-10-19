#ifndef __TEST_FACTORY_H
#define __TEST_FACTORY_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* @file TestCaseFactory.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <e32hashtab.h> 
#include <e32test.h>
#include "testdebug.h"
#include "basetestcase.h"

const TInt KTestCaseIdLength(18);

// Test object

extern RTest gtest;
 
/**
*/
namespace NUnitTesting_USBDI
	{
	
/**
This class describes an identity that consists of a string and will
be used for the key for the association map in the test factory
*/
class TStringIdentity
	{
public:
	/**
	Generate a unique has value from the key
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
	
private:
	/**
	The identity as a string symbian descriptor
	*/
	TBuf<64> iIdentity;
	};


/**
*/	
class TBaseTestCaseFunctor
	{
public:
	virtual CBaseTestCase* operator()(TBool) const = 0;
	};

	
/**
This class represents the test case factory
Design pattern used: Singleton,Factory
*/
class RTestFactory
	{
private:
	// The signature for the container of test case associations

	typedef RHashMap<TStringIdentity,TBaseTestCaseFunctor const*> RFactoryMap;
	
public:

	/**
	Destructor, Destroy the test factory
	*/	
	~RTestFactory();

public:
	
	/**
	Register a test case with this factory.  If the test case could not be registered, the resultant
	error will be logged and when requested to be created the factory should state that the test case could
	not be supported.
	@param aTestCaseId the identity of the test case
	@param aCreationMethod the method used to create the test case
	*/
	static void RegisterTestCase(const TDesC& aTestCaseId,TBaseTestCaseFunctor const* aFunctor);	

	/**
	Obtain a test case object that is for the given test case identity
	@param aTestCaseId the identity of the test case
	@return the test case object for the given identity
	@leave KErrNotSupported if the test case object was not found
	*/
	static CBaseTestCase* CreateTestCaseL(const TDesC& aTestCaseId,TBool aHostRole);
	
	/**
	Display through the use of the debug port, a list of all the test cases that 
	have registered themselves with the factory
	*/
	static void ListRegisteredTestCases();

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
This functor class represents the receipt object that when instantiated registers a test case class
in the Test case factory under its test case identity.
*/
template<typename TestCase,typename Parameter>
class TFunctorTestCase : public TBaseTestCaseFunctor
	{
private:
	/**
	The signature of the method to create the test case.  All test cases that have a receipt will have a method of being 
	created by this factory.
	*/
	typedef TestCase* (*TSymbianTwoPhaseCreationMethod)(Parameter);

public:
	/**
	Constructor, builds on instantiation a factory receipt object for a test case
	@param aTestCaseId the identity of the test case for which this is a receipt for
	*/
	explicit TFunctorTestCase(const TDesC& aTestCaseId)
		{
		iCreationMethod = TestCase::NewL;
		RTestFactory::RegisterTestCase(aTestCaseId,this);		
		}
	
	/**
	The invoker function to create a test case
	@param aHostFlag the flag to indicate at construction time a host or client test case
	@return the test case
	*/
	CBaseTestCase* operator()(TBool aHostFlag) const
		{
		return iCreationMethod(aHostFlag);
		}
	
private:
	/**
	The creation method that will creation the test case when instructed to by the factory
	*/
	TSymbianTwoPhaseCreationMethod iCreationMethod;
	};	
	
	
	
	
	

	}

#endif

