#!/usr/bin/perl -w
# Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of the License "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - initial contribution.
#
# Contributors:
#
# Description:
# USBperformance - write file to / read file from
# a USB Mass Storage device,
# measure the time taken,
# and display the data rate achieved.
# 
#

use strict;
use POSIX;
use File::Copy;
use Getopt::Long;
use Pod::Usage;

# Check if Time::HiRes package is installed
my $TimeHiRes = 1;
eval
	{
	require Time::HiRes;
	};
if ($@)
	{
	# Not installed
	$TimeHiRes = 0;
	}

pod2usage("$0: Specify at least a target drive.\n")  if (@ARGV < 1);

my $help = 0;
my $drive = "";
my $size = 400;
my $reading = 0;
my $logfile = "";
my $local_file = "gobble.dat";

# Error codes
my $KErrNone = 0;			# No error - Test passed
my $KErrLogFile = -1;		# Can't open log file
my $KErrWriteFile = -2;		# Can't open file for writing
my $KErrReadFile = -3;		# Can't open file for reading
my $KErrRmFile = -4;		# Can't remove file
my $KErrRmDir = -5;			# Can't remove directory
my $KErrCmp = -6;			# File comparison failed
my $KErrOther = -7;			# Other

my $returncode = $KErrNone;

my %opts = ('help' => \$help,
			'drive=s' 	=> \$drive,
			'size=s' => \$size,
			'read=s' => \$reading,
			'logfile=s'=> \$logfile);

GetOptions(%opts) || pod2usage(2);
pod2usage(-exitval => 1, -verbose => 2) if $help;

$size = 1 if $size < 1;

# Check OS
# ME : "Windows" "4.90"
# 2k : "Windows NT" "5.0"
# XP : "Windows NT" "5.1"
# Mac: "Darwin" "7.4.1"
LogPrint("START - usbperformance.pl\n");
LogPrint("drive=$drive size=$size read=$reading\n");
LogPrint((uname)[0] . " v" . (uname)[2] . ":" . (uname)[3] . "\n");
if (not $TimeHiRes)
	{
	LogPrint("!WARNING! Package Time::HiRes is not installed\n");
	}

my $remote_file = "$drive$local_file";

# If set to nonzero $¦ forces a flush right away and after every write or print
# on the currently selected output channel.
$¦ = 1;

if ($reading == 0)
{
	LogPrint("Writing a $size MB file to the remote directory.\n");
	LogPrint("Creating source file \"$local_file\"...\n");
	create_gobblefile();
	LogPrint("Writing target file \"$remote_file\"...\n");
	my $write_time = writefile();
	LogPrint("File write took $write_time ms.\n");
}
else
{
	LogPrint("Reading file from a remote directory.\n");
	if (not -e $remote_file)
		{
		LogPrint("Remote file \"$remote_file\" doesn't seem to exist\n");
		Leave($KErrReadFile);
		}
	if (-z $remote_file)
		{
		LogPrint("Remote file \"$remote_file\" seems to be empty\n");
		Leave($KErrReadFile);
		}
	my $size_bytes = -s $remote_file;
	$size = $size_bytes / 1024 / 1024;
	LogPrint("Reading source file \"$remote_file\" ($size_bytes bytes = $size MB)...\n");
	my $read_time = readfile();
	LogPrint("File read took $read_time ms.\n");
	unlink $local_file;
}

Leave($returncode);
exit $KErrOther;

# --- subs

sub Leave
	{
	my ($errcode) = @_;
	LogPrint("END - usbperformance.pl - return code:$errcode\n");
	exit $errcode
	}

sub LogPrint
	{
	if ($logfile ne "")
		{
		open LOGFILE, ">>$logfile" or exit $KErrLogFile;
		print LOGFILE @_;
		close LOGFILE;
		}
	print @_;
	}

sub writefile
{
	my ($start, $end, $duration);
	if ($TimeHiRes)
		{
		$start = Time::HiRes::time();
		}
	else
		{
		$start = time();
		}
	move($local_file, $remote_file) or Leave($KErrWriteFile);
	if ($TimeHiRes)
		{
		$end = Time::HiRes::time();
		}
	else
		{
		$end = time();
		}
	# Duration in ms
	$duration = int(1000 * ($end - $start));
	return ($duration);
}

sub readfile
	{
	my ($start, $end, $duration);
	if ($TimeHiRes)
		{
		$start = Time::HiRes::time();
		}
	else
		{
		$start = time();
		}
	move($remote_file, $local_file) or Leave($KErrReadFile);
	if ($TimeHiRes)
		{
		$end = Time::HiRes::time();
		}
	else
		{
		$end = time();
		}
	# Duration in ms
	$duration = int(1000 * ($end - $start));
	return ($duration);
	}

sub create_gobblefile
{
	if (not open FILE, ">$local_file ")
		{
		LogPrint "Can't create file: $!";
		Leave($KErrWriteFile);
	 	}
	my $realsize = ($size * 1024);
	my $i = 0;
	while ($i++ < $realsize)
	{
		# Add a 1024-byte block
		if (not print FILE '012345ABCDEFGHIJKLMNOPQRSTUVWXYZ'x32)
			{
			LogPrint("Write error: $!\n");
			Leave($KErrWriteFile);
			}
	}
	close FILE;
}

######################################################################

__END__

=head1 NAME

usbperformance.pl - Create a large file and write it to a Mass Storage device, or
	read it back.

=head1 SYNOPSIS

Usage: usbperformance.pl [options]

=head1 OPTIONS

=over 3

=item --help

Displays this help.

=item --drive=<drive>

The path to / drive letter of the USB Mass Storage device.

=item --size=<size>

The size of the file to be created to measure the speed connection.

Size is expected in MB. Default value if no option given is 400 MB.

=item --read=<0/1>

0 to write
1 to read

Default value if no option given is 0 (write).

Ensure the card/device has been ejected and thus flushed before you do the
reading test after the writing case, otherwise the content will be cached by
the local OS and the time measured won't be realistic.

'Write' will leave the transferred file on the remote disk, but 'Read' will
delete it (moving it to the local PC). So a practical test order is:

'Write' test, then disconnect/re-connect USB drive, then 'Read' test.

This sequence will start and finish with an empty USB disk.

=item --logfile=<file>

Append test output to specified file.

=back

=head1 DESCRIPTION

This is a simple utility to create a file on a mass storage unit and measure
the performance in MB / seconds of the USB connection.

=head2 Test Case Specification

 TestCaseID:	PBASE-PREQ709-0045-Performance
 TestType: 	IT
 TestCaseDesc:  Test Mass Storage performance on different platforms
	        (Windows 2000/XP/ME, MacOS) (Manual test)

 TestActions:

Build a ROM with usbmsapp.exe on it. Connect the board to the PC with an USB
cable. Execute usbmsapp.exe on the device to map the mass storage unit (MMC/SD
card) onto the PC. Execute this script on the PC, and make sure the card has
free space for the file that is going to be created. The default size is 400
MB.

=cut
