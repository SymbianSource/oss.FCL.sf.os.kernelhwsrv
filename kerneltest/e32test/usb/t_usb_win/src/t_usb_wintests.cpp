// Copyright (c) 2001-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// e32test\usb\t_usb_win\src\t_usb_wintests.cpp
// Win32 USB test program performs I/O with a device running the
// Symbian OS test program T_USB_DEVICE, using the generic USB device driver USBIO.
// === Includes ===
// 
//

#include "stdafx.h"
#include <process.h>    /* _beginthread, _endthread */

#include <sys/timeb.h>

#include "usbio.h"											// USBIO Dev Kit
#include "usbiopipe.h"										// ditto

#include "t_usb_win.h"
#include "t_usb_winDlg.h"

#include "global.h"

#include "UsbcvControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern BOOL EjectVolume(char driveLetter);
extern BOOL WaitDriveReady (char driveLetter);
extern DWORD PerlScript(char * scriptName);

// === Global Vars ===

extern char gScriptFileName[];
extern char gManufacturerName[];
extern char gProductName[];
extern char gSerialNumber[];
extern char gLogfileName[];
extern BOOL gVerboseMode;
extern BOOL gKeepOpen;
extern BOOL gAbort;
extern int gTransferMode;
extern CT_usb_winDlg* gMainWindow;
extern FILE *logStream;
extern int gReadWriteTimeOut;

// Our own private GUID (also used in the .inf file as 'DriverUserInterfaceGuid')
// {55606403-E62D-4707-9F56-40D48C6736D0}
GUID gUsbioID =
	{0x55606403, 0xe62d, 0x4707, {0x9f, 0x56, 0x40, 0xd4, 0x8c, 0x67, 0x36, 0xd0}};

// Another provate GUID used in the AtenSwitch.inf file  
// {A6942415-49EE-4ef2-9CE8-D33E888EF133}
const GUID gSwitchID =
	{0xA6942415, 0x49EE, 0x4ef2, {0x9C, 0xE8, 0xD3, 0x3E, 0x88, 0x8E, 0xF1, 0x33}};

// Handle for USB device list
static HDEVINFO gDevList = NULL;

// USBIO supported device
CUsbIo gUsbDev;

static int gDevNumber = -1;
static UCHAR gEndpointMap [MAX_INTERFACES] [MAX_SETTINGS] [MAX_ENDPOINTS];
static WORD gPacketSizeMap [MAX_INTERFACES] [MAX_SETTINGS] [MAX_ENDPOINTS];
static UCHAR gAlternateSettings [MAX_INTERFACES];
static bool gIsoEndpoint [MAX_INTERFACES] [MAX_SETTINGS] [MAX_ENDPOINTS];

static USHORT vendorId = 0;
static USHORT productId = 0;
static USHORT productIdMS = 0;

// endpoints

// Read/write buffer
static const DWORD KMaxTransferSize = 1024*1024;
static const DWORD KPreambleLength = 8;

static bool gFinishAllTests = false;

static HANDLE gThreadStartedEvent; 
static HANDLE gThreadEndedEvents[MAX_THREADS]; 
static HANDLE gThreadWaitEvents[MAX_THREADS]; 

HANDLE gDeviceConnectEvent; 
HANDLE gDeviceDisconnectEvent;

static __int64 gDeviceIdleCount = -1;
static struct _timeb gHostTime;

enum
	{
	EXml,
	ELoop,
	ECompare,
	EReceiveOnly,
	ETransmitOnly,
	EFileReceive,
	EFileTransmit,
	EComment,
	EConnect,
	EDisconnect,
	EWait,
	EFail,
	ESwitch,
	EWaitConnect,
	EWaitDisconnect,
	EDo,
	ERepeat,
	EUsbcv,
	EMassStorage,
	EEject,
	EPerl,
	EWaitDrive,
	};

enum Ep0Requests
	{
	EStop = 0x1,
	EVersion = 0x10,
	ETestParam = 0x20,
	ETestResult = 0x30,
	ETestFail = 0x40,
	ETestConnect = 0x50,
	ETestDisconnect = 0x60,
	ETestMassStorage = 0x70,
	ETestIdleCounter = 0x80,
	};

struct TestParamHost
	{
	WORD displayRate;
	DWORD outRate;
	DWORD inRate;
	BYTE index;
	bool signalNextSettingThread;
	};
typedef struct TestParamHost TestParamHostType;

struct TestParam
	{
	DWORD minSize;
	DWORD maxSize;
	DWORD packetNumber;
	BYTE interfaceNumber;
	BYTE alternateSetting;
	BYTE outPipe;
	BYTE inPipe;
	WORD repeat;
	WORD settingRepeat;
	BYTE beforeIndex;
	BYTE afterIndex;
	TestParamHostType host;
	TestParam ();
	};
typedef struct TestParam TestParamType;
typedef TestParamType* TestParamPtr;

// The version of T_USB we require (at least)
static const DWORD KTusbVersionMajor = 1;
static const DWORD KTusbVersionMinor = 0;
static const DWORD KTusbVersionMicro = 1;

static const DWORD KHostLogFileSize = 64*1024;

#define ERR_SCRIPT_NAME		0xF0000001L

// use a timeout for a WaitConnect to activate a scan for hardware changes
#define CONNECT_RESCAN_TIMEOUT	300000
#define RESCAN_APPNAME "C:\\Program Files\\devcon\\i386\\devcon.exe"
#define RESCAN_CMDLINE " rescan"

/////////////////////////////////////////////////////////////////////////////
// global functions

//
// print to output window
//
// Note:
// This is a global function that is called from any thread context.
// It is not possible to call MFC objects from a thread context that is
// created by using non-MFC means.
// So we use SendMessage to access the OutputWindow.
//
void PrintOut(BOOL screenFlag, BOOL logFlag, BOOL timeFlag, const char *format, ...)
{
	if (screenFlag)
	{
		const int buffersize = 512;
		va_list argptr;
  
		char buffer[buffersize];

		// print to buffer
		va_start(argptr,format);
		_vsnprintf(buffer,buffersize-1,format,argptr);
		buffer[buffersize-1] = 0;

		// with a time stamp and carriage return linefeed 
		if (timeFlag)
		{
			SYSTEMTIME localTime;

			GetLocalTime(&localTime);

			unsigned int len = strlen(buffer);

			GetDateFormat(LOCALE_USER_DEFAULT,0,&localTime,"dd MMM yyyy",&buffer[len],buffersize-len);

			len = strlen(buffer);

			GetTimeFormat(LOCALE_USER_DEFAULT,0,&localTime," H:mm:ss"NL,&buffer[len],buffersize-len);
		}

		// send buffer to output window (synchronous behavior for buffer protection)
		gMainWindow->SendMessage(WM_USER_PRINTOUT,0,(LPARAM)buffer); \

		if (logStream && logFlag)
			fputs(buffer,logStream);

		va_end(argptr);
	}
}

static void Delay(int milliseconds)
	{
	PRINT_IF_VERBOSE "* Short wait... ");
	Sleep(milliseconds);
	PRINT_IF_VERBOSE "done"NL);
	}

TestParam::TestParam()
	{
	minSize = 10;
	maxSize = 100;
	packetNumber = 0;
	interfaceNumber = 0;
	alternateSetting = 0;
	outPipe = 2;
	inPipe = 1;
	repeat = 0;
	settingRepeat = 0;
	beforeIndex = 0;
	host.displayRate = 100;
	host.outRate = 0;
	host.inRate = 0;
	host.index = 0;
	host.signalNextSettingThread = FALSE;
	}
//
// Process a line from the test script file
// returns the test type and sets up the test parameter structure
//
static int ParseTestLine(char* aLine,TestParamPtr aParam, USBIO_CLASS_OR_VENDOR_REQUEST* aRequest)
	{
	int testType = EComment;

	struct testKey
		{
		int testId;
		char testString[20];
		};
	typedef struct testKey testKeyType;
	size_t tokenLength = 0;

	testKeyType testKeyTable[] =
		{
		EXml,"Xml",
		ELoop,"Loop",
		ECompare,"Compare",
		EReceiveOnly,"Stream",
		EFileReceive,"File",
		EConnect,"Connect",
		EDisconnect,"Disconnect",
		EWait,"Wait",
		EFail,"Fail",
		ESwitch,"Switch",
		EWaitConnect,"WaitConnect",
		EWaitDisconnect, "WaitDisconnect",
		ERepeat, "Repeat",
		EDo, "Do",
		EUsbcv, "Usbcv",
		EMassStorage, "MStore",
		EEject, "Eject",
		EPerl, "Perl",
		EWaitDrive, "WaitDrive",
		};

	size_t  numKeys = sizeof( testKeyTable ) / sizeof( testKeyType );

	char delim[]   = " ,\t\n";
	char stringDelim[]   = "\"";
	char * token = strtok(aLine,delim);
	if (token == NULL)
		{
		return testType;
		}

	tokenLength = strlen(token);

	memset (aParam,0,sizeof(TestParamType));

	// default display rate 
	aParam->host.displayRate = 100;

	for (int i = 0; i < (int)numKeys; i++)
		{
		if (strnicmp (token,testKeyTable[i].testString,tokenLength) == 0)
			{
			testType = testKeyTable[i].testId;
			break;
			}
		}


	int paramCount = 0;
	// now set up test type in request setup packet
	switch (testType)
		{
	case EXml:
		aRequest->Value = 'X';
		paramCount = 2;
		break;

	case ELoop:
		aRequest->Value = 'L';
		break;

	case ECompare:
		aRequest->Value = 'C';
		break;

	case EReceiveOnly:
		aRequest->Value = 'S';
		paramCount = 1;
		break;

	case EFileReceive:
		aRequest->Value = 'F';
		paramCount = 1;
		break;

	case EComment:
		return testType;
		break;

	case EConnect:
		aRequest->Value = 'R';
		break;

	case EDisconnect:
		aRequest->Value = 'D';
		break;

	case EWait:
		aParam->minSize = INFINITE;
		break;

	case EWaitConnect:
		aParam->minSize = INFINITE;
		break;

	case EWaitDisconnect:
		aParam->minSize = INFINITE;
		break;

	case EFail:
	case ESwitch:
	case EDo:
	case ERepeat:
	case EUsbcv:
	case EMassStorage:
	case EEject:
	case EPerl:
	case EWaitDrive:
		break;

		}

	if (testType == EPerl)
		{
		// a space at the beginning of the line
		aLine[0] = ' ';
		strcpy(&aLine[1],&aLine[tokenLength+1]);
		// strip off carriage return or line feed
		if (iscntrl(aLine[strlen(aLine)-1]))
			{
			aLine[strlen(aLine)-1] = '\0';
			if (iscntrl(aLine[strlen(aLine)-1]))
				aLine[strlen(aLine)-1] = '\0';
			}
		return testType;
		}

	token = strtok(NULL,delim);
	if (testType == EEject || testType == EWaitDrive)
		{
		strcpy(aLine,token);

		return testType;
		}

	if (testType == EFileReceive)
		{
		// The first parameter is the drive letter on the device to use
		if (token != NULL)
			aParam->minSize = token[0];
		else
			aParam->minSize = 'D';

		token = strtok(NULL,delim);
		}

	while (token != NULL)
		{
		unsigned long ulParam = strtoul (token,NULL,0);
		switch (paramCount)
			{
		case 0 :
			aParam->minSize = (DWORD)ulParam;
			break;

		case 1 :
			aParam->maxSize = (DWORD)ulParam;
			break;

		case 2 :
			aParam->packetNumber = (DWORD)ulParam;
			break;
				
		case 3 :
			aParam->interfaceNumber = (BYTE)ulParam;
			break;
				
		case 4:
			aParam->alternateSetting = (BYTE)ulParam;
			// skip outpipe for Xml IN
			if (testType == EXml)
				{
				paramCount++;
				}
			break;

		case 5:
			aParam->outPipe = (BYTE)ulParam;
			break;

		case 6 :
			aParam->inPipe = (BYTE)ulParam;
			if (aParam->inPipe > 31)
				{
				if (testType == EReceiveOnly)
					testType = ETransmitOnly;
				if (testType == EFileReceive)
					testType = EFileTransmit;
				}
			break;

		case 7 :
			aParam->repeat = (WORD)ulParam;
			break;

		case 8 :
			aParam->host.displayRate = (WORD)ulParam;
			break;

		case 9 :
			aParam->settingRepeat = (WORD)ulParam;
			break;

		case 10 :
			aParam->beforeIndex = (BYTE)ulParam;
			break;

		case 11 :
			aParam->afterIndex = (BYTE)ulParam;
			break;

		case 12 :
			aParam->host.outRate = (DWORD)ulParam;
			break;

		case 13 :
			aParam->host.inRate = (DWORD)ulParam;
			break;
			}

		paramCount++;
		token = strtok(NULL,delim);
		}

	return testType;
	}

static void ShowErrorText (DWORD errCode)
	{
	char textBuf[80];
	
	CUsbIo::ErrorText(textBuf,80,errCode);

	PRINT_ALWAYS "%s"NL,textBuf); 
	
	gAbort = true;
	}

static DWORD OpenUsbDevice()
	{
	DWORD dwRC;
	USB_DEVICE_DESCRIPTOR DevDesc;
	// buffer used to store string descriptor
	unsigned char StrDescBuffer[sizeof(USB_STRING_DESCRIPTOR)+512];
	// pointer to USB string descriptor
	USB_STRING_DESCRIPTOR* StrDesc;
	DWORD StrDescSize;
	WCHAR * WideString;
	// helper variables  
	bool deviceFound = false;
	bool moreDevices = true;

	if (gDevList)
		{
		CUsbIo::DestroyDeviceList(gDevList);
		gDevList = NULL;
		gDevNumber = -1;
		gUsbDev.Close();	
		}


	// Enumerate attached USB devices supported by USBIO
	gDevList = CUsbIo::CreateDeviceList(&gUsbioID);


    if ( gDevList==NULL )
		{
		PRINT_ALWAYS "Unable to build a device list!"NL);
	    return USBIO_ERR_FAILED;
		}
	
    // open and query each device (max. 127)
    for ( int i=0; i < 127 && !deviceFound && moreDevices; i++ )
		{
		StrDesc = NULL;

		// open the device, using the global CUsbIo instance
		dwRC = gUsbDev.Open(i,gDevList,&gUsbioID);
		if ( dwRC != USBIO_ERR_SUCCESS )
			{
			// no more devices, or error leave loop
			moreDevices = false;
			break;
			}

		// we have found a device, query the device descriptor
		dwRC = gUsbDev.GetDeviceDescriptor(&DevDesc);
		if ( dwRC == USBIO_ERR_SUCCESS )
			{
			deviceFound = true;
			// Check manufacturer string
			if (strlen(gManufacturerName))
				{
			    // Does the device have a manufacturer?
				if ( DevDesc.iManufacturer != NULL )
					{
					WideString = new(WCHAR [strlen(gManufacturerName)+1]);
					for (unsigned int j = 0; j < strlen(gManufacturerName); j++)
						{
						WideString[j] = (WCHAR)gManufacturerName[j];
						}
					WideString[strlen(gManufacturerName)] = (WCHAR)0;
					// query the corresponding string descriptor
					StrDesc = (USB_STRING_DESCRIPTOR*)StrDescBuffer;
					StrDescSize = sizeof(StrDescBuffer);
					dwRC = gUsbDev.GetStringDescriptor(StrDesc,StrDescSize,DevDesc.iManufacturer);
					if ( dwRC == USBIO_ERR_SUCCESS )
						{
						size_t stringSize = wcslen (WideString);
						if (stringSize != (size_t)(StrDesc->bLength - 2)/2)
							deviceFound = false;
						else
							{
							if (wcsncmp (WideString,StrDesc->bString,stringSize) != 0)
								deviceFound = false;
							}
 						}
					else
						{
							deviceFound = false;
						}
					delete WideString;
					}
				else
					{
					deviceFound = false;
					}
				}
			
			// check product string
			if (strlen(gProductName))
				{
			    // Does the device have a manufacturer?
				if ( DevDesc.iProduct !=0 )
					{
					WideString = new(WCHAR [strlen(gProductName)+1]);
					for (unsigned int j = 0; j < strlen(gProductName); j++)
						{
						WideString[j] = (WCHAR)gProductName[j];
						}
					WideString[strlen(gProductName)] = (WCHAR)0;
					// query the corresponding string descriptor
					StrDesc = (USB_STRING_DESCRIPTOR*)StrDescBuffer;
					StrDescSize = sizeof(StrDescBuffer);
					dwRC = gUsbDev.GetStringDescriptor(StrDesc,StrDescSize,DevDesc.iProduct);
					if ( dwRC == USBIO_ERR_SUCCESS )
						{
						size_t stringSize = wcslen (WideString);
						if (stringSize != (size_t)(StrDesc->bLength - 2)/2)
							deviceFound = false;
						else
							{
							if (wcsncmp (WideString,StrDesc->bString,stringSize) != 0)
								deviceFound = false;
							}
						}
					else
						{
							deviceFound = false;
						}
					delete WideString;
					}
				else
					{
					deviceFound = false;
					}
				}

			// check serial number string
			if (strlen(gSerialNumber))
				{
			    // Does the device have a serial number?
				if ( DevDesc.iSerialNumber !=0 )
					{
					WideString = new(WCHAR [strlen(gSerialNumber)+1]);
					for (unsigned int j = 0; j < strlen(gSerialNumber); j++)
						{
						WideString[j] = (WCHAR)gSerialNumber[j];
						}
					WideString[strlen(gSerialNumber)] = (WCHAR)0;
					// query the corresponding string descriptor
					StrDesc = (USB_STRING_DESCRIPTOR*)StrDescBuffer;
					StrDescSize = sizeof(StrDescBuffer);
					dwRC = gUsbDev.GetStringDescriptor(StrDesc,StrDescSize,DevDesc.iSerialNumber);
					if ( dwRC == USBIO_ERR_SUCCESS )
						{
						size_t stringSize = wcslen (WideString);
						if (stringSize != (size_t)(StrDesc->bLength - 2)/2)
							deviceFound = false;
						else
							{
							if (wcsncmp (WideString,StrDesc->bString,stringSize) != 0)
								deviceFound = false;
							}
						}
					else
						{
							deviceFound = false;
						}
					delete WideString;
					}
				else
					{
					deviceFound = false;
					}
				}
			if (deviceFound)
				{
				gDevNumber = i;
				}
			else
				{
				// close device
				gUsbDev.Close();
				}
			}
		else
			{
			// close device
			gUsbDev.Close();
			}
		} // for 


	if (dwRC == USBIO_ERR_VERSION_MISMATCH)
		{
		PRINT_ALWAYS "\n* Error: \"The API version reported by the TUSBWIN driver"NL \
			   "*         does not match the expected version.\""NL); 
		PRINT_ALWAYS "* The driver will need to be updated as follows:"NL); 
		PRINT_ALWAYS "* 1. Connect the device to the PC & start T_USB,"NL \
			   "* then find the USB device in the Windows Device Manager"NL \
			   "* ('Control Panel'->'System'->'Hardware'->'Device Manager')."NL \
			   "* Right click on the device name and choose 'Uninstall...'."NL); 
		PRINT_ALWAYS "* 2. In c:\\winnt\\inf\\, find (by searching for \"Symbian\") and"NL \
			   "* delete the *.INF file that was used to install the existing"NL \
			   "* version of TUSBWIN.SYS. Make sure to also delete the"NL \
			   "* precompiled version of that file (<samename>.PNF)."NL); 
		PRINT_ALWAYS "* 3. In c:\\winnt\\system32\\drivers\\, delete the file TUSBWIN.SYS."NL);
		PRINT_ALWAYS "* Then unplug & reconnect the USB device and, when prompted, install"NL \
			   "* the new TUSBWIN.SYS driver using the .INF file from this distribution."NL \
			   "* (All files can be found under e32test\\usb\t_usb_win\\bin_distribution\\.)"NL); 
		}

	return dwRC;
	}

static DWORD OpenSwitchDevice()
	{
	DWORD dwRC;

	if (gDevList)
		{
		CUsbIo::DestroyDeviceList(gDevList);
		gDevList = NULL;
		gUsbDev.Close();	
		}

	// Enumerate attached USB devices supported by USBIO
	gDevList = CUsbIo::CreateDeviceList(&gSwitchID);

	if (gDevList)
		{
		// Open first device in list
		dwRC = gUsbDev.Open(0, gDevList, &gSwitchID);

		PRINT_IF_VERBOSE "\nCUsbIo::Open returned <0x%X>"NL, dwRC);
		}
	else
		{
		dwRC = USBIO_ERR_DEVICE_NOT_FOUND;
		}

	return dwRC;
	}

static void CloseUsbDevice()
	{
	// Close the device
	gUsbDev.Close();
	PRINT_IF_VERBOSE "CUsbIo::Close called"NL);
	}

static DWORD GetDeviceDescriptor()
	{
	DWORD dwRC;
	USB_DEVICE_DESCRIPTOR DeviceDescriptor;

	memset(&DeviceDescriptor, 0, sizeof(USB_DEVICE_DESCRIPTOR));

	// Get device descriptor
	dwRC = gUsbDev.GetDeviceDescriptor(&DeviceDescriptor);
	PRINT_IF_VERBOSE "CUsbIo::GetDeviceDescriptor returned <0x%X>"NL, dwRC);

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		vendorId = DeviceDescriptor.idVendor;
		productId = DeviceDescriptor.idProduct;
		}

	if (gVerboseMode && (dwRC == USBIO_ERR_SUCCESS))
		{
		PRINT_ALWAYS "\nDEVICE DESCRIPTOR:"NL
			   "bLength = <%u>"NL
			   "bDescriptorType = <%u>"NL
			   "bcdUSB = <%u>"NL
			   "bDeviceClass = <%u>"NL
			   "bDeviceSubClass = <%u>"NL
			   "bDeviceProtocol = <%u>"NL
			   "bMaxPacketSize0 = <%u>"NL
			   "idVendor = <%u>"NL
			   "idProduct = <%u>"NL
			   "bcdDevice = <%u>"NL
			   "iManufacturer = <%u>"NL
			   "iProduct = <%u>"NL
			   "iSerialNumber = <%u>"NL
			   "bNumConfigurations = <%u>\n"NL,
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

	if (gVerboseMode)
	{
		USBIO_BANDWIDTH_INFO bwInfo;

		// Get bandwidth available
		dwRC = gUsbDev.GetBandwidthInfo (&bwInfo);
		if (dwRC == USBIO_ERR_SUCCESS)
			{
			PRINT_ALWAYS "Bandwidth total %d consumed %d"NL,bwInfo.TotalBandwidth,bwInfo.ConsumedBandwidth); 
			}
	}

	return dwRC;
	}


static DWORD GetConfigurationDescriptor()
	{
	DWORD dwRC;
	int ifListIndex = 0;

	CHAR szBuffer[MAX_DESCRIPTOR_BUFFER_SIZE] = "";
	USB_CONFIGURATION_DESCRIPTOR* pConfigDescriptor = NULL;
	USB_INTERFACE_DESCRIPTOR* pInterfaceDescriptor = NULL;
	USB_ENDPOINT_DESCRIPTOR* pEndpointDescriptor = NULL;

	DWORD dwByteCount = MAX_DESCRIPTOR_BUFFER_SIZE;

	memset(szBuffer, 0, sizeof(szBuffer));

	memset (gEndpointMap, 0, sizeof(gEndpointMap));
	memset (gPacketSizeMap, 0, sizeof(gPacketSizeMap));

	// force a set interface 
	memset (gAlternateSettings, 0, sizeof(gAlternateSettings));

	// Get first configuration descriptor
	dwRC = gUsbDev.GetConfigurationDescriptor((USB_CONFIGURATION_DESCRIPTOR*) szBuffer,
											dwByteCount, 0);
	PRINT_IF_VERBOSE "CUsbIo::GetConfigurationDescriptor returned <0x%X>"NL, dwRC);

	USBIO_SET_CONFIGURATION SetConfig;

	memset(&SetConfig, 0, sizeof(USBIO_SET_CONFIGURATION));

	// Set the first configuration as active
	SetConfig.ConfigurationIndex = 0;

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		USB_COMMON_DESCRIPTOR* Desc;
		ULONG rl = dwByteCount;
		ULONG ulDescLength = 0;
		CHAR* data = szBuffer;
		int interfaceNumber = 0;
		int alternateSetting = 0;
		int endpointIndex = 0;

		while (rl > 0)
			{
			Desc = (USB_COMMON_DESCRIPTOR*) data;
			ulDescLength = Desc->bLength;
			if ((ulDescLength > rl) || (ulDescLength == 0))
				{
				PRINT_ALWAYS "Length remaining too short!"NL); 
				rl = 0;
				}
			else
				{
				switch (Desc->bDescriptorType)
					{
				case USB_CONFIGURATION_DESCRIPTOR_TYPE:
					pConfigDescriptor =
						(USB_CONFIGURATION_DESCRIPTOR*) data;
					PRINT_IF_VERBOSE "\nCONFIGURATION DESCRIPTOR:"NL
							   "bLength = <%u>"NL
							   "bDescriptorType = <%u>"NL
							   "wTotalLength = <%u>"NL
							   "bNumInterfaces = <%u>"NL
							   "bConfigurationValue = <%u>"NL
							   "iConfiguration = <%u>"NL
							   "bmAttributes = <%u>"NL
							   "MaxPower = <%u>"NL,
							   pConfigDescriptor->bLength,
							   pConfigDescriptor->bDescriptorType,
							   pConfigDescriptor->wTotalLength,
							   pConfigDescriptor->bNumInterfaces,
							   pConfigDescriptor->bConfigurationValue,
							   pConfigDescriptor->iConfiguration,
							   pConfigDescriptor->bmAttributes,
							   pConfigDescriptor->MaxPower); 
					SetConfig.NbOfInterfaces = pConfigDescriptor->bNumInterfaces;
					break;

				case USB_INTERFACE_DESCRIPTOR_TYPE:
					pInterfaceDescriptor = (USB_INTERFACE_DESCRIPTOR*) data;
					interfaceNumber = pInterfaceDescriptor->bInterfaceNumber;
					alternateSetting = pInterfaceDescriptor->bAlternateSetting;
					endpointIndex = 0;
					PRINT_IF_VERBOSE "\nINTERFACE DESCRIPTOR: "NL
							   "bLength = <%u>"NL
							   "bDescriptorType = <%u>"NL
							   "bInterfaceNumber = <%u>"NL
							   "bAlternateSetting = <%u>"NL
							   "bNumEndpoints = <%u>"NL
							   "bInterfaceClass = <%u>"NL
							   "bInterfaceSubClass = <%u>"NL
							   "bInterfaceProtocol = <%u>"NL
							   "iInterface = <%u>"NL,
							   pInterfaceDescriptor->bLength,
							   pInterfaceDescriptor->bDescriptorType,
							   pInterfaceDescriptor->bInterfaceNumber,
							   pInterfaceDescriptor->bAlternateSetting,
							   pInterfaceDescriptor->bNumEndpoints,
							   pInterfaceDescriptor->bInterfaceClass,
							   pInterfaceDescriptor->bInterfaceSubClass,
							   pInterfaceDescriptor->bInterfaceProtocol,
							   pInterfaceDescriptor->iInterface); 
					if (alternateSetting == 0)
						{
						SetConfig.InterfaceList[ifListIndex].InterfaceIndex = interfaceNumber;
						SetConfig.InterfaceList[ifListIndex++].MaximumTransferSize = KMaxTransferSize;
						}
					break;

				case USB_ENDPOINT_DESCRIPTOR_TYPE:
					pEndpointDescriptor =
						(USB_ENDPOINT_DESCRIPTOR*) data;
					gEndpointMap[interfaceNumber] [alternateSetting] [endpointIndex] = pEndpointDescriptor->bEndpointAddress;
					gPacketSizeMap[interfaceNumber] [alternateSetting] [endpointIndex++] = pEndpointDescriptor->wMaxPacketSize;
					PRINT_IF_VERBOSE "\nENDPOINT DESCRIPTOR: "NL
							   "bLength = <%u>"NL
							   "bDescriptorType = <%u>"NL
							   "bEndpointAddress = <%X>"NL
							   "bmAttributes = <%u>"NL
							   "wMaxPacketSize = <%u>"NL
							   "bInterval = <%u>"NL,
							   pEndpointDescriptor->bLength,
							   pEndpointDescriptor->bDescriptorType,
							   pEndpointDescriptor->bEndpointAddress,
							   pEndpointDescriptor->bmAttributes,
							   pEndpointDescriptor->wMaxPacketSize,
							   pEndpointDescriptor->bInterval); 
					break;
				default:
					break;
					}
				}
			data += ulDescLength;
			rl -= ulDescLength;
			}
		}

	dwRC = gUsbDev.SetConfiguration(&SetConfig);
	PRINT_IF_VERBOSE "CUsbIo::SetConfiguration returned <0x%X>"NL, dwRC);

		
	PRINT_IF_VERBOSE ""NL);

	return dwRC;
	}


static DWORD GetStringDescriptor()
	{
	DWORD dwRC;

	CHAR szBuffer[MAX_DESCRIPTOR_BUFFER_SIZE] = "";
	USB_STRING_DESCRIPTOR* pStringDescriptor = NULL;
	DWORD dwByteCount = MAX_DESCRIPTOR_BUFFER_SIZE;

	memset(szBuffer, 0, sizeof(szBuffer));

	// Get string descriptor
	dwRC = gUsbDev.GetStringDescriptor((USB_STRING_DESCRIPTOR*) szBuffer,
										dwByteCount, 1, 0);
	PRINT_IF_VERBOSE "CUsbIo::GetStringDescriptor returned <0x%X>"NL, dwRC);

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		pStringDescriptor = (USB_STRING_DESCRIPTOR*) szBuffer;
		PRINT_IF_VERBOSE "\nSTRING DESCRIPTOR:"NL
				   "bLength = <%u>"NL
				   "bDescriptorType = <%u>"NL
				   "bString = <",							// output continues below!
				   pStringDescriptor->bLength,
				   pStringDescriptor->bDescriptorType); 
		INT i = 0;
		CHAR* Ptr = szBuffer;
		for (i = 2, Ptr += 2;
			 i < pStringDescriptor->bLength;
			 i += 2, Ptr += 2)
			{
			PRINT_IF_VERBOSE "%c", *Ptr);
			}
		PRINT_IF_VERBOSE ">\n"NL);
		}

	return dwRC;
	}


static DWORD GetConfigurationInfo()
	{
	DWORD dwRC;

	USBIO_CONFIGURATION_INFO ConfigInfo;
	USHORT i = 0;

	memset(&ConfigInfo, 0, sizeof(USBIO_CONFIGURATION_INFO));

	dwRC = gUsbDev.GetConfigurationInfo(&ConfigInfo);
	PRINT_IF_VERBOSE "CUsbIo::GetConfigurationInfo returned <0x%X>"NL, dwRC);

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		PRINT_IF_VERBOSE "\nCONFIGURATION INFO:"NL
				   "NbOfInterfaces = <%lu>"NL
				   "NbOfPipes = <%lu>"NL,
				   ConfigInfo.NbOfInterfaces,
				   ConfigInfo.NbOfPipes); 
		for (i = 0; i < ConfigInfo.NbOfInterfaces; i++)
			{
			PRINT_IF_VERBOSE "\nINTERFACE <%u>:"NL, i + 1); 
			PRINT_IF_VERBOSE "InterfaceNumber = <%u>"NL
					   "AlternateSetting = <%u>"NL
					   "Class = <%u>"NL
					   "SubClass = <%u>"NL
					   "Protocol = <%u>"NL
					   "NumberOfPipes = <%u>"NL
					   "reserved1 = <%u>"NL
					   "reserved2 = <%u>"NL,
					   ConfigInfo.InterfaceInfo[i].InterfaceNumber,
					   ConfigInfo.InterfaceInfo[i].AlternateSetting,
					   ConfigInfo.InterfaceInfo[i].Class,
					   ConfigInfo.InterfaceInfo[i].SubClass,
					   ConfigInfo.InterfaceInfo[i].Protocol,
					   ConfigInfo.InterfaceInfo[i].NumberOfPipes,
					   ConfigInfo.InterfaceInfo[i].reserved1,
					   ConfigInfo.InterfaceInfo[i].reserved2); 
			}
		for (i = 0; i < ConfigInfo.NbOfPipes; i++)
			{
			PRINT_IF_VERBOSE ""NL);
			PRINT_IF_VERBOSE "PIPE <%u>"NL, i + 1); 
			PRINT_IF_VERBOSE "PipeType = <%d>"NL
					   "MaximumTransferSize = <%lu>"NL
					   "MaximumPacketSize = <%u>"NL
					   "EndpointAddress = <%X>"NL
					   "Interval = <%u>"NL
					   "InterfaceNumber = <%u>"NL
					   "reserved1 = <%u>"NL
					   "reserved2 = <%u>"NL
					   "reserved3 = <%u>"NL,
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
	PRINT_IF_VERBOSE ""NL);

	return dwRC;
	}


static DWORD OpenPipes(CUsbIoPipe * inPipePtr, CUsbIoPipe * outPipePtr, TestParamPtr tpPtr)
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;
	UCHAR epAddress;

	// Create the bulk OUT pipe
	if (outPipePtr)
		{
		epAddress = gEndpointMap[tpPtr->interfaceNumber][tpPtr->alternateSetting][tpPtr->outPipe-1];
		dwRC = outPipePtr->Bind(gDevNumber, epAddress, gDevList, &gUsbioID);

		PRINT_IF_VERBOSE "CUsbIoPipe::Bind OUT address <0x%X> returned <0x%X>"NL, epAddress,dwRC);
		
		if (dwRC == USBIO_ERR_SUCCESS)
			{
			// Set up for time statistics on OUT pipe
			dwRC = outPipePtr->SetupPipeStatistics(1000L);
			PRINT_IF_VERBOSE "CUsbIoPipe::SetupPipeStatistics OUT returned <0x%X>"NL, dwRC);
			}
		else
			ShowErrorText(dwRC);
		}

	if (inPipePtr && dwRC == USBIO_ERR_SUCCESS)
		{
		// Create the IN pipe
		epAddress = gEndpointMap[tpPtr->interfaceNumber][tpPtr->alternateSetting][tpPtr->inPipe-1];
		dwRC = inPipePtr->Bind(gDevNumber, epAddress, gDevList, &gUsbioID);

		PRINT_IF_VERBOSE "CUsbIoPipe::Bind IN address <0x%X> returned <0x%X>"NL, epAddress,dwRC);

		if (dwRC == USBIO_ERR_SUCCESS)
			{
			// Set up for time statistics on IN pipe
			dwRC = inPipePtr->SetupPipeStatistics(1000L);
			PRINT_IF_VERBOSE "CUsbIoPipe::SetupPipeStatistics IN returned <0x%X>"NL, dwRC);
			}
		else
			ShowErrorText(dwRC);

		}

	PRINT_IF_VERBOSE ""NL);

	return dwRC;
	}


static DWORD ClosePipes(CUsbIoPipe * inPipePtr, CUsbIoPipe * outPipePtr)
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;

	// Close down the OUT pipe
	if (outPipePtr)
		{
		dwRC = outPipePtr->Unbind();
		PRINT_IF_VERBOSE "CUsbIoPipe::Unbind OUT returned <0x%X>"NL, dwRC);
		}

	if (inPipePtr && dwRC == USBIO_ERR_SUCCESS)
		{
		// Close down the IN pipe
		dwRC = inPipePtr->Unbind();
		PRINT_IF_VERBOSE "CUsbIoPipe::Unbind IN returned <0x%X>"NL, dwRC);
		}

	return dwRC;
	}

static void SendLogFile()
	{
	logStream = fopen (gLogfileName, "rb");
	if (logStream == NULL)
		{
		PRINT_TIME "fopen failed  ");
		return;
		}

	char logBuffer[KHostLogFileSize];
	CUsbIoPipe* outPipePtr = new CUsbIoPipe();
	CUsbIoBuf bufPtr((VOID*) logBuffer, KHostLogFileSize);			// the data buffer
	int bytesRead;

	DWORD dwRC = USBIO_ERR_SUCCESS;
	UCHAR epAddress;

	// Create the bulk OUT pipe
	epAddress = gEndpointMap[0][0][1];
	dwRC = outPipePtr->Bind(gDevNumber, epAddress, gDevList, &gUsbioID);
	
	if (dwRC != USBIO_ERR_SUCCESS)
		{
		PRINT_TIME "bind failed  ");
		return;
		}

	bytesRead = fread ( logBuffer, sizeof(char), KHostLogFileSize, logStream );
	if (bytesRead > 0)
		{
		PRINT_TIME "fread success. %d bytes read", bytesRead);
		bufPtr.NumberOfBytesToTransfer = bytesRead;
		outPipePtr->Write(&bufPtr);
		dwRC = outPipePtr->WaitForCompletion(&bufPtr, gReadWriteTimeOut);
		PRINT_TIME "write completes with %l",dwRC);
		if (dwRC != USBIO_ERR_SUCCESS)
			{
			return;
			}
		}
	
	// The sending of empty buffer is removed because mobile receives only one packet 
	// (in CActiveControl::PrintHostLog function) from PC in both non-shared chunk 
	// and shared chunk implementation, and then mobile exits the application.
	// PC may be dead if PC sends this redundant packet while mobile is exiting 
	//CUsbIoBuf bufPtrEnd((VOID*) logBuffer, 0);
	//bufPtrEnd.NumberOfBytesToTransfer = 0;
	//outPipePtr->Write(&bufPtrEnd);
	//dwRC = outPipePtr->WaitForCompletion(&bufPtrEnd, gReadWriteTimeOut);
	
	return;
	}

static DWORD ReceiveCounter()
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;

	DWORD bytes_read = sizeof(__int64);

	// Here we (hope to) read an 8 byte packet containing a counter value.
	PRINT_NOLOG "* Waiting for T_USB counter packet to arrive..."); 

	__int64 counter;

	USBIO_CLASS_OR_VENDOR_REQUEST request;
	request.Flags = USBIO_SHORT_TRANSFER_OK;
	request.Type = RequestTypeClass;
	request.Recipient = RecipientDevice;
	request.RequestTypeReservedBits = 0;
	request.Request = ETestIdleCounter;
	request.Value = gDeviceIdleCount < 0 ? 0 : 1;
	request.Index = 0;


	dwRC = gUsbDev.ClassOrVendorInRequest(&counter,bytes_read,&request);

	PRINT_NOLOG " done."NL); 
	if (dwRC != USBIO_ERR_SUCCESS)
		{
		PRINT_ALWAYS "\n* Error: CUsbIoPipe::Read (version)"); 
		ShowErrorText(dwRC);
		return dwRC;
		}
	
	if (gDeviceIdleCount < 0)
		{
		// Initialise counter
		gDeviceIdleCount = counter;
		_ftime(&gHostTime);
		PRINT_TIME " * Start Device Idle Counter"NL); 
		}
	else
		{
		// if the counter is zero then the tests havent completed on the device
		while (counter < gDeviceIdleCount)
			{
			Delay(WAIT_TEST_COMPLETE);
			dwRC = gUsbDev.ClassOrVendorInRequest(&counter,bytes_read,&request);
			if (dwRC != USBIO_ERR_SUCCESS)
				{
				PRINT_ALWAYS "\n* Error: CUsbIoPipe::Read (version)"); 
				ShowErrorText(dwRC);
				return dwRC;
				}
			}
		__int64 diff = counter - gDeviceIdleCount;
		struct _timeb tmp = gHostTime;
		_ftime(&gHostTime);
		gDeviceIdleCount = counter;
		int ms = (gHostTime.time - tmp.time) * 1000 + gHostTime.millitm - tmp.millitm;
		PRINT_TIME " * Stop Device Idle Counter: %I64d increments in %dms (%dinc/ms)"NL,
			diff, ms, diff / ms);
		}

	return dwRC;
	}

static DWORD ReceiveVersion()
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;

	// Here we (hope to) read an 8 byte packet containing the T_USB version.
	PRINT_NOLOG "* Waiting for T_USB version packet to arrive..."); 

	BYTE versionData[VERSION_DATAIN_SIZE];

	// The first 3 bytes are interpreted as a version.
	DWORD bytes_read = VERSION_DATAIN_SIZE;

	USBIO_CLASS_OR_VENDOR_REQUEST request;
	request.Flags = USBIO_SHORT_TRANSFER_OK;
	request.Type = RequestTypeClass;
	request.Recipient = RecipientDevice;
	request.RequestTypeReservedBits = 0;
	request.Request = EVersion;
	request.Value = 0;
	request.Index = 0;


	dwRC = gUsbDev.ClassOrVendorInRequest(&versionData[0],bytes_read,&request);

	PRINT_NOLOG " done."NL); 
	if (dwRC != USBIO_ERR_SUCCESS)
		{
		PRINT_ALWAYS "\n* Error: CUsbIoPipe::Read (version)"); 
		ShowErrorText(dwRC);
		return dwRC;
		}
	if (versionData[0] < KTusbVersionMajor || 
		(versionData[0] == KTusbVersionMajor && versionData[1] < KTusbVersionMinor) || 
		(versionData[0] == KTusbVersionMajor && versionData[1] == KTusbVersionMinor && versionData[2] < KTusbVersionMicro)) 
		{
		PRINT_ALWAYS "* Inadequate version of T_USB: %d.%d.%d (we need at least %d.%d.%d)"NL,
			   versionData[0],versionData[1],versionData[2],KTusbVersionMajor,KTusbVersionMinor,KTusbVersionMicro); 
		dwRC = USBIO_ERR_FAILED;
		return dwRC;
		}
	PRINT_NOLOG "* Suitable version of T_USB_DEVICE found: %d.%d.%d."NL, versionData[0],versionData[1],versionData[2]); 

	char * scriptFilePtr;
	scriptFilePtr = strstr((char *)&versionData[3],"/script=");

	if (scriptFilePtr != NULL)
		{
		* scriptFilePtr = '\0';
		scriptFilePtr += 8;
		PRINT_ALWAYS "Script file name %s"NL,scriptFilePtr);
		if (strstr(gScriptFileName,scriptFilePtr) == NULL)
			{
			dwRC = ERR_SCRIPT_NAME;
			}
		}

	PRINT_ALWAYS "Device configuration file  %s"NL,&versionData[3]);

	return dwRC;
	}


static DWORD SendVersion()
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;

	// Here we send an 8 byte packet containing T_USB_WIN's + USBIO's versions.
	PRINT_NOLOG "* Sending our version packet to T_USB..."); 

	BYTE versionData[VERSION_DATAOUT_SIZE];
	versionData[0] = VERSION_MAJOR;
	versionData[1] = VERSION_MINOR;
	versionData[2] = VERSION_MICRO;
	versionData[3] = USBIO_VERSION_MAJOR;
	versionData[4] = USBIO_VERSION_MINOR;

	USBIO_CLASS_OR_VENDOR_REQUEST request;
	request.Flags = USBIO_SHORT_TRANSFER_OK;
	request.Type = RequestTypeClass;
	request.Recipient = RecipientDevice;
	request.RequestTypeReservedBits = 0;
	request.Request = EVersion;
	request.Value = 0;
	request.Index = 0;

	DWORD data_bytes = VERSION_DATAOUT_SIZE;
	dwRC = gUsbDev.ClassOrVendorOutRequest(&versionData[0],data_bytes,&request);
	PRINT_NOLOG " done."NL); 
	if (dwRC != USBIO_ERR_SUCCESS)
		{
		PRINT_ALWAYS "\n* Error: CUsbIoPipe::Write (version)"); 
		ShowErrorText(dwRC);
		}

	return dwRC;
	}


static DWORD ExchangeVersions()
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;


	dwRC = SendVersion();
	if (dwRC != USBIO_ERR_SUCCESS)
		return dwRC;
	dwRC = ReceiveVersion();

	return dwRC;
	}


static DWORD ChangeInterfaceSetting(BYTE aInterfaceNumber, BYTE aAlternateSetting)
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;
	
	if (gAlternateSettings[aInterfaceNumber] != aAlternateSetting)
		{
		gAlternateSettings[aInterfaceNumber] = aAlternateSetting;
		USBIO_INTERFACE_SETTING ifSetting = {aInterfaceNumber,aAlternateSetting,4096};
		dwRC = gUsbDev.SetInterface(&ifSetting);
		if (dwRC == USBIO_ERR_SUCCESS)
			{
			PRINT_NOLOG "Set Interface %d to Alternate Setting %d"NL,aInterfaceNumber,aAlternateSetting); 
			}
		else
			{
			PRINT_ALWAYS "\n* Error: Setting Interface %d Alternate Setting %d",aInterfaceNumber,aAlternateSetting); 
			ShowErrorText(dwRC);
			}
		}
	
	return dwRC;
	}

static DWORD ReadData(CUsbIoPipe * inPipePtr,CUsbIoBuf* bufPtr, TestParamPtr tpPtr)
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;
	UCHAR epAddress = gEndpointMap[tpPtr->interfaceNumber][tpPtr->alternateSetting][tpPtr->inPipe-1];

	// We have to setup a read for at least one byte in order to get
	// the host to issue IN tokens for our zero-byte read:
	inPipePtr->Read(bufPtr);
	dwRC = inPipePtr->WaitForCompletion(bufPtr, gReadWriteTimeOut);

	if (dwRC != USBIO_ERR_SUCCESS)
		{
		PRINT_ALWAYS "\n* Error: EP Number %d Address <0x%X> CUsbIoPipe::Read %d bytes",tpPtr->inPipe,epAddress, bufPtr->NumberOfBytesToTransfer); 
		ShowErrorText(dwRC);
		}
	else
		{
		if (bufPtr->BytesTransferred != bufPtr->NumberOfBytesToTransfer)
			{
			DWORD pktNum = *(DWORD *)bufPtr->Buffer();
			PRINT_ALWAYS "* EP Address <0x%X> Read more/less bytes (%d) than expected (%d) packet number 0x%x"NL, 
				   epAddress, bufPtr->BytesTransferred,bufPtr->NumberOfBytesToTransfer,pktNum); 
			dwRC = USBIO_ERR_FAILED;
			}
		else
			{
			PRINT_IF_VERBOSE "* EP Address <0x%X> Read %d bytes."NL, epAddress,bufPtr->BytesTransferred);
			}
		}

	return dwRC;
	}


static DWORD WriteData(CUsbIoPipe * outPipePtr,CUsbIoBuf* bufPtr, TestParamPtr tpPtr)
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;
	UCHAR epAddress = gEndpointMap[tpPtr->interfaceNumber][tpPtr->alternateSetting][tpPtr->outPipe-1];
	
	outPipePtr->Write(bufPtr);
	dwRC = outPipePtr->WaitForCompletion(bufPtr, gReadWriteTimeOut);

	if (dwRC == USBIO_ERR_SUCCESS)
		{
		PRINT_IF_VERBOSE "* EP Address <0x%X> Wrote %d bytes."NL, epAddress, bufPtr->BytesTransferred);
		}
	else
		{
		PRINT_ALWAYS "\n* Error: EP Number %d Address <0x%X> CUsbIoPipe::Write %d bytes",tpPtr->outPipe,epAddress, bufPtr->NumberOfBytesToTransfer); 
		ShowErrorText(dwRC);
		}

	return dwRC;
	}


static DWORD CheckAndPrintStats(CUsbIoPipe * inPipePtr, CUsbIoPipe * outPipePtr, BYTE testIndex, DWORD loopCount, DWORD * averageInRate, DWORD * averageOutRate, DWORD inRate = 0, DWORD outRate = 0)
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;
	USBIO_PIPE_STATISTICS pipeStats;
	
	PRINT_NOLOG "*** Test Index %d Iteration %d ***"NL,testIndex,loopCount); 
	if (outPipePtr)
		{
		dwRC = outPipePtr->QueryPipeStatistics (&pipeStats);
	
		if (dwRC == USBIO_ERR_SUCCESS)
			{
			__int64 byteCount = (((__int64)pipeStats.BytesTransferred_H)<<32) + pipeStats.BytesTransferred_L;
			PRINT_NOLOG "OUT %I64u bytes  Succeeded %lu Failed %lu Rate %lu bytes/sec."NL,
				byteCount,pipeStats.RequestsSucceeded,pipeStats.RequestsFailed,pipeStats.AverageRate); 
			
			if (pipeStats.RequestsFailed)
				{
				PRINT_ALWAYS "%d OUT Requests Failed"NL,pipeStats.RequestsFailed);
				dwRC = USBIO_ERR_FAILED;
				}
			* averageOutRate = pipeStats.AverageRate;
			if (pipeStats.AverageRate < outRate)
				{
				PRINT_ALWAYS "Failure Average OUT Rate %lu < Minimum Rate %lu"NL,pipeStats.AverageRate,outRate);
				dwRC = USBIO_ERR_FAILED;
				}
			}
		else
			{
			PRINT_ALWAYS "\n* Error: CUsbIoPipe::Query OUT Pipe Statistics"); 
			ShowErrorText(dwRC);
			}

		}

	if (inPipePtr && dwRC == USBIO_ERR_SUCCESS)
		{
		dwRC = inPipePtr->QueryPipeStatistics (&pipeStats);
	
		if (dwRC == USBIO_ERR_SUCCESS)
			{
			__int64 byteCount = (((__int64)pipeStats.BytesTransferred_H)<<32) + pipeStats.BytesTransferred_L;
			PRINT_NOLOG "IN %I64u bytes  Succeeded %lu Failed %lu Rate %lu bytes/sec."NL,
				byteCount,pipeStats.RequestsSucceeded,pipeStats.RequestsFailed,pipeStats.AverageRate); 

			if (pipeStats.RequestsFailed)
				{
				PRINT_ALWAYS "%d IN Requests Failed"NL,pipeStats.RequestsFailed);
				dwRC = USBIO_ERR_FAILED;
				}
			* averageInRate = pipeStats.AverageRate;
			if (pipeStats.AverageRate < inRate)
				{
				PRINT_ALWAYS "Failure Average IN Rate %lu < Minimum Rate %lu"NL,pipeStats.AverageRate,inRate);
				dwRC = USBIO_ERR_FAILED;
				}

			}
		else
			{
			PRINT_ALWAYS "\n* Error: CUsbIoPipe::Query IN Pipe Statistics"); 
			ShowErrorText(dwRC);
			}
		}

	return dwRC;
	}


UINT LoopTransfer( LPVOID pParam )
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;

	TestParamType testParams = *(TestParamPtr)pParam;

	PRINT_NOLOG "* Thread %d Loop Transfers -- reading & writing alternately."NL,testParams.host.index); 

	ResetEvent (gThreadEndedEvents[testParams.host.index]);
	SetEvent (gThreadStartedEvent);

	CUsbIoPipe* inPipePtr = new CUsbIoPipe();
	CUsbIoPipe* outPipePtr = new CUsbIoPipe();

	BYTE* DataPtr = new BYTE[testParams.maxSize];
	CUsbIoBuf buf((VOID*) DataPtr, testParams.maxSize);			// the data buffer

	BYTE* WaitSettingDataPtr = new BYTE[WAIT_SETTING_BUFFER_SIZE];
	CUsbIoBuf bufWaitSetting((VOID*) WaitSettingDataPtr, WAIT_SETTING_BUFFER_SIZE);			// another data buffer
	bufWaitSetting.NumberOfBytesToTransfer = WAIT_SETTING_BUFFER_SIZE;

	if (!testParams.settingRepeat)
		{
		OpenPipes (inPipePtr,outPipePtr,&testParams);
		}

	DWORD averageInRate;
	DWORD averageOutRate;
	int repeatCount = 0;
	buf.NumberOfBytesToTransfer = testParams.minSize;
	while ((dwRC == USBIO_ERR_SUCCESS) && !gFinishAllTests && !gAbort && (!testParams.repeat || repeatCount < testParams.repeat))
		{
		if (testParams.settingRepeat)
			{
			if ((repeatCount % testParams.settingRepeat) == 0)
				{
				if (repeatCount)
					{
					// Wait for this extra read to ensure that ok to change interface setting
					inPipePtr->Read(&bufWaitSetting);
					dwRC = inPipePtr->WaitForCompletion(&bufWaitSetting, WAIT_SETTING_TIMEOUT);
					// Not necessary if different interfaces used as setting interface closes pipes,
					// but if the same interface is used then prevents errors when opening pipes
					ClosePipes (inPipePtr,outPipePtr);
					PRINT_IF_VERBOSE "Signal for thread %d"NL,testParams.host.index);
					SetEvent (gThreadWaitEvents[testParams.host.index]);
					}
				
				PRINT_IF_VERBOSE "Wait for thread %d"NL,testParams.beforeIndex);
				dwRC = WaitForSingleObject (gThreadWaitEvents[testParams.beforeIndex],WAIT_BEFORETHREAD_TIMEOUT);
				if (dwRC != WAIT_OBJECT_0)
					{
					PRINT_ALWAYS "Error waiting for alternate setting before thread"NL);
					return 0;
					}
				dwRC = ChangeInterfaceSetting (testParams.interfaceNumber,testParams.alternateSetting);
				OpenPipes (inPipePtr,outPipePtr,&testParams);
				}
			}

		dwRC = ReadData(inPipePtr,&buf,&testParams);
		if (dwRC == USBIO_ERR_SUCCESS)
			{
			// Now we send the received data back to the client.
			dwRC = WriteData(outPipePtr,&buf,&testParams);
			}

		repeatCount++;
		if (dwRC == USBIO_ERR_SUCCESS)
			{
			if (repeatCount == testParams.repeat)
				{
				dwRC = CheckAndPrintStats(inPipePtr,outPipePtr,testParams.host.index,repeatCount,&averageInRate,&averageOutRate,testParams.host.inRate,testParams.host.outRate);
				}
			else
				{
				if (testParams.host.displayRate && (repeatCount % testParams.host.displayRate) == 0)
					{
					dwRC = CheckAndPrintStats(inPipePtr,outPipePtr,testParams.host.index,repeatCount,&averageInRate,&averageOutRate);
					}
				}
			}

		if (buf.NumberOfBytesToTransfer == testParams.maxSize)
			{
			buf.NumberOfBytesToTransfer = testParams.minSize;
			}
		else
			{
			buf.NumberOfBytesToTransfer++;
			}
		}


	Delay (WAIT_TEST_COMPLETE);
	USBIO_CLASS_OR_VENDOR_REQUEST request;
	request.Flags = USBIO_SHORT_TRANSFER_OK;
	request.Type = RequestTypeClass;
	request.Recipient = RecipientDevice;
	request.RequestTypeReservedBits = 0;
	request.Request = ETestResult;
	request.Index = testParams.host.index;
	request.Value = (dwRC == USBIO_ERR_SUCCESS) ? true : false;
	DWORD dataBytes = 0;
	dwRC = gUsbDev.ClassOrVendorOutRequest(NULL,dataBytes,&request);

	PRINT_TIME "Test Index %d IN rate %lu OUT rate %lu Loop %s  ",testParams.host.index,averageInRate,averageOutRate,request.Value ? "Passed" : "Failed"); 

	dwRC = ClosePipes (inPipePtr,outPipePtr);
	delete DataPtr;
	delete inPipePtr;
	delete outPipePtr;

	if (request.Value)
		{
		if (testParams.host.signalNextSettingThread)
			{
			PRINT_IF_VERBOSE "Signal for thread %d"NL,testParams.host.index);
			SetEvent (gThreadWaitEvents[testParams.host.index]);
			}
		}
	else
		{	
		gAbort = true;
		}

	SetEvent (gThreadEndedEvents[testParams.host.index]);

	return 0;
	}


UINT ReceiveOnlyTransfer(LPVOID pParam)
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;
	BYTE* DataPtr;										// the read/write buffer

	TestParamType testParams = *(TestParamPtr)pParam;

	PRINT_NOLOG "* Receive-only transfers (IN)."NL); 

	ResetEvent (gThreadEndedEvents[testParams.host.index]);
	SetEvent (gThreadStartedEvent);

	CUsbIoPipe * inPipePtr = new CUsbIoPipe();

	DataPtr = new BYTE[testParams.maxSize];
	CUsbIoBuf buf((VOID*) DataPtr, testParams.maxSize);			// the data buffer

	if (!testParams.settingRepeat)
		{
		OpenPipes (inPipePtr,NULL,&testParams);
		}

	buf.NumberOfBytesToTransfer = testParams.maxSize;
	DWORD averageInRate;
	DWORD averageOutRate;
	int repeatCount = 0;
	DWORD expected = testParams.packetNumber;
	DWORD pktNum = 0;
	int saveTimeout = gReadWriteTimeOut;
	gReadWriteTimeOut = FIRSTFILEREAD_TIMEOUT;
	while ((dwRC == USBIO_ERR_SUCCESS) && !gFinishAllTests && !gAbort && (!testParams.repeat || repeatCount < testParams.repeat))
		{
		if (testParams.settingRepeat)
			{
			if ((repeatCount % testParams.settingRepeat) == 0)
				{
				if (repeatCount)
					{
					// Wait to ensure that ok to change interface setting
					Delay (WAIT_SETTING_DELAY);
					// Not necessary if different interfaces used as setting interface closes pipes,
					// but if the same interface is used then prevents errors when opening pipes
					ClosePipes (inPipePtr,NULL);
					PRINT_IF_VERBOSE "Signal for thread %d"NL,testParams.host.index);
					SetEvent (gThreadWaitEvents[testParams.host.index]);
					}
				
				PRINT_IF_VERBOSE "Wait for thread %d"NL,testParams.beforeIndex);
				dwRC = WaitForSingleObject (gThreadWaitEvents[testParams.beforeIndex],WAIT_BEFORETHREAD_TIMEOUT);
				if (dwRC != WAIT_OBJECT_0)
					{
					PRINT_ALWAYS "Error waiting for alternate setting before thread"NL); 
					return 0;
					}
				dwRC = ChangeInterfaceSetting(testParams.interfaceNumber,testParams.alternateSetting);
				OpenPipes (inPipePtr,NULL,&testParams);
				}
			}

		// Then we read the data bytes
		dwRC = ReadData(inPipePtr,&buf,&testParams);
		if (gDeviceIdleCount < 0)
			{
			gReadWriteTimeOut = saveTimeout;
			ReceiveCounter();
			}
		pktNum = *(DWORD *)buf.Buffer();
		if (pktNum != expected)
			{
			PRINT_ALWAYS "\n* Error: rcv'd wrong pkt number: 0x%x (expected: 0x%x)"NL, pktNum, expected); 
			expected = pktNum;
			}
		expected++;
		// Finally, sometimes we print some statistics
		repeatCount++;
		if (dwRC == USBIO_ERR_SUCCESS)
			{
			if (repeatCount == testParams.repeat)
				{
				dwRC = CheckAndPrintStats(inPipePtr,NULL,testParams.host.index,repeatCount,&averageInRate,&averageOutRate,testParams.host.inRate,testParams.host.outRate);
				}
			else
				{
				if (testParams.host.displayRate && (repeatCount % testParams.host.displayRate) == 0)
					{
					dwRC = CheckAndPrintStats(inPipePtr,NULL,testParams.host.index,repeatCount,&averageInRate,&averageOutRate);
					}
				}
			}
		else
			{
			PRINT_ALWAYS "\n* Error: Receive-only transfer::read"); 
			ShowErrorText(dwRC);
			}
		}

	Delay (WAIT_TEST_COMPLETE);
	USBIO_CLASS_OR_VENDOR_REQUEST request;
	request.Flags = USBIO_SHORT_TRANSFER_OK;
	request.Type = RequestTypeClass;
	request.Recipient = RecipientDevice;
	request.RequestTypeReservedBits = 0;
	request.Request = ETestResult;
	request.Index = testParams.host.index;
	request.Value = (dwRC == USBIO_ERR_SUCCESS) ? true : false;
	DWORD dataBytes = 0;
	dwRC = gUsbDev.ClassOrVendorOutRequest(NULL,dataBytes,&request);

	PRINT_TIME "Test Index %d IN rate %lu Receieve-Only %s  ",testParams.host.index,averageInRate,request.Value ? "Passed" : "Failed"); 
	
	dwRC = ClosePipes (inPipePtr,NULL);

	delete DataPtr;
	delete inPipePtr;

	if (request.Value)
		{
		if (testParams.host.signalNextSettingThread)
			{
			PRINT_IF_VERBOSE "Signal for thread %d"NL,testParams.host.index);
			SetEvent (gThreadWaitEvents[testParams.host.index]);
			}
		}
	else
		{	
		gAbort = true;
		}

	SetEvent (gThreadEndedEvents[testParams.host.index]);

	return 0;
	}


UINT TransmitOnlyTransfer(LPVOID pParam)
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;
	BYTE* DataPtr;										// the read/write buffer

	PRINT_NOLOG "* Transmit-only transfers (OUT)."NL); 

	TestParamType testParams = *(TestParamPtr)pParam;

	ResetEvent (gThreadEndedEvents[testParams.host.index]);
	SetEvent (gThreadStartedEvent);

	CUsbIoPipe * outPipePtr = new CUsbIoPipe();

	DataPtr = new BYTE[testParams.maxSize];
	CUsbIoBuf buf((VOID*) DataPtr, testParams.maxSize);			// the data buffer

	BYTE* WaitSettingDataPtr = new BYTE[WAIT_SETTING_BUFFER_SIZE];
	CUsbIoBuf bufWaitSetting((VOID*) WaitSettingDataPtr, WAIT_SETTING_BUFFER_SIZE);			// another data buffer
	bufWaitSetting.NumberOfBytesToTransfer = WAIT_SETTING_BUFFER_SIZE;

	if (!testParams.settingRepeat)
		{
		OpenPipes (NULL,outPipePtr,&testParams);
		}

	buf.NumberOfBytesToTransfer = testParams.maxSize;
	DWORD averageInRate;
	DWORD averageOutRate;
	int repeatCount = 0;
	DWORD pktNum = testParams.packetNumber;
	while ((dwRC == USBIO_ERR_SUCCESS) && !gFinishAllTests && !gAbort && (!testParams.repeat || repeatCount < testParams.repeat))
		{
		if (testParams.settingRepeat)
			{
			if ((repeatCount % testParams.settingRepeat) == 0)
				{
				if (repeatCount)
					{
					// Wait to ensure that ok to change interface setting
					Delay (WAIT_SETTING_DELAY);
					// Not necessary if different interfaces used as setting interface closes pipes,
					// but if the same interface is used then prevents errors when opening pipes
					ClosePipes (NULL,outPipePtr);
					PRINT_IF_VERBOSE "Signal for thread %d"NL,testParams.host.index);
					SetEvent (gThreadWaitEvents[testParams.host.index]);
					}
				
				PRINT_IF_VERBOSE "Wait for thread %d"NL,testParams.beforeIndex);
				dwRC = WaitForSingleObject (gThreadWaitEvents[testParams.beforeIndex],WAIT_BEFORETHREAD_TIMEOUT);
				if (dwRC != WAIT_OBJECT_0)
					{
					PRINT_ALWAYS "Error waiting for alternate setting before thread"NL); 
					return 0;
					}
				dwRC = ChangeInterfaceSetting (testParams.interfaceNumber,testParams.alternateSetting);
				OpenPipes (NULL,outPipePtr,&testParams);
				}
			}
		
		
		// Then we write 'Length' bytes
		*(DWORD *)buf.Buffer() = pktNum++;
		dwRC = WriteData(outPipePtr,&buf,&testParams);

		repeatCount++;
		// Finally, sometimes we print some statistics
		if (dwRC == USBIO_ERR_SUCCESS)
			{
			if (repeatCount == testParams.repeat)
				{
				dwRC = CheckAndPrintStats(NULL,outPipePtr,testParams.host.index,repeatCount,&averageInRate,&averageOutRate,testParams.host.inRate,testParams.host.outRate);
				}
			else
				{
				if (testParams.host.displayRate && (repeatCount % testParams.host.displayRate) == 0)
					{
					dwRC = CheckAndPrintStats(NULL,outPipePtr,testParams.host.index,repeatCount,&averageInRate,&averageOutRate);
					}
				}
			}
		else
			{
			PRINT_ALWAYS "\n* Error: Transmit-only transfer::write"); 
			ShowErrorText(dwRC);
			}
		}

	Delay (WAIT_TEST_COMPLETE);
	USBIO_CLASS_OR_VENDOR_REQUEST request;
	request.Flags = USBIO_SHORT_TRANSFER_OK;
	request.Type = RequestTypeClass;
	request.Recipient = RecipientDevice;
	request.RequestTypeReservedBits = 0;
	request.Request = ETestResult;
	request.Index = testParams.host.index;
	request.Value = (dwRC == USBIO_ERR_SUCCESS) ? true : false;
	DWORD dataBytes = 0;
	dwRC = gUsbDev.ClassOrVendorOutRequest(NULL,dataBytes,&request);

	PRINT_TIME "Test Index %d OUT rate %lu Transmit Only %s  ",testParams.host.index,averageOutRate,request.Value ? "Passed" : "Failed"); 

	ClosePipes (NULL,outPipePtr);
	delete DataPtr;
	delete outPipePtr;

	if (request.Value)
		{
		if (testParams.host.signalNextSettingThread)
			{
			PRINT_IF_VERBOSE "Signal for thread %d"NL,testParams.host.index);
			SetEvent (gThreadWaitEvents[testParams.host.index]);
			}
		}
	else
		{	
		gAbort = true;
		}

	SetEvent (gThreadEndedEvents[testParams.host.index]);

	return 0;
	}


static BOOL CreateEvents()
	{
    // Create a manual-reset event object.
	// Any thread sets this to signaled when it has started. 
    gThreadStartedEvent = CreateEvent( 
        NULL,         // no security attributes
        TRUE,         // manual-reset event
        FALSE,         // initial state is non-signaled
        "ThreadStarted"  // object name
        ); 
    if (gThreadStartedEvent == NULL)
		{ 
		PRINT_ALWAYS "Error creating thread started event"NL); 
        // error exit
		return FALSE;
		}
    // Create a manual-reset event object. 
    // Set to signaled by CT_usb_winDlg when the device is connected. 
    gDeviceConnectEvent = CreateEvent( 
        NULL,         // no security attributes
        TRUE,         // manual-reset event
        FALSE,         // initial state is non-signaled
        "DeviceConnected"  // object name
        ); 
	if (gDeviceConnectEvent == NULL)
		{ 
		PRINT_ALWAYS "Error creating device connected event"NL); 
        // error exit
		return FALSE;
		}
    // Create a manual-reset event object. 
    // Set to nonsignaled by CT_usb_winDlg when the device is disconnected. 
    gDeviceDisconnectEvent = CreateEvent( 
        NULL,         // no security attributes
        TRUE,         // manual-reset event
        FALSE,         // initial state is non-signaled
        "DeviceDisconnected"  // object name
        ); 
   if (gDeviceDisconnectEvent == NULL)
		{ 
		PRINT_ALWAYS "Error creating device disconnected event"NL); 
        // error exit
		return FALSE;
		}

	for (int i = 0; i < MAX_THREADS; i++)
		{	
	    // Create manual-reset event object for each thread. Any thread sets 
		// this to nonsignaled when it has started. 
	    gThreadEndedEvents[i] = CreateEvent( 
		    NULL,         // no security attributes
			TRUE,         // manual-reset event
			FALSE,        // initial state is non-signaled
			""			  // object name
			); 
	    if (gThreadEndedEvents[i] == NULL)
			{ 
			PRINT_ALWAYS "Error creating thread %d ended event"NL,i); 
			// error exit
			return FALSE;
			}
	    // Create auto-reset event object for each thread. Any thread sets 
		// this to nonsignaled when it has started. 
	    gThreadWaitEvents[i] = CreateEvent( 
		    NULL,         // no security attributes
			FALSE,        // automatically-reset event
			FALSE,        // initial state is non-signaled
			""			  // object name
			); 
	    if (gThreadEndedEvents[i] == NULL)
			{ 
			PRINT_ALWAYS "Error creating thread %d ended event"NL,i); 
			// error exit
			return FALSE;
			}
		}
	return TRUE;
	}


static BOOL GetDeviceInfo()
	{
	DWORD dwRC;

	dwRC = GetDeviceDescriptor();
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		dwRC = GetConfigurationDescriptor();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		dwRC = GetStringDescriptor();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		// In order to give the USB device-side program (t_usb_device)
		// enough time after getting configured to carry out
		// some device tests, we wait here for a short while
		// before proceeding:
		Delay(DEVICE_CONFIG_DELAY);
		dwRC = GetConfigurationInfo();
		}
	if (dwRC == USBIO_ERR_SUCCESS)
		{
		dwRC = ExchangeVersions();
		}
	if (dwRC != USBIO_ERR_SUCCESS)
		{
		if (dwRC == ERR_SCRIPT_NAME)
			PRINT_ALWAYS "*** Warning Script Name Mismatch "NL);
		else
			ShowErrorText(dwRC);
		return FALSE;
		}

	return TRUE;
	}

UINT DoTests(LPVOID pParam)
	{
	DWORD dwRC = USBIO_ERR_SUCCESS;
	bool firstSetting = true;
	BYTE threadIndex = 0;
	BYTE lastSettingThreadIndex = 0;
	TestParamType testParams;
	USBIO_CLASS_OR_VENDOR_REQUEST request;
	DWORD dataBytes;
	FILE* inStream;
	char line[256];
	long repeatPosition = -1;
	long repeatCount = -1;
	UsbcvControl * usbcv;
	bool hostTestFail = false;
	bool mstoreDriveReady = false;
	DWORD waitConnectTime = 0;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

	if (!CreateEvents())
		{
		return FALSE;
		}
	if( (inStream  = fopen( gScriptFileName, "r" )) == NULL )
		{
		PRINT_ALWAYS  "The test script file %s was not opened"NL,gScriptFileName ); 
		return FALSE;
		}
	else
		{
		PRINT_IF_VERBOSE  "The test script file %s opened"NL,gScriptFileName);   
		}

	PRINT_ALWAYS "Script file  %s"NL,gScriptFileName);

	// set up the EP 0 request structure	
	request.Flags = USBIO_SHORT_TRANSFER_OK;
	request.Type = RequestTypeClass;
	request.Recipient = RecipientDevice;
	request.RequestTypeReservedBits = 0;

	gTransferMode = EComment;

	PRINT_TIME "Tests Started  ");

	/* Cycle until end of file reached: */
	while( !feof( inStream ) && !gAbort)
		{
		testParams.host.index = threadIndex;

		// for loopback tests 
		if (gTransferMode < EComment)
			{
			if (testParams.settingRepeat == 0)
				{
				dwRC = ChangeInterfaceSetting (testParams.interfaceNumber,testParams.alternateSetting);
				}
			request.Request = ETestParam;
			request.Index = testParams.host.index;
			if (firstSetting && testParams.settingRepeat)
				{
				request.Index |= FIRST_SETTING_THREAD_MASK;
				PRINT_IF_VERBOSE "Signal for thread %d"NL,testParams.beforeIndex);
				lastSettingThreadIndex = testParams.beforeIndex;
				SetEvent (gThreadWaitEvents[lastSettingThreadIndex]);
				firstSetting = false;
				}
			if (testParams.settingRepeat && threadIndex == lastSettingThreadIndex)
				{
				request.Index |= LAST_SETTING_THREAD_MASK;
				}
			dataBytes = sizeof(testParams) - sizeof(TestParamHostType);
			dwRC = gUsbDev.ClassOrVendorOutRequest(&testParams,dataBytes,&request);
			if (dwRC != USBIO_ERR_SUCCESS)
				{
				PRINT_ALWAYS "\n* Error: CUsbIo::ClassOrVendorOutRequest "); 
				ShowErrorText(dwRC);
				}
			}

		testParams.host.signalNextSettingThread = (testParams.settingRepeat && threadIndex != lastSettingThreadIndex);
		switch (gTransferMode)
			{
			case EXml:
				break;

			case ELoop:
			case ECompare:
				if (gDeviceIdleCount < 0)
					{
					ReceiveCounter();
					}
				AfxBeginThread (LoopTransfer, &testParams);
				break;

			case EReceiveOnly:
			case EFileReceive:
				if (gDeviceIdleCount < 0 && gTransferMode != EFileReceive)
					{
					ReceiveCounter();
					}
				AfxBeginThread (ReceiveOnlyTransfer, &testParams);
				break;

			case ETransmitOnly:
			case EFileTransmit:
				if (gDeviceIdleCount < 0)
					{
					ReceiveCounter();
					}
				AfxBeginThread (TransmitOnlyTransfer, &testParams);
				break;

			case EConnect:
				// ensure that connect and disconnect events are reset
				ResetEvent (gDeviceConnectEvent);
				ResetEvent (gDeviceDisconnectEvent);
				request.Request = ETestConnect;
				request.Value = (USHORT)testParams.minSize;
				dataBytes = 0;
				dwRC = gUsbDev.ClassOrVendorOutRequest(NULL,dataBytes,&request);
				if (dwRC != USBIO_ERR_SUCCESS)
					{
					PRINT_ALWAYS "\n* Error: CUsbIo::ClassOrVendorOutRequest "); 
					ShowErrorText(dwRC);
					}
				break;

			case EDisconnect:
				if (gUsbDev.IsOpen())
					{
					CloseUsbDevice();
					}
				break;

			case EComment:
				break;

			case EWait:
				if (testParams.host.index > 0)
					{
					PRINT_ALWAYS "Waiting for %d tests to complete\n",testParams.host.index); 
					dwRC = WaitForMultipleObjects (testParams.host.index,gThreadEndedEvents,TRUE,testParams.minSize);
					if (dwRC == WAIT_TIMEOUT)
						{
						PRINT_ALWAYS "Timeout on waiting for test completion\n");
						bool anyTestNotCompleted = false;
						for (int i = 0; i < testParams.host.index; i++)
							{
							dwRC = WaitForSingleObject(gThreadEndedEvents[i],0);
							if (dwRC != WAIT_OBJECT_0)
								{
								anyTestNotCompleted = true;
								PRINT_ALWAYS "Thread %d not completed\n",i);
								}
							}
						if (anyTestNotCompleted)
							{
							gFinishAllTests = true;
							dwRC = WaitForMultipleObjects (testParams.host.index,gThreadEndedEvents,TRUE,INFINITE);
							gFinishAllTests = false;
							gAbort = true;
							}
						}
					if (dwRC < WAIT_OBJECT_0 || dwRC >= (WAIT_OBJECT_0 + testParams.host.index))
						{
						PRINT_ALWAYS "Error waiting for all threads to complete"NL); 
						gAbort = true;
						gFinishAllTests = true;
						}
					ReceiveCounter();
					gDeviceIdleCount = -1;
					PRINT_TIME "Test Group Complete  "); 
					}
				threadIndex = 0;
				firstSetting = true;
				break;

			case EFail:
				request.Request = ETestFail;
				dataBytes = 0;
				dwRC = gUsbDev.ClassOrVendorOutRequest(NULL,dataBytes,&request);
				if (dwRC != USBIO_ERR_SUCCESS)
					{
					PRINT_ALWAYS "\n* Error: CUsbIo::ClassOrVendorOutRequest "); 
					ShowErrorText(dwRC);
					}
				gAbort = true;
				break;

			case ESwitch:
				PRINT_NOLOG "Switching USB to %d"NL,testParams.minSize); 
				dwRC = OpenSwitchDevice ();
				if (dwRC == USBIO_ERR_SUCCESS)
					{
					dwRC = GetConfigurationDescriptor();		
					}
				if (dwRC == USBIO_ERR_SUCCESS)
					{
					// Delay to allow the switch to take place
					Delay (BEFORE_SWITCH_DELAY);

					request.Recipient = RecipientInterface;
					request.Request = SWITCH_REQUEST;
					request.Index = SWITCH_INDEX;
					request.Value = SWITCH_VALUE;
					BYTE switchData[] = {SWITCH_DATA0,((BYTE)testParams.minSize << 4) | SWITCH_DATA1};
					DWORD dataBytes = sizeof(switchData);
					dwRC = gUsbDev.ClassOrVendorOutRequest(&switchData[0],dataBytes,&request);
					request.Recipient = RecipientDevice;					
					CloseUsbDevice();

					// Delay to allow the switch to take place
					Delay (AFTER_SWITCH_DELAY);
					}
				if (dwRC != USBIO_ERR_SUCCESS)
					{
					PRINT_ALWAYS "\n* Error: GetConfigurationDescriptor or CUsbIo::ClassOrVendorOutRequest "); 				
					gAbort = true;
					}
				break;

			case EWaitConnect:
				PRINT_NOLOG "Waiting for device connection"NL); 
				waitConnectTime = 0;
				while (!gAbort && !gUsbDev.IsOpen())
					{
					ResetEvent (gDeviceConnectEvent);
					ResetEvent (gDeviceDisconnectEvent);
					dwRC = OpenUsbDevice();
					if (dwRC == USBIO_ERR_SUCCESS)
						{
						if (!GetDeviceInfo())
							{
							dwRC = USBIO_ERR_FAILED;
							PRINT_ALWAYS "\n* Error: GetDeviceInfo"NL);
							gAbort = TRUE;
							hostTestFail = FALSE;
							}
						}
					else
						{
						dwRC = WaitForSingleObject (gDeviceConnectEvent,CONNECT_RESCAN_TIMEOUT);
						if ((dwRC == WAIT_ABANDONED) || (dwRC == WAIT_FAILED))
							{
							PRINT_ALWAYS "Error waiting for device connection"NL); 
							gAbort = TRUE;
							hostTestFail = FALSE;
							}
						if (dwRC == WAIT_TIMEOUT)
							{
							waitConnectTime += CONNECT_RESCAN_TIMEOUT;
							if (testParams.minSize && waitConnectTime >= testParams.minSize)
								{
								PRINT_ALWAYS "Timeout for device connection"NL); 
								gAbort = TRUE;
								hostTestFail = FALSE;
								}
							else
								{
							    ZeroMemory( &si, sizeof(si) );
								si.cb = sizeof(si);
								ZeroMemory( &pi, sizeof(pi) );

								dwRC = CreateProcess (RESCAN_APPNAME,RESCAN_CMDLINE,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);
								if (dwRC < 0)
									PRINT_ALWAYS "Error %d running Devcon Rescan"NL,dwRC); 
								}
							}
						}
					}
				if (hostTestFail)
					{
					// try to send a test fail to the device
					request.Request = ETestResult;
					request.Index = HOST_ERROR_INDEX;
					request.Value = false;
					dataBytes = 0;
					dwRC = gUsbDev.ClassOrVendorOutRequest(NULL,dataBytes,&request);
					if (dwRC != USBIO_ERR_SUCCESS)
						{
						PRINT_ALWAYS "\n* Error: CUsbIo::ClassOrVendorOutRequest "); 
						ShowErrorText(dwRC);
						}
					gAbort = true;
					}
				break;
			
			case EWaitDisconnect:
				PRINT_NOLOG "Waiting for device disconnection"NL); 
				dwRC = WaitForSingleObject (gDeviceDisconnectEvent,testParams.minSize);
				if (dwRC != WAIT_OBJECT_0)
					{
					PRINT_ALWAYS "Error waiting for device disconnection"NL); 
					gAbort = true;
					}
				// ensure that connect and disconnect events are reset
				ResetEvent (gDeviceConnectEvent);
				ResetEvent (gDeviceDisconnectEvent);
				break;

			case EDo:
				repeatPosition = ftell(inStream);
				repeatCount = 0;
				PRINT_IF_VERBOSE "Repeating from file position %ld"NL,repeatPosition); 
				break;

			case ERepeat:
				if (repeatPosition <= 0)
					PRINT_ALWAYS "No REPEAT in file"NL);
				else
					{
					if (repeatCount++ < (long)testParams.minSize)
						{
						if (!fseek( inStream, repeatPosition, SEEK_SET))
							PRINT_ALWAYS "Repeat Iteration %d"NL,repeatCount);
						else
							PRINT_ALWAYS "ERROR: Repeat Iteration %d"NL,repeatCount);
						}
					}
				break;

			case EUsbcv:
				CloseUsbDevice();
				PRINT_TIME "USBCV Test Started  ");
				usbcv = new UsbcvControl();
				dwRC = usbcv->Create();
				if (dwRC == USBIO_ERR_SUCCESS)
					{
					dwRC = usbcv->TestAllDevices(vendorId,productId, productIdMS);
					}
				delete usbcv;
				if (dwRC == USBIO_ERR_SUCCESS)
					{
					PRINT_TIME "USBCV Test Complete - %s  ",gAbort ? "Failed" : "Passed");
					}
				else
					{
					gAbort = true;
					PRINT_ALWAYS "Error in Running USBCV\n");
					}
				if (gAbort)
					{
					// allow further tests to carry on until waitConnect
					hostTestFail = true;
					gAbort = false;
					}
				// ensure that connect and disconnect events are reset
				ResetEvent (gDeviceConnectEvent);
				ResetEvent (gDeviceDisconnectEvent);
				break;

			case EMassStorage:
				mstoreDriveReady = false;
				ResetEvent (gDeviceConnectEvent);
				ResetEvent (gDeviceDisconnectEvent);
				PRINT_TIME "Mass Storage Test Started  ");
				request.Request = ETestMassStorage;
				request.Value = (USHORT)testParams.minSize;			// product id
				productIdMS = (USHORT)testParams.minSize;
				dataBytes = 0;
				dwRC = gUsbDev.ClassOrVendorOutRequest(NULL,dataBytes,&request);
				if (dwRC != USBIO_ERR_SUCCESS)
					{
					PRINT_ALWAYS "\n* Error: CUsbIo::ClassOrVendorOutRequest "); 
					ShowErrorText(dwRC);
					}
				CloseUsbDevice();
				if (gAbort)
					{
					// allow further tests to carry on until waitConnect
					hostTestFail = true;
					gAbort = false;
					}
				break;

			case EEject:
				if (mstoreDriveReady)
					{
					PRINT_TIME "Mass Storage Test Completed  ");
					if (strlen(line) == 0)
						gAbort = !EjectVolume(' ');
					else
						gAbort = !EjectVolume(line[0]);
					PRINT_TIME "Mass Storage Test Complete - %s  ",gAbort || hostTestFail ? "Failed" : "Passed");
					if (gAbort)
						{
						// allow further tests to carry on until waitConnect
						hostTestFail = true;
						gAbort = false;
						}
					}
				break;

			case EPerl:
				if (mstoreDriveReady)
					{
					PRINT_TIME "Starting Perl Script %s ",line);
					if (strlen(gLogfileName))
						{
						strcat (line," --logfile=");
						strcat (line,gLogfileName);
						fclose (logStream);
						}
					dwRC = PerlScript (line);
					if (strlen(gLogfileName))
						{
						logStream = fopen (gLogfileName, "a+");
						}
					PRINT_TIME "Perl Script Complete - %s  ",dwRC == 0 ? "Passed" : "Failed");
					if (dwRC != 0 || gAbort)
						{
						ShowErrorText (dwRC);
						// allow further tests to carry on until waitConnect
						hostTestFail = true;
						gAbort = false;
						}
					}
				break;

			case EWaitDrive:
				if (!hostTestFail)
					{
					PRINT_TIME "Waiting for removable drive - %c  ",line[0]);
					gAbort = !WaitDriveReady(line[0]);
					PRINT_TIME "Waiting completed - %s  ",gAbort ? "Failed" : "Successfully");
					if (gAbort)
						{
						// allow further tests to carry on until waitConnect
						hostTestFail = true;
						gAbort = false;
						}
					else
						{
						mstoreDriveReady = true;
						}
					}
				break;

			default:
				gAbort = true;
				break;
			}
		if (!gAbort && gTransferMode < EComment)
			{
			dwRC = WaitForSingleObject (gThreadStartedEvent,1500);
			if (dwRC != WAIT_OBJECT_0)
				{
				PRINT_ALWAYS "Error waiting for thread to become ready"NL); 
				gAbort = true;
				}
			threadIndex++;
			dwRC = ResetEvent (gThreadStartedEvent);
			}
		if (!gAbort)
			{
			if( fgets( line, 100, inStream ) != NULL)
				{
				gTransferMode = ParseTestLine (line,&testParams,&request);
				}
			}
		}

	if (gAbort)
		{
		PRINT_TIME "Tests Not Completed - Failed ");
		}
	else
		{
		request.Request = EStop;
		request.Value = (logStream == NULL) ? false : true;
		dataBytes = 0;
		dwRC = gUsbDev.ClassOrVendorOutRequest(NULL,dataBytes,&request);
		if (dwRC != USBIO_ERR_SUCCESS)
			{
			PRINT_ALWAYS "\n* Error: CUsbIo::ClassOrVendorOutRequest "); 
			ShowErrorText(dwRC);
			}

		PRINT_TIME "Tests Completed - All Passed  ");
		}


	PRINT_ALWAYS NL);

	if (logStream != NULL)
		{
		fclose (logStream);
		if (!gAbort)
			{
			SendLogFile();
			}
		}

	PRINT_NOLOG "Waiting for device disconnection"NL); 
	dwRC = WaitForSingleObject (gDeviceDisconnectEvent,WAITDISCONNECT_TIMEOUT);
	if (dwRC != WAIT_OBJECT_0 && !gAbort)
		{
		PRINT_ALWAYS "Error waiting for device disconnection"NL); 
		}

	CloseUsbDevice();

	if (!gKeepOpen)
		{
		ExitProcess(0);
		}

	return TRUE;
	}

// --eof
