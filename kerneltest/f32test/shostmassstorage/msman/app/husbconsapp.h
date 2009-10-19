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
//



/**
 @file
 @internalComponent
*/

#ifndef HUSBMSCONSAPP_H
#define HUSBMSCONSAPP_H




class CHeartBeat : public CActive
	{
public:
	static CHeartBeat* NewLC(CDisplay& iDisplay);
private:
	CHeartBeat(CDisplay& iDisplay);
	void ConstructL();
	~CHeartBeat();

private:
	void RunL();
	void DoCancel();

private:
	CDisplay& iDisplay;
	RTimer iTimer;
	TUint iUpTime;
	};






#endif // HUSBCONSAPP_H
