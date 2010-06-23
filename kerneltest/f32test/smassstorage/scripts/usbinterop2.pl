#!perl -w
# Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# USBinterop2
# 
#

use strict;
use Digest::MD5;
use POSIX;
use Getopt::Long;
use Pod::Usage;

my $drive = ".";
my $size = 1024;
my $pause = 1;
my $help = 0;
my $filename = "file0000.txt";

my %opts = ( 'drive=s' 	=> \$drive,
	     'size=i' 	=> \$size,
	     'pause!' 	=> \$pause,
	     'help!' 	=> \$help);

GetOptions(%opts) || pod2usage(2);
pod2usage(-exitval => 1, -verbose => 2) if $help;

$drive =~ s/\\/\//g;
$drive .= "/" unless ($drive =~ m/\/$/);
$size = 1 if $size < 1;

# Check OS
# ME : "Windows" "4.90"
# 2k : "Windows NT" "5.0"
# XP : "Windows NT" "5.1"
# Mac: "Darwin" "7.4.1"
print((uname)[0] . " v" . (uname)[2] . ":" . (uname)[3] . "\n");

$filename = "$drive$filename";
my $writeDigest = writefile($filename, $size);

if ($pause)
{
	print "Unplug and replug the USB cable, then press enter...";
	$pause = <STDIN>;
}

my $readDigest = readfile($filename);
print "$filename\t$writeDigest\t$readDigest\t" . ($writeDigest eq $readDigest ? "ok" : "ERROR") . "\n";
unlink($filename) or die("Can't remove $filename: $!\n");

sub makeblock
{
	my $length = shift;
	my @list = ();
	for (my $i = 0; $i < $length; $i++)
	{
		push @list, int((91 - 65) * rand()) + 65;
	}
	return pack "C$length", @list;
}


sub writefile
{
	my $file = shift;
	my $length = shift;
	my $block = 1024;
	open(FILE, ">$file") or warn ("Unable to open $file for writing: $!\n");
	my $md5 = Digest::MD5->new();
	while ($length > 0)
	{	
		my $data = makeblock(($length > $block) ? $block : $length);
		$md5->add($data);
		print(FILE $data);
		$length -= $block;
	}
	close(FILE);
	return $md5->hexdigest();
}


sub readfile 
{
	my $file = shift;
	open(FILE, $file) or warn ("Unable to open $file for reading: $!\n");
	my $md5 = Digest::MD5->new();
	$md5->addfile(*FILE);
	close(FILE);
	return $md5->hexdigest();
}



######################################################################

__END__

=head1 NAME

usbinterop2.pl - Create drive-filling file, read back and compare

=head1 SYNOPSIS

usage:   usbinterop2.pl [options]

=head1 OPTIONS

=over 4

=item --size=<size of files to create>

The size in bytes for the test file.

Default value is "1024".

=item --drive=<USB drive location>

The path to the USB drive in which to write the files.

Default value is ".", the current working directory.

=item --help=<file>

Displays this help.

=back

=head1 DESCRIPTION

This is a simple utility to create a file that fills the USB drive as
much as possible, and reads it back to verify its contents.

=head2 Test Case Specification

 TestCaseID:	Interoperability_2
 TestType: 	IT
 TestCaseDesc:  Test Mass Storage functionality on different platforms
	        (Windows 2000/XP/ME, MacOS) (Manual test)
 FssID: 	Base/emstore/1.1.1
 FssID: 	Base/emstore/3.1.1

 TestActions:
	Connect device to a host PC. Enable MS. Start perl script on
 PC.  This script formats drive and queries size of it.  Than script
 create file with size close to drive size and check free space.  Then
 script prompt ask to unplug/plug USB cable (to flash OS read cache)
 and then read the file back and compare.

 TestExpectedResults:
	File creation should succeed. Read data from file should match
 with written.  Sum of file size and free space should be close to
 drive size.

=cut
