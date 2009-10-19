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

#ifndef TSPCCLIENTINTERFACE_H
#define TSPCCLIENTINTERFACE_H


class TPeripheralInfo;
class TSenseInfo;
class MTransport;


/**
This class supports the commands defined in the SCSI PRIMARY COMMAND (SPC)
interface for a SCSI client. A minimal set of commands is implemented for
interfacing with SCSI BLOCK devices (SCSI host). The interface class MTransport
is used to send the command via the transport layer.

@internalTechnology

*/
class TSpcClientInterface
    {
public:
	TSpcClientInterface(MTransport& aTransport);
	~TSpcClientInterface();

public:
    MTransport& Transport() const;

	TInt InquiryL(TPeripheralInfo& aInfo);
	TInt RequestSenseL(TSenseInfo& aSenseInfo);
	TInt TestUnitReadyL();
    TInt PreventAllowMediumRemovalL(TBool aPrevent);

private:
    MTransport& iTransport;
    };

/**
Return a reference to the transport.

@return MTransport& Reference to the transport
*/
inline MTransport& TSpcClientInterface::Transport() const
    {
    return iTransport;
    }

#endif // TSPCCLIENTINTERFACE_H