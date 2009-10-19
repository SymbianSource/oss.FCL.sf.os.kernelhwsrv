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
// e32test\emul\d_guiconfig.h
// 
//

#ifndef __D_GUICONFIG_H__
#define __D_GUICONFIG_H__

#include <e32cmn.h>

#ifndef __KERNEL_MODE__
#include <e32std.h>
#endif

_LIT(KLddName,"Test");

class TCapsTestV01
	{
public:
	TVersion	iVersion;
	};

class RGuiConfigTest : public RBusLogicalChannel
	{
public:
	enum TControl
		{
		EGetConfig=0,
		EGenerateKeyEvent=1
		};
public:
	inline TInt Open();
	inline TInt GetConfig();
	inline TInt GenerateKeyEvent();
	static inline TInt Unload();
	};

#include "d_guiconfig.inl"

#endif //__D_GUICONFIG_H__
