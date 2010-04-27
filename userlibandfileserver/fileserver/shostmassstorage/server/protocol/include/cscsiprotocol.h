// Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CSCSIPROTOCOL_H
#define CSCSIPROTOCOL_H

class CMassStorageFsm;

class TSbcClientInterface;

class RMediaChangeNotifier
    {
public:
    RMediaChangeNotifier();
    ~RMediaChangeNotifier();

	void Register(const RMessage2& aMessage);
    void DoNotifyL();
    void DoCancelL();

private:
	void CompleteNotifierL(TInt aReason);

private:
    /** Notification service */
	RMessage2 iNotifier;
    /** Flag to indicate that media change notification is active */
	TBool iRegistered;
    };


/**
SCSI Protocol procedures
*/
class CScsiProtocol: public CBase, public MProtocol, public MBlockTransferProtocol
    {
public:
   /** SCSI state */
   enum TScsiState
        {
        EEntry,
        EMediaNotPresent,
        EConnected,
        EDisconnected
        };

    static CScsiProtocol* NewL(TLun aLun, MTransport& aTransport);
	~CScsiProtocol();
private:
    void ConstructL(TLun aLun);
	CScsiProtocol(MTransport& aTransport);

public:
    void InitialiseUnitL();

    // Stop unit command to uninitialise the lun
    void UninitialiseUnitL();

    // Read command to read the media
    void ReadL(TPos aPos, TDes8& aCopybuf, TInt aLen);

    // Write command to write to the media
    void WriteL(TPos aPos, TDesC8& aCopybuf, TInt aLen);

    // ReadCapacity command to find the capacity of the media
    void GetCapacityL(TCapsInfo& aCapsInfo);

    // unit testing
    void CreateSbcInterfaceL(TUint32 aBlockLen, TUint32 aLastLba);

	TBool DoScsiReadyCheckEventL();

	void NotifyChange(const RMessage2& aMessage);
    void ForceCompleteNotifyChangeL();
    void CancelChangeNotifierL();

    void SuspendL();
	void ResumeL();
    TBool IsConnected();

    // Supported Mass Storage commands
    TInt MsInquiryL();
    TInt MsTestUnitReadyL();
    TInt MsStartStopUnitL(TBool aStart);
    TInt MsPreventAllowMediaRemovalL(TBool aPrevent);

    TInt MsReadCapacityL();
    TInt MsModeSense10L();
    TInt MsModeSense6L();

    TInt MsRequestSenseL();

    TBool MsIsSbcSet() const;
    TBool MsIsRemovableMedia() const;
    const TSenseInfo& MsSenseInfo() const;

    // MBlockTransferProtocol interface
    void BlockReadL(TPos aPos, TDes8& aBuf, TInt aLength);
    void BlockWriteL(TPos aPos, TDesC8& aBuf, TUint aOffset, TInt aLength);

private:
    void ResetSbc();

    TInt DoCheckConditionL();

	TInt GetSystemWideSenseError(const TSenseInfo& aSenseInfo);
	TInt ProcessAsCodes(const TSenseInfo& aSenseInfo);

private:

    /** State machine for device initialisation protocol */
    CMassStorageFsm* iFsm;

    /** SCSI SPC interface methods */
    TSpcClientInterface iSpcInterface;
    /** SCSI SBC interface methods */
    TSbcClientInterface* iSbcInterface;
    // buffers for block manipulation (for use in iSbcInterface)
    RBuf8 iHeadbuf;
	RBuf8 iTailbuf;    		

    // Logical Unit properties
    /** LU removable */
    TBool iRemovableMedia;
    /** LU write protected */
    TBool iWriteProtect;

    /** Result of the last SCSI command */
    TSenseInfo iSenseInfo;

    /** State of the LUN represented by this object */
	TScsiState iState;

    /** Notifier for media changes */
    RMediaChangeNotifier iMediaChangeNotifier;
    };


/**
Returns the state of the SBC interface. INQUIRY command is used to detect if
device supports SBC and initialise the SBC interface.

@return TBool ETrue is SBC interface is initialised
*/
inline TBool CScsiProtocol::MsIsSbcSet() const
    {
    return iSbcInterface ? ETrue : EFalse;
    }

/**
Returns the device removable property. MODE SENSE command is used to detect if
the device is removable.

@return TBool ETrue if device is removable
*/
inline TBool CScsiProtocol::MsIsRemovableMedia() const
    {
    return iRemovableMedia;
    }


/**
Helper to return current SenseInfo. SCSI REQUEST SENSE command sets the sense
info. The protocol will retrieve device's sense info in the event of a SCSI
command error.

@return const TSenseInfo&
*/
inline const TSenseInfo& CScsiProtocol::MsSenseInfo() const
    {
    return iSenseInfo;
    }

#endif // CSCSIPROTOCOL_H
