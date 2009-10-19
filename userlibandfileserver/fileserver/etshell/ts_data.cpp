// Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\etshell\ts_data.cpp
// 
//

#include "ts_std.h"

GLDEF_D TVersion TheShellVersion(KShellMajorVersionNumber,KShellMinorVersionNumber,KShellBuildVersionNumber);

CShell* ShellFunction::TheShell=NULL;
TBuf<KMaxFileName> CShell::drivePaths[26]={_L("A:\\"),_L("B:\\"),_L("C:\\"),_L("D:\\"),_L("E:\\"),_L("F:\\"),_L("G:\\"),_L("H:\\"),_L("I:\\"),_L("J:\\"),_L("K:\\"),_L("L:\\"),_L("M:\\"),
	_L("N:\\"),_L("O:\\"),_L("P:\\"),_L("Q:\\"),_L("R:\\"),_L("S:\\"),_L("T:\\"),_L("U:\\"),_L("V:\\"),_L("W:\\"),_L("X:\\"),_L("Y:\\"),_L("Z:\\")};//One for each drive letter
