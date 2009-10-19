// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\sfsrv\cl_plugin_shim.cpp
// 
//

#include "cl_std.h"

/*******************************************************
*						   RFs						   *
*******************************************************/

TInt RFs::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{return RSessionBase::SendReceive(aFunction, aArgs);}

/*******************************************************
*						  RFile						   *
*******************************************************/

TInt RFile::CreateSubSession(const RSessionBase& aSession,TInt aFunction,const TIpcArgs& aArgs)
	{return RSubSessionBase::CreateSubSession(aSession, aFunction, aArgs);}

void RFile::CloseSubSession(TInt aFunction)
	{RSubSessionBase::CloseSubSession(aFunction);}

TInt RFile::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{return RSubSessionBase::SendReceive(aFunction, aArgs);}

/*******************************************************
*						  RDir						   *
*******************************************************/

TInt RDir::SendReceive(TInt aFunction,const TIpcArgs& aArgs) const
	{return RSubSessionBase::SendReceive(aFunction, aArgs);}
