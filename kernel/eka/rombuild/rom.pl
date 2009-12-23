#!perl
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

# rom.pl - Build a rom
#
# Pre-processes the .oby/iby files then invokes rombuild.exe
# (or other specified builder)

# First, read our config file

use strict;
use Getopt::Long;
use Cwd;

#Getopt::Long::Configure("ignore_case");
my %opts=();
my $param_count = scalar(@ARGV);
my $result = GetOptions (\%opts, "assp=s",
						 "inst=s",
						 "type=s",
						 "variant=s",
						 "build=s", 
						 "conf=s",
						 "name=s",
						 "modules=s",
						 "xabi=s",
						 "clean",
						 "quiet",
						 "help",
						 "_debug",
						 "zip",
						 "symbol",
						 "noheader",
						 "kerneltrace=s",
						 "rombuilder=s",
						 "define=s@",
						 "rofsbuilder=s",
						 "compress"
						 );

my (@assps, @builds, %variants, @templates, %flags, %insts, %zip, %builder);
my $main;
my $kmain;
my $toroot;
my $e32path;
my $rombuildpath;
my $euserdir;
my $elocldir;
my $kbdir;
my $romname;
my $single;
my $smain;
my $pagedCode;
my $debug = $opts{_debug};
my $quiet;
$quiet = $opts{quiet} unless $debug;

my $drive;

# discover where this script is running from to set the $toroot and $e32path variables

my $toolpath;
my $Epoc32Path;
my $EpocRoot;
my $drive;
my $BasePath;

BEGIN {

	$EpocRoot = $ENV{EPOCROOT};
	die "ERROR: Must set the EPOCROOT environment variable.\n" if (!defined($EpocRoot));
	$EpocRoot =~ s-/-\\-go;	# for those working with UNIX shells
	die "ERROR: EPOCROOT must be an absolute path, " .
		"not containing a drive letter.\n" if ($EpocRoot !~ /^\\/);
	die "ERROR: EPOCROOT must not be a UNC path.\n" if ($EpocRoot =~ /^\\\\/);
	die "ERROR: EPOCROOT must end with a backslash.\n" if ($EpocRoot !~ /\\$/);
	die "ERROR: EPOCROOT must specify an existing directory.\n" if (!-d $EpocRoot);

	# The cpp needed a drive apparently
	my $cwd = Cwd::cwd();
	$cwd =~ /^(.)/;
	$drive = "$1:";

	my $fp0 = $0;
	$cwd =~ s/\//\\/g;
	$fp0 =~ s/\//\\/g;
	unless ($fp0 =~ /^([A-Za-z]:)?\\/) {
		if ($cwd =~ /\\$/) {
			$fp0 = "$cwd$fp0";
		} else {
			$fp0 = "$cwd\\$fp0";
		}
	}

	my @path = split(/\\/, $cwd);
	shift(@path);
	$toroot = ('..\\') x @path;
	$e32path = $fp0;
	$e32path =~ s/\\kernelhwsrv\\kernel\\eka\\rombuild\\rom\.pl$//i;
	$e32path =~ s/^[A-Za-z]://;
	$rombuildpath = $toroot."os\\kernelhwsrv\\kernel\\eka\\rombuild";
	$Epoc32Path = $toroot;
	$Epoc32Path =~ s/\\$//;
	$Epoc32Path .= $EpocRoot . "epoc32";
	$toolpath = "$Epoc32Path\\tools\\";
	push @INC, $toolpath;
	$BasePath = $toroot;
	$BasePath =~ s/\\$/$e32path\\/;
}

use E32Plat;
{
        Plat_Init($toolpath);
}

if ($debug) {
	print "EpocRoot = $EpocRoot\n";
	print "Epoc32Path = $Epoc32Path\n";
	print "drive = $drive\n";
	print "toolpath = $toolpath\n";
	print "toroot = $toroot\n";
	print "e32path = $e32path\n";
	print "rombuildpath = $rombuildpath\n";
	print "BasePath = $BasePath\n";
}

my $cppflags="-P -undef -traditional -lang-c++ -nostdinc -iwithprefixbefore $rombuildpath -I $rombuildpath -I $Epoc32Path ";

# Include variant hrh file defines when processing oby and ibys with cpp
# (Code copied from \\EPOC\master\cedar\generic\tools\romkit\tools\buildrom.pm -
# it used relative path to the working dir but we are using absolute path seems neater)
use E32Variant;
use Pathutl;
my $variantMacroHRHFile = Variant_GetMacroHRHFile();
if($variantMacroHRHFile){
	# Using absolute paths so must include drive letter otherwise cpp will fail
	# Also adding the directory containing the HRH file to main includes paths in
	# case it includes files
	my $variantFilePath = Path_Split('Path',$variantMacroHRHFile);
	$cppflags .= " -I $drive$variantFilePath -include $drive$variantMacroHRHFile";
}

if($param_count == 0 || $opts{'help'} || !$result) {
	usage();
	exit 0;
}

# Now check that the options we have make sense

checkopts();

if (!$quiet) {
	print "Starting directory: ", Cwd::cwd(), "\n";
	print <<EOF;
OPTIONS:
\tTYPE: $opts{'type'}
\tVARIANT: $opts{'variant'}
\tINSTRUCTION SET: $opts{'inst'}
\tBUILD: $opts{'build'}
\tMODULES: $opts{'modules'}
EOF
}

#Pick out the type file
my $skel;

if (-e "$opts{'type'}.oby") {
	$skel="$opts{'type'}.oby";
} elsif (-e "$rombuildpath\\$opts{'type'}.oby") {
	$skel="$rombuildpath\\$opts{'type'}.oby";
} else {
	die "Can't find type file for type $opts{'type'}, $!";
}

print "Using type file $skel\n" if !$quiet;

# If clean is specified, zap all the image and .oby files

if($opts{'clean'}) {
	unlink glob("*.img");
	unlink "rom.oby";
	unlink "rombuild.log";
	unlink glob("*.rofs");
	unlink "rofsbuild.log";
}

# Now pre-pre-process this file to point to the right places for .ibys
# Unfortunately cpp won't do macro replacement in #include strings, so
# we have to do it by hand

my $k = $opts{kerneltrace};

if ($opts{assp}=~/^m(\S+)/i) {
	$kbdir="kb$1";
	$kbdir="kbarm" if (lc $1 eq 'eig');
} else {
	$kbdir="kb$opts{assp}";
}
$single=1 if ($opts{assp}=~/^s(\S+)/i);

if ($single) {
	# Hackery to cope with old compiler
	if ($main eq 'MARM') {
		$smain='SARM';
	} else {
		$smain="S$main";
	}
} else {
	$smain=$main;
}

open(X, "$skel") || die "Can't open type file $skel, $!";
open(OUT, "> rom1.tmp") || die "Can't open output file, $!";

# First output the ROM name
print OUT "\nromname=$romname\n";
while(<X>) {
	s/\#\#ASSP\#\#/$opts{'assp'}/;
	s/\#\#VARIANT\#\#/$opts{'variant'}/;
	s/\#\#BUILD\#\#/$opts{'build'}/;
	s/\#\#MAIN\#\#/$main/;
	s/\#\#KMAIN\#\#/$kmain/;
	s/\#\#E32PATH\#\#/$e32path/;
	s/\#\#BASEPATH\#\#/$BasePath/;
	s/\#\#EUSERDIR\#\#/$euserdir/;
	s/\#\#ELOCLDIR\#\#/$elocldir/;
	s/\#\#KBDIR\#\#/$kbdir/;
	print OUT;
}

close X;
close OUT;

# Use cpp to pull in include chains and replace defines

my $defines = "";
$defines .= "-D MAIN=$main ";
$defines .= "-D KMAIN=$kmain ";
$defines .= "-D EUSERDIR=$euserdir ";
$defines .= "-D ELOCLDIR=$elocldir ";
$defines .= "-D E32PATH=$e32path ";
$defines .= "-D BASEPATH=$BasePath ";
$defines .= "-D EPOCROOT=$EpocRoot ";
$defines .= "-D SMAIN=$smain " if $smain;

foreach (@{$opts{'define'}}) {
	my @array=split(/,/,$_);
	foreach (@array) {
		$defines.="-D ".uc $_." ";
		$pagedCode = 1 if $_ eq 'PAGED_CODE';
		}
	}

if ($opts{'modules'}) {
	my @array=split(/,/,$opts{'modules'});
	foreach (@array) {
		$defines.="-D ".uc $_." ";
		}
	}

foreach (keys %opts) {
	next if ($_ eq 'name');
	next if ($_ eq 'modules');
	next if ($_ eq 'zip');
	next if ($_ eq 'symbol');
	next if ($_ eq 'kerneltrace');
	next if ($_ eq 'define');
	$defines.="-D ".uc $_."=".$opts{$_}." ";
	$defines.="-D ".uc $_."_".$opts{$_}." ";
}

$defines.="-D SINGLE " if ($single);

sub IsRVCTBuild($) {
    my ($build)=@_;
    return 1 if ($build =~ /^ARMV/i);
	my @customizations = Plat_Customizations('ARMV5');
	return 1 if (grep /$build/, @customizations);
	return 0;
}

$defines.="-D RVCT " if (IsRVCTBuild($main));

print "Using defines $defines\n" if !$quiet;

my $ret=1;
my $cppcmd;
if($opts{'build'}=~/^u/i) {
	# Unicode build
	$cppcmd = "cpp $cppflags -D UNICODE $defines rom1.tmp rom2.tmp";
} else {
	$cppcmd = "cpp $cppflags $defines rom1.tmp rom2.tmp";
}
print "Executing CPP:\n\t$cppcmd\n" if $debug;
$ret = system($cppcmd);
die "ERROR EXECUTING CPP\n" if $ret;

# Zap any ## marks REMS or blank lines

cleanup("rom2.tmp", "rom3.tmp", $k);

# scan tmp file and generate auxiliary files, if required
open TMP, "rom3.tmp" or die("Can't open rom3.tmp\n");
my $line;
while ($line=<TMP>)
	{
	if ($line=~/\s*gentestpaged/i) {
		genfile("paged");	}
	if ($line=~/\s*gentestnonpaged/i) {
		genfile("nonpaged");	}
	}

parsePatchData("rom3.tmp", "rom4.tmp");

# break down the oby file into rom, rofs, extensions and smr oby files

my $oby_index =0;
my $dumpfile="rom.oby";
my $rofs=0;
my $smr=0;
my $extension=0;
my $corerofsname="";
my $smrname="";
open DUMPFILE, ">$dumpfile" or die("Can't create $dumpfile\n");
my $line;
open TMP, "rom4.tmp" or die("Can't open rom4.tmp\n");
while ($line=<TMP>)
	{
	if ($line=~/^\s*rofsname/i)
		{
		close DUMPFILE;							# close rom.oby or previous rofs#/extension#.oby
		$oby_index=1;
		$corerofsname=$line;
		$corerofsname =~ s/rofsname\s*=\s*//i;		# save core rofs name
		$corerofsname =~ s/\s*$//g; 			# remove trailing \n
		unlink $corerofsname || print "unable to delete $corerofsname";
		my $dumpfile="rofs".$rofs.".oby";
		$rofs++;
		open DUMPFILE, ">$dumpfile" or (close TMP and die("Can't create $dumpfile\n"));
		}

	if ($line=~/^\s*imagename/i)
		{
		close DUMPFILE;							# close rom.oby or previous rofs#/extension#.oby
		$smrname=$line;
		$smrname =~ s/imagename\s*=\s*//i;		# save smr name
		$smrname =~ s/\s*$//g; 			# remove trailing \n
		unlink $smrname || print "unable to delete $smrname";
		my $dumpfile="smr".$smr.".oby";
		$smr++;
		open DUMPFILE, ">$dumpfile" or (close TMP and die("Can't create $dumpfile\n"));
		}

	if ($line=~/^\s*coreimage/i)
		{
		close DUMPFILE;							# close rofs.oby
		if ($oby_index ne 1) {
			close TMP;
			die "Must specify ROFS image before ROFS extension\n";
		}
		my $name=$line;
		$name =~ s/coreimage\s*=\s*//i;		# read core rofs name
		$name =~ s/\s*$//g; 			# remove trailing \n
		if ($name ne $corerofsname) {
			close TMP;
			die "This extension does not relate to previous ROFS\n";
		}
		$oby_index=33;						# open window
		my $dumpfile="extension".$extension.".oby";
		$extension++;
		open DUMPFILE, ">$dumpfile" or (close TMP and die("Can't create $dumpfile\n"));
		}

	if ($line=~/^\s*extensionrofs/i)
		{
		$oby_index=3 if ($oby_index eq 2);
		}

	if (($oby_index eq 2) && !($line=~/^\s*$/)) {
		close TMP;
		die "Bad ROFS extension specification\n";
	}
	print DUMPFILE $line;
	$oby_index=2 if ($oby_index eq 33);		# close window
	}
close DUMPFILE;
close TMP;

# For paged roms that use rofs, move all data= lines in rom which are not 'paging_unmovable' to rofs, so that paged ram-loaded code
# is automatically put into rofs
rename('rom.oby', 'rom4.tmp') || die;

open(IN, 'rom4.tmp') || die "Can't read rom4.tmp";
open(ROM, '>rom.oby') || die "Can't write to rom.oby";

if ($oby_index >= 1 && $pagedCode)	{
	open(ROFS, '>>rofs0.oby') || die "Can't append to rofs0.oby";
}

while ($line=<IN>)
{
	if(($oby_index >= 1) && ($pagedCode) && ($line=~/^\s*data\s*=/) && !($line=~/\.*paging_unmovable\s*/)) {
		print ROFS $line;
	}
	else {
		$line=~s/paging_unmovable//;
		print ROM $line;
	}
}

close IN;
close ROM;

if ($oby_index >= 1 && $pagedCode)	{
	close ROFS;
}
	unlink 'rom4.tmp';

my $flags;

foreach (@{$flags{$opts{'assp'}}}) {
	$flags.=" -$_";
}

if($opts{'noheader'}) {
	$flags.=" -no-header";
}

if($opts{'compress'}) {
	$flags.=" -compress";
}

my $builder = $opts{'rombuilder'};
$builder = "rombuild" unless ($builder);



print "$builder $flags -type-safe-link -S rom.oby 2>&1\n\n";

open(Y, "$builder $flags -type-safe-link -S rom.oby 2>&1 |") || 
	die "Can't start $builder command, $!";

my $nerrors=0;
my $nwarnings=0;

while(<Y>) {
	my $error=(/^error:/i);
	my $warning=(/^warning:/i);
	print if ($error or $warning or !$quiet);
	$nerrors++ if ($error);
	$nwarnings++ if ($warning);
}

print "\nGenerated .oby file is rom.oby\n" if !$quiet;
print "\nGenerated image file is $romname\n" if (!$nerrors);

my$rerrors;
my $rofsbuilder;
if ($rofs) {
	$rofsbuilder = $opts{'rofsbuilder'};
	$rofsbuilder = "rofsbuild" unless ($rofsbuilder);
	for(my $i=0;$i<$rofs;++$i) {
		print "Executing $rofsbuilder on main rofs\n" if !$quiet;
		my $image="rofs".$i.".oby";
		system("$rofsbuilder $image");
		if ($? != 0)
			{
			print "$rofsbuilder $image returned $?\n";
			$rerrors++;
			}
		rename "rofsbuild.log", "rofs$i.log"
		}
}

if ($rofs and $extension) {
	for(my $i=0;$i<$extension;++$i) {
		print "Executing $rofsbuilder on extension rofs\n" if !$quiet;
		my $image="extension".$i.".oby";
		system("$rofsbuilder $image");
		if ($? != 0)
			{
			print "$rofsbuilder $image returned $?\n";
			$rerrors++;
			}
		rename "rofsbuild.log", "extension$i.log"
		}
}

if ($smr) {
	$rofsbuilder = $opts{'rofsbuilder'};
	$rofsbuilder = "rofsbuild" unless ($rofsbuilder);
	for(my $i=0;$i<$smr;++$i) {
		print "Executing $rofsbuilder on smr partition\n" if !$quiet;
		my $image="smr".$i.".oby";
		system("$rofsbuilder -smr=$image");
		if ($? != 0)
			{
			print "$rofsbuilder -smr=$image returned $?\n";
			$rerrors++;
			}
		rename "rofsbuild.log", "smr$i.log"
		}
}

if ($nerrors) {
	print "\n\n Errors found during $builder!!\n\nLeaving tmp files\n";
} elsif ($nwarnings) {
	print "\n\n Warnings during $builder!!\n\nLeaving tmp files\n";
} elsif ($rerrors) {
	print "\n\n Errors during $rofsbuilder!!\n\nLeaving tmp files\n";
} else {
	unlink glob("*.tmp") if !$debug;
}
if ($opts{zip} or $zip{$opts{assp}}) {
	my $zipname=$romname;
	$zipname =~ s/\.(\w+)$/\.zip/i;
	unlink $zipname;
	system("zip $zipname $romname");
}
if ($opts{symbol}) {
	my $logname=$romname;
	$logname =~ s/\.(\w+)$/\.log/i;
	my $obyname=$romname;
	$obyname =~ s/\.(\w+)$/\.oby/i;
	unlink $logname;
	unlink $obyname;
	system("rename rombuild.log $logname");
	system("rename rom.oby $obyname");
	system("maksym $logname");
}

if ($nerrors || $nwarnings || $rerrors) {
	exit 4;
}	
	
exit 0;


################################ Subroutines  ##################################

sub usage {
	print <<EOT;

rom <options>

Generate a rom image for the specified target, along with a rom.oby file
that can be fed to (a) rombuild to regenerate the image.

The following options are required:
  --variant=<variant>         e.g. --variant=assabet
  --inst=<instruction set>    e.g. --inst=arm4
  --build=<build>             e.g. --build=udeb
  --type=<type of rom>  
         tshell for a text shell rom
         e32tests for a rom with e32tests
         f32tests for rom with f32tests
         alltests for all the tests

The following are optional:
  --name=<image name>               Give image file specified name
  --noheader                        Pass -no-header option on to rombuild
  --help                            This help message.
  --clean                           Remove existing generated files first
  --quiet                           Be less verbose
  --modules=<comma separated list>  List of additional modules for this ROM
  --define=<comma separated list>   List of CPP macros to define

Options may be specified as a short abbreviation 
e.g. -b udeb instead of --build udeb

EOT
}

sub cleanup($$$) {
	my ($in, $out, $k) = @_;
	my ($line, $lastblank);

	open(OUTPUT_FILE, "> $out") or die "Cannot open $out for output";
	open(INPUT_FILE, "< $in") or die "Cannot open for $in input";
  
	while ($line=<INPUT_FILE>) {
		$line=~s/##//g;

		# file=\epoc32\...  ==> file=%EPOCROOT%\epoc32\...
		$line =~ s/(=\s*)\\epoc32/\1${EpocRoot}Epoc32/i;

		# Now compress blank lines down to one
	
		if($line=~/^\s*$/) {
			if($lastblank) {
				# Do nothing
			} else {
				# This is the first blank line
				$lastblank=1;
				print OUTPUT_FILE $line;
			}
		} else {
			# Not blank
			$lastblank=0;
			if ($k and $line=~/^\s*kerneltrace/i) {
				$line = "kerneltrace $k\n";
			}
			print OUTPUT_FILE $line if !($line=~/^\s*REM\s+/i);
		}
	}
	close(INPUT_FILE);
	close(OUTPUT_FILE);
}

sub IsSmp($) {
	my %SmpKernelDirs=(
		'x86smp' => 1,
		'x86gmp' => 1,
		'arm4smp' => 1,
		'armv4smp' => 1,
		'armv5smp' => 1
	);

	my ($kdir) = @_;
	return $SmpKernelDirs{lc $kdir};
}

sub checkopts {
	unless($opts{variant}) { die "No Variant specified"; }
	$opts{'build'}="UDEB" unless($opts{'build'});
	$opts{'type'}="TSHELL" unless($opts{'type'});
	$opts{'inst'}="ARM4" unless($opts{'inst'});

	my $additional;
	if ($opts{'modules'}) {
		$additional="_".$opts{modules};
		$additional=~ s/,/_/ig;
	}
	my $build=lc $opts{build};
	my $inst=uc $opts{'inst'};
	if ($inst eq "MARM") {
		# Hackery to cope with old compiler
		$main="MARM";
		$euserdir="MARM";
		$elocldir="MARM";
	}
	else {
		$main=$inst;
		if ($main eq "THUMB") {
			$euserdir="ARMI";
		} else {
			$euserdir=$main;
		}
		if ($main eq "ARMI" or $main eq "THUMB") {
			$elocldir="ARM4";
		} else {
			$elocldir=$main;
		}
	}
	$kmain = $opts{'xabi'};
	$kmain = $main unless ($kmain);
	if (IsSmp($kmain)) {
		$euserdir = $kmain;
	}
	if ($opts{name}) {
		$romname=$opts{name};
	} else {
		$romname=uc($opts{variant}.$additional.$main);
		if ($build=~/^\w*DEB$/i) {
			$romname.='D';
		}
		$romname.='.IMG';
	}
}

sub lookupFileInfo($$)
{
	my ($infile, $fullname) = @_;

	my ($name, $ext) = $fullname =~ /^(.+)\.(\w+)$/ ? ($1, $2) : ($fullname, undef);

	open TMP, $infile or die("Can't open $infile\n");
	while(<TMP>)
	{
		$_ = lc;
		if(/^\s*(\S+)\s*=\s*(\S+)\s+(\S+)/i)
		{
			my ($src, $dest) = ($2, $3);

			my $destFullname = $dest =~ /^.*\\(.+)$/ ? $1 : $dest;
			my ($destName, $destExt) = $destFullname =~ /^(.+?)\.(\w+)$/ ? ($1, $2) : ($destFullname, undef);

			if ($destName eq $name && (!$ext || $ext eq $destExt))
			{
				close TMP;
				return ($src, $dest);
			}
		}
	}

	die "patchdata: Can't find file $fullname\n";
}

sub lookupSymbolInfo($$)
{
	my ($file, $name) = @_;

	open TMP, $file or die "Can't read $file\n";

	# ignore local symbols.
	while (<TMP> !~ /Global Symbols/) { }

	while (<TMP>)
	{
		if (/^\s*(\S+)\s+(\S+)\s+data\s+(\S+)/i)
		{
			my ($symbol, $addr, $size) = ($1, $2, $3);
			if ($symbol eq $name)
			{
				close TMP;
				return ($addr, $size);
			}
		}

		# This is a quick fix for RVCT 3.1, which uses the text "(EXPORTED)"
		# in the map file. Here is an example:
		#
		# KHeapMinCellSize (EXPORTED) 0x0003d81c Data 4 mem.o(.constdata)
		#
		elsif (/^\s*(\S+)\s+\(exported\)\s+(\S+)\s+data\s+(\S+)/i)
		{
			my ($symbol, $addr, $size) = ($1, $2, $3);
			if ($symbol eq $name)
			{
				close TMP;
				return ($addr, $size);
			}
		}
	}

	die "patchdata: Can't find symbol $name\n";
}

sub parsePatchData($$)
{
	my ($infile, $outfile) = @_;

	open IN, $infile or die("Can't read $infile\n");
	open OUT, ">$outfile" or die("Can't write $outfile\n");

	my $line;
	while($line = <IN>)
	{
		if ($line =~ /^\s*patchdata\s+(.+?)\s*$/i)
		{
			if ($1 !~ /(\S+)\s*@\s*(\S+)\s+(\S+)\s*$/)
			{
				die "Bad patchdata command: $line\n";
			}

			my ($file, $symbol, $value) = (lc $1, $2, $3);
			my ($srcFile, $destFile) = lookupFileInfo($infile, $file);
			my ($index, $elementSize) = (undef, undef);
			if ($symbol =~ s/:(\d+)\[(\d+)\]$//)
			{
				($index, $elementSize) = ($2, $1);
				$index = hex($index) if $index =~ /^0x/i;
			}

			if ($srcFile =~ /\\armv5(smp)?\\/i)
			{
				my ($symbolAddr, $symbolSize) = lookupSymbolInfo("$srcFile.map", $symbol);

				my $max;
				if (defined($index))
				{
					my $bytes;
					$bytes = 1, $max = 0xff       if $elementSize ==  8;
					$bytes = 2, $max = 0xffff     if $elementSize == 16;
					$bytes = 4, $max = 0xffffffff if $elementSize == 32;
					die("patchdata: invalid element size $elementSize: $line\n") unless defined($bytes);

					if ($bytes > 1 && (($symbolSize & ($bytes-1)) != 0))
					{
						die("patchdata: unexpected symbol size $symbolSize for array $symbol ($elementSize-bit elements)\n");
					}

					if ($index >= int($symbolSize / $bytes))
					{
						die("patchdata: index $index out of bounds for $symbol of $symbolSize bytes ($elementSize-bit elements)\n");
					}

					$symbolAddr = hex($symbolAddr) if $symbolAddr =~ /^0x/i;
					$symbolAddr += $index * $bytes;
					$symbolAddr = sprintf("0x%x", $symbolAddr);

					$symbolSize = $bytes;
				}
				elsif ($symbolSize == 1) { $max = 0xff; }
				elsif ($symbolSize == 2) { $max = 0xffff; }
				elsif ($symbolSize == 4) { $max = 0xffffffff; }
				else { die "patchdata: Unexpected symbol size $symbolSize for $symbol\n"; }

				$value = hex($value) if $value =~ /^0x/i;
				if ($value > $max)
				{
					print("Warning:  Value overflow of $symbol\n");
					$value &= $max;
				}					
				$value = sprintf("0x%08x", $value);

				$line = "patchdata $destFile addr $symbolAddr $symbolSize $value\n";
			}
			else
			{
				$line = "";
			}

		}

		print OUT $line;
	}

	close IN;
	close OUT;
}

sub genfile {
	my $count=0;
	if($_[0] eq 'paged') {
		my $file='gentestpaged.txt';
		unlink $file;
		open(OUTFILE, ">$file") or die "Can't open output file, $!";
		for(my $i=0;$i<50000;++$i) {
			if(($i >5) && ($i % 40 ==0)) {
			print OUTFILE "\n";
			$count++;
			} 
			if(($i+$count) % 5 ==0) {
			print OUTFILE "SATOR ";
			}
			if(($i+$count) % 5 ==1) {
			print OUTFILE "AREPO ";
			}
			if(($i+$count) % 5 ==2) {
			print OUTFILE "TENET ";
			}
			if(($i+$count) % 5 ==3) {
			print OUTFILE "OPERA ";
			}
			if(($i+$count) % 5 ==4) {
			print OUTFILE "ROTAS ";
			}
		}
	} else {
		my $file='gentestnonpaged.txt';
		unlink $file;
		open(OUTFILE, ">$file") or die "Can't open output file, $!";
		for(my $i=0;$i<20000;++$i) {
			if(($i >5) && ($i % 40 ==0)) {
			print OUTFILE "\n";
			$count++;
			} 
			if(($i+$count) % 4 ==0) {
			print OUTFILE "STEP ";
			}
			if(($i+$count) % 4 ==1) {
			print OUTFILE "TIME ";
			}
			if(($i+$count) % 4 ==2) {
			print OUTFILE "EMIT ";
			}
			if(($i+$count) % 4 ==3) {
			print OUTFILE "PETS ";
			}
		}
	}
}

__END__

# Tell emacs that this is a perl script even 'though it has a .bat extension
# Local Variables:
# mode:perl
# tab-width:4
# End:

