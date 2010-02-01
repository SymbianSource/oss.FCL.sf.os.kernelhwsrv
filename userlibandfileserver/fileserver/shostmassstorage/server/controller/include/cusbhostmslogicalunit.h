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
//

/**
 @file
 @internalTechnology
*/

#ifndef CUSBHOSTMSLOGICALUNIT_H
#define CUSBHOSTMSLOGICALUNIT_H


class MTransport;
class MProtocol;
class THostMassStorageConfig;
class TReadWrite;

/**
This class represents a Logical Unit supporting SCSI Mass Storage Class. The
class provides inteface methods to read, write and get the media capacity of the
Logical Unit. Each instance is identified by the LUN.
*/
class CUsbHostMsLogicalUnit : public CBase
{
public:
	static CUsbHostMsLogicalUnit* NewL(TLun aLun);
	~CUsbHostMsLogicalUnit();
private:
    void ConstructL();
    CUsbHostMsLogicalUnit(TLun aLun);

public:
	void InitL();
	void UnInitL();
	void ReadL(const RMessage2& aMessage);
	void WriteL(const RMessage2& aMessage);
	void EraseL(const RMessage2& aMessage);
	void CapsL(const RMessage2& aMessage);

	void NotifyChange(const RMessage2& aMessage);
    void ForceCompleteNotifyChangeL();
    void CancelChangeNotifierL();

	void ReadyToSuspend();
	TBool IsConnected();
	TBool IsReadyToSuspend();
	void CancelReadyToSuspend();

	void SuspendL();
	void ResumeL();
	void DoLunReadyCheckL();

	TInt InitialiseProtocolL(TLun aLun,
                             THostMassStorageConfig& aConfig,
                             MTransport& aTransport);

    TLun Lun() const;

private:
    TInt CheckPosition(const TReadWrite& aReadWrite);

    /** The Protocol interface (Owned by this class) */
	MProtocol* iProtocol;
    /** LUN */
    TLun iLun;
    /** The Size (updated by CapsL) */
    TPos iSize;

	TBool iSuspendRequest;
    
    RBuf8 iDataBuf;
};


/**
Get the LUN

@return TLun The LUN
*/
inline TLun CUsbHostMsLogicalUnit::Lun() const
    {
    return iLun;
    }

#endif // CUSBHOSTMSLOGICALUNIT_H

