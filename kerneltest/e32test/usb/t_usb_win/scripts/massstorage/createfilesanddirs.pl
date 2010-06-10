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
# USBinterop1
# 
#

use strict;
use Digest::MD5;
use POSIX;
use File::Path;
use Getopt::Long;
use Pod::Usage;

my $spread = 3;
my $depth = 1;
my $drive = "";
my $rootfolder = "";
my $size = 1024;
my $reading = 0;
my $logfile = "";
my $help = 0;

# Error codes
my $KErrNone = 0;			# No error - Test passed
my $KErrLogFile = -1;		# Can't open log file
my $KErrWriteFile = -2;		# Can't open file for writing
my $KErrReadFile = -3;		# Can't open file for reading
my $KErrRmFile = -4;		# Can't remove file
my $KErrRmDir = -5;			# Can't remove directory
my $KErrCmp = -6;			# File comparison failed
my $KErrOther = -7;			# Other
my $KErrDirectory = -8;		# Creating Directories
my $KErrFormat = -9;		# Format Device

my $returncode = $KErrNone;

my %opts = ( 'spread=i' => \$spread,
	     'depth=i' 	=> \$depth,
	     'drive=s' 	=> \$drive,
	     'size=i' 	=> \$size,
	     'read=i'	=> \$reading,
	     'logfile=s'=> \$logfile,
	     'help!' 	=> \$help);

GetOptions(%opts) || pod2usage(2);
pod2usage(-exitval => 1, -verbose => 2) if $help;

$drive =~ s/\\/\//g;
$rootfolder = $drive;
$rootfolder .= "/" unless ($drive =~ m/\/$/);
$spread = 1 if $spread < 1;
$depth = 1 if $depth < 1;
$size = 1 if $size < 1;

# Check OS
# ME : "Windows" "4.90"
# 2k : "Windows NT" "5.0"
# XP : "Windows NT" "5.1"
# Mac: "Darwin" "7.4.1"
LogPrint("START - usbinterop1.pl\n");
LogPrint("spread=$spread depth=$depth drive=$drive size=$size read=$reading\n");
LogPrint((uname)[0] . " v" . (uname)[2] . ":" . (uname)[3] . "\n");

# Set the random number seed
srand(2008 * $spread * $depth);

LogPrint("Generating Folder Names\n");
my @folders = createfolders($rootfolder, $spread, $depth);

if ($reading == 0)
	{
	LogPrint("Creating Folders\n");
    foreach my $path (@folders) {
		next if -d $path;
		if (!mkdir($path)) {
			LogPrint("Error Creating Directory so formating device ".$drive."\n");
			open (INPUTPIPE,"| format ".$drive." /FS:FAT") or Leave($KErrFormat);
			close (INPUTPIPE);
			LogPrint("Format complete\n");
			$returncode = $KErrDirectory;
			last;
			}
		}
	if ($returncode == $KErrDirectory) { 
		foreach my $path (@folders) {
			next if -d $path;
			Leave($KErrDirectory) if (!mkdir($path));
			}
		}
	$returncode = $KErrNone;
	}

LogPrint("Creating Files\n");
my %digests = createfiles(\@folders, $size, $spread);

if ($reading == 1)
	{
	LogPrint("Checking Files\n");
	checkfiles(\%digests);
	LogPrint("Removing Files\n");
	removefiles(\%digests);
	LogPrint("Removing Folders\n");
	removefolders(\@folders);
	}

Leave($returncode);
exit $KErrOther;

sub Leave
	{
	my ($errcode) = @_;
	LogPrint("END - usbinterop1.pl - return code:$errcode\n");
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

sub createfolder
{
	my $dirlist = shift;
	my $fbase = shift;
	my $spread = shift;
	my $depth = shift;

	return unless $depth > 0;

	for (my $i = 0; $i < $spread; $i++)
	{	
		my $dir = sprintf("%sdir%05d/", $fbase, $i + 1);
		push @$dirlist, $dir;
		createfolder($dirlist, $dir, $spread, $depth - 1);
	}
}

sub createfolders
{
	my $drive = shift;
	$drive = "" unless defined $drive;
	my $spread = shift;
	$spread = 1 unless defined $spread;
	my $depth = shift;
	$depth = 1 unless defined $depth;

	my @dirlist = ();
	createfolder(\@dirlist, $drive, $spread, $depth);
	return @dirlist;
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
			LogPrint "Unable to open $file for writing: $!\n";
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
		$returncode = $KErrReadFile unless($returncode);
		return "";
		};
	my $md5 = Digest::MD5->new();
	$md5->addfile(*FILE);
	close(FILE);
	return $md5->hexdigest();
}


sub createfiles
{
	my $dirlist = shift;
	my $size = shift;
	$size = 1024 unless defined $size;
	my $nfiles = shift;
	$nfiles = 1 unless defined $nfiles;

	my %digest;

	foreach (@$dirlist)
	{
		for (my $i = 0; $i < $nfiles; $i++)
		{	
			my $file = sprintf("${_}file%04d.txt", $i + 1);
			$digest{$file} = writefile($file, $size);
		}
	}
	return %digest;
}

sub checkfiles
{
	my $digests = shift;
	
	foreach (sort keys %$digests)
	{
		my $readDigest = readfile($_);
		if ($readDigest) {
			if ($digests->{$_} ne $readDigest)
				{
				LogPrint "$_\t" . "ERROR" . "\t$digests->{$_}\t$readDigest" . "\n";
				$returncode = $KErrCmp unless($returncode);
				}
			}
	}
}


sub removefiles
{
	my $digests = shift;
	my @cant = grep {not unlink} (keys %$digests);
	if (@cant)
		{
		LogPrint "Unable to remove @cant\n";
		$returncode = $KErrRmFile unless($returncode);
		}
}
	

sub removefolders
{
	my $folders = shift;
	foreach (@$folders)
	{
		if (-e)
		{
			if (not rmtree($_))
				{
				LogPrint "Unable to remove $_: $!\n";
				$returncode = $KErrRmDir unless($returncode);
				}
		}
	}
}

######################################################################

__END__

=head1 NAME

usbinterop1.pl - Create directories and files, read back and compare

=head1 SYNOPSIS

usage:   usbinterop1.pl [options]

=head1 OPTIONS

=over 4

=item --spread=<number of directories and files>

Spread is the number of directories and files that are created at each
level of the created tree.  For example, a spread of three would
create three directories and three files in each of the directories.
Spread is a measure of the "bushiness" of the directory tree.

Default value is "3".

=item --depth=<directory nesting level>

Each directory can have subdirectories up to the limit of the depth
parameter.  Depth is a measure of the "height" of the directory tree.

Default value is "1".

=item --size=<size of files to create>

The size in bytes for each test file.  Be careful as the disk space
used is a function of this parameter as well as the depth and spread
parameters: 

total size = size*(sp^(dep+1)+sp^(dep)+sp^(dep-1)+..+sp^2)

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

This is a simple utility to create folders and files with varying
levels of nesting and sizes on a USB drive, and read them back to
verify their contents.

=head2 Test Case Specification

 TestCaseID: 	Interoperability_1
 TestType: 	IT
 TestCaseDesc:  Test Mass Storage functionality on different platforms
	        (Windows 2000/XP/ME, MacOS) (Manual test)
 FssID:		Base/emstore/1.1.1
 FssID:		Base/emstore/3.1.1

 TestActions: 
	Connect device to a host PC. Enable MS. Start perl script on
 PC.  This script formats drive and creates several folders with
 levels of nested folders and writes set of files to them.  File sizes
 varied from several kilobytes to several megabytes.  The number of
 folders, nest depth, number of files placed in each folder and there
 sizes should be configurable.  Then script prompt ask to unplug/plug
 USB cable (to flash OS read cache) and then read all files back and
 compare.

 TestExpectedResults:  
	Read data from files should match with written.

=cut
