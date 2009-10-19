// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This is a header file included in test application. It defines a 
// literal descriptorS initiatized with some test data. This data is
// used to transmit to UART. The size of  KTestTxDataLarge is 
// approximately 3000 bytes. 
// 
//
 
_LIT8(KTestTxDataZero, "");
_LIT8(KTestTxDataSmall, "<< TEST DATA FOR TUTORIAL DEVICE DRIVER >>");
_LIT8(KTestTxDataMedium,"<< TEST DATA FOR TUTORIAL DEVICE DRIVER - PHASE(2):INTERRUPTS AND DFC>>");
_LIT8(KTestTxDataLarge,

"DEVICE DRIVER GUIDE"\
"--------------------------------------------------------------------------------"\
""\
" » Device Driver Guide » Symbian OS device driver model"\
" "\
""\
"--------------------------------------------------------------------------------"\
""\
"Symbian OS device driver model"\
"Device drivers are DLLs that allow code running in Symbian OS to communicate "\
"with hardware the Variant or kernel extensions."\
""\
"Device driver DLLs are loaded into the kernel process after the kernel has "\
"booted.They live on the Kernel side and use the kernel heap. They may be "\
"execute-in-place (XIP) or loaded into RAM."\
""\
"User side code accesses a device driver through a specific API provided by the "\
"Kernel. This is the RBusLogicalChannel class. This provides functions that are "\
"used to open a channel to a device driver and to make requests.These functions "\
"are protected, and the device driver author provides a class derived from "\
"RBusLogicalChannel that implements functions that are specific to the device "\
"driver."\
""\
"The LDD/PDD model"\
""\
"Device driver structure"\
""\
""\
"The LDD factory"\
""\
"The PDD factory"\
""\
"The logical channel"\
""\
"The physical channel"\
""\
"The running model"\
""\
"Synchronous request"\
""\
"Asynchronous request"\
""\
"Kernel side implementation"\
""\
"What the LDD DLL implements"\
""\
"What the PDD DLL implements"\
""\
"The user side API"\
""\
"--------------------------------------------------------------------------------"\
""\
"The LDD/PDD model"\
"Device driver DLLs come in two types - the logical device driver (LDD), and "\
"the physical device driver (PDD)."\
""\
"Where a device driver performs a generic task, it is desirable to make as much "\
"code as possible common to all the device driver variants and to use the same "\
"user side API. This is the role of the LDD. Typically, a single LDD supports "\
"functionality common to a class of hardware devices, whereas a PDD supports a "\
"specific member of that class."\
""\
"Many PDDs may be associated with a single LDD. For example, there is a single "\
"serial communications LDD (ECOMM.LDD) which is used with all UARTs. This LDD "\
"provides buffering and flow control functions that are required with all types "\
"of UART. On a particular hardware platform, this LDD will be accompanied by "\
"one or more PDDs, which support the different types of UART present. A single "\
"PDD can support more than one UART of the same type; separate PDDs are only "\
"required for UARTs with different programming interfaces."\
""\
"Only LDDs communicate with user side code; PDDs communicate only with the "\
"corresponding LDD, with the variant or extensions, and with the hardware "\
"itself.The following diagram shows the general idea"\
""\
"Typically, the PDD is kept as thin as possible"\
""\
"Depending on the device or the type of device to be accessed, this split "\
"between LDD and PDD may not be necessary; the device driver can then simply "\
"consist of an LDD alone."
""
""\
""\
"--------------------------------------------------------------------------------"\
""\
"Device driver structure"\
"Both LDDs and PDDs are DLLs."\
""\
"Device driver DLLs conform to a specific interface which provides a means for "\
"their initialisation and for user side code to communicate with them. They "\
"have a DLL entry point which is normally used only to run constructors (and "\
"possibly destructors) for global C++ objects in the DLL. It is possible that a "\
"device driver is also an extension, in which case the entry point will also be "\
"used for extension initialisation"\
""\
"An LDD must contain the standard declaration DECLARE_STANDARD_LDD(), while a "\
"PDD must contain the standard declaration DECLARE_STANDARD_PDD(). If the DLLs "\
"are also intended to be kernel extensions,then the standard declarations used "\
"are DECLARE_EXTENSION_LDD() and DECLARE_EXTENSION_PDD() respectively."\
""\
"Both LDD and PDD DLLs contain an exported function at ordinal 1 whose purpose "\
"is to create what is called a factory object, and return a pointer to it. "\
"These factory objects are then responible for creating a logical channel and, "\
"if appropriate, a physical channel."\
""\
"We refer to these factory objects as the LDD factory and the PDD factory."\
);
	
//
//  End of t_exint_testdata

