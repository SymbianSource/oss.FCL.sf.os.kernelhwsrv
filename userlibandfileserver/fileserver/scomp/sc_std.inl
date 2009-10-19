// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// f32\scomp\sc_std.inl
// 
//

// Class TCompMount
inline TCompMount::TCompMount(CFileSystem* aFs, CMountCB* aMount)
			{iFs = aFs; iMount=aMount;}

// Class CCompMountCB
inline void CCompMountCB::SetMountNumber(TInt aNum) 
			{iMountNumber=aNum;}		

inline CMountCB* CCompMountCB::RomMount() const 
			{return(iMounts[0].iMount);}

inline void CCompMountCB::NullCompFileSystem(void) 
			{iFileSystem=NULL;}	

// Class CCompFileCB
inline void CCompFileCB::SetTrueFile(CFileCB* aFile) 
			{iTrueFile=aFile;}

inline CFileCB* CCompFileCB::TrueFile() const 
			{return(iTrueFile);}

// Class CCompFileSystem
inline void CCompFileSystem::NullMount(void) 
			{iMount=NULL;}	

