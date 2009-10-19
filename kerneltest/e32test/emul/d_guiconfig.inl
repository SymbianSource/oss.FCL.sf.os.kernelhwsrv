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
// e32test\emul\d_guiconfig.inl
// 
//

#ifndef __KERNEL_MODE__
inline TInt RGuiConfigTest::Open()
	{ return DoCreate(KLddName,TVersion(0,1,1),KNullUnit,NULL,NULL); }
inline TInt RGuiConfigTest::GetConfig()
	{ return DoControl(EGetConfig); }
inline TInt RGuiConfigTest::GenerateKeyEvent()
	{ return DoControl(EGenerateKeyEvent); }
inline TInt RGuiConfigTest::Unload()
	{ return User::FreeLogicalDevice(KLddName); }
#endif
