#!/usr/bin/perl -w
# Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# This scipt automates the file operations that have to be performed by the user
# See usage for more details
# 
#

use strict;
use File::Copy;
use FindBin qw($Bin);

sub FormatCard($$)
	{
	my ($location, $hash) = @_;

	# Read files
	print "This script will now attempt to format the memory card. If you are using\n";
	print "Windows, please press ENTER, otherwise Ctrl+C this script and format the card\n";
	print "making sure the volume name is '".$hash->{VolumeName}."'.\n";
	$/ = "\n";
	<STDIN>;
	print  "format $location /v:".$hash->{VolumeName}." /x\n";
	system "format $location /v:".$hash->{VolumeName}." /x";
	my @files = glob("$location/*");
	if (@files != 0)
		{
		print "Files found on root directory\n";
		return -1;
		}
	return 0;
	}

sub ReadFiles1($$)
	{
	my ($location, $hash) = @_;
	# Check current volume name
	
	# Not supported...
	
	# Read files
	print "Check number of entries in root dir...";
	my @files = glob("$location/*");
	if (@files != $hash->{RootEntries} - $hash->{DeleteRootDirs})
		{
		printf "Expected %d, got %d\n", $hash->{RootEntries} - $hash->{DeleteRootDirs}, @files;
		return -1;
		}
	else
		{
		print "OK!\n";
		}
	#
	print "Check directories are all here...";
	my $i;
	for ($i = $hash->{DeleteRootDirs}; $i < $hash->{RootEntries} / 2; $i++)
		{
		my $dirname = sprintf("$location/dir%03d", $i);
		if (not -d $dirname)
			{
			print "Cannot find $dirname\n";
			return -1;
			}
		}
	print "OK!\n";
	#
 	print "Check files and size of expanded files...";
 	for ($i = $hash->{RootEntries} / 2; $i < $hash->{RootEntries}; $i++)
 		{
 		my $filename;
 		if ($i - $hash->{RootEntries} / 2 == 0)
 			{
 			$filename = "$location/LONG FILE NAME";
 			}
 		elsif ($i - $hash->{RootEntries} / 2 == 1)
 			{
 			$filename = "$location/Large File";
 			}
 		else
 			{
 			$filename = sprintf("$location/file%03d", $i - $hash->{RootEntries} / 2);
 			}
 		if (not -f $filename)
 			{
 			print "Cannot find $filename\n";
 			return -1;
 			}
 		if ($i - $hash->{RootEntries} / 2 == 1)
			{
			if (-s $filename != $hash->{LargeFileSize} * (1 << 20))
				{
				printf "$filename: expected size %d got %d", $hash->{ExpandRootFilesSize} * (1 << 20), -s $filename;
				return -1;
				}
			}
		elsif (($i - $hash->{RootEntries} / 2 > 1) and ($i - $hash->{RootEntries} / 2 - 2 < $hash->{ExpandRootFilesNumber}))
			{
			if (-s $filename != $hash->{ExpandRootFilesSize} * (1 << 20))
				{
				printf "$filename: expected size %d got %d", $hash->{ExpandRootFilesSize} * (1 << 20), -s $filename;
				return -1;
				}
			}
 		}
 		print "OK!\n";
 	#
 	print "Check subdir entries...";
	@files = glob(sprintf("$location/dir%03d/*", $hash->{RootEntries} / 2 - 1));
	if (@files != $hash->{SubDirEntries})
		{
		printf "Expected %d entries in '$location/dir%03d', got %d", $hash->{SubDirEntries}, $hash->{RootEntries} / 2 - 1, @files;
		return -1;
		}
	for ($i = 0; $i < $hash->{SubDirEntries}; $i++)
		{
		my $filename = sprintf("$location/dir%03d/file%04d", $hash->{RootEntries} / 2 - 1, $i);
		if (not -f $filename)
			{
			print "Cannot find $filename\n";
			return -1;
			}
		}
	print "OK!\n";
	return 0;
	}

sub FileOperations1($$)
	{
	
	my ($location, $hash) = @_;
	my $i;
	
	print "Create root directories...";
	for ($i = 0; $i < $hash->{RootEntries} / 2; $i++)
		{
		my $dirname = sprintf("$location/dir%03d", $i);
		if (!mkdir($dirname))
			{
			print "Error when making $dirname: $!\n";
			return -1;
			}
		}
	print "OK!\n";
	#
	print "Create root files...";
	for ($i = $hash->{RootEntries} / 2; $i < $hash->{RootEntries}; $i++)
		{
		my $filename = sprintf("$location/file%03d", $i - $hash->{RootEntries} / 2);
		open ROOTFILE, ">$filename" or die "Cannot open $filename for output: $!";
		print ROOTFILE "SD";
		close ROOTFILE;
		}
	print "OK!\n";
	#
	print "Expand root files...";
	for ($i = 2; $i < $hash->{ExpandRootFilesNumber} + 2; $i++)
		{
		my $filename = sprintf("$location/file%03d", $i);
		open EXPANDROOTFILE, ">$filename" or die "Cannot open $filename for output: $!";
		print EXPANDROOTFILE "A" x ($hash->{ExpandRootFilesSize} * (1 << 20));
		close EXPANDROOTFILE;
		}
	print "OK!\n";
	#
	print "Delete root directories...";
	for ($i = $hash->{DeleteRootDirs} - 1; $i >= 0; $i--)
		{
		my $dirname = sprintf("$location/dir%03d", $i);
		if (!rmdir($dirname))
			{
			print "Error when removing $dirname";
			return -1;
			}
		}
	print "OK!\n";
	#
	print "Rename 'file000' to 'LONG FILE NAME'...";
	if (!rename("$location/file000", "$location/LONG FILE NAME"))
			{
			print "failed\n";
			return -1;
			}
	print "OK!\n";
	#
	print "Create subdir entries...";
	for ($i = 0; $i < $hash->{SubDirEntries}; $i++)
		{
		my $filename = sprintf("$location/dir%03d/file%04d", $hash->{RootEntries} / 2 - 1, $i);
		open SUBDIRFILE, ">$filename" or die "Can't open $filename for output: $!\n";
		print SUBDIRFILE "SD";
		close SUBDIRFILE;
		}
	print "OK!\n";
	#
	print "Rename 'file001' to 'Large File'...";
	if (!rename("$location/file001", "$location/Large File"))
			{
			print "failed\n";
			return -1;
			}
	print "OK!\n";
	#
	print "Expand Large File...";
	open EXPANDLARGEFILE, ">$location/Large File" or die "Cannot open '$location/Large File' for output: $!";
	print EXPANDLARGEFILE "B" x ($hash->{LargeFileSize} * (1 << 20));
	close EXPANDLARGEFILE;
	print "OK!\n";
	#
	return 0;
	}

sub FileOperations2($$)
	{
	my ($location, $hash) = @_;
	my $i;
	
	print "Change volume name...\n";
	print "WARNING: If not under Windows, change the volume name manually to '".$hash->{VolumeName}."' afterwards.\n";
	print "label $location ".$hash->{VolumeName};
	system "label $location ".$hash->{VolumeName};
	print "OK!\n";
	#
	print "Delete subdir entries...";
	for ($i = 0; $i < $hash->{SubDirEntries}; $i++)
		{
		my $filename = sprintf("$location/dir%03d/file%04d", $hash->{RootEntries} / 2 - 1, $i);
		if (!unlink($filename))
			{
			print "Cannot delete $filename\n";
			return -1;
			}
		}
	print "OK!\n";
	#
	print "Create subdir entries...";
	for ($i = 0; $i < $hash->{SubDirEntries}; $i++)
		{
		my $filename = sprintf("$location/dir%03d/file%04d", $hash->{RootEntries} / 2 - 2, $i);
		open SUBDIRFILE, ">$filename" or die "Can't open $filename for output: $!\n";
		print SUBDIRFILE "SD";
		close SUBDIRFILE;
		}
	print "OK!\n";
	#
	print "Move 'LONG FILE NAME' to tmp dir...";
	if (!move("$location/LONG FILE NAME", $ENV{TMP}."/"))
		{
		print "Cannot move '$location/LONG FILE NAME' to '".$ENV{TMP}."/'\n";
		return -1;
		}
	print "OK!\n";
	#
	print "Copy 'file002' to tmp dir...";
	if (!copy("$location/file002", $ENV{TMP}."/"))
		{
		print "Cannot copy '$location/file002' to '".$ENV{TMP}."/'\n";
		return -1;
		}
	print "OK!\n";
	#
	print "Copy 'file002' back to card...";
	if (!copy($ENV{TMP}."/file002", "$location/BACK"))
		{
		print "Cannot copy '".$ENV{TMP}."/file002' to '$location/BACK'\n";
		return -1;
		}
	print "OK!\n";
	return 0;
	}	

sub Usage()
	{
	print STDERR <<USAGE_END;
fileoperations.pl
Description:
    This script works in conjunction with the SD manual interoperability tests.
    When running these tests, the user will be invited to perform specific file
    operations from another device than the one under test. This Perl script
    automates the execution of those file operations.
    
    There are two sets of actions that can be performed by this Perl script,
    'INBOUND' and 'OUTBOUND'. The user will be clearly notified as to which set
    of actions to carry out.
    
    This script can be run from any directory, but should be left within the
    source code directory tree as it requires access to the test INI file.
Usage:
    fileoperations.pl <type> <location>
Where:
    type       'OUTBOUND' or 'INBOUND'
    location   Memory card root directory
E.g.:
    fileoperations.pl OUTBOUND M:
    fileoperations.pl INBOUND /mnt/sdcard
USAGE_END
	}

sub Main()
	{
	# Verify arguments
	if (@ARGV != 2)
		{
		print STDERR "ERROR: Wrong number of arguments\n";
		Usage();
		return -1;
		}
	my ($type, $location) = @ARGV;
	$type = uc($type);
	if ($type ne 'OUTBOUND' and $type ne 'INBOUND')
		{
		print STDERR "ERROR: Invalid type '$type'\n";
		Usage();
		return -1;
		}
	if (not -d "$location/")
		{
		print STDERR "ERROR: '$location/' is not a valid directory\n";
		Usage();
		return -1;
		}
	if (not -e "$Bin/../testdata/btsd.ini")
		{
		print STDERR "ERROR: Cannot find $Bin/../testdata/btsd.ini\n";
		return -1;
		}
	
	# Get values from INI file
	my %inival = (
		RootEntries => 0,
		ExpandRootFilesNumber => 0,
		ExpandRootFilesSize => 0,
		DeleteRootDirs => 0,
		SubDirEntries => 0,
		LargeFileSize => 0,
		VolumeName => '');
	open INIFILE, "$Bin/../testdata/btsd.ini" or die "ERROR: Cannot open $Bin/../testdata/btsd.ini for read: $!\n";
	while (<INIFILE>)
		{
		$/ = '[';
		next unless /INTEROP_$type\]/;
		
		if (not /FileOperationsRootEntries=(\d+)/)
			{
			print STDERR "ERROR: FileOperationsRootEntries entry not found in INI file\n";
			return -1;
			}
		$inival{RootEntries} = $1;
		
		if (not /FileOperationsExpandRootFilesNumber=\s*(\d+)/)
			{
			print STDERR "ERROR: FileOperationsExpandRootFilesNumber entry not found in INI file\n";
			return -1;
			}
		$inival{ExpandRootFilesNumber} = $1;
		
		if (not /FileOperationsExpandRootFilesSize=\s*(\d+)/)
			{
			print STDERR "ERROR: FileOperationsExpandRootFilesSize entry not found in INI file\n";
			return -1;
			}
		$inival{ExpandRootFilesSize} = $1;
		
		if (not /FileOperationsDeleteRootDirs=\s*(\d+)/)
			{
			print STDERR "ERROR: FileOperationsDeleteRootDirs entry not found in INI file\n";
			return -1;
			}
		$inival{DeleteRootDirs} = $1;
		
		if (not /FileOperationsSubDirEntries=\s*(\d+)/)
			{
			print STDERR "ERROR: FileOperationsSubDirEntries entry not found in INI file\n";
			return -1;
			}
		$inival{SubDirEntries} = $1;

		if (not /FileOperationsLargeFileSize=\s*(\d+)/)
			{
			print STDERR "ERROR: FileOperationsLargeFileSize entry not found in INI file\n";
			return -1;
			}
		$inival{LargeFileSize} = $1;
				
		if (not /FileOperationsVolumeName=\s*([^\n]+)/)
			{
			print STDERR "ERROR: FileOperationsVolumeName entry not found in INI file\n";
			return -1;
			}
		$inival{VolumeName} = $1;
		$inival{VolumeName} =~ s/\s+$//; # Remove trailing whitespaces	
		}
	close INIFILE;
	print "Values read from $Bin/../testdata/btsd.ini:\n";
	print "FileOperationsRootEntries=${inival{RootEntries}}\n";
	print "FileOperationsExpandRootFilesNumber=${inival{ExpandRootFilesNumber}}\n";
	print "FileOperationsExpandRootFilesSize=${inival{ExpandRootFilesSize}}\n";
	print "FileOperationsDeleteRootDirs=${inival{DeleteRootDirs}}\n";
	print "FileOperationsSubDirEntries=${inival{SubDirEntries}}\n";
	print "FileOperationsLargeFileSize=${inival{LargeFileSize}}\n";
	print "FileOperationsVolumeName=${inival{VolumeName}}\n\n";	

	my $retval;
	print "READ CONTENTS OF MEMORY CARD\n";
	if ($type eq 'INBOUND')
		{
		$retval = FormatCard($location, \%inival);
		}
	else
		{
		$retval = ReadFiles1($location, \%inival);
		}
	if ($retval)
		{
		return $retval;
		}
	print "PERFORM MORE FILE OPERATIONS\n";
	if ($type eq 'INBOUND')
		{
		$retval = FileOperations1($location, \%inival);
		}
	else
		{
		$retval = FileOperations2($location, \%inival);
		}
	if (not $retval)
		{
		print "FILE OPERATIONS WERE SUCCESFUL\n";
		print "Now insert card back into the device under test or disconnect USB cable.\n";
		}
	return $retval;
	}

exit Main();
