#
# Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#

use Cwd;
use Getopt::Long;
my %opts=();
my $result = GetOptions (	\%opts,
							"iby=s",
							"build=s",
							"drive=s",
							"udir=s",
							"kdir=s",
							"zip=s",
							"verbose"
							);


if (!$result || !$opts{iby} || !$opts{build}) {
	usage();
	exit;
}
my $IbyFileName = $opts{iby};
open IBY, $IbyFileName or die "Cannot open IBY file $IbyFileName\n";
my $build = $opts{build};
my $srcdrv = Cwd::getcwd();
$srcdrv=~/(\w\:).*/;
$srcdrv=$1;
if ($opts{drive}) {
	$srcdrv=$opts{drive};
}
my $udir = $opts{udir};
my $kdir = $opts{kdir};
my $zip = $opts{zip};
my $verbose = $opts{verbose};

if ($verbose) {
	print "IBY file  : $IbyFileName\n";
	print "Build     : $build\n";
	print "Drive     : $srcdrv\n";
	print "User Dir  : $udir\n";
	print "Kernel Dir: $kdir\n";
	print "ZIP file  : $zip\n";
}

while(<IBY>) {
	if (/^\s*\/\//) {
		print "Comment: $_\n" if ($verbose);
		next;
	}
	/^\s*(\w+)(.*)$/;
	my $keyword=$1;
	my $rest=$2;
	unless ($keyword eq 'file' or $keyword eq 'data' or $keyword eq 'dll' or $keyword eq 'device') {
		next;
	}
	if ($rest=~/^\s*\[\w+\](.*)$/) {
		$rest=$1;	# lose [MAGIC]
	}
	next unless ($rest=~/^\s*\=\s*(.*)$/);
	$rest=$1;
	my @word=split /\s+/, $rest;
	my $source=$word[0];
	my $dest=$word[1];
	$source=~ s/##BUILD##/$build/g;
	$source=~ s/##MAIN##/$udir/g;
	$source=~ s/##ASSP##/$kdir/g;
	$source=~ s/##KMAIN##/$kdir/g;
	$source=~ s/##ELOCLDIR##/$udir/g;
	$source=$srcdrv.$source;
#	print "$source->$dest\n";
	my @destpath=split /(\/|\\)/,$dest;
	while ($destpath[0]=~/^\s*$/ or $destpath[0] eq '\\' or $destpath[0] eq '\/') {
		shift @destpath;
	}
	my $npathc=scalar(@destpath);
	my $destfilename=pop @destpath;
	my $destdirname=join '', @destpath;
	if ($destdirname=~/^(.*)\\\s*$/) {
		$destdirname=$1;
	}
	if ($destdirname=~/^\\(.*?)$/) {
		$destdirname=$1;
	}
	print "$source->$destfilename @ $destdirname $npathc\n" if ($verbose);
	unless (-d $destdirname) {
		system "md $destdirname";
	}
	if (-e $source) {
		system "copy $source $destdirname\\$destfilename >NUL";
	} else {
		warn "$source not found\n";
	}
}
if ($zip) {
	unless ($zip =~ /^(.*?)\.zip$/i) {
		$zip .= '.zip';
	}
	system "del $zip";
	system "zip -r $zip *";
}

exit;

# END OF MAIN

sub usage {
	print <<EOT;

perl cptest.pl <options>

Copy tests to target directory structure

The following options are required:
  -i, --iby = <IBY file name>
  -b, --build = <build>         UDEB or UREL

The following are optional:
  -d, --drive = <drive with executables on>

         Defaults to current drive

  -u, --udir = <user directory>	  e.g. ARM4, X86
  -k, --kdir = <kernel directory> e.g. MISA, NX86
  -z, --zip = <zipfile name>
  -v, --verbose

EOT
}

