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
// e32test\resourceman\resourceman_psl\rescontrol_extended_psl.h
// 
//

#ifndef __RESCONTROL_EXTENDED_PSL_H__
#define __RESCONTROL_EXTENDED_PSL_H__

//Macro to establish dependency between the specified resource
#define CREATE_DEPENDENCY_BETWEEN_NODES(pN1, pN2, pRes1, pRes2, priRes1, priRes2)			\
	pN1->iPriority = priRes1;																\
	pN1->iResource = pRes1;																	\
	pN1->iPropagatedLevel = 0;																\
	pN1->iVisited = EFalse;																	\
	pN2->iPriority = priRes2;																\
	pN2->iResource = pRes2;																	\
	pN2->iPropagatedLevel = 0;																\
	pN2->iVisited = EFalse;																	\
	pN1->iResource->AddNode(pN2);															\
	pN2->iResource->AddNode(pN1);														

//class definition for multilevel single positive sense dependent resource
NONSHARABLE_CLASS(DMLSLGLSPDependResource) : public DStaticPowerResourceD
	{
public:
    DMLSLGLSPDependResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for multilevel single negative sense dependent resource
NONSHARABLE_CLASS(DMLSIGLSNDependResource) : public DStaticPowerResourceD
	{
public:
    DMLSIGLSNDependResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for binary single postive sense dependent resource
NONSHARABLE_CLASS(DBSIGLSPDependResource) : public DStaticPowerResourceD
	{
public:
    DBSIGLSPDependResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for multilevel shared postive sense dependent resource
NONSHARABLE_CLASS(DMLSHIGLSPDependResource) : public DStaticPowerResourceD
	{
public:
    DMLSHIGLSPDependResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for binary shared negative sense dependent resource
NONSHARABLE_CLASS(DBSHLGLSNDependResource) : public DStaticPowerResourceD
	{
public:
    DBSHLGLSNDependResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for multilevel shared negative sense dependent resource
NONSHARABLE_CLASS(DMLSHLGLSNDependResource) : public DStaticPowerResourceD
	{
public:
    DMLSHLGLSNDependResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

//class definition for multilevel shared custom sense dependent resource
NONSHARABLE_CLASS(DMLSHLGLSCDependResource) : public DStaticPowerResourceD
	{
public:
    DMLSHLGLSCDependResource();
    TInt DoRequest(TPowerRequest &req);
    TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
    TInt iMinLevel;
    TInt iMaxLevel;
    TInt iCurrentLevel;
	TInt iBlockTime; //Time duration the thread will be blocked. 
	};

#endif //__RESCONTROL_EXTENDED_PSL_H__
