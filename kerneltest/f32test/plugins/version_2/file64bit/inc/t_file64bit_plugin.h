/**
* Copyright (c) 1996-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* //File Name:	f32test\plugins\version_2\file64bit\inc\t_file64bit_plugin.h
* //Description: Header file for t_file64bit_plugin.cpp
* 
*
*/




#if !defined(__T_FILE64BIT_PLUGIN_H__)
#define __T_FILE64BIT_PLUGIN_H__

class RFile64BitPlugin : public RPlugin
	{
public:
	TInt DoControl(TInt aFunction,TDes8& a1) const;
	};

#endif
