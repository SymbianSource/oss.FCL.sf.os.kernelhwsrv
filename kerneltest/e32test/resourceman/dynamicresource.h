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
// e32test\resourceman\dynamicresource.h
// 
//

#ifndef __DYNAMICRESOURCE_H__
#define __DYNAMICRESOURCE_H__

//Class definitions of dynamic resource
//Binary single postive dynamic resource
NONSHARABLE_CLASS(DBIGISSPDynamicResource) : public DDynamicPowerResource
	{
public:
	DBIGISSPDynamicResource();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	};

//Multilevel shared negative dynamic resource
NONSHARABLE_CLASS(DMLIGLSSHNDynamicResource) : public DDynamicPowerResource
	{
public:
	DMLIGLSSHNDynamicResource();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	TInt iBlockTime;
	};

//Binary shared negative dynamic resource
NONSHARABLE_CLASS(DBLGLSSHNDynamicResource) : public DDynamicPowerResource
	{
public:
	DBLGLSSHNDynamicResource();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	TInt iBlockTime;
	};

//Multilevel shared postive dynamic resource
NONSHARABLE_CLASS(DMLLGLSSHPDynamicResource) : public DDynamicPowerResource
	{
public:
	DMLLGLSSHPDynamicResource();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	TInt iBlockTime;
	};

//Multilevel single positive dynamic dependent resource
NONSHARABLE_CLASS(DDynamicResourceD01) : public DDynamicPowerResourceD
	{
public:
	DDynamicResourceD01();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	TInt iBlockTime;
	};
	
//Binary shared positive dynamic dependent resource
NONSHARABLE_CLASS(DDynamicResourceD02) : public DDynamicPowerResourceD
	{
public:
	DDynamicResourceD02();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	TInt iBlockTime;
	};

//Multilevel shared negative dependent resource
NONSHARABLE_CLASS(DDynamicResourceD03) : public DDynamicPowerResourceD
	{
public:
	DDynamicResourceD03();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	TInt iBlockTime;
	};

//Binary single positive dependent resource
NONSHARABLE_CLASS(DDynamicResourceD04) : public DDynamicPowerResourceD
	{
public:
	DDynamicResourceD04();
	TInt DoRequest(TPowerRequest &req);
	TInt GetInfo(TDes8* aInfo) const;
	TChangePropagationStatus TranslateDependentState(TInt aDepId, TInt aDepState, TInt& aResState);
private:
	TInt iMinLevel;
	TInt iMaxLevel;
	TInt iCurrentLevel;
	TInt iBlockTime;
	};

#endif /*__DYANMICRESOURCE_H__*/
