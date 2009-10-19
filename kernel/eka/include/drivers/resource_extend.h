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
// e32\include\drivers\resource_extend.h
// 
// WARNING: This file contains some APIs which are internal and are subject
//          to change without notice. Such APIs should therefore not be used
//          outside the Kernel and Hardware Services package.
//

#ifndef __RESOURCE_EXTEND_H__
#define __RESOURCE_EXTEND_H__

#include <drivers/resource.h>

#define ADD_DEPENDENCY_NODE(aNode, aDependencyList)																	\
	{																												\
	SNode* pDL = aDependencyList;																					\
	SNode* prev = NULL;																								\
	if(pDL == NULL)																									\
		{																											\
		aDependencyList = aNode;																					\
		aNode->iNext = NULL;																						\
		}																											\
	else																											\
		{																											\
		while(pDL != NULL)																							\
			{																										\
			if(aNode->iPriority == pDL->iPriority)																	\
				return KErrAlreadyExists;																			\
			if(aNode->iPriority < pDL->iPriority)																	\
				{																									\
				if(prev == NULL) /*Add it to the head	*/															\
					{																								\
					aDependencyList = aNode;																		\
					aNode->iNext = pDL;																				\
					break;																							\
					}																								\
				prev->iNext = aNode;																				\
				aNode->iNext = pDL;																					\
				break;																								\
				}																									\
			if(pDL->iNext == NULL) /* Add it to the end	*/															\
				{																									\
				pDL->iNext = aNode;																					\
				aNode->iNext = NULL;																				\
				break;																								\
				}																									\
			prev = pDL;																								\
			pDL = pDL->iNext;																						\
			}																										\
		}																											\
	}

//Check whether the priority already exists
#define CHECK_IF_PRIORITY_ALREADY_EXISTS(aDependencyList, aPriority)			\
	{																			\
	for(SNode* node = aDependencyList; node != NULL; node = node->iNext)		\
		{																		\
		if(node->iPriority == aPriority)										\
			return KErrAlreadyExists;											\
		}																		\
	}


static const TUint KIdMaskStaticWithDependencies	= 0x00010000;
static const TUint KIdMaskDynamic					= 0x00020000;
static const TUint KIdMaskDynamicWithDependencies	= 0x00030000;
static const TUint KIdMaskResourceWithDependencies = 0x00010000;
static const TInt KDynamicResourceDeRegistering = -2; 

struct SNode;
struct SPowerResourceClientLevel;

//Various stages of resource dependency state change operation.
enum TPropagation
	{
	EChangeStart,
	ECheckChangeAllowed,
	ERequestStateChange,
	EIssueNotifications
	};

//Return value of translate dependency state function. This is implemented by PSL for each resource.
enum TChangePropagationStatus {EChange, ENoChange, EChangeNotAccepted};

/**
@publishedPartner
@prototype 9.5
class to represent dynamic resources
*/
class DDynamicPowerResource : public DStaticPowerResource
	{
public:
	IMPORT_C DDynamicPowerResource(const TDesC8& aName, TInt aDefaultLevel);
	IMPORT_C ~DDynamicPowerResource();
public:
	TBool InUse(); //Used by RC on deregistration to see if another client is having requirement on this resource
	inline void Lock() {++iCount;} //Resource is locked whenever operation is scheduled in RC thread. 
	inline void UnLock() {--iCount;}
	inline TUint LockCount() { return iCount;}
protected:
	TUint iCount;
	TUint iOwnerId; //Stores the ID of the client that registers the resource
	friend class DPowerResourceController;
	};

/**
@publishedPartner
@prototype 9.5
*/
typedef TBool (*TDependencyCustomFunction) (TInt& /*aClientId*/,
                                            const TDesC8& /*aClientName*/,
                                            TUint /*aResourceId*/,
                                            TCustomOperation /*aCustomOperation*/,
                                            TInt& /*aLevel*/,
                                            TAny* /*aLevelList*/,
                                            TAny* /*aResourceLevelList */,
                                            TAny* /*aReserved*/); // For future use

/**
@publishedPartner
@prototype 9.5
class to represent static resource with dependency
*/
class DStaticPowerResourceD : public DStaticPowerResource
	{
public:
	DStaticPowerResourceD(const TDesC8& aName, TInt aDefaultLevel);
	TInt AddNode(SNode* aNode);
	virtual TInt HandleChangePropagation(TPowerRequest aRequest, TPropagation aProp, TUint aOriginatorId, const TDesC8& aOriginatorName);
	virtual TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState) = 0;
public:
	SPowerResourceClientLevel* iResourceClientList; //To capture the dependent resource requirement on this resource
	TDependencyCustomFunction iDepCustomFunction;
private:
	SNode* iDependencyList; //Dependency resource list
	friend class DPowerResourceController;
	};

/**
@publishedPartner
@prototype 9.5
class to represent dynamic resource with dependency
*/
class DDynamicPowerResourceD : public DDynamicPowerResource
	{
public:
	IMPORT_C DDynamicPowerResourceD(const TDesC8& aName, TInt aDefaultLevel);
	IMPORT_C ~DDynamicPowerResourceD();
	IMPORT_C virtual TInt HandleChangePropagation(TPowerRequest aRequest, TPropagation aProp, TUint aOriginatorId, const TDesC8& aOriginatorName);
	virtual TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState) = 0;
public:
	SPowerResourceClientLevel* iResourceClientList; //To capture the dependent resource requirement on this resource
	TDependencyCustomFunction iDepCustomFunction;
private:
	SNode* iDependencyList; //Dependency resource list
	friend class DPowerResourceController;
	};

/**
@publishedPartner
@prototype 9.5
structure to represent resource dependency information. This is used when registering resource dependency
*/
struct SResourceDependencyInfo
	{
	TUint iResourceId;
	TUint8 iDependencyPriority;
	};

/**
@publishedPartner
@prototype 9.5
structure to encapsulate dependent resource information. 
*/
struct SNode
	{
	DStaticPowerResourceD* iResource;
	TInt iPropagatedLevel;
	TUint8 iRequiresChange;
	TUint8 iVisited;
	TUint8 iPriority;
	TUint8 iSpare;
	SNode* iNext;
	};

#endif
