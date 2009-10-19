#ifndef __TEST_ENDPOINT_BASE_H
#define __TEST_ENDPOINT_BASE_H

/*
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* @file testendpointbase.h
* @internalComponent
* 
*
*/



#include <e32base.h>
#include <d32usbc.h>

namespace NUnitTesting_USBDI
	{

/**
This class describes a resource endpoint for use in transfers.
An endpoint is configured with respect to the host so an 'in' endpoint
is one where data is transfered to the host and an 'out' endpoint is where
data is received from the host
*/
class TEndpoint
	{
	friend class CInterfaceBase;
	friend class CInterfaceSettingBase;
	friend class CControlXfer;
	
public:
	// Compiler copy constructor and assignment ok
	
protected:
	/**
	Constructor 
	@param aEndpointNumber the number of the endpoint on the setting for the interface
		   specify only EEndpoint1, EEndpoint2, EEndpoint3, EEndpoint4 or EEndpoint5
	*/
	explicit TEndpoint(TEndpointNumber aEndpointNumber) : iEndpointNumber(aEndpointNumber)
		{
		}

	/**
	Non-virtual Destructor
	*/
	~TEndpoint()
		{
		}
	
protected:
	/**
	The information for the endpoint
	*/
	TUsbcEndpointInfo iEndpointInfo;
	
	/**
	The number for the endpoint w.r.t the interface it is associated with
	*/
	TEndpointNumber iEndpointNumber;
	};
	
	
/**
This class represents an endpoint that a device can use to send
data to the host via a transfer pipe
*/
class TInEndpoint : public TEndpoint
	{
protected:
	/**
	Constructor, build an endpoint that will be used to transfer data
	to the host
	@param the number for the endpoint
	*/
	explicit TInEndpoint(TEndpointNumber aEndpointNumber)
	:	TEndpoint(aEndpointNumber)
		{
		iEndpointInfo.iDir = KUsbEpDirIn;
		}
	};
	
	
/**
This class represents an endpoint that a device can use to receive
data from the host via a transfer pipe
*/
class TOutEndpoint : public TEndpoint
	{
protected:
	/**
	Constructor, build an endpoint that will be used to receive data
	from the host in a transfer
	@param the number for the endpoint
	*/
	explicit TOutEndpoint(TEndpointNumber aEndpointNumber)
	:	TEndpoint(aEndpointNumber)
		{
		iEndpointInfo.iDir = KUsbEpDirOut;
		}
	};
	
	
/**
This class represents an allocated endpoint for bi-directional capacity
via a transfer pipe
*/
class TBiEndpoint : public TEndpoint
	{
protected:
	/**
	Constructor, build an endpoint that will be used to send/receive data with host
	@param the number for the endpoint
	*/
	explicit TBiEndpoint(TEndpointNumber aEndpointNumber)
	:	TEndpoint(aEndpointNumber)
		{
		iEndpointInfo.iDir = KUsbEpDirBidirect;
		}
	};
	
/**
This class describes an endpoint for Bulk in tansfers
*/
class TBulkInEndpoint : public TInEndpoint
	{
public:
	/**
	Constructor, build a bulk in endpoint
	@param the number for the endpoint
	*/
	explicit TBulkInEndpoint(TEndpointNumber aEndpointNumber)
	:	TInEndpoint(aEndpointNumber)
		{
		iEndpointInfo.iType = KUsbEpTypeBulk;
		}
	};
	
/**
This class describes an endpoint for Bulk out transfers
*/
class TBulkOutEndpoint : public TOutEndpoint
	{
public:
	/**
	Constructor, build a bulk out endpoint
	@param the number for the endpoint
	*/
	explicit TBulkOutEndpoint(TEndpointNumber aEndpointNumber)
	:	TOutEndpoint(aEndpointNumber)
		{
		iEndpointInfo.iType = KUsbEpTypeBulk;
		}
	};
		

/**
This class describes an endpoint for Control transfers
*/
class TCtrlEndpoint : public TBiEndpoint
	{
public:
	/**
	Constructor, build a control endpoint
	@param the number for the endpoint
	*/
	explicit TCtrlEndpoint(TEndpointNumber aEndpointNumber)
	:	TBiEndpoint(aEndpointNumber)
		{
		iEndpointInfo.iType = KUsbEpTypeControl;
		}
	};
	
	
/**
This class describes an endpoint for Interrupt in transfers
*/
class TIntInEndpoint : public TInEndpoint
	{
public:
	/**
	Constructor, build a interrupt in endpoint
	@param the number for the endpoint
	@param the interrupt transfer polling interval for the host
	*/
	explicit TIntInEndpoint(TEndpointNumber aEndpointNumber,TInt aPollingInterval)
	:	TInEndpoint(aEndpointNumber)
		{
		iEndpointInfo.iType = KUsbEpTypeInterrupt;
		iEndpointInfo.iInterval = aPollingInterval;
		}
	};
	
/**
This class describes an endpoint for Interrupt out transfers
*/
class TIntOutEndpoint : public TOutEndpoint
	{
public:
	/**
	Constructor, build an interrupt out endpoint
	@param the number for the endpoint
	*/
	explicit TIntOutEndpoint(TEndpointNumber aEndpointNumber)
	:	TOutEndpoint(aEndpointNumber)
		{
		iEndpointInfo.iType = KUsbEpTypeInterrupt;
		}
	};

/**
This class describes an endpoint for Isochronous in transfers
*/
class TIsochInEndpoint : public TInEndpoint
	{
public:
	/**
	Constructor, build an isochronous in endpoint
	@param the number for the endpoint
	*/
	explicit TIsochInEndpoint(TEndpointNumber aEndpointNumber)
	:	TInEndpoint(aEndpointNumber)
		{
		iEndpointInfo.iType = KUsbEpTypeIsochronous;
		}
	};

/**
This class describes an endpoint for Isochronous out transfers
*/
class TIsochOutEndpoint : public TOutEndpoint
	{
public:
	/**
	Constructor, build an isochronous out endpoint
	@param the number for the endpoint
	*/
	explicit TIsochOutEndpoint(TEndpointNumber aEndpointNumber)
	:	TOutEndpoint(aEndpointNumber)
		{
		iEndpointInfo.iType = KUsbEpTypeIsochronous;
		}
	};
	
	
	}
	
	
#endif

