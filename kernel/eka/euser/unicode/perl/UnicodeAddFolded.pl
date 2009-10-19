# Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# Adds folding information to Unicode data
# Added as the third field after the 'Symbian:' marker in the following format:
# Symbian:<grapheme-role>;<excluded-from-composition>;<folded-form>
# where <folded-form> is null or a sequence of hex unicode values
# separated by spaces representing the folded form of the character.
# Usage:
# perl -w UnicodeAddFolded.pl CaseFolding.txt < <output-of-UnicodeCompositionEx>
# 
#

use strict;

if (scalar(@ARGV) != 1)
	{
	print (STDERR "Usage:\nperl -w UnicodeAddFolded.pl CaseFolding.txt < <output-of-UnicodeCompositionEx>\n");
	exit 1;
	}

open(FOLDING, $ARGV[0]) or die("Could not open file $ARGV[0]\n");

my %Fold = ();
my %MappingLine = ();
my $lineNo = 0;
while (<FOLDING>)
	{
	$lineNo++;
	my ($line, $comment) = split(/#/, $_, 2);
	if ($line =~ /^[ \t]*(1?[0-9a-fA-F]{4,5});[ \t]*([LEICSFT]);[ \t]*([0-9a-fA-F][0-9a-fA-F \t]*);[ \t]*$/)
		{
		my $code = hex($1);
		my $type = $2;
		my $folded = $3;
		# We'll deal with Turkic mappings with our own hack.
		# F = Full mappings (fold is longer than one character)
		# T = I = Turkic mapping
		if ($type !~ /[FTI]/ && $folded !~ /[ \t]/)
			{
			die ("$code has two mappings: lines $MappingLine{$code} and $lineNo.")
				if (exists $Fold{$code});
			$Fold{$code} = $folded;
			$MappingLine{$code} = $lineNo;
			}
		}
	elsif ($line !~ /^[ \t]*$/)
		{
		die ("Did not understand line $lineNo of $ARGV[0]");
		}
	}

close FOLDING;

# Turkic hack:
# Map dotted capital I and dotless small I to lower case i.
# This makes all the 'i's fold the same, which isn't very nice for Turkic
# languages, but it at least gives us behaviour consistent across locales
# which does at least map dotted I, and i to the same value, as well
# as mapping I and dotless i to the same value, and mapping I and i
# to the same value.
$Fold{0x49} = '0069';
$Fold{0x130} = '0069';
$Fold{0x131} = '0069';

$lineNo = 0;
while (my $line = <STDIN>)
	{
	chomp $line;
	$lineNo++;
	# Split into fields: make sure trailing null strings are not
	# deleted by adding a dummy final field
	my @attribute = split(/;/, $line.';dummy');
	# Delete the dummy field
	pop @attribute;
	die ("Line $lineNo is missing 'Symbian:' entries. Has UnicodeCompositionEx been run?")
		if (scalar(@attribute) == 15);
	if (scalar(@attribute) == 16)
		{
		die ("Line $lineNo is missing 'Symbian:' entries. Has UnicodeCompositionEx been run?")
			if ($attribute[15] !~ /^[ \t]*symbian:/i);
		my $code = $attribute[0];
		die("First attribute '$code' not a valid Unicode codepoint at line $lineNo")
			unless $code =~ /^1?[0-9a-fA-F]{4,5}$/;
		$code = hex($code);
		$attribute[16] = exists $Fold{$code}? $Fold{$code} : '';
		print join(';', @attribute);
		}
	elsif ($line !~ /^[ \t]*$/)
		{
		die 'Do not understand line '.$lineNo;
		}
	else
		{
		print $line;
		}
	print "\n";
	}
