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
use File::Copy;
use Getopt::Long;
use Pod::Usage;

pod2usage("$0: Specify at least a target drive.\n")  if (@ARGV < 1);

my $help = 0;
my $size = 400;
my $reading = 0;
my $local_file = "gobble.dat";

my %opts = ('help' => \$help,
			'size=s' => \$size,
			'read=s' => \$reading);

GetOptions(%opts) || pod2usage(2);
pod2usage(-exitval => 1, -verbose => 2) if $help;

$size = 1 if $size < 1;

my $drive = $ARGV[@ARGV - 1];
$drive =~ s/\\/\//g;
$drive .= "/" unless ($drive =~ m/\/$/);
print "Remote drive: " . $drive . "\n";

my $remote_file = "$drive$local_file";

# If set to nonzero $¦ forces a flush right away and after every write or print
# on the currently selected output channel.
$¦ = 1;

if ($reading == 0)
{
	print "Writing a $size MB file to the remote directory.\n";
	print "Creating source file \"$local_file\"...\n";
	create_gobblefile();
	print "Writing target file \"$remote_file\"...\n";
	my $write_time = writefile();
	print "File write took $write_time seconds.\n";
	printf "That's about %.2f MB/s.\n", ($size / $write_time);
}
else
{
	print "Reading file from a remote directory.\n";
	-e $remote_file or die "Remote file \"$remote_file\" doesn't seem to exist";
	-z $remote_file and die "Remote file \"$remote_file\" seems to be empty";
	my $size_bytes = -s $remote_file;
	$size = $size_bytes / 1024 / 1024;
	print "Reading source file \"$remote_file\" ($size_bytes bytes = $size MB)...\n";
	my $read_time = readfile();
	print "File read took $read_time seconds.\n";
	printf "That's about %.2f MB/s.\n", ($size / $read_time);
	unlink $local_file;
}


# --- subs

sub writefile
{
	my $start = time();
	move($local_file, $remote_file);
	return (time() - $start);
}

sub readfile
	{
	my $start = time();
	move($remote_file, $local_file);
	return (time() - $start);
	}

sub create_gobblefile
{
	open FILE, ">$local_file " or die $!;
	my $realsize = ($size * 1024 * 1024) / 40;
	my $i = 0;
	while ($i++ < $realsize)
	{
		# Add 40 characters
		print FILE '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcd' or die $!;
	}
	close FILE;
}

######################################################################

__END__

=head1 NAME

usbperformance.pl - Create a large file and write it to a Mass Storage device, or
	read it back.

=head1 SYNOPSIS

Usage: usbperformance.pl [options] <USB drive>

<USB drive>: The path to / drive letter of the USB Mass Storage device.

=head1 OPTIONS

=over 3

=item --help

Displays this help.

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
