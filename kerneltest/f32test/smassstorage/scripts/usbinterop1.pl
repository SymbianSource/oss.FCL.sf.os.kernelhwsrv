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
my $size = 1024;
my $pause = 1;
my $help = 0;

my %opts = ( 'spread=i' => \$spread,
	     'depth=i' 	=> \$depth,
	     'drive=s' 	=> \$drive,
	     'size=i' 	=> \$size,
	     'pause!' 	=> \$pause,
	     'help!' 	=> \$help);

GetOptions(%opts) || pod2usage(2);
pod2usage(-exitval => 1, -verbose => 2) if $help;

$drive =~ s/\\/\//g;
$drive .= "/" unless ($drive =~ m/\/$/);
$spread = 1 if $spread < 1;
$depth = 1 if $depth < 1;
$size = 1 if $size < 1;

# Check OS
# ME : "Windows" "4.90"
# 2k : "Windows NT" "5.0"
# XP : "Windows NT" "5.1"
# Mac: "Darwin" "7.4.1"
print((uname)[0] . " v" . (uname)[2] . ":" . (uname)[3] . "\n");

my @folders = createfolders($drive, $spread, $depth);
mkpath(\@folders);
my %digests = createfiles(\@folders, $size, $spread);

if ($pause)
{
	print "Unplug and replug the USB cable, then press enter...";
	$pause = <STDIN>;
}

checkfiles(\%digests);
removefiles(\%digests);
removefolders(\@folders);



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
		print "$_\t$digests->{$_}\t$readDigest\t" . ($digests->{$_} eq $readDigest ? "ok" : "ERROR") . "\n";
	}
}


sub removefiles
{
	my $digests = shift;
	my @cant = grep {not unlink} (keys %$digests);
	warn "Unable to remove @cant\n" if @cant;
}
	

sub removefolders
{
	my $folders = shift;
	foreach (@$folders)
	{
		if (-e)
		{
			rmtree($_) or warn "Unable to remove $_: $!\n";
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
