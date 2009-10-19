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
// f32\sfat32\inc\sl_leafdir_cache.inl
// 
//

/**
 @file
 @internalTechnology
 
 Overloaded == operator 
 
 @param	aDirPos	the position to compare
 @return	EFalse	if aDirPos matches self, else
 ETrue
*/
TBool TLeafDirData::operator==(const TLeafDirData &aDirPos) const
	{
	return (aDirPos.iClusterNum == iClusterNum);
	}

/**
Overloaded != operator 

@param	aDirPos	the position to compare
@return	EFlase	if aDirPos matches self, else
		ETrue
*/
TBool TLeafDirData::operator!=(const TLeafDirData &aDirPos) const 
	{
	return (aDirPos.iClusterNum != iClusterNum);
	}

/**
'Get' function to retrieve the 'parent' node

@return	the parent node   
*/
CLeafDirTreeNode* CLeafDirTreeNode::Parent()
	{
	return iParent;
	}

/**
Set Parent node

@param	the parent node to be set   
*/
void CLeafDirTreeNode::SetParent(CLeafDirTreeNode* aNode)
	{
	iParent = aNode;
	}

/**
'Get' function to retrieve children nodes

@return	the children nodes   
*/
RPointerArray<CLeafDirTreeNode>& CLeafDirTreeNode::Children()
	{
	return iChildren;
	}

/**
'Get' function to retrieve dir location store by this node 

@return	the location of the directory
*/
TUint32 CLeafDirTreeNode::StartClusterNum() const
	{
	return iLeafDirData.iClusterNum;
	}

const TLeafDirData& CLeafDirTreeNode::LeafDirData() const
	{
	return iLeafDirData;
	}

/**
Set position of the direcotry this node represents.

@param	aDirPos	the position to be set
*/
void CLeafDirTreeNode::SetLeafDirData(const TLeafDirData& aLeafDirData)
	{
	iLeafDirData = aLeafDirData;
	}

/**
'Get' function to retrieve dir path store by this node 

@return	the path of the directory
*/
const TDesC& CLeafDirTreeNode::Path() const
	{
	return iPath;
	}

/**
Test if self is ERoot type

@return	ETrue if self is ERoot
		EFalse	if self is of other types
*/
TBool CLeafDirTreeNode::IsRoot() const 
	{
	return (iNodeType == ERoot);
	}

/**
Test if self is ELeaf type

@return	ETrue if self is ELeaf
		EFalse	if self is of other types
*/
TBool CLeafDirTreeNode::IsLeaf() 
	{
	return (iNodeType == ELeaf);
	}

/**
Test if self is ELeafIntermediary type

@return	ETrue if self is ELeafIntermediary
		EFalse	if self is of other types
*/
TBool CLeafDirTreeNode::IsLeafIntermediary() 
	{
	return (iNodeType == ELeafIntermediary);
	}

/**
Test if self is EPureIntermediary type

@return	ETrue if self is EPureIntermediary
		EFalse	if self is of other types
*/
TBool CLeafDirTreeNode::IsPureIntermediary() 
	{
	return (iNodeType == EPureIntermediary);
	}

