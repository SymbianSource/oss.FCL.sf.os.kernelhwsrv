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
# UnicodeCompositionEx
# adds composition exclusion information to unicode data
#
# Added as a new field:
# Symbian:<excluded-from-composition>
# where <excluded-from-composition> is E or null.
#
# Usage:
# perl -w UnicodeAddComposeEx.pl CompositionExclusions.txt < <Unicode-data-file>

use strict;

if (scalar(@ARGV) != 1)
	{
	print (STDERR "Usage:\nperl -w UnicodeAddComposeEx.pl CompositionExclusions.txt < <Unicode-data-file>\n");
	exit 1;
	}

open(EXCLUSIONS, $ARGV[0]) or die("Could not open file $ARGV[0]\n");

my $lineNo = 0;
my %Excluded = ();
while (<EXCLUSIONS>)
	{
	$lineNo++;
	# try to parse the line if there is some non-whitespace before the comment
	if (!/^[ \t]*([#].*)?$/)
		{
		/^[ \t]*([0-9A-Fa-f]{4,6})[ \t]*([#].*)?$/ or die("Did not understand line $lineNo of $ARGV[0]");
		my $code = hex($1);
		die ("Value $code outside Unicode range at line $lineNo of $ARGV[0]")
			unless ($code < 0x110000);
		$Excluded{$code} = 1;
		#printf("Excluding %X because it is in the exclusion list\n", $code);
		}
	}

close EXCLUSIONS;
# This is a two-pass operation, so we must store the lines ready for output later.
my @DataFileLines = ();
my %DataFileLineCodes = ();
# The first pass will collect all the relevant data:
# The first character of the decomposition if there is more than one
my %FirstOfDecompositionString = ();
# The singleton decomposition if it is a singleton
my %SingletonDecomposition = ();
# The decompositions tag, if any
my %DecompTag = ();
# The combining class
my %CombiningClass = ();
# We will also be marking all singleton decompositions for exclusion
$lineNo = 0;
while (my $line = <STDIN>)
	{
	chomp $line;
	$DataFileLines[$lineNo] = $line;
	$lineNo++;
	# Split into fields: make sure trailing null strings are not
	# deleted by adding a dummy final field
	my @attribute = split(/;/, $line.';dummy');
	# Delete the dummy field
	pop @attribute;

	if (scalar(@attribute) == 15)
		{
		my $code = $attribute[0];
		die("First attribute '$code' not a valid Unicode codepoint at line $lineNo")
			unless $code =~ /^1?[0-9a-fA-F]{4,5}$/;
		$code = hex($code);
		my $combiningClass = $attribute[3];
		die("Fourth attribute '$combiningClass' is not a valid Unicode combining class at line $lineNo")
			unless (0 <= $combiningClass && $combiningClass < 256);
		my $decompositionString = $attribute[5];
		die ("Sixth attribute '$decompositionString' is not a valid decomposition string at line $lineNo")
			unless ($decompositionString =~ /^(<.*>)?[0-9a-fA-F \t]*$/);
		my @decomposition = split(/[ \t]+/, $decompositionString);
		if (@decomposition && $decomposition[0] =~ /^<.*>$/)
			{
			$DecompTag{$code} = shift @decomposition;
			}
		if (scalar(@decomposition) == 1)
			{
			# We want to exclude codes such as these, with a singleton
			# decomposition mapping, but at the moment we don't know if the
			# character mapped to has a decomposition mapping, so we will
			# defer this to another stage.
			die("Decomposition $decomposition[0] not understood at line $lineNo")
				unless ($decomposition[0] =~ /^[0-9A-Fa-f]+$/);
			$SingletonDecomposition{$code} = hex($decomposition[0]);
			}
		elsif (1 < scalar(@decomposition))
			{
			die("Decomposition $decomposition[0] not understood at line $lineNo")
				unless ($decomposition[0] =~ /^[0-9A-Fa-f]+$/);
			$FirstOfDecompositionString{$code} = hex($decomposition[0]);
			}
		$CombiningClass{$code} = $combiningClass;
		$DataFileLineCodes{$lineNo-1} = $code;
		}
	elsif ($line !~ /^[ \t]*$/)
		{
		die 'Do not understand line '.$lineNo;
		}
	}

# Each code that has a decomposition string longer than one character
# where the first character has non-zero combining class is excluded
foreach my $code (keys %FirstOfDecompositionString)
	{
	my $decomp = $FirstOfDecompositionString{$code};
	if (exists($CombiningClass{$decomp}))
		{
		if ($CombiningClass{$decomp} != 0)
			{
			$Excluded{$code} = 1;
			#printf("Excluding %X because its decomposition starts with a non-starter(%X)\n", $code, $decomp);
			}
		}
	}

# Each code that has a singleton decomposition string may be excluded if
# that code has only a singleton mapping itself.
foreach my $code (sort (keys %SingletonDecomposition))
	{
	my $mapsTo = $code;
	while (exists $SingletonDecomposition{$mapsTo} && !exists $DecompTag{$code})
		{
		$mapsTo = $SingletonDecomposition{$mapsTo};
		}
	if (!exists $FirstOfDecompositionString{$mapsTo})
		{
		#printf("Excluding %X because its decomposition is a singleton(%X)\n", $code, $mapsTo);
		$Excluded{$code} = 1;
		}
	}

# Now we output the file with the extra filed appended to each line
for(my $i = 0; $i != scalar(@DataFileLines); $i++)
	{
	print $DataFileLines[$i];
	if (exists($DataFileLineCodes{$i}))
		{
		print ';Symbian:';
		if (exists($Excluded{ $DataFileLineCodes{$i} }))
			{
			print 'E';
			}
		}
	print "\n";
	}
