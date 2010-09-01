// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\drivers\camerasc.inl
// 
//

inline DCameraScPdd* DCameraScLdd::Pdd()
	{return((DCameraScPdd*)iPdd);}

inline DThread* DCameraScLdd::OwningThread()
	{return(iOwningThread);}

inline TInt DCameraScLdd::CurrentFrameHeight()
	{return(iCaptureModeConfig[iCaptureMode].iFrameHeight);}

inline TInt DCameraScLdd::CurrentFrameWidth()
	{return(iCaptureModeConfig[iCaptureMode].iFrameWidth);}

inline TInt DCameraScPdd::SpecifyCustomConfig(TDes8& /*aConfig*/)
	{return(KErrArgument);}

inline TBool TCameraScRequestQueue::IsEmpty()
	{return(iPendRequestQ.IsEmpty());}
