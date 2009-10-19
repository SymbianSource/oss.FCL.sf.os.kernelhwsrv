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
// e32\include\e32capability.h
// Platform security capability definitions
// Used by both source code and tools
// This file can be directly included into C++ tools such as ROMBUILD to allow
// capabilities to be specified by name. To do the same for MAKMAKE (in MMP
// files) some perl code will be needed to parse this file and extract the
// capability names and values.
// 
//

#ifndef __E32CAPABILITY_H__
#define __E32CAPABILITY_H__

/**
An enumeration that defines the set of all supported capabilities.

@publishedAll
@released
*/
enum TCapability
	{
	/**
	Grants write access to executables and shared read-only resources.
	
	This is the most critical capability as it grants access to executables and
	therefore to their capabilities. It also grants write access to
	the /sys and /resource directories.
	*/
	ECapabilityTCB				= 0,
	
	
	/**
	Grants direct access to all communication device drivers. This includes:
	the EComm, Ethernet, USB device drivers etc.
	*/
	ECapabilityCommDD			= 1,
	
	
	/**
	Grants the right:
	
	- to kill any process in the system
	- to power off unused peripherals
	- to switch the machine into standby state
	- to wake the machine up
	- to power the machine down completely.
	
	Note that this does not control access to anything and everything
	that might drain battery power.
	*/
	ECapabilityPowerMgmt		= 2,


    /**
    Grants direct access to all multimedia device drivers.
    
    This includes the sound, camera, video device drivers etc.
    */
	ECapabilityMultimediaDD		= 3,


    /**
    Grants read access to network operator, phone manufacturer and device
    confidential settings or data.
    
    For example, the pin lock code, the list of applications that are installed.
    */
	ECapabilityReadDeviceData	= 4,
	
	
    /**
    Grants write access to settings that control the behaviour of the device.
    
    For example, device lock settings, system time, time zone, alarms, etc.
    */	
	ECapabilityWriteDeviceData	= 5,


    /**
    Grants access to protected content.
    
    DRM (Digital Rights Management) agents use this capability to decide whether
    or not an application should have access to DRM content. 
    Applications granted DRM are trusted to respect the rights associated
    with the content.
    */	
	ECapabilityDRM				= 6,


    /**
    Grants the right to create a trusted UI session, and therefore to display
    dialogs in a secure UI environment.
    
    Trusted UI dialogs are rare. They must be used only when confidentiality
    and security are critical; for example, for password dialogs. 
    
    Normal access to the user interface and the screen does not require
    this capability.
    */	
	ECapabilityTrustedUI		= 7,


    /**
    Grants the right to a server to register with a protected name.
    
    Currently, protected names start with a "!" character. The kernel prevents
    servers without this capability from using such a name, and therefore
    prevents protected servers from being impersonated.
    */	
	ECapabilityProtServ			= 8,


    /**
    Grants access to disk administration operations that affect more than one
    file or one directory (or overall filesystem integrity/behaviour, etc).
    
    For examples, reformatting a disk partition.
    */	
	ECapabilityDiskAdmin		= 9,


    /**
    Grants the right to modify or access network protocol controls.
    
    Typically when an action can change the behaviour of all existing and
    future connections, it should be protected by this capability.
    
    For example, forcing all existing connections on a specific protocol
    to be dropped, or changing the priority of a call.
    */	
	ECapabilityNetworkControl	= 10,


    /**
    Grants read access to the entire file system; grants write access to
    the private directories of other processes.
    
    This capability is very strictly controlled and should rarely be granted.
    */	
	ECapabilityAllFiles			= 11,


    /**
    Grants the right to generate software key & pen events, and to capture any
    of them regardless of the status of the application.
    
    Note that after obtaining the focus, normal applications do not need this
    capability to be dispatched key and pen events.
    */	
	ECapabilitySwEvent			= 12,


    /**
    A user capability that grants access to remote services without any
    restriction on its physical location.
    
    Typically, such a location is unknown to the phone user, and such services
    may incur cost for the phone user.
    
    Voice calls, SMS, and internet services are good examples of
    such network services. They are supported by GSM, CDMA and all IP transport
    protocols including Bluetooth profiles over IP.
    */	
	ECapabilityNetworkServices	= 13,


    /**
    A user capability that grants access to remote services in the close
    vicinity of the phone.
    
    The location of the remote service is well-known to the phone user, and in
    most cases, such services will not incur cost for the phone user.
    */	
	ECapabilityLocalServices	= 14,


    /**
    A user capability that grants read access to data that is confidential to
    the phone user. 
    
    This capability supports the management of the user's privacy.
    
    Typically, contacts, messages and appointments are always seen user
    confidential data.
    */	
	ECapabilityReadUserData		= 15,


    /**
    A user capability that grants write access to user data. 
    
    This capability supports the management of the integrity of user data.
    
    Note that this capability is not symmetric with the ECapabilityReadUserData
    capability. For example, you may want to prevent rogue applications from
    deleting music tracks but you may not want to restrict read access to them.
    */	
    ECapabilityWriteUserData	= 16,
    
    
    /**
    A user capability that grants access to the location of the device.
    
    This capability supports the management of the user's privacy with regard
    to the phone location.
    */
	ECapabilityLocation			= 17,


	/**
	Grants access to logical device drivers that provide input information about
	the surroundings of the device. 

	Good examples of drivers that require this capability would be GPS and biometrics
	device drivers. For complex multimedia logical device drivers that provide both
	input and output functions, such as Sound device driver, the  MultimediaDD
	capability should be used if it is too difficult to separate the input from the
	output calls at its API level.
	*/
	ECapabilitySurroundingsDD	= 18,


	/**
	Grants access to live confidential information about the user and his/her
	immediate environment. This capability protect the user's privacy.

	Examples are audio, picture and video recording, biometrics (such as blood
	pressure) recording.

	Please note that the location of the device is excluded from this capability.
	The protection of this is achieved by using the dedicated capability Location
	*/
	ECapabilityUserEnvironment	= 19,


	ECapability_Limit,					/**< @internalTechnology */

	ECapability_HardLimit		= 255,	/**< @internalTechnology */

	ECapability_None			= -1,	/**< Special value used to specify 'do not care' or 'no capability'.*/

	ECapability_Denied			= -2	/**< Special value used to indicate a capability that is never granted. */	
	};


/** Define this macro to reference the names of the capabilities. This is here so
	that ROMBUILD can accept capability names.
*/
#ifdef __REFERENCE_CAPABILITY_NAMES__

extern const char* const CapabilityNames[ECapability_Limit];

#endif	// __REFERENCE_CAPABILITY_NAMES__

/** Define this macro to include the names of the capabilities. This is here so
	that ROMBUILD can accept capability names.
*/
#ifdef __INCLUDE_CAPABILITY_NAMES__

/** List of names of all supported capabilities
	Must be in the same order as the enumerators in TCapability

@publishedAll
@released
*/
extern const char* const CapabilityNames[ECapability_Limit] =
	{
	"TCB",
	"CommDD",
	"PowerMgmt",
	"MultimediaDD",
	"ReadDeviceData",
	"WriteDeviceData",
	"DRM",
	"TrustedUI",
	"ProtServ",
	"DiskAdmin",
	"NetworkControl",
	"AllFiles",
	"SwEvent",
	"NetworkServices",
	"LocalServices",
	"ReadUserData",
	"WriteUserData",
	"Location",
	"SurroundingsDD",
	"UserEnvironment"
	};

#endif	// __INCLUDE_CAPABILITY_NAMES__

#endif	// __E32CAPABILITY_H__
