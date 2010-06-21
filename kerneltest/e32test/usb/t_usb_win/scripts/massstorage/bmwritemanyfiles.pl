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
# USBperformance_multifile - write 100 files to / read them from
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

pod2usage("$0: Specify a target drive.\n")  if (@ARGV < 1);

my $help = 0;
my $drive = "";
my $size = 4;									  # size of a single file in MB
my $files = 100;										# total number of files
my $reading = 0;
my $logfile = "";

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

my %opts = ('help'		=> \$help,
			'drive=s'	=> \$drive,
			'read=s'	=> \$reading,
			'files=i'	=> \$files,
			'size=i'	=> \$size,
			'logfile=s'	=> \$logfile);

GetOptions(%opts) || pod2usage(2);
pod2usage(-exitval => 1, -verbose => 2) if $help;

$size = 1 if $size < 1;
$files = 1 if $files < 1;

# Check OS
# ME : "Windows" "4.90"
# 2k : "Windows NT" "5.0"
# XP : "Windows NT" "5.1"
# Mac: "Darwin" "7.4.1"
LogPrint("START - usbperformance_multifile.pl\n");
LogPrint("drive=$drive files=$files size=$size read=$reading\n");
LogPrint((uname)[0] . " v" . (uname)[2] . ":" . (uname)[3] . "\n");
if (not $TimeHiRes)
	{
	LogPrint("!WARNING! Package Time::HiRes is not installed\n");
	}

# If set to nonzero $¦ forces a flush right away and after every write or print
# on the currently selected output channel.
$¦ = 1;

if ($reading == 0)
{
	LogPrint("Writing $files $size-MB files to the remote directory.\n");
	LogPrint("Creating source files...\n");
	create_files();
	LogPrint("Writing target files...\n");
	my $write_time = write_files();
	LogPrint("File write took $write_time ms.\n");
}
else
{
	LogPrint("Reading files back from remote directory.\n");
	LogPrint("Reading source files...\n");
	my $read_time = read_files();
	LogPrint("File read took $read_time ms.\n");
	delete_files();
}

Leave($returncode);
exit $KErrOther;

# --- subs

sub Leave
	{
	my ($errcode) = @_;
	LogPrint("END - usbperformance_multifile.pl - return code:$errcode\n");
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

sub write_files
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
	for (my $i = 1; $i <= $files; $i++)
		{
		my $filename = "usbfile" . $i . ".dat";
		move($filename, "$drive" . $filename) or Leave($KErrWriteFile);
		}
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

sub read_files
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
	for (my $i = 1; $i <= $files; $i++)
		{
		my $filename = "usbfile" . $i . ".dat";
		move("$drive" . $filename, $filename) or Leave($KErrReadFile);
		}
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

sub create_files
{
	for (my $i = 1; $i <= $files; $i++)
	{
		my $filename = "usbfile" . $i . ".dat";
		LogPrint "Creating file $filename\n";
		if (not open FILE, ">$filename ")
			{
			LogPrint("Can't write to $filename: $!\n");
			Leave($KErrWriteFile);
			}
		my $fills = ($size * 1024 * 1024) / 64;
		my $j = 0;
		while ($j++ < $fills)
		{
			# Add 64 characters at a time
			if (not print FILE '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz?!')
				{
				LogPrint("Can't write to $filename: $!\n");
				Leave($KErrWriteFile);
				}
		}
		close FILE;
	}
	print "\n";
}

sub delete_files
{
	for (my $i = 1; $i <= $files; $i++)
	{
		my $filename = "usbfile" . $i . ".dat";
		unlink $filename;
	}
}

######################################################################

__END__

=head1 NAME

usbperformance_multifile.pl - Create 100 files and write them to a Mass Storage device or
	read them back.

=head1 SYNOPSIS

Usage: usbperformance_multifile.pl [options]

=head1 OPTIONS

=over 2

=item --help

Displays this help.

=item --drive=<drive>

The path to / drive letter of the USB Mass Storage device.

=item --read=<0/1>

0 to write
1 to read

Default value if no option given is 0 (write).

Ensure the card/device has been ejected and thus flushed before you do the
reading test after the writing case, otherwise the content will be cached by
the local OS and the time measured won't be realistic.

'Write' will leave the transferred files on the remote disk, but 'Read' will
delete them (moving them to the local PC). So a practical test order is:

'Write' test, then disconnect/re-connect USB drive, then 'Read' test.

This sequence will start and finish with an empty USB disk.

=item --files=<number of files>

The number of file to write to the device (default: 100)

=item --size=<file size>

Size of files in MB (default: 4)

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
