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
my $reading = 0;
my $logfile = "";
my $help = 0;
my $filename = "file0000.txt";

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

my %opts = ( 'drive=s' 	=> \$drive,
	     'size=i' 	=> \$size,
	     'read=i'	=> \$reading,
	     'logfile=s'=> \$logfile,
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
LogPrint("START - usbinterop2.pl\n");
LogPrint("drive=$drive size=$size read=$reading\n");
LogPrint((uname)[0] . " v" . (uname)[2] . ":" . (uname)[3] . "\n");

# Set the random number seed
srand(1515 * $size);

$filename = "$drive$filename";

my $writeDigest = writefile($filename, $size);

if ($reading == 1)
	{
	my $readDigest = readfile($filename);
	print "$filename\t$writeDigest\t$readDigest\t" . ($writeDigest eq $readDigest ? "ok" : "ERROR") . "\n";
	if ($writeDigest ne $readDigest)
		{
		$returncode = $KErrCmp;
		}
	if (not unlink($filename))
		{
		LogPrint("Can't remove $filename: $!\n");
		Leave($KErrRmFile);
		}
	}

Leave($returncode);
exit $KErrOther;

sub Leave
	{
	my ($errcode) = @_;
	LogPrint("END - usbinterop2.pl - return code:$errcode\n");
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
	if ($reading == 0)
		{
		if (not open(FILE, ">$file"))
			{
			LogPrint("Unable to open $file for writing: $!\n");
			Leave($KErrWriteFile);
			}
		}
	my $md5 = Digest::MD5->new();
	while ($length > 0)
	{	
		my $data = makeblock(($length > $block) ? $block : $length);
		$md5->add($data);
		if ($reading == 0)
			{
			print(FILE $data);
			}
		$length -= $block;
	}
	if ($reading == 0)
		{
		close(FILE);
		}
	return $md5->hexdigest();
}


sub readfile 
{
	my $file = shift;
    if (!sysopen(FILE, $file, O_RDONLY)) {
		LogPrint("Unable to open $file for reading: $!\n");
		Leave($KErrReadFile);
		};
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

=item --logfile=<file>

Append test output to specified file.

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
