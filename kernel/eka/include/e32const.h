// Copyright (c) 1994-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// e32\include\e32const.h
// 
//

#ifndef __E32CONST_H__
#define __E32CONST_H__

#include <e32err.h>
#include <e32lang.h>
#include <e32reg.h>


/**
@publishedAll
@released

The default width value used when appending and justifying data
in a descriptor.

@see TDes16::AppendJustify()
@see TDes16::Justify()
@see TDes8::AppendJustify()
@see TDes8::Justify()
*/
const TInt KDefaultJustifyWidth=(-1);



/**
@publishedAll
@released

Defines the number of TUids that form a TUidType.

@see TUid
@see TUidType
*/
const TInt KMaxCheckedUid=3;


/**
@publishedAll
@released

Defines the number of 32-bit debug trace mask words.

*/
const TInt KNumTraceMaskWords = 8;


/**
@publishedAll
@released

Defines the maximum length for the text form of a UID name.

@see TUidName
@see TUid::Name()
*/
const TInt KMaxUidName=10;




/**
@publishedAll
@released

Defines the maximum length of a module name.
*/
const TInt KMaxModuleVersionName=10;




/**
@publishedAll
@released

Defines the maximum length of the name of a reference counted object.

@see TName
*/
const TInt KMaxName=0x80;




/**
@publishedAll
@released

Defines the maximum length of the name of a reference counted kernel-side object. 
*/
const TInt KMaxKernelName=0x50;




/**
@publishedAll
@released

Defines the maximum size of a process name.
*/
const TInt KMaxProcessName=(KMaxKernelName-KMaxUidName-KMaxModuleVersionName-4);




/**
@publishedAll
@released

Defines the maximum size of a library name.
*/
const TInt KMaxLibraryName=(KMaxKernelName-KMaxUidName-KMaxModuleVersionName);




/**
@publishedAll
@released

Defines the maximum length of a TInfoName object.
*/
const TInt KMaxInfoName=0x10;




/**
@publishedAll
@released

Defines the maximum length of the full name of a reference counted object.

@see TFullName
*/
const TInt KMaxFullName=(KMaxName<<1);




/**
@publishedAll
@released

The maximum length for a password buffer.

@see TPassword
*/
const TInt KMaxPassword=0x10;




/**
@publishedAll
@released

The maximum length of a category name.

@see TExitCategoryName
*/
const TInt KMaxExitCategoryName=0x10;




/**
@publishedAll
@released

The maximum length of the full text name for a day of the week.

@see TDayName
*/
const TInt KMaxDayName=0x20;




/**
@publishedAll
@released

The maximum length of the abbreviated text name for a day of the week.

@see TDayNameAbb
*/
const TInt KMaxDayNameAbb=0x08;




/**
@publishedAll
@released

Defines the maximum size of arrays or data structures required
to hold the names of the days of the week.
*/
const TInt KMaxDays=7;




/**
@publishedAll
@released

The maximum length of the full text name for a month.

@see TDayName
*/
const TInt KMaxMonthName=0x20;




/**
@publishedAll
@released

The maximum length of the abbreviated text name for a month.

@see TMonthNameAbb
*/
const TInt KMaxMonthNameAbb=0x08;




/**
@publishedAll
@released

Defines the maximum size of arrays or data structures required
to hold the names of the months of the year.
*/
const TInt KMaxMonths=12;




/**
@publishedAll
@released

The maximum length of the text for a date suffix.

@see TDateSuffix
*/
const TInt KMaxSuffix=0x04;




/**
@publishedAll
@released

Defines the maximum size of arrays and data structures required
to hold date suffix strings.
*/
const TInt KMaxSuffixes=31;




/**
@publishedAll
@released

The maximum length of the text for AM and PM.

@see TAmPmName
*/
const TInt KMaxAmPmName=0x04;




/**
@publishedAll
@released

Defines the maximum size of arrays and data structures required
to hold am/pm strings.
*/
const TInt KMaxAmPms=2;




/**
@publishedAll
@released

Defines the maximum number of date separator characters in a date string.
*/
const TInt KMaxDateSeparators=4;




/**
@publishedAll
@released

Defines the maximum number of time separator characters in a time string.
*/
const TInt KMaxTimeSeparators=4;




/**
@publishedAll
@released

Defines the maximum size of data structures to hold the translate tables
for Western European alphabetic conversions.
*/
const TInt KMaxTranslateTable=0x100;




/**
@publishedAll
@released

The maximum length of the text for a currency symbol.

@see TCurrencySymbol
*/
const TInt KMaxCurrencySymbol=0x08;




/**
@publishedAll
@released

The maximum length of the short date format specification text.

@see TShortDateFormatSpec
*/
const TInt KMaxShortDateFormatSpec=40;




/**
@publishedAll
@released

The maximum length of the short date format specification text.

@see TLongDateFormatSpec
*/
const TInt KMaxLongDateFormatSpec=80;




/**
@publishedAll
@released

The maximum length of the time string formatting commands.

@see TTimeFormatSpec
*/
const TInt KMaxTimeFormatSpec=60;




/**
@publishedAll
@released

Defines the maximum length of a filename.
*/
const TInt KMaxFileName=0x100;




/**
@publishedAll
@released

The maximum length of the character representation of version information.

@see TVersion::Name()
*/
const TInt KMaxVersionName=0x10;




/**
@publishedAll
@released

Defines the maximum length of a path.

@see TPath
*/
const TInt KMaxPath=0x100;




/**
@publishedAll
@released

Defines the maximum length of a TDeviceInfo object.

@see TDeviceInfo
*/
const TInt KMaxDeviceInfo=0x80;




/**
@publishedAll
@released

The maximum size of the password required to unlock a media drive.
*/
const TInt KMaxMediaPassword=16;




/**
@publishedAll
@released

Defines the minimum size of a new heap.

Functions that require a new heap to be allocated will either panic,
or will reset the required heap size to this value if a smaller heap
size is specified.

@see UserHeap
@see RThread::Create()
*/
const TInt KMinHeapSize=0x100;




/**
@publishedAll
@released

Not used by Symbian OS.
*/
const TInt KDstHome=0x01;




/**
@publishedAll
@released

Not used by Symbian OS.
*/
const TInt KDstEuropean=0x02;




/**
@publishedAll
@released

Not used by Symbian OS.
*/
const TInt KDstNorthern=0x04;




/**
@publishedAll
@released

Not used by Symbian OS.
*/
const TInt KDstSouthern=0x08;




/**
@publishedAll
@released

A default stack size that can be used when creating threads.
*/
#ifdef __X86GCC__
const TInt KDefaultStackSize=0x4000;
#else
const TInt KDefaultStackSize=0x2000;
#endif // __X86GCC__




/**
@publishedAll
@released

Indicates an undefined character, used internally when formatting text. 
*/
const TUint KNoChar=0xffffffffu;




/**
@publishedAll
@released

Defines an index value that is interpreted by the TKey class,
and derived classes, as having a specific meaning.

@see TKey::SetPtr()
@see TKey::At()
*/
const TInt KIndexPtr=(-1);




/**
@publishedAll
@released

A flag used by the kernel to mark a handle as not being closable.
*/
const TInt KHandleNoClose=0x00008000;




/**
@publishedAll
@released

A flag used by the kernel to mark a handle as being local.
*/
const TInt KHandleFlagLocal=0x40000000;




/**
@publishedAll
@released

A flag used by the Kernel to indicate the current process.
*/
const TInt KCurrentProcessHandle=0xffff0000|KHandleNoClose;




/**
@publishedAll
@released

A flag used by the Kernel to indicate the current thread.
*/
const TInt KCurrentThreadHandle=0xffff0001|KHandleNoClose;




/**
@publishedAll
@released

Defines a handle number value of zero.

@see RHandleBase
*/
const TInt KNullHandle=0;




/**
@publishedAll
@released

Defines a default unit. Not generally used by Symbian OS.
*/
const TInt KDefaultUnit=0x00;




/**
@publishedAll
@released

The device unit that must be passed in a call
to RBusLogicalChannel::DoCreate(), if units are not permitted.

@see RBusLogicalChannel
*/
const TInt KNullUnit=0xffffffff;




/**
@publishedAll
@released

The maximum unit number that can be passed in a call
to RBusLogicalChannel::DoCreate().

@see RBusLogicalChannel
*/
const TInt KMaxUnits=0x20;




/**
@publishedAll
@released

Defines the maximum number of message arguments that can be passed
across the user side/kernel side boundary.
*/
const TInt KMaxMessageArguments=0x04;




/**
@publishedAll
@released

The default width of the character representation of a real number, used by
the default constructor of a TRealFormat object and the formatting functions
of descriptors.

@see TRealFormat
@see TDes16::AppendFormat()
@see TDes8::AppendFormat()
*/
const TInt KDefaultRealWidth=20;




/**
@publishedAll
@released

The default value used by UserHeap::ChunkHeap() for defining increments to
the size of a chunk, when no explicit value specified by the caller.

@see UserHeap::ChunkHeap()
*/
const TInt KMinHeapGrowBy=0x1000;




/**
@publishedAll
@released

Not used by Symbian OS.
*/
const TInt KMaxExponentConversion=99;




/**
@publishedAll
@released

Defines a Null UID value.

@see TUid
*/
const TInt KNullUidValue=0;




/**
@publishedAll
@deprecated

The timer granularity used by a CDeltaTimer object is
now the tick period and this constant is obsolete.

@see CDeltaTimer
*/
const TInt KDeltaTimerDefaultGranularity=100000;




/**
@publishedAll
@released

The largest possible value for a TInt8.
*/
const TInt KMaxTInt8=0x7f;




/**
@publishedAll
@released

The smallest possible value for a TInt8.
*/
const TInt KMinTInt8=(-128);




/**
@publishedAll
@released

The largest possible value for a TUint8.
*/
const TUint KMaxTUint8=0xffu;




/**
@publishedAll
@released

The largest possible value for a TInt16.
*/
const TInt KMaxTInt16=0x7fff;




/**
@publishedAll
@released

The smallest possible value for a TInt16.
*/
const TInt KMinTInt16=(-32768);




/**
@publishedAll
@released

The largest possible value for a TUint16.
*/
const TUint KMaxTUint16=0xffffu;




/**
@publishedAll
@released

The largest possible value for a TInt32.
*/
const TInt KMaxTInt32=0x7fffffff;




/**
@publishedAll
@released

The smallest possible value for a TInt32.
*/
const TInt KMinTInt32=(TInt)0x80000000;




/**
@publishedAll
@released

The largest possible value for a TUint32.
*/
const TUint KMaxTUint32=0xffffffffu;




/**
@publishedAll
@released

The largest possible value for a TInt.
*/
const TInt KMaxTInt=0x7fffffff;




/**
@publishedAll
@released

The smallest possible value for a TInt.
*/
const TInt KMinTInt=(TInt)0x80000000;




/**
@publishedAll
@released

The largest possible value for a TUint.
*/
const TUint KMaxTUint=0xffffffffu;




/**
@publishedAll
@released

The largest possible value for a TInt64.
*/
const TInt64 KMaxTInt64 = I64LIT(0x7fffffffffffffff);




/**
@publishedAll
@released

The smallest possible value for a TInt64.
*/
const TInt64 KMinTInt64 = UI64LIT(0x8000000000000000);




/**
@publishedAll
@released

The largest possible value for a TUint64.
*/
const TUint64 KMaxTUint64 = UI64LIT(0xffffffffffffffff);




/**
@publishedAll
@released

Defines the character *, and represents any number of characters in any
part of a path component, filename or extension.

It is used in a TParse file specification.

@see TParse
*/
const TUint KMatchAny='*';




/**
@publishedAll
@released

Defines the character ?, and represents a single character in
a path component, filename or extension.

It is used in a TParse file specification.

@see TParse
*/
const TUint KMatchOne='?';




/**
@publishedAll
@released

Defines the maximum number of local drives.
*/
const TInt KMaxLocalDrives=16;




/**
@publishedAll
@released

Defines the maximum number of peripheral bus sockets.
*/
const TInt KMaxPBusSockets=4;




/**
@publishedAll
@released

Not used by Symbian OS.
*/
const TInt KNoCallEntryPoint = 0x01;




/**
@publishedAll
@released

The value to which CActive::iStatus is set by an active object's
service provider before the service provider initiates an asynchronous request.

@see CActive
*/
const TInt KRequestPending=(-KMaxTInt);




// Drive capabilities

/**
@publishedAll
@released

Defines the possible media types. 
*/
enum TMediaType
	{
	EMediaNotPresent,
	
	EMediaUnknown,
	
	EMediaFloppy,
	
	/** Solid-state media. */
	EMediaHardDisk,
	
	EMediaCdRom,
	
	EMediaRam,
	
	EMediaFlash,
	
	EMediaRom,
	
	EMediaRemote,
	
	EMediaNANDFlash,
	
	/** Rotating media. */
	EMediaRotatingMedia  
	};




/**
@publishedAll
@released

Defines the state of a battery, if supported.
*/
enum TBatteryState {EBatNotSupported,EBatGood,EBatLow};


/**
@publishedAll
@released

Defines the possible connection types used to interface to the media.
*/
enum TConnectionBusType {EConnectionBusInternal, EConnectionBusUsb};


/**
@publishedAll
@released

Drive attribute - drive is local.
*/
const TUint KDriveAttLocal=0x01;




/**
@publishedAll
@released

Drive attribute - ROM drive.
*/
const TUint KDriveAttRom=0x02;




/**
@publishedAll
@released

Drive attribute - output from a process on one drive is redirected
to another drive.
*/
const TUint KDriveAttRedirected=0x04;




/**
@publishedAll
@released

Drive attribute - drive letter has been substituted (assigned a path).
*/
const TUint KDriveAttSubsted=0x08;




/**
@publishedAll
@released

Drive attribute - drive is internal (not removable).
*/
const TUint KDriveAttInternal=0x10;




/**
@publishedAll
@released

Drive attribute - drive is removable.
*/
const TUint KDriveAttRemovable=0x20;




/**
@publishedAll
@released

Drive attribute - drive is remote.
*/
const TUint KDriveAttRemote=0x40;




/**
@publishedAll
@released

Drive attribute -.
*/
const TUint KDriveAttTransaction=0x80;


/**
@publishedAll
@released

Drive attribute - drive is used for paging.
*/
const TUint KDriveAttPageable=0x100;



/**
@publishedAll
@released

Drive attribute - drive is logically removable (can be taken offline from Symbian OS).
If not logically removable then physically removable e.g. a card can be take out.
*/
const TUint KDriveAttLogicallyRemovable=0x200; 


/**
@publishedAll
@released

Drive attribute - drive is hidden.
A drive which has its hidden attribute set would be excluded from the list of available drives.
*/
const TUint KDriveAttHidden=0x400; 


/**
@publishedAll
@released

Drive attribute - drive is external.
*/
const TUint KDriveAttExternal=0x800;


/**
@publishedAll
@released

Drive attribute - It can be set in a search in order to instruct that all drives should be returned.
*/
const TUint KDriveAttAll=0x100000;


/**
@publishedAll
@released

Drive attribute - It can be set in combination with other drive attributes in order to exclude during a drive search, drives with 
these attributes set. 
*/
const TUint KDriveAttExclude=0x40000;



/**
@publishedAll
@released

Drive attribute - It can be set in combination with other drive attributes in order to search and return exclusively drives with these attributes set.
*/
const TUint KDriveAttExclusive=0x80000;



/**
@internalTechnology

Used as a mask in order to extract the actual drive attributes.

*/
const TUint KDriveAttMatchedFlags=0xFFF;



/**
@internalTechnology

Used as a mask in order to extract the extra(ex KDriveAttAll ,KDriveAttExclude, KDriveAttExclusive ,0) drive attributes.
*/
const TUint KDriveAttMatchedAtt=0x0FFF0000;




/**
@publishedAll
@released

Media attribute - the media capacity can change over time.
*/
const TUint KMediaAttVariableSize=0x01;




/**
@publishedAll
@released

Media attribute - media is dual density.
*/
const TUint KMediaAttDualDensity=0x02;




/**
@publishedAll
@released

Media attribute - media is formattable.
*/
const TUint KMediaAttFormattable=0x04;




/**
@publishedAll
@released

Media attribute - media is write-protected.
*/
const TUint KMediaAttWriteProtected=0x08;




/**
@publishedAll
@released

Media attribute - media is lockable; this is provided for
lockable multi-media cards
*/
const TUint KMediaAttLockable=0x10;




/**
@publishedAll
@released

Media attribute - media is locked; this is provided for
lockable multi-media cards
*/
const TUint KMediaAttLocked=0x20;



/**
@publishedAll
@released

Media attribute - media has password.
*/
const TUint KMediaAttHasPassword=0x40;

/**
@publishedAll
@released
*/
const TUint KMediaAttReadWhileWrite=0x80;

/**
@publishedAll
@released

Media attribute - media supports TBusLocalDrive::DeleteNotify()
*/
const TUint KMediaAttDeleteNotify=0x100;

/**
@publishedAll
@released

Media attribute - media supports paging
*/
const TUint KMediaAttPageable=0x200;



/**
@publishedAll
@released

Identifies a FAT file system
*/
const TUint KDriveFileSysFAT=0x01;




/**
@publishedAll
@released

Identifies a ROM file system.
*/
const TUint KDriveFileSysROM=0x02;




/**
@publishedAll
@released

Identifies an LFFS file system.
*/
const TUint KDriveFileSysLFFS=0x03;




/**
@publishedAll
@released

Identifies a read-only file system.
*/
const TUint KDriveFileSysROFS=0x04;




/**
@publishedAll
@released

Identifies a non-file system.  That is a partition without any file system layer.
*/
const TUint KDriveFileNone=0x05;




/**
@publishedAll
@released

An enumerator with a single enumeration value that defines the Boolean value 
false in Symbian OS.

@see TBool
*/
enum TFalse {
            /**
            Defines the value false that is passed to a TBool type.
            */
            EFalse=FALSE
            };
            
            
            
            
/**
@publishedAll
@released

An enumerator with a single enumeration value that defines the Boolean value 
true in Symbian OS.

@see TBool
*/
enum TTrue {
           /**
           Defines the value true that is passed to a TBool type.
           */
           ETrue=TRUE
           };




/**
@publishedAll
@released

Defines flags that can be used to indicate whether duplicates, for example in 
a list, are allowed.
*/
enum TAllowDuplicates {
                      /**
                      No duplicates allowed.
                      */
                      ENoDuplicates,
                      
                      /**
                      Duplicates allowed.
                      */
                      EAllowDuplicates
                      };




/**
@publishedAll
@released

An enumeration whose enumerators determine the number system to be used
when converting numbers into a character format.

@see TDes8::Num()
@see TDes8::NumUC()
@see TDes8::AppendNum()
@see TDes8::AppendNumUC()
@see TDes16::Num()
@see TDes16::NumUC()
@see TDes16::AppendNum()
@see TDes16::AppendNumUC()
*/
enum TRadix {
            /**
            Convert number into binary character representation.
            */
            EBinary=2,
            /**
            Convert number into octal character representation.
            */            
            EOctal=8,
            /**
            Convert number into decimal character representation.
            */            
            EDecimal=10,
          	/**
          	Convert number into hexadecimal character representation.
          	*/            
            EHex=16
            };





/**
@publishedAll
@released

The mask for the dialect bits
*/
const TUint KDialectMask=0x03FF;





/**
@publishedAll
@released

Defines the date formats.
*/
enum TDateFormat {
                 /**
                 US format (mm/dd/yyyy)
                 */
                 EDateAmerican,
                 
                 /**
                 European format (dd/mm/yyyy)
                 */                 
                 EDateEuropean,
                 
                 /**
                 Japanese format (yyyy/mm/dd)
                 */
                 EDateJapanese};




/**
@publishedAll
@released

Defines the time formats as either 12 hour or 24 hour.
*/
enum TTimeFormat {
                 ETime12,
                 ETime24
                 };




/**
@publishedAll
@released

Defines the clock display formats, as either analog or digital.
*/
enum TClockFormat {
                  EClockAnalog,
                  EClockDigital
                  };




/** 
@publishedAll
@released

Enumerates the units of measurement as either Imperial or Metric.
*/
enum TUnitsFormat {
                  EUnitsImperial,
                  EUnitsMetric
                  };




/**
@publishedAll
@released

Identifies a time as being am or pm.
*/
enum TAmPm {
           EAm,
           EPm
           };




/**
@publishedAll
@released

Defines whether:

1. the currency symbol is located before or after the currency amount.

2. the am/pm text is located before or after the time.
*/
enum TLocalePos 
	{
	/**
	The currency symbol is located before the currency amount.
	The am/pm text is located before the time.
	*/
	ELocaleBefore,
	
	/**
	The currency symbol is located after the currency amount.
	The am/pm text is located after the time.
	*/
	ELocaleAfter
	};




/**
@publishedAll
@released

Number Modes available to select.
*/
enum TDigitType
	{
	EDigitTypeUnknown = 0x0000,
	EDigitTypeWestern = 0x0030,
	EDigitTypeArabicIndic = 0x0660,
	EDigitTypeEasternArabicIndic = 0x6F0,
	EDigitTypeDevanagari = 0x0966,
	EDigitTypeBengali = 0x09E6,
	EDigitTypeGurmukhi = 0x0A66,
	EDigitTypeGujarati = 0x0AE6,
	EDigitTypeOriya = 0x0B66,
	EDigitTypeTamil = 0x0BE6,
	EDigitTypeTelugu = 0x0C66,
	EDigitTypeKannada = 0x0CE6,
	EDigitTypeMalayalam = 0x0D66,
	EDigitTypeThai = 0x0E50,
	EDigitTypeLao = 0x0ED0,
	EDigitTypeTibetan = 0x0F20,
	EDigitTypeMayanmar = 0x1040,
	EDigitTypeKhmer = 0x17E0,
	EDigitTypeAllTypes = 0xFFFF
	};




/**
@publishedAll
@released

Defines the daylight saving zones.
*/
enum TDaylightSavingZone
	{
	/**
	The home daylight saving zone. Its value is usually the same as that of the 
	zone in which the home city is located, but may differ. In this case, the 
	value for home overrides the value of the zone in which home is located.
	*/
	EDstHome=0x40000000,
	
	/**
	No daylight saving zone.
	*/
	EDstNone=0,
	
	/**
	The European daylight saving zone.
	*/
	EDstEuropean=1,
	
	/**
	The Northern hemisphere (non-European) daylight saving zone.
	*/
	EDstNorthern=2,
	
	/**
	Southern hemisphere daylight saving zone.
	*/
	EDstSouthern=4
	};




/**
@internalComponent

Indicates how negative currency values are formatted.
*/
enum TNegativeCurrencyFormat // must match TLocale:: version, included here so ELOCL.DLL can see it
	{
	E_NegC_LeadingMinusSign,
	E_NegC_InBrackets,// this one must be non-zero for binary compatibility with the old TBool TLocale::iCurrencyNegativeInBrackets which was exposed in the binary interface because it was accessed via *inline* functions
	E_NegC_InterveningMinusSignWithSpaces,
	E_NegC_InterveningMinusSignWithoutSpaces,
	E_NegC_TrailingMinusSign
	};

/**
@internalComponent


Indicates how the device universal time is maintained
*/
enum TDeviceTimeState // must match TLocale:: version
	{
	/** Universal time is maintained by the device RTC and the user selection 
	of the locale of the device indicating offset from GMT and daylight saving*/
	EDeviceUserTime,

	/** Universal time and offset from GMT is supplied by the mobile network
	and maintained by device RTC */
	ENITZNetworkTimeSync
	};

/**
@internalComponent

Indicates the type of conversion required for FAT filenames
*/
enum TFatFilenameConversionType
	{
	/** Undefined conversion scheme; conversion obtained is whatever the
	default policy is for this version of the OS. */
	EFatConversionDefault = 0,
	/** x-fat<nnn>.dll is loaded, where <nnn> is the FAT filename conversion number. */
	EFatConversionNonStandard = 1,
	/** cp<nnn>.dll is loaded, where <nnn> is the FAT filename conversion number. */
	EFatConversionMicrosoftCodePage = 2
	};


/**
@publishedAll
@released

Defines the days of the week.

The enumerator symbol names correspond with the days of the week,
i.e. EMonday refers to Monday etc.
*/
enum TDay
	{
	EMonday,
	ETuesday,
	EWednesday,
	EThursday,
	EFriday,
	ESaturday,
	ESunday
	};




/**
@publishedAll
@released

Defines the months of the year.

The enumerator symbol names correspond with the months of the year,
i.e. EJanuary refers to January etc.
*/
enum TMonth
	{
	EJanuary,
	EFebruary,
	EMarch,
	EApril,
	EMay,
	EJune,
	EJuly,
	EAugust,
	ESeptember,
	EOctober,
	ENovember,
	EDecember
	};




/**
@publishedAll
@released

Handle ownership flags.

The flags indicate whether a handle being opened is owned by a process or 
a thread.

Ownership by a process means that the handle instance can be used by all
threads in the process to access the Kernel side object that the
handle represents.

Ownership by a thread means that the handle instance can only be used by the 
thread that creates or opens the handle.

An enumerator of this type is passed to all member functions of RHandleBase, 
and classes derived from RHandleBase, which open a handle.
*/
enum TOwnerType {
	             /**
	             Ownership of the handle is to be vested in the process.
	             */
                 EOwnerProcess,
                 
                 /**
                 Ownership of the handle is to be vested in the thread.
                 */
                 EOwnerThread
                };




const TInt KCreateProtectedObject = (TInt)0x80000000;





/**
@publishedAll
@released

Defines process priorities.

The enumerator values are passed to RProcess::SetPriority().

The priorities are listed in relative order stating with the lowest.
*/
enum TProcessPriority
	{
	EPriorityLow=150,
	EPriorityBackground=250,
	EPriorityForeground=350,
	EPriorityHigh=450,
	EPriorityWindowServer=650,
	EPriorityFileServer=750,
	EPriorityRealTimeServer=850,
	EPrioritySupervisor=950
	};




/**
@publishedAll
@released

Defines thread priorities.

The enumerator values are passed to RThread::SetPriority().

The relative priorities are listed in order starting with the lowest.

The absolute thread priorities are listed in order starting with the lowest.
*/
enum TThreadPriority
	{
	EPriorityNull=(-30),
	EPriorityMuchLess=(-20),
	EPriorityLess=(-10),
	EPriorityNormal=0,
	EPriorityMore=10,
	EPriorityMuchMore=20,
	EPriorityRealTime=30,
	EPriorityAbsoluteVeryLow=100,
	EPriorityAbsoluteLowNormal=150,
	EPriorityAbsoluteLow=200,
	EPriorityAbsoluteBackgroundNormal=250,
	EPriorityAbsoluteBackground=300,
	EPriorityAbsoluteForegroundNormal=350,
	EPriorityAbsoluteForeground=400,
	EPriorityAbsoluteHighNormal=450,
	EPriorityAbsoluteHigh=500,
	EPriorityAbsoluteRealTime1=810,
	EPriorityAbsoluteRealTime2=820,
	EPriorityAbsoluteRealTime3=830,
	EPriorityAbsoluteRealTime4=840,
	EPriorityAbsoluteRealTime5=850,
	EPriorityAbsoluteRealTime6=860,
	EPriorityAbsoluteRealTime7=870, 
	EPriorityAbsoluteRealTime8=880
	};


/**
@publishedAll
@released

A list of exception types which a thread's exception handler might deal with.
An enumerator of this type is passed to User::IsExceptionHandled()
and User::RaiseException().

While an exception handler can deal with exceptions as listed in the exception
constants set, a TExcType is simply a type of exception.
For example, EExcIntegerDivideByZero and EExcIntegerOverflow are types
of KExceptionInteger exception.
*/
enum TExcType
	{
	EExcGeneral=0, ///<A miscellaneous exception.
	EExcIntegerDivideByZero=1, ///<An attempt was made to divide an integer by zero.
	EExcSingleStep=2, ///<Raised after executing an instruction, when CPU is in single-step mode.
	EExcBreakPoint=3, ///<A break point was hit.
	EExcIntegerOverflow=4, ///<An integer value overflowed.
	EExcBoundsCheck=5, ///<Access to an out of bounds array element was caught.
	EExcInvalidOpCode=6, ///<The CPU reached an invalid instruction.
	EExcDoubleFault=7, ///<A fault occurred while handling a previous exception or interrupt.
	EExcStackFault=8, ///<The thread ran out of stack space.
	EExcAccessViolation=9, ///<The thread attempted to access memory in an unauthorized area.
	EExcPrivInstruction=10, ///<Attempted to execute an instruction in wrong machine mode.
	EExcAlignment=11, ///<The thread tried to read or write non-aligned data.
	EExcPageFault=12, ///<Thread could not access the memory page requested.
	EExcFloatDenormal=13, ///<An operand in a floating point operation was denormal.
	EExcFloatDivideByZero=14, ///<An attempt was made to divide a floating point number by zero.
	EExcFloatInexactResult=15, ///<The result of a floating point operation could not be represented precisely.
	EExcFloatInvalidOperation=16, ///<The result of a floating point operation was an ill-defined quantity.
	EExcFloatOverflow=17, ///<The result of a floating point operation was too large to be represented.
	EExcFloatStackCheck=18, ///<The result of a floating point operation caused the stack to over or underflow.
	EExcFloatUnderflow=19, ///<The result of a floating point operation was too small to be represented.
	EExcAbort=20, ///<This exception is not raised by the Kernel, though it may be raised by a user thread on itself.
	EExcKill=21, ///<This exception is not raised by the Kernel, though it may be raised by a user thread on itself.
	EExcUserInterrupt=22, ///<May be used to indicate a general exception.
	EExcDataAbort=23, ///<The thread has tried to read data from an invalid address.
	EExcCodeAbort=24, ///<The thread has tried to fetch an instruction from an invalid address.
	EExcMaxNumber=25, ///<Do not use.
	EExcInvalidVector=26, ///<Do not use.
	};



/**
@publishedAll
@released

Identifies how a thread or process has ended.

While a thread or process is alive, its exit type is always EExitPending.

Both RThread::ExitType() and RProcess::ExitType() return a TExitType.

@see RThread::ExitType()
@see RProcess::ExitType()
@see User::Exit()
@see RThread::Kill()
@see RProcess::Kill()
*/
enum TExitType {
               /**
               The thread or process has ended under normal conditions, i.e. as a result of either:
               1. The thread or process running to completion.
               2. User::Exit() being invoked.
               3. RThread::Kill() or RProcess::Kill() being invoked on the RThread or RProcess handle, respectively.
			   */
               EExitKill,
                
               /**
               The thread or process has ended as a result of a terminate,
               i.e. Terminate() has been called on the RThread or RProcess handle.
               */
               EExitTerminate,

               /**
               The thread or process has been panicked.
               */
               EExitPanic,

               /**
               The thread or process is alive.
               */
               EExitPending
               };




/**
@publishedAll
@released

An enumeration whose enumerators govern the alignment of data which is copied 
or formatted into a descriptor.

@see TDes8::Justify()
@see TDes8::AppendJustify()
@see TDes16::Justify()
@see TDes16::AppendJustify()
*/
enum TAlign {
            /**
            Data is left aligned
            */
            ELeft,
            /**
            Data is centered
            */            
            ECenter,
            /**
            Data is right aligned
            */            
            ERight
            };




/**
@publishedAll
@released

A mask for the set of flags that govern the general format
of the character representation of a real number.

These are the flags with symbols starting KRealFormat...
*/
const TInt KRealFormatTypesMask=0x00000007;




/**
@publishedAll
@released

Defines the general format of the character representation of a real number.
The TRealFormat::iType data member is set to one of these.

The real number is converted to fixed format which has the general pattern:
"nnn.ddd", where nnn is the integer portion and ddd is the decimal portion.
A negative value is prefixed by a minus sign.

The number of decimal places generated is defined by the value of
TRealFormat::iPlaces. Trailing zeroes are generated as required.
If necessary, the decimal portion is rounded to fit the specification.
If this value is zero, no decimal point and no decimal portion is generated.

Triad separation is available,
defined by TRealFormat::iTriad and TRealFormat::iTriLen.

Note that a zero value is converted either to the form "0.000..." with
iPlaces '0' characters after the decimal point, if iPlaces is greater than
zero, or to "0" if iPlaces is zero.

@see TRealFormat
*/
const TInt KRealFormatFixed=1;




/**
@publishedAll
@released

Defines the general format of the character representation of a real number.
The TRealFormat::iType data member is set to one of these.

The real number is converted to scientific format with one non-zero digit
before the decimal point and a number of digits after the decimal point.
Hence the number has the general pattern:
"n.dddE+ee" or "n.dddE-ee", or "n.dddE+eee" or "n.dddE-eee".

The decimal portion is followed by the character 'E', a sign ('+' or '-')
and the exponent as two digits, including leading zeroes, if necessary.
If necessary, the decimal portion is rounded. 

A negative value is prefixed by a minus sign.

If the flag KUseSigFigs is not set, TRealFormat::iPlaces defines the number
of digits which follow the decimal point. If the flag KUseSigFigs is set,
iPlaces defines the maximum number of significant digits to be generated.

Note that, by default, exponents are limited to two digits.
Those numbers that require three digits must have the flag
KAllowThreeDigitExp set. If iPlaces is zero, the value is rounded to one digit
of precision and no decimal point is included.

Triad separation is not available.

Note that a zero value is converted either to the form "0.000...E+00" with
iPlaces '0' characters after the decimal point, if iPlaces is greater than
zero, or to "0E+00" if iPlaces is zero.

@see TRealFormat
*/
const TInt KRealFormatExponent=2;




/**
@publishedAll
@released

Defines the general format of the character representation of a real number.
The TRealFormat::iType data member is set to one of these.

The real number is converted either to fixed or scientific format.
The format chosen is the one which can present the greater number of
significant digits. Where both formats can present the same number of
significant digits, fixed format is used.

The number of decimal places generated depends only on the value of
TRealFormat::iWidth; the value of the iPlaces member is ignored.

Trailing zeroes in the decimal portion are discarded.

Triad separation is not available.

Note that a zero value is converted to "0".

@see TRealFormat
*/
const TInt KRealFormatGeneral=3;




/**
@publishedAll
@released

Defines the general format of the character representation of a real number.
The TRealFormat::iType data member is set to one of these.

The same as KRealFormatFixed but the TRealFormat::iPlaces is interpreted as
specifying the maximum number of significant digits.

Trailing zeroes in the decimal portion are discarded.

@see TRealFormat
*/
const TInt KRealFormatNoExponent=4;




/**
@publishedAll
@released

Defines the general format of the character representation of a real number.
The TRealFormat::iType data member is set to one of these.

The same as KRealFormatGeneral but TRealFormat::iPlaces is interpreted as
specifying the maximum number of significant digits, and the number is
displayed without an exponent whenever possible.

Trailing zeroes in the decimal portion are discarded.

@see TRealFormat
*/
const TInt KRealFormatCalculator=5;




// Extra flags ORed in with the previous types


/**
@publishedAll
@released

A bitmask for all flags except those with symbols starting KRealFormat...
*/
const TInt KRealFormatTypeFlagsMask=0x7E000000;




/**
@publishedAll
@released

A flag that modifies the format of the character representation of a real
number.

It reduces the effective width by one character. This forces a large enough
value for TRealFormat::iWidth to be chosen to guarantee that positive and
negative numbers can be shown to the same precision.

It applies when TRealFormat::iType is set to KRealFormatFixed
or KRealFormatGeneral, and should be ORed into TRealFormat::iType after one of
these types has been set.
*/
const TInt KExtraSpaceForSign=0x40000000;



/**
@publishedAll
@released

A flag that modifies the format of the character representation of a real
number.

It allows an exponent to be formatted whose magnitude is greater than 100.
If this flag is not set, an attempt to format such a number fails.

If set, three digit exponents are allowed. If not set, only two digit
exponents are allowed.

Applies when TRealFormat::iType is set to KRealFormatExponent
or KRealFormatGeneral, and should be ORed into TRealFormat::iType after one of
these types has been set.
*/
const TInt KAllowThreeDigitExp=0x20000000;




/**
@publishedAll
@released

A flag that modifies the format of the character representation of a real
number.

If set, the TRealFormat::iPlaces member is interpreted as the maximum number
of significant digits to be generated.

Applies when TRealFormat::iType is set to KRealFormatExponent, and should be
ORed into TRealFormat::iType after this type has been set.
*/
const TInt KUseSigFigs=0x10000000;




/**
@publishedAll
@released

A flag that modifies the format of the character representation of a real
number.

It disables triad separation.

Applies when TRealFormat::iType is set to KRealFormatFixed
or KRealFormatNoExponent, and should be ORed into TRealFormat::iType after one of
these types has been set.
*/
const TInt KDoNotUseTriads=0x08000000;




/**
@publishedAll
@released

A flag that modifies the format of the character representation of a real
number.

If set, this flag limits the precision to KPrecisionLimit digits.
If not set, the precision defaults to KMaxPrecision digits.

This flag should be ORed into TRealFormat::iType.
*/
const TInt KGeneralLimit=0x04000000;




/**
@publishedAll
@released

A flag that modifies the format of the character representation of a real
number.

If set, this flag allows enough digits of precision such that the mapping from
numeric to string form is injective. For a TReal (=double) input argument
this means KIEEEDoubleInjectivePrecision digits.
This flag overrides the KGeneralLimit flag if both are set.

This flag should be ORed into TRealFormat::iType.
*/
const TInt KRealInjectiveLimit=0x02000000;




/**
@publishedAll
@released

A value, which when passed to the new operator, indicates that the operation
is to leave if insufficient memory available.
*/
enum TLeave {ELeave};




/**
@publishedAll
@released

Defines the way in which the first week in a year is determined.
*/
enum TFirstWeekRule {
                    /**
                    The first week in the year is always the week containing
                    the first day of the year.
                    */
                    EFirstWeek,
                    /**
                    If at least four days of the new year occur during the week
                    containing the first day then this is the first week in the
                    year. Otherwise the first week in the year is the following
                    week. This is the default and complies with the
                    international standard.
                    */
                    EFirstFourDayWeek,
                    /**
                    The first week in the year is the first week of which all
                    seven days occur within the new year.
                    */
                    EFirstFullWeek
                    };




/**
@publishedAll
@released

Timer lock specifications.

They are used by CTimer::Lock() to define the fraction of a second in which 
to call its RunL() function.

@see CTimer
*/
enum TTimerLockSpec
	{
	/** Timer tick is at 1/12 past the second. */
	EOneOClock,

	/** Timer tick is at 2/12 past the second */
	ETwoOClock,

	/** Timer tick is at 3/12 past the second */
	EThreeOClock,

	/** Timer tick is at 4/12 past the second */
	EFourOClock,

	/** Timer tick is at 5/12 past the second */
	EFiveOClock,

	/** Timer tick is at 6/12 past the second */
	ESixOClock,

	/** Timer tick is at 7/12 past the second */
	ESevenOClock,

	/** Timer tick is at 8/12 past the second */
	EEightOClock,

	/** Timer tick is at 9/12 past the second */
	ENineOClock,

	/** Timer tick is at 10/12 past the second */
	ETenOClock,

	/** Timer tick is at 11/12 past the second */
	EElevenOClock,

	/** Timer tick is on the second */
	ETwelveOClock
	};




/**
@publishedAll
@released

Defines the possible environment changes which may be reported by
a change notifier through the RChangeNotifier interface.

Each enumerator corresponds to a distinct type of event.

The changes are reported through a TRequestStatus object when a request to
the change notifier completes. As each enumerator value represents
a separate bit, any combination of events can be reported.

@see RChangeNotifier
@see TRequestStatus
@see TLocale
*/
enum TChanges
	{
	/**
	The system locale has changed.

    Typically this event occurs as a result of a call to TLocale::Set().
	*/
	EChangesLocale=0x01,

	
	/**
	The system time has passed midnight.
	*/
	EChangesMidnightCrossover=0x02,
	
	
	/**
	A thread has died.
	
	This event is reported when any thread in the system dies.
    */
	EChangesThreadDeath=0x04,
	
	
	/**
	The status of the power supply has changed.
	*/
	EChangesPowerStatus=0x08,


	/**
	The system time has changed.
	*/
	EChangesSystemTime=0x10,
	

	/**
	The free memory level has crossed a specified threshold value.
	
	On systems that support data paging, this is also generated where the available swap space
	crosses one of the specified threshold values.
	*/
	EChangesFreeMemory=0x20,

	
	/**
	A memory allocation has failed due to insufficient free memory.
	*/
	EChangesOutOfMemory=0x40,
	

	/**
	The free memory level has fallen below the low-memory threshold
	@see UserSvr::SetMemoryThresholds()
	*/
	EChangesLowMemory=0x80,

	/**
	On systems that support data paging, this is generated where the thrashing level crosses one of
	the specified threshold values.
	*/
	EChangesThrashLevel=0x100,

	/**********************************************************************************
	**  IF YOU ADD A NEW VALUE HERE, YOU NEED TO UPDATE DChangeNotifier CONSTRUCTOR  **
	**********************************************************************************/
	};




/**
@publishedAll
@released

Defines a pointer to a thread function which takes a pointer of
type TAny and returns a TInt.

A function of this type is passed as parameter to RThread::Create()
when creating a thread. Control passes to this function when the thread
is first scheduled for execution.

@see RThread
*/
typedef TInt (*TThreadFunction)(TAny*);




/**
@publishedAll
@released

Defines a function that takes no arguments but returns a TInt.

This is a type which is returned from a call to RLibrary::Lookup().

@see RLibrary
*/
typedef TInt (*TLibraryFunction)();




/**
@publishedAll
@released

Defines a function that takes a single argument of type TInt and returns a TInt.

This is a type which is returned from a call to RLibrary::EntryPoint().

@see RLibrary
*/
typedef TInt (*TLibraryEntry)(TInt);



/**
@publishedAll
@released

Defines an exception handler function which takes a TExcType as an argument,
and returns void.

A function of this type is an exception handler used by member functions
of a thread handle, RThread.

@see RThread
@see TExcType
*/
typedef void (*TExceptionHandler)(TExcType);




// masking constants

/**
@publishedAll
@released

One of a set of flags that categorizes exceptions - associated with
the abort exception only.

@see RThread::SetExceptionHandler()
@see RThread::ModifyExceptionMask()
*/
const TUint KExceptionAbort=0x01;




/**
@publishedAll
@released

One of a set of flags that categorizes exceptions - associated with
the kill exception only.

@see RThread::SetExceptionHandler()
@see RThread::ModifyExceptionMask()
*/
const TUint KExceptionKill=0x02;




/**
@publishedAll
@released

One of a set of flags that categorizes exceptions - general
and user exceptions.

@see RThread::SetExceptionHandler()
@see RThread::ModifyExceptionMask()
*/
const TUint KExceptionUserInterrupt=0x04;




/**
@publishedAll
@released

One of a set of flags that categorizes exceptions - exceptions caused
by illegal floating point operations. This exception is not guaranteed
to be raised when a hardware floating point implementation is in use.

@see RThread::SetExceptionHandler()
@see RThread::ModifyExceptionMask()
*/
const TUint KExceptionFpe=0x08;




/**
@publishedAll
@released

One of a set of flags that categorizes exceptions - exceptions associated
with executing instructions; includes protection faults,
illegal instruction codes, page faults etc

@see RThread::SetExceptionHandler()
@see RThread::ModifyExceptionMask()
*/
const TUint KExceptionFault=0x10;




/**
@publishedAll
@released

One of a set of flags that categorizes exceptions - exceptions caused
by illegal operations on integer values.
*/
const TUint KExceptionInteger=0x20;




/**
@publishedAll
@released

One of a set of flags that categorizes exceptions - exceptions raised
when debugging code.

@see RThread::SetExceptionHandler()
@see RThread::ModifyExceptionMask()
*/
const TUint KExceptionDebug=0x40;




/**
@publishedAll
@released

Aligns the specified value on the boundary defined by __Size.
This is usually 4 for byte alignment or 2 for double-byte alignment.

@param s The value to be aligned.
*/
#define __Align(s) ((((s)+__Size-1)/__Size)*__Size)




/**
@publishedAll
@released

Defines the type of environment data passed to a process
when that process is created.

The data can be either a handle or just binary data.
*/
enum TProcessParameterType
	{
	EHandle=1,
	EBinaryData=2,
	};




// bitwise constants

/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit0= 0x00000001;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit1= 0x00000002;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit2= 0x00000004;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit3= 0x00000008;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit4= 0x00000010;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit5= 0x00000020;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit6= 0x00000040;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit7= 0x00000080;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit8= 0x00000100;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit9= 0x00000200;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit10=0x00000400;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit11=0x00000800;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit12=0x00001000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit13=0x00002000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit14=0x00004000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit15=0x00008000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit16=0x00010000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit17=0x00020000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit18=0x00040000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit19=0x00080000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit20=0x00100000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit21=0x00200000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit22=0x00400000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit23=0x00800000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit24=0x01000000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit25=0x02000000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit26=0x04000000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit27=0x08000000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit28=0x10000000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit29=0x20000000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit30=0x40000000;




/**
@publishedAll
@released

Constant that defines the specified bit value.
*/
const TUint32 KBit31=0x80000000;




/**
@publishedAll
@released

Constant that defines the specified bit value.

This is often used as a bit mask.
*/
const TUint32 KSet32=0xffffffff;




/**
@publishedAll
@released


Constant that defines the specified bit value.

This is often used as a bit mask.
*/
const TUint32 KClear32=0x00000000;
  
  
  

/**
@publishedAll
@released

Constant that defines the specified  value.
*/
const TInt KKilo=1024;




/**
@publishedAll
@released

Constant that defines the specified  value.
*/
const TInt KMega=1024*1024;




/**
@publishedAll
@released

Client/Server Session types.
*/
enum TIpcSessionType
	{
	// These values are used at session creation time to describe the extent to which
	// the new session may be shared.
	//
	// They are *also* used at server creation time, to specify the *maximum* degree
	// of session sharability that the server supports. Thus, if a server instance was
	// created with mode EIpcSession_Sharable, you can open Sharable or Unsharable
	// sessions with it, but not globally sharable ones.
	EIpcSession_Unsharable					= 0x00000000,
	EIpcSession_Sharable					= 0x00000001,	// sharable within one process
	EIpcSession_GlobalSharable				= 0x00000002	// sharable across processes
	};

enum TIpcServerRole
	{
	EServerRole_Default = 0,								// No role specified; treated as Standalone
	EServerRole_Standalone,									// Explicitly neither Master nor Slave
	EServerRole_Master,										// Master: may transfer sessions to a Slave
	EServerRole_Slave										// Slave: accepts sessions from the Master
	};

enum TIpcServerOpts
	{
	// The first few bits specify whether memory referred to by descriptors
	// passed from the client to the server should automatically be pinned
	// All other bits are reserved for future expansion ...
	EServerOpt_PinClientDescriptorsDefault	= 0x00000000,	/**<@internalComponent*/
	EServerOpt_PinClientDescriptorsEnable 	= 0x00000004,	/**<@internalComponent*/
	EServerOpt_PinClientDescriptorsDisable	= 0x00000008,	/**<@internalComponent*/
	EServerOpt_PinClientDescriptorsMask		= 0x0000000c	/**<@internalComponent*/
	};




/**
@publishedAll
@released
*/
const TInt KNullDebugPort=-2;


/**
A constant which represents a thread ID which will never be assigned to a thread.
I.e. The following statement is always true; RThread::Id()!=KNullThreadId
@publishedAll
@released
@see TThreadId
*/
const TUint KNullThreadId = 0xffffffffu;



/**
A constant which represents a process ID which will never be assigned to a process.
I.e. The following statement is always true; RProcess::Id()!=KNullProcessId
@publishedAll
@released
@see TProcessId
*/
const TUint KNullProcessId = 0xffffffffu;



/**
@publishedAll
@released

Hardware floating point types.
*/
enum TFloatingPointType
	{
	/** No hardware floating point. */
	EFpTypeNone=0,
	/** ARM VFPv2 */
	EFpTypeVFPv2=1,
	/** ARM VFPv3 */
	EFpTypeVFPv3=2,
	/** ARM VFPv3-D16 (VFP only, no NEON) */
	EFpTypeVFPv3D16=3,
	};



/**
@publishedAll
@released

Hardware floating point execution modes.
*/
enum TFloatingPointMode
	{
	/**
	Run in the fastest mode available - results of calculations may not be
	exactly as the IEEE standard in some cases. On ARM VFPv2 hardware this
	corresponds to RunFast mode.
	*/
	EFpModeRunFast=0,

	/**
	Perform all calculations as specified in the IEEE standard, but do not
	generate floating point exceptions. This is compatible with the Java
	floating point model. This is the default.
	*/
	EFpModeIEEENoExceptions=1
	};




/**
@publishedAll
@released

Hardware floating point rounding modes.
*/
enum TFloatingPointRoundingMode
	{
	/**
	Round to the nearest value. This is the default.
	*/
	EFpRoundToNearest=0,

	/**
	Round toward positive infinity.
	*/
	EFpRoundToPlusInfinity=1,

	/**
	Round toward negative infinity.
	*/
	EFpRoundToMinusInfinity=2,

	/**
	Round toward zero.
	*/
	EFpRoundToZero=3,

	/**
	@internalComponent
	*/
	EFpRoundNumModes=4
	};



#include <e32capability.h>

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <e32const_private.h>
#endif

#endif
