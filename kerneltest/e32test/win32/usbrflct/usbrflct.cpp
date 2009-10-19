// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\win32\usbrflct\usbrflct.cpp
// Win32 USB test program USBRFLCT: performs I/O with a device running the
// Symbian OS test program T_USB, using the generic USB device driver USBIO.
// === Includes ===
// 
//

#include <windows.h>
#include <stdio.h>
#include <iostream.h>
#include <time.h>

#include "usbio.h"											// USBIO Dev Kit
#include "usbiopipe.h"										// ditto


// === Global Defines ===

// The version of this program
#define VERSION_MAJOR  1
#ifdef VERSION_MINOR
# undef VERSION_MINOR						  // VERSION_MINOR sometimes yields funny compiler warning (c4005)
#endif
#define VERSION_MINOR  6
#ifdef VERSION_MICRO
# undef VERSION_MICRO						  // ditto
#endif
#define VERSION_MICRO  0

// The version of the USBIO driver this program is compiled against
#define USBIO_VERSION_MAJOR  2
#define USBIO_VERSION_MINOR  41

#define MAX_DESCRIPTOR_BUFFER_SIZE 2047

// === Global Vars ===

// Our own private GUID (also used in the .inf file as 'DriverUserInterfaceGuid')
// {55606403-E62D-4707-9F56-40D48C6736D0}
static const GUID g_UsbioID =
	{0x55606403, 0xe62d, 0x4707, {0x9f, 0x56, 0x40, 0xd4, 0x8c, 0x67, 0x36, 0xd0}};

// Handle for USB device list
static HDEVINFO g_DevList = NULL;

// USBIO supported device
static CUsbIo g_UsbDev;
static CUsbIoPipe g_BulkOutPipe;
static CUsbIoPipe g_BulkInPipe;

// 2 Bulk endpoints
static UCHAR g_ucBulkOutEndpoint = 0;
static UCHAR g_ucBulkInEndpoint = 0;

// Read/write buffer
static const DWORD KBufferSize = 1024 * 1024;
static const DWORD KPreambleLength = 8;
static BYTE Data[KBufferSize];								// the read/write buffer
static DWORD Length;										// Length of a transfer
static time_t T_0;											// the starting time

static CUsbIoBuf g_Buf((VOID*) Data, KBufferSize);		// the data buffer
static CUsbIoBuf g_ZlpBuf (NULL, 0);					// the data buffer

static DWORD dwRC = USBIO_ERR_SUCCESS;						// global error indicator

enum
	{
	ELoop,
	ELoopDebug,
	EReceiveOnly,
	ETransmitOnly
	};

static int TransferMode = ELoop;
static bool VerboseMode = false;
static bool ZlpMode = false;

static unsigned int maxOutPacketSize = 0;

// The version of T_USB we require (at least)
static const DWORD KTusbVersion = 20070524;

// After how many iterations to update the CRT:
static const int KLoopModeDisplayUpdate = 1024;
// After how many iterations (= MBytes) to update the CRT:
static const int KUniModeDisplayUpdate = 10;

// Helper #defines

#define PRINT_IF_VERBOSE(string) \
		do { \
		if (VerboseMode) \
			{ \
			printf(string); \
			} \
		} while (0)

#define PRINT_IF_VERBOSE1(string, a) \
		do { \
		if (VerboseMode) \
			{ \
			printf(string, a); \
			} \
		} while (0)


// === Functions ===

//
// Process the command line arguments, printing a helpful message
// if none are supplied.
//
static int ProcessCmdLine(int argc, char* argv[])
	{
	char help_text[] =
		"* Syntax: usbrflct [options]\n"
		"* Options:\n"
		"* /[r|t]    receive|transmit only; default is to loop\n"
		"* /z        zero length packet at end of a transmission\n"
		"*	    (only if last packet is full length)\n"
		"* /l        loop mode with stats printed for every iteration\n"
		"* /v        verbose driver & program output\n"
		"* /[h|?]    displays this help text\n"
		"\n";

	for (int i = 1; i < argc; i++)
		{
		strupr(argv[i]);
		if ((argv[i][0] == '-') || (argv[i][0] == '/'))
			{
			switch (argv[i][1])
				{
			case 'R':
			case 'r':
				TransferMode = EReceiveOnly;
				break;
			case 'T':
			case 't':
				TransferMode = ETransmitOnly;
				break;
			case 'L':
			case 'l':
				TransferMode = ELoopDebug;
				break;
			case 'V':
			case 'v':
				VerboseMode = true;
				break;
			case 'Z':
			case 'z':
				ZlpMode = true;
				break;
			case '?':
			case 'H':
			case 'h':
				cout << help_text;
				return -1;
			default:
				cout << "* Invalid argument: " << argv[i] << "\n";
				cout << help_text;
				return -1;
				}
			}
		}
	return 0;
	}


static void OpenUsbDevice()
	{
	CUsbIo::DestroyDeviceList(g_DevList);

	// Enumerate attached USB devices supported by USBIO
	g_DevList = CUsbIo::CreateDeviceList(&g_UsbioID);

	// Open first device in list
	dwRC = g_UsbDev.Open(0, g_DevList, &g_UsbioID);

	PRINT_IF_VERBOSE1("\nCUsbIo::Open returned <0x%X>\n", dwRC);

	if (dwRC == USBIO_ERR_VERSION_MISMATCH)
		{
		printf("\n* Error: \"The API version reported by the USBRFLCT driver\n" \
			   "*         does not match the expected version.\"\n");
		printf("* The driver will need to be updated as follows:\n");
		printf("* 1. Connect the device to the PC & start T_USB,\n" \
			   "* then find the USB device in the Windows Device Manager\n" \
			   "* ('Control Panel'->'System'->'Hardware'->'Device Manager').\n" \
			   "* Right click on the device name and choose 'Uninstall...'.\n");
		printf("* 2. In c:\\winnt\\inf\\, find (by searching for \"Symbian\") and\n" \
			   "* delete the *.INF file that was used to install the existing\n" \
			   "* version of USBRFLCT.SYS. Make sure to also delete the\n" \
			   "* precompiled version of that file (<samename>.PNF).\n");
		printf("* 3. In c:\\winnt\\system32\\drivers\\, delete the file USBRFLCT.SYS.\n");
		printf("* Then unplug & reconnect the USB device and, when prompted, install\n" \
			   "* the new USBRFLCT.SYS driver using the .INF file from this distribution.\n" \
			   "* (All files can be found under e32test\\win32\\usbrflct_distribution\\.)\n");
		}
	}


static void CloseUsbDevice()
	{
	// Close the device
	g_UsbDev.Close();
	PRINT_IF_VERBOSE("CUsbIo::Close called\n");
	}


static void GetDeviceDescriptor()
	{
	USB_DEVICE_DESCRIPTOR DeviceDescriptor;

	memset(&DeviceDescriptor, 0, sizeof(USB_DEVICE_DESCRIPTOR));

	// Get device descriptor
	dwRC = g_UsbDev.GetDeviceDescriptor(&DeviceDescriptor);
	PRINT_IF_VERBOSE1("CUsbIo::GetDeviceDescriptor returned <0x%X>\n", dwRC);

	if (VerboseMode && (dwRC == USBIO_ERR_SUCCESS))
		{
		printf("\nDEVICE DESCRIPTOR:\n"
			   "bLength = <%u>\n"
			   "bDescriptorType = <%u>\n"
			   "bcdUSB = <%u>\n"
			   "bDeviceClass = <%u>\n"
			   "bDeviceSubClass = <%u>\n"
			   "bDeviceProtocol = <%u>\n"
			   "bMaxPacketSize0 = <%u>\n"
			   "idVendor = <%u>\n"
			   "idProduct = <%u>\n"
			   "bcdDevice = <%u>\n"
			   "iManufacturer = <%u>\n"
			   "iProduct = <%u>\n"
			   "iSerialNumber = <%u>\n"
			   "bNumConfigurations = <%u>\n\n",
			   DeviceDescriptor.bLength,
			   DeviceDescriptor.bDescriptorType,
			   DeviceDescriptor.bcdUSB,
			   DeviceDescriptor.bDeviceClass,
			   DeviceDescriptor.bDeviceSubClass,
			   DeviceDescriptor.bDeviceProtocol,
			   DeviceDescriptor.bMaxPacketSize0,
			   DeviceDescriptor.idVendor,
			   DeviceDescriptor.idProduct,
			   DeviceDescriptor.bcdDevice,
			   DeviceDescriptor.iManufacturer,
			   DeviceDescriptor.iProduct,
			   DeviceDescriptor.iSerialNumber,
			   DeviceDescriptor.bNumConfigurations);
		}
	}


static void GetConfigurationDescriptor()
	{
	CHAR szBuffer[MAX_DESCRIPTOR_BUFFER_SIZE] = "";
	USB_CONFIGURATION_DESCRIPTOR* pConfigDescriptor = NULL;
	USB_INTERFACE_DESCRIPTOR* pInterfaceDescriptor = NULL;
	USB_ENDPOINT_DESCRIPTOR* pEndpointDescriptor = NULL;

	DWORD dwByteCount = MAX_DESCRIPTOR_BUFFER_SIZE;

	memset(szBuffer, 0, sizeof(szBuffer));

	// Get first configuration descriptor
	dwRC =
		g_UsbDev.GetConfigurationDescriptor((USB_CONFIGURATION_DESCRIPTOR*) szBuffer,
											dwByteCount, 0);
	PRINT_IF_VERBOSE1("CUsbIo::GetConfigurationDescriptor returned <0x%X>\n", dwRC);

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		USB_COMMON_DESCRIPTOR* Desc;
		ULONG rl = dwByteCount;
		ULONG ulDescLength = 0;
		CHAR* data = szBuffer;

		while (rl > 0)
			{
			Desc = (USB_COMMON_DESCRIPTOR*) data;
			ulDescLength = Desc->bLength;
			if ((ulDescLength > rl) || (ulDescLength == 0))
				{
				printf("Length remaining too short!\n");
				rl = 0;
				}
			else
				{
				switch (Desc->bDescriptorType)
					{
				case USB_CONFIGURATION_DESCRIPTOR_TYPE:
					pConfigDescriptor =
						(USB_CONFIGURATION_DESCRIPTOR*) data;
					if (VerboseMode)
						{
						printf("\nCONFIGURATION DESCRIPTOR:\n"
							   "bLength = <%u>\n"
							   "bDescriptorType = <%u>\n"
							   "wTotalLength = <%u>\n"
							   "bNumInterfaces = <%u>\n"
							   "bConfigurationValue = <%u>\n"
							   "iConfiguration = <%u>\n"
							   "bmAttributes = <%u>\n"
							   "MaxPower = <%u>\n",
							   pConfigDescriptor->bLength,
							   pConfigDescriptor->bDescriptorType,
							   pConfigDescriptor->wTotalLength,
							   pConfigDescriptor->bNumInterfaces,
							   pConfigDescriptor->bConfigurationValue,
							   pConfigDescriptor->iConfiguration,
							   pConfigDescriptor->bmAttributes,
							   pConfigDescriptor->MaxPower);
						}
					break;
				case USB_INTERFACE_DESCRIPTOR_TYPE:
					pInterfaceDescriptor =
						(USB_INTERFACE_DESCRIPTOR*) data;
					if (VerboseMode)
						{
						printf("\nINTERFACE DESCRIPTOR: \n"
							   "bLength = <%u>\n"
							   "bDescriptorType = <%u>\n"
							   "bInterfaceNumber = <%u>\n"
							   "bAlternateSetting = <%u>\n"
							   "bNumEndpoints = <%u>\n"
							   "bInterfaceClass = <%u>\n"
							   "bInterfaceSubClass = <%u>\n"
							   "bInterfaceProtocol = <%u>\n"
							   "iInterface = <%u>\n",
							   pInterfaceDescriptor->bLength,
							   pInterfaceDescriptor->bDescriptorType,
							   pInterfaceDescriptor->bInterfaceNumber,
							   pInterfaceDescriptor->bAlternateSetting,
							   pInterfaceDescriptor->bNumEndpoints,
							   pInterfaceDescriptor->bInterfaceClass,
							   pInterfaceDescriptor->bInterfaceSubClass,
							   pInterfaceDescriptor->bInterfaceProtocol,
							   pInterfaceDescriptor->iInterface);
						}
					break;
				case USB_ENDPOINT_DESCRIPTOR_TYPE:
					pEndpointDescriptor =
						(USB_ENDPOINT_DESCRIPTOR*) data;
					if (VerboseMode)
						{
						printf("\nENDPOINT DESCRIPTOR: \n"
							   "bLength = <%u>\n"
							   "bDescriptorType = <%u>\n"
							   "bEndpointAddress = <%u>\n"
							   "bmAttributes = <%u>\n"
							   "wMaxPacketSize = <%u>\n"
							   "bInterval = <%u>\n",
							   pEndpointDescriptor->bLength,
							   pEndpointDescriptor->bDescriptorType,
							   pEndpointDescriptor->bEndpointAddress,
							   pEndpointDescriptor->bmAttributes,
							   pEndpointDescriptor->wMaxPacketSize,
							   pEndpointDescriptor->bInterval);
						}
					break;
				default:
					break;
					}
				}
			data += ulDescLength;
			rl -= ulDescLength;
			}
		}
	PRINT_IF_VERBOSE("\n");
	}


static void GetStringDescriptor()
	{
	CHAR szBuffer[MAX_DESCRIPTOR_BUFFER_SIZE] = "";
	USB_STRING_DESCRIPTOR* pStringDescriptor = NULL;
	DWORD dwByteCount = MAX_DESCRIPTOR_BUFFER_SIZE;

	memset(szBuffer, 0, sizeof(szBuffer));

	// Get string descriptor
	dwRC = g_UsbDev.GetStringDescriptor((USB_STRING_DESCRIPTOR*) szBuffer,
										dwByteCount, 1, 0);
	PRINT_IF_VERBOSE1("CUsbIo::GetStringDescriptor returned <0x%X>\n", dwRC);

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		pStringDescriptor = (USB_STRING_DESCRIPTOR*) szBuffer;
		if (VerboseMode)
			{
			printf("\nSTRING DESCRIPTOR:\n"
				   "bLength = <%u>\n"
				   "bDescriptorType = <%u>\n"
				   "bString = <",							// output continues below!
				   pStringDescriptor->bLength,
				   pStringDescriptor->bDescriptorType);
			}
		INT i = 0;
		CHAR* Ptr = szBuffer;
		for (i = 2, Ptr += 2;
			 i < pStringDescriptor->bLength;
			 i += 2, Ptr += 2)
			{
			PRINT_IF_VERBOSE1("%c", *Ptr);
			}
		PRINT_IF_VERBOSE(">\n\n");
		}
	}


static void SetConfiguration()
	{
	USBIO_SET_CONFIGURATION SetConfig;

	memset(&SetConfig, 0, sizeof(USBIO_SET_CONFIGURATION));

	// Set the first configuration as active
	SetConfig.ConfigurationIndex = 0;
	SetConfig.NbOfInterfaces = 1;
	SetConfig.InterfaceList[0].InterfaceIndex = 0;
	SetConfig.InterfaceList[0].AlternateSettingIndex = 0;
	SetConfig.InterfaceList[0].MaximumTransferSize = KBufferSize;
	dwRC = g_UsbDev.SetConfiguration(&SetConfig);
	PRINT_IF_VERBOSE1("CUsbIo::SetConfiguration returned <0x%X>\n", dwRC);
	}


static void GetConfigurationInfo()
	{
	USBIO_CONFIGURATION_INFO ConfigInfo;
	USHORT i = 0;

	memset(&ConfigInfo, 0, sizeof(USBIO_CONFIGURATION_INFO));

	dwRC = g_UsbDev.GetConfigurationInfo(&ConfigInfo);
	PRINT_IF_VERBOSE1("CUsbIo::GetConfigurationInfo returned <0x%X>\n", dwRC);

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		if (VerboseMode)
			{
			printf("\nCONFIGURATION INFO:\n"
				   "NbOfInterfaces = <%lu>\n"
				   "NbOfPipes = <%lu>\n",
				   ConfigInfo.NbOfInterfaces,
				   ConfigInfo.NbOfPipes);
			}
		for (i = 0; i < ConfigInfo.NbOfInterfaces; i++)
			{
			if (VerboseMode)
				{
				printf("\nINTERFACE <%u>:\n", i + 1);
				printf("InterfaceNumber = <%u>\n"
					   "AlternateSetting = <%u>\n"
					   "Class = <%u>\n"
					   "SubClass = <%u>\n"
					   "Protocol = <%u>\n"
					   "NumberOfPipes = <%u>\n"
					   "reserved1 = <%u>\n"
					   "reserved2 = <%u>\n",
					   ConfigInfo.InterfaceInfo[i].InterfaceNumber,
					   ConfigInfo.InterfaceInfo[i].AlternateSetting,
					   ConfigInfo.InterfaceInfo[i].Class,
					   ConfigInfo.InterfaceInfo[i].SubClass,
					   ConfigInfo.InterfaceInfo[i].Protocol,
					   ConfigInfo.InterfaceInfo[i].NumberOfPipes,
					   ConfigInfo.InterfaceInfo[i].reserved1,
					   ConfigInfo.InterfaceInfo[i].reserved2);
				}
			}
		for (i = 0; i < ConfigInfo.NbOfPipes; i++)
			{
			PRINT_IF_VERBOSE("\n");
			if ((ConfigInfo.PipeInfo[i].PipeType == PipeTypeBulk) &&
				!(ConfigInfo.PipeInfo[i].EndpointAddress & 0x80))
				{
				PRINT_IF_VERBOSE("Bulk OUT pipe found:\n");
				g_ucBulkOutEndpoint = ConfigInfo.PipeInfo[i].EndpointAddress;
				maxOutPacketSize = ConfigInfo.PipeInfo[i].MaximumPacketSize;
				}
			else if ((ConfigInfo.PipeInfo[i].PipeType == PipeTypeBulk) &&
					 (ConfigInfo.PipeInfo[i].EndpointAddress & 0x80))
				{
				PRINT_IF_VERBOSE("Bulk IN pipe found:\n");
				g_ucBulkInEndpoint = ConfigInfo.PipeInfo[i].EndpointAddress;
				}
			if (VerboseMode)
				{
				printf("PIPE <%u>\n", i + 1);
				printf("PipeType = <%d>\n"
					   "MaximumTransferSize = <%lu>\n"
					   "MaximumPacketSize = <%u>\n"
					   "EndpointAddress = <%u>\n"
					   "Interval = <%u>\n"
					   "InterfaceNumber = <%u>\n"
					   "reserved1 = <%u>\n"
					   "reserved2 = <%u>\n"
					   "reserved3 = <%u>\n",
					   ConfigInfo.PipeInfo[i].PipeType,
					   ConfigInfo.PipeInfo[i].MaximumTransferSize,
					   ConfigInfo.PipeInfo[i].MaximumPacketSize,
					   ConfigInfo.PipeInfo[i].EndpointAddress,
					   ConfigInfo.PipeInfo[i].Interval,
					   ConfigInfo.PipeInfo[i].InterfaceNumber,
					   ConfigInfo.PipeInfo[i].reserved1,
					   ConfigInfo.PipeInfo[i].reserved2,
					   ConfigInfo.PipeInfo[i].reserved3);
				}
			}
		}
	PRINT_IF_VERBOSE("\n");
	}


static void OpenPipes()
	{
	CUsbIo::DestroyDeviceList(g_DevList);

	// Enumerate attached USB devices supported by USBIO
	g_DevList = CUsbIo::CreateDeviceList(&g_UsbioID);

	// Create the bulk OUT pipe
	dwRC = g_BulkOutPipe.Bind(0, g_ucBulkOutEndpoint, g_DevList, &g_UsbioID);
	PRINT_IF_VERBOSE1("CUsbIoPipe::Bind (Bulk OUT) returned <0x%X>\n", dwRC);

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		// Create the bulk IN pipe
		dwRC = g_BulkInPipe.Bind(0, g_ucBulkInEndpoint, g_DevList, &g_UsbioID);
		PRINT_IF_VERBOSE1("CUsbIoPipe::Bind (Bulk IN) returned <0x%X>\n", dwRC);
		}
	PRINT_IF_VERBOSE("\n");
	}


static void ClosePipes()
	{
	// Close down the bulk OUT pipe
	dwRC = g_BulkOutPipe.Unbind();
	PRINT_IF_VERBOSE1("CUsbIoPipe::Unbind (Bulk OUT) returned <0x%X>\n", dwRC);

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		// Close down the bulk IN pipe
		dwRC = g_BulkInPipe.Unbind();
		PRINT_IF_VERBOSE1("CUsbIoPipe::Unbind (Bulk IN) returned <0x%X>\n", dwRC);
		}
	}


static void ReceiveVersion()
	{
	// Here we (hope to) read an 8 byte packet containing the T_USB version.
	printf("* Waiting for T_USB version packet to arrive...");

	// The first 4 bytes are interpreted as an int32 value.
	DWORD bytes_read = 0;
	g_Buf.NumberOfBytesToTransfer = KPreambleLength;
	g_BulkInPipe.Read(&g_Buf);
	dwRC = g_BulkInPipe.WaitForCompletion(&g_Buf, INFINITE);
	printf(" done.\n");
	bytes_read = g_Buf.BytesTransferred;
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		if (bytes_read < KPreambleLength)
			{
			printf("* Read less bytes (%d) than expected (%d).\n",
				   bytes_read, KPreambleLength);
			dwRC = USBIO_ERR_FAILED;
			return;
			}
		}
	else
		{
		printf("\n* Error: CUsbIoPipe::Read (version) returned <0x%X>\n", dwRC);
		return;
		}
	// First make sure it's actually the version packet, and not
	// a data preamble packet of an old T_USB.
	if (!(Data[4] == 'V' &&
		  Data[5] == 'e' &&
		  Data[6] == 'r' &&
		  Data[7] == 's'))
		{
		printf("* Inadequate version of T_USB: no version packet was sent (we need at least %d)\n",
			   KTusbVersion);
		dwRC = USBIO_ERR_FAILED;
		return;
		}
	DWORD tusb_version = *((ULONG*) Data);						// first 4 bytes
	if (tusb_version < KTusbVersion)
		{
		printf("* Inadequate version of T_USB: %d (we need at least %d)\n",
			   tusb_version, KTusbVersion);
		dwRC = USBIO_ERR_FAILED;
		return;
		}
	printf("* Suitable version of T_USB found: %d.\n", tusb_version);
	}


static void SendVersion()
	{
	// Here we send an 8 byte packet containing USBRFLCT's + USBIO's versions.
	printf("* Sending our version packet to T_USB...");

	DWORD bytes_written = 0;
	g_Buf.NumberOfBytesToTransfer = KPreambleLength;
	Data[0] = VERSION_MAJOR;
	Data[1] = VERSION_MINOR;
	Data[2] = VERSION_MICRO;
	Data[3] = USBIO_VERSION_MAJOR;
	Data[4] = USBIO_VERSION_MINOR;
	g_BulkOutPipe.Write(&g_Buf);
	dwRC = g_BulkOutPipe.WaitForCompletion(&g_Buf, INFINITE);
	printf(" done.\n");
	bytes_written = g_Buf.BytesTransferred;
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		if (bytes_written < KPreambleLength)
			{
			printf("* Wrote less bytes (%d) than requested (%d).\n",
				   bytes_written, KPreambleLength);
			dwRC = USBIO_ERR_FAILED;
			}
		}
	else
		{
		printf("\n* Error: CUsbIoPipe::Write (version) returned <0x%X>\n", dwRC);
		}
	}


static void ExchangeVersions()
	{
	SendVersion();
	if (dwRC != USBIO_ERR_SUCCESS)
		return;
	ReceiveVersion();
	}


static void GetLength()
	{
	// The first two bytes are interpreted as a length value.
	DWORD bytes_read = KPreambleLength;
	g_Buf.NumberOfBytesToTransfer = KPreambleLength;

	g_BulkInPipe.Read(&g_Buf);
	dwRC = g_BulkInPipe.WaitForCompletion(&g_Buf, INFINITE);
	bytes_read = g_Buf.BytesTransferred;

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		if (bytes_read < KPreambleLength)
			{
			printf("* Read less bytes (%d) than expected (%d).\n",
				   bytes_read, KPreambleLength);
			dwRC = USBIO_ERR_FAILED;
			return;
			}
		}
	else
		{
		printf("\n* Error: CUsbIoPipe::Read (length) returned <0x%X>\n", dwRC);
		return;
		}
	Length = *((ULONG*) Data);								// first 4 bytes
	if (Length > KBufferSize)
		{
		printf("* This is too much: %d (our buffer is too small: %d)\n",
			   Length, KBufferSize);
		dwRC = USBIO_ERR_FAILED;
		}
	if (VerboseMode)
		{
		printf("* Just read %d bytes, now assuming transfer length is %d bytes.\n",
			   bytes_read, Length);
		}
	else if (TransferMode == EReceiveOnly || TransferMode == ETransmitOnly)
		{
		printf("* Single transfer size: %d bytes.\n", Length);
		}
	}


static void ReadData()
	{
	// We have to setup a read for at least one byte in order to get
	// the host to issue IN tokens for our zero-byte read:
	if (Length == 0)
		g_Buf.NumberOfBytesToTransfer = 1;
	else
		g_Buf.NumberOfBytesToTransfer = Length;

	g_BulkInPipe.Read(&g_Buf);
	dwRC = g_BulkInPipe.WaitForCompletion(&g_Buf, INFINITE);
	DWORD bytes_read = g_Buf.BytesTransferred;

	if (dwRC != USBIO_ERR_SUCCESS)
		{
		printf("\n* Error: CUsbIoPipe::Read (data) returned <0x%X>\n", dwRC);
		}
	else
		{
		if (bytes_read != Length)
			{
			printf("* Read more/less bytes (%d) than expected (%d).\n",
				   bytes_read, Length);
			dwRC = USBIO_ERR_FAILED;
			}
		else
			{
			PRINT_IF_VERBOSE1("* Read %d bytes.\n", Length);
			}
		}
	}


static void WriteData()
	{
	DWORD bytes_written = Length;

	g_Buf.NumberOfBytesToTransfer = bytes_written;

	g_BulkOutPipe.Write(&g_Buf);
	dwRC = g_BulkOutPipe.WaitForCompletion(&g_Buf, INFINITE);
	if (ZlpMode && (Length >= maxOutPacketSize) && ((Length % maxOutPacketSize) == 0))
	{
		// writes a zero length packet
		g_BulkOutPipe.Write(&g_ZlpBuf);
		dwRC = g_BulkOutPipe.WaitForCompletion(&g_ZlpBuf, INFINITE);
	}


	bytes_written = g_Buf.BytesTransferred;

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		PRINT_IF_VERBOSE1("* Wrote %d bytes.\n", bytes_written);
		}
	else
		{
		printf("\n* Error: CUsbIoPipe::Write returned <0x%X>\n", dwRC);
		}
	}


void PrintStats()
	{
	static DWORD loop = 0;									// the loop counter
	static double xfer_size = 0;							// the total transfer amount so far
	time_t t_1 = time(NULL);								// current time
	double t_diff = difftime(t_1, T_0);						// this yields current seconds since start
	xfer_size += (KPreambleLength + (2 * Length)) * KLoopModeDisplayUpdate;	//
	double xfer_rate = xfer_size / t_diff;					// mean transfer rate since start
	loop += KLoopModeDisplayUpdate;
	printf("* Iter: %d (%d bytes) Total: %.0f bytes Rate: %.0f bytes/s  \r",
		   loop, Length, xfer_size, xfer_rate);
	}


void PrintStatsEveryLoop()
	{
	static DWORD loop = 0;									// the loop counter
	static double xfer_size = 0;							// the total transfer amount so far
	time_t t_1 = time(NULL);								// current time
	double t_diff = difftime(t_1, T_0);						// this yields current seconds since start
	xfer_size += (KPreambleLength + (2 * Length));			//
	double xfer_rate = xfer_size / t_diff;					// mean transfer rate since start
	printf("* Iter: %d (%d bytes) Total: %.0f bytes Rate: %.0f bytes/s  \r",
		   ++loop, Length, xfer_size, xfer_rate);
	}


void PrintUnidirStats()
	{
	static DWORD loop = 0;									// the loop counter
	static double xfer_size = 0;							// the total transfer amount so far
	time_t t_1 = time(NULL);								// current time
	double t_diff = difftime(t_1, T_0);						// this yields current seconds since start
	xfer_size += Length * KUniModeDisplayUpdate;
	double xfer_rate = xfer_size / t_diff;					// mean transfer rate since start
	loop += KUniModeDisplayUpdate;
	printf("* Iter: %d (%d bytes) Total: %.0f bytes Rate: %.0f bytes/s  \r",
		   loop, Length, xfer_size, xfer_rate);
	}


static void LoopTransfer()
	{
	printf("* Loop Transfers -- reading & writing alternately.\n");

	T_0 = time(NULL);										// starting time

	while (dwRC == USBIO_ERR_SUCCESS)
		{
		static DWORD n = 0;

		// First we get the length (+ the packet size)
		GetLength();

		if (dwRC == USBIO_ERR_SUCCESS)
			{
			// Then we read 'Length' bytes
			ReadData();
			}

		if (dwRC == USBIO_ERR_SUCCESS)
			{
			// Now we send the received data back to the client.
			WriteData();
			}

		if (dwRC == USBIO_ERR_SUCCESS)
			{
			// Finally, sometimes we print some statistics
			if (TransferMode == ELoopDebug)
				PrintStatsEveryLoop();
			else if ((++n % KLoopModeDisplayUpdate) == 0)
				PrintStats();
			}
		}
	}


static void ReceiveOnlyTransfer()
	{
	printf("* Receive-only transfers (IN).\n");

	// First (and only once) we get the transfer length (+ the packet size)
	GetLength();

	T_0 = time(NULL);										// starting time

	while (dwRC == USBIO_ERR_SUCCESS)
		{
		static DWORD n = -1;
		static DWORD pktNum;

		// Then we read 'Length' bytes
		ReadData();
		pktNum = *(DWORD *)&Data;
		if (pktNum != ++n)
			{
			printf ("\n* Error: rcv'd wrong pkt number: 0x%x (expected: 0x%x)\n", pktNum, n);
			// reset from the received packet number, so that ...
			// if a packet is lost or duplicated a single error is reported
			n = pktNum;
			}
		// Finally, sometimes we print some statistics
		if ((n % KUniModeDisplayUpdate) == 0)
			PrintUnidirStats();
		}
	}


static void TransmitOnlyTransfer()
	{
	printf("* Transmit-only transfers (OUT).\n");

	// First (and only once) we get the transfer length (+ the packet size)
	GetLength();

	T_0 = time(NULL);										// starting time

	while (dwRC == USBIO_ERR_SUCCESS)
		{
		static DWORD n = 0;
		// First the packet number is put into the first four bytes
		*(DWORD *)&Data = n++;
		// Then we write 'Length' bytes
		WriteData();

		// Finally, sometimes we print some statistics
		if ((n % KUniModeDisplayUpdate) == 0)
			PrintUnidirStats();
		}
	}


static void DoTransfers()
	{
	switch (TransferMode)
		{
	case ELoop:
	case ELoopDebug:
		LoopTransfer();
		break;
	case EReceiveOnly:
		ReceiveOnlyTransfer();
		break;
	case ETransmitOnly:
		TransmitOnlyTransfer();
		break;
	default:
		dwRC = -1;
		break;
		}
	}


static void Delay(int milliseconds)
	{
	printf("* Short wait... ");
	Sleep(milliseconds);
	printf("done.\n");
	}


static void PrintHello()
	{
	printf("*--------------------------------------------------\n");
	printf("* USBRFLCT v%d.%d.%d (for use with USBRFLCT.SYS v%d.%d)\n",
		   VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO,
		   USBIO_VERSION_MAJOR, USBIO_VERSION_MINOR);
	printf("* USB Reflector Test Program / T_USB Host-side Part\n");
	printf("*   Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).\n");
	printf("*--------------------------------------------------\n");
	}


int main(int argc, char* argv[])
	{
	PrintHello();

	if (ProcessCmdLine(argc, argv) != 0)
		return -1;

	OpenUsbDevice();

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		GetDeviceDescriptor();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		GetConfigurationDescriptor();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		GetStringDescriptor();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		SetConfiguration();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		// In order to give the USB device-side program (t_usb)
		// enough time after getting configured to carry out
		// some device tests, we wait here for a short while
		// before proceeding:
		Delay(2000);
		GetConfigurationInfo();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		OpenPipes();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		ExchangeVersions();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		DoTransfers();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		ClosePipes();
		}

	CloseUsbDevice();

	return 0;
	}


// --eof
