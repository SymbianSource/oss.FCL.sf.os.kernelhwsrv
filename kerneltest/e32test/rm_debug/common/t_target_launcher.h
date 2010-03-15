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
// Description:
// Definitions for target launcher
// 
//

/**
 @file
 @internalTechnology
 @released
*/

#ifndef RMDEBUG_TARGET_LAUNCHER_H
#define RMDEBUG_TARGET_LAUNCHER_H

// Controls how many times the target applications are launched
const TInt KNumLaunches = 3;

// Controls how many applications are attached and launched
// If changing this, need to make sure there are enough apps
// being built. see KTargetExe and t_rmdebug_app*
const TInt KNumApps = 4;  

_LIT(KLaunchMutexName, "t_rmdebug_launch_mutex");
_LIT(KLaunchMutexNameSearchString, "t_rmdebug_launch_mutex*");
_LIT(KTargetExe,"z:\\sys\\bin\\t_rmdebug_app%d.exe");
_LIT8(KTargetExeName,"t_rmdebug_app%d.exe");
_LIT(KProcessFinder,"*t_rmdebug_app%d*");
_LIT(KTargetOptions,"-f%d");

_LIT(KZSysBin,"z:\\sys\\bin\\");
_LIT(KLauncherExe,"z:\\sys\\bin\\t_rmdebug_target_launcher.exe");

_LIT_SECURITY_POLICY_PASS(KAllowAllPolicy);

#endif // RMDEBUG_TARGET_LAUNCHER_H
