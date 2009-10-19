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
//
//

#ifndef __global_H__
#define __global_H__


#define WM_USER_PRINTOUT           (WM_USER+100)

// The version of this program
#define VERSION_MAJOR  1
#define VERSION_MINOR  2
#define VERSION_MICRO  0

// The version of the USBIO driver this program is compiled against
#define USBIO_VERSION_MAJOR  2
#define USBIO_VERSION_MINOR  41

#define VERSION_DATAOUT_SIZE 5
#define VERSION_DATAIN_SIZE 96

#define MAX_DESCRIPTOR_BUFFER_SIZE 2047

#define MAX_INTERFACES 128
#define MAX_SETTINGS 10
#define MAX_ENDPOINTS 5

#define MAX_THREADS 10

#define WAIT_SETTING_BUFFER_SIZE 8
#define WAIT_SETTING_TIMEOUT 250
#define WAIT_BEFORETHREAD_TIMEOUT 30000
#define READWRITE_TIMEOUT 10000
#define FIRSTFILEREAD_TIMEOUT 500000
#define WAITDISCONNECT_TIMEOUT 10000

#define FIRST_SETTING_THREAD_MASK 0x8000
#define LAST_SETTING_THREAD_MASK 0x4000

#define HOST_ERROR_INDEX 0xFFF

// a number of definitions for using the Aten USB switch 
#define SWITCH_REQUEST 0x09
#define SWITCH_INDEX 0x0001
#define SWITCH_VALUE 0x0202
#define SWITCH_DATA0 0x02
#define SWITCH_DATA1 0x03

// delays
#define WAIT_TEST_COMPLETE 50
#define WAIT_SETTING_DELAY 50
#define BEFORE_SWITCH_DELAY 3000
#define AFTER_SWITCH_DELAY 5000
#define DEVICE_CONFIG_DELAY 2000

#define NL "\r\n"

// Helper #defines for print messages
#define PRINT_ALWAYS PrintOut (TRUE,TRUE,FALSE,
#define PRINT_TIME PrintOut (TRUE,TRUE,TRUE,
#define PRINT_NOLOG PrintOut (TRUE,FALSE,FALSE,
#define PRINT_IF_VERBOSE PrintOut (gVerboseMode,gVerboseMode,FALSE,

#endif //__global_H__
