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
// Simple app to dump out the contents of a drive
// 
//

#include <e32std.h>
#include <e32std_private.h>
#include <e32svr.h>
#include <e32test.h>
#include "user_config.h"

RTest test( _L("TF_DUMP") );


const TInt KBytesPerLine = 32;


class CDumper : public CBase
	{
	public:
		~CDumper();

		void CreateL();
		void DoDump();

	private:
		void Add3Bytes( const TUint8* aBytes );

	private:
		TBusLocalDrive	iDrive;
		TBool			iDriveOpened;
		TBool			iDriveSize;

		TBuf<KBytesPerLine>	iLineBuf;
	};


CDumper::~CDumper()
	{
	if( iDriveOpened )
		{
		iDrive.Disconnect();
		}
	}



void CDumper::CreateL()
	{
	//
	// Load the device driver
	//
#ifdef UNMOUNT_DRIVE
	RFs fs;
	test( KErrNone == fs.Connect() );
#if 0
	// XXX not EKA2
	test( KErrNone == fs.SetDefaultPath( _L("Z:\\") ) );
#endif
	TFullName name;
	fs.FileSystemName( name, KLffsLogicalDriveNumber );
	if( name.Length() > 0 )
		{
		test.Printf( _L("Unmounting drive") );
		test( KErrNone == fs.DismountFileSystem( _L("Lffs"), KLffsLogicalDriveNumber) );
		User::After( 2000000 );
		test.Printf( _L("Drive unmounted") );
		}
	fs.Close();
#endif

	//
	// Open a TBusLogicalDevice to it
	//
	test.Printf( _L("Opening media channel") );
	TBool changedFlag = EFalse;
	User::LeaveIfError( iDrive.Connect( KDriveNumber, changedFlag ) );
	iDriveOpened = ETrue;

	//
	// Get size of Flash drive
	//
	TLocalDriveCapsV2Buf info;
    iDrive.Caps(info);
	iDriveSize = I64LOW(info().iSize);


	iLineBuf.Zero();
	test.Printf( _L("Drive size is 0x%x bytes"), iDriveSize );
	}


void CDumper::Add3Bytes( const TUint8* aBytes )
	{
	TUint8	buf[4];		// produces four output bytes

	buf[0] = 0x30 + (aBytes[0] >> 2);	// first 6 bits
	buf[1] = 0x30 + ( ((aBytes[0] << 4)&0x30) | (aBytes[1] >> 4));
	buf[2] = 0x30 + ( ((aBytes[1] << 2)&0x3C) | (aBytes[2] >> 6));
	buf[3] = 0x30 + (aBytes[2] & 0x3F);	// last 6 bits

	TInt avail = iLineBuf.MaxLength() - iLineBuf.Length();
	if( avail > 4 )
		{
		avail = 4;
		}

	TUint8* p = &buf[0];
	for( TInt i = 0; i < avail; i++ )
		{
		iLineBuf.Append( *p++ );
		}
	if( avail < 4 )
		{
		test.Printf( iLineBuf );
		iLineBuf.Zero();
		for( TInt i = 4 - avail; i > 0; i-- )
			{
			iLineBuf.Append( *p++ );
			}
		}
	}



void CDumper::DoDump()
	{
	TBuf8<300> buf;

	TInt remain = iDriveSize;
	TInt64 offset = 0;
	while( remain > 0 )
		{
		TInt readLen = Min( remain, 300 );
		TInt r = iDrive.Read( offset, readLen, buf );
		if( KErrNone != r )
			{
			RDebug::Print( _L("Error: drive read failed %d"), r );
			return;
			}
		TUint8* p = (TUint8*)buf.Ptr();
		TInt remainder = readLen % 3;
		if( remainder != 0 )
			{
			// leftover bytes, must be end of media, pad buffer
			buf.AppendFill( 0x0, 3 - remainder );
			readLen += 3 - remainder;	// round up
			}
		for( TInt i = readLen / 3; i > 0; i-- )
			{
			Add3Bytes( p );
			p += 3;
			}
		offset += readLen;
		remain -= readLen;
		}
	test.Printf( iLineBuf );	// remaining bit
	}


TInt E32Main()
	{
	test.Title();
	test.Start(_L("Dumping Flash contents...."));

	CDumper dumper;
	TRAPD( ret, dumper.CreateL() );
	if( KErrNone == ret )
		{
		dumper.DoDump();
		}
	return KErrNone;
	}
