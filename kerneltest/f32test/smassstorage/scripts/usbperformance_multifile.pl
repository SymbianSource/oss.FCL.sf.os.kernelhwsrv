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
use File::Copy;
use Getopt::Long;
use Pod::Usage;

pod2usage("$0: Specify a target drive.\n")  if (@ARGV < 1);

my $help = 0;
my $size = 4;									  # size of a single file in MB
my $files = 100;										# total number of files
my $reading = 0;

my %opts = ('help' => \$help,
			'read=s' => \$reading);

GetOptions(%opts) || pod2usage(2);
pod2usage(-exitval => 1, -verbose => 2) if $help;

my $drive = $ARGV[@ARGV - 1];
$drive =~ s/\\/\//g;
$drive .= "/" unless ($drive =~ m/\/$/);
print "Remote drive: " . $drive . "\n";

# If set to nonzero $¦ forces a flush right away and after every write or print
# on the currently selected output channel.
$¦ = 1;

if ($reading == 0)
{
	print "Writing $files $size-MB files to the remote directory.\n";
	print "Creating source files...\n";
	create_files();
	print "Writing target files...\n";
	my $write_time = write_files();
	print "File write took $write_time seconds.\n";
	printf "That's about %.2f MB/s.\n", ($size * $files / $write_time);
}
else
{
	print "Reading files back from remote directory.\n";
	print "Reading source files...\n";
	my $read_time = read_files();
	print "File read took $read_time seconds.\n";
	printf "That's about %.2f MB/s.\n", ($size * $files / $read_time);
	delete_files();
}


# --- subs

sub write_files
{
	my $start = time();
	for (my $i = 1; $i <= $files; $i++)
	{
		my $filename = "usbfile" . $i . ".dat";
		print "Writig $filename\r";
		move($filename, "$drive" . $filename);
	}
	print "\n";
	return (time() - $start);
}

sub read_files
{
	my $start = time();
	for (my $i = 1; $i <= $files; $i++)
	{
		my $filename = "usbfile" . $i . ".dat";
		print "Reading $filename\r";
		move("$drive" . $filename, $filename);
	}
	print "\n";
	return (time() - $start);
}

sub create_files
{
	for (my $i = 1; $i <= $files; $i++)
	{
		my $filename = "usbfile" . $i . ".dat";
		print "Creating file $filename\r";
		open FILE, ">$filename " or die $!;
		my $fills = ($size * 1024 * 1024) / 64;
		my $j = 0;
		while ($j++ < $fills)
		{
			# Add 64 characters at a time
			print FILE '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz?!' or die $!;
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

Usage: usbperformance_multifile.pl [options] <USB drive>

<USB drive>: The path to / drive letter of the USB Mass Storage device.

=head1 OPTIONS

=over 2

=item --help

Displays this help.

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
