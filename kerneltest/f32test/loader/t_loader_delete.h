// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// \f32test\loader\t_loader_delete.h
// 
//

#ifndef F32TEST_LOADER_T_LOADER_DELETE_H
#define F32TEST_LOADER_T_LOADER_DELETE_H

#include <e32ldr.h>

/** Test that RLoader::Delete handles the case of a bad desciptor being 
passed as a filename 
*/
_LIT(KBadDescriptor,"BadDescriptor");

/** Test cam delete this TCB-writable file with RLoader::Delete. */
_LIT(KTldTcbFile, "c:\\sys\\temp\\tld_target.dat");
/** Test can delete this AllFiles writable file outside of \sys\.  */
_LIT(KTldAllFilesFile, "c:\\private\\12345678\\tld_target.dat");
/** Test cannot delete unqualified filename. */
_LIT(KTldFileNoPath, "tld_target.dat");
/** Test cannot delete filename with no drive. */
_LIT(KTldFileNoDrive, "\\sys\\temp\\tld_target.dat");
/** Test cannot delete this file, which lives in a path that does not exist. */
_LIT(KTldFileNonExist, "c:\\sys\\temp\\tld_target_nonexist.dat");
/** Test cannot delete this file, which does not exist. */
_LIT(KTldFileNonExistRoot, "c:\\tld_target_nonexist.dat");
/** Test cannot delete this file, which does not exist. */
_LIT(KTldFileNonExistDir, "c:\\tld_dir_nonexist\\tld_target_nonexist.dat");
/**
	tld_helper uses this panic category to communicate the result of
	RLoader::Delete to t_loader_delete.
 */

_LIT(KTldPanicCat, "tldpc");

#endif	// #ifndef F32TEST_LOADER_T_LOADER_DELETE_H
