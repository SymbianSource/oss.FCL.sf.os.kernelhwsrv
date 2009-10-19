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
# UnicodeMaxDecompose.pl
#
# Adds maximal decompositions of the character and maximal decompositions of
# its folded varient to the Unicode data.
#
# Added as the fourth field after the 'Symbain:' marker in the following format:
#
# Symbian:<grapheme-role>;<excluded>;<folded>;<max-decomposition>;<folded-decomposition>
# where each of <max-decomposition> and <folded-decomposition> are strings
# of hex numbers separated by spaces, representing the complete decomposition
# of the character and its folded equivalent respectively.
#
# Usage:
# perl -w UnicodeMaxDecompose.pl < <output-of-UnicodeAddFolded>

use strict;

if (scalar(@ARGV) != 0)
	{
	print (STDERR "Usage:\nperl -w UnicodeMaxDecompose.pl < <output-of-UnicodeAddFolded>\n");
	exit 1;
	}

my %StatedDecomposition = ();
my %CompleteDecomposition = ();

sub Decompose
	{
	my ($code) = @_;
	return unless exists $StatedDecomposition{$code};
	my $stated = $StatedDecomposition{$code};
	delete $StatedDecomposition{$code};
	my @complete = ();
	foreach my $hexelt ( split(' ', $stated) )
		{
		if ($hexelt)
			{
			Decompose($hexelt);
			if (exists $CompleteDecomposition{$hexelt})
				{
				push @complete, $CompleteDecomposition{$hexelt};
				}
			else
				{
				push @complete, $hexelt;
				}
			}
		}
	$CompleteDecomposition{$code} = join(' ', @complete);
	}

my %Folded = ();
my %LineToCode = ();
my @RawLine = ();

my $lineNo = 0;
while (my $line = <STDIN>)
	{
	chomp $line;
	$lineNo++;
	# Split into fields: make sure trailing null strings are not
	# deleted by adding a dummy final field
	my @attribute = split(/;/, $line.';dummy');
	# Delete the dummy field
	pop @attribute;
	die ("Line $lineNo is missing 'Symbian:' entries. Has UnicodeAddFolded been run?")
		if (scalar(@attribute) == 16);
	if (scalar(@attribute) == 17)
		{
		die ("Line $lineNo is missing 'Symbian:' entries. Has UnicodeAddFolded been run?")
			if ($attribute[15] !~ /^[ \t]*symbian:/i);
		my $code = $attribute[0];
		die("First attribute '$code' not a valid Unicode codepoint at line $lineNo")
			unless ($code =~ /^1?[0-9a-fA-F]{4,5}$/ && hex($code) < 0x110000);
		my $decomposition = $attribute[5];
		die("Decomposition '$decomposition' at line $lineNo is not a valid Unicode decomposition.")
			unless $decomposition =~ /^[ \t]*(<.*>[ \t]*[0-9a-fA-F])?[0-9a-fA-F \t]*$/;
		my $folded = $attribute[16];
		die ("'$folded' not a valid string of hex values at line $lineNo.")
			unless $folded =~ /[0-9a-fA-F \t]*/;
		# Store all decompositions that  have no tag and at least one value
		if ($decomposition =~ /^[ \t]*[0-9a-fA-F]/)
			{
			$StatedDecomposition{$code} = $decomposition;
			}
		if ($folded =~ /[0-9a-fA-F]/)
			{
			$Folded{$code} = $folded;
			}
		$LineToCode{$lineNo-1} = $code;
		}
	elsif ($line !~ /^[ \t]*$/)
		{
		die 'Do not understand line '.$lineNo;
		}
	$RawLine[$lineNo-1] = $line;
	}

# Completely decompose all strings in the %StatedDecomposition
foreach my $code (keys %StatedDecomposition)
	{
	Decompose($code);
	}

# Now decompose all the folded versions
foreach my $code (keys %Folded)
	{
	my @result = ();
	foreach my $hexelt (split(' ', $Folded{$code}))
		{
		if (exists $CompleteDecomposition{$hexelt})
			{
			push @result, split(' ', $CompleteDecomposition{$hexelt});
			}
		else
			{
			push @result, $hexelt;
			}
		}
	$Folded{$code} = join(' ', @result);
	}

# Now output all the results
for (my $i = 0; $i != scalar(@RawLine); $i++)
	{
	print $RawLine[$i];
	if (exists $LineToCode{$i})
		{
		my $code = $LineToCode{$i};
		print ';';
		my $decomp = '';
		$decomp = $CompleteDecomposition{$code}
			if exists $CompleteDecomposition{$code};
		print $decomp.';';
		if (exists $Folded{$code})
			{
			print $Folded{$code}
			}
		else
			{
			# If there is no folded value, but there is a decomposition
			# sequence, the character must fold to the decomposition
			# sequence too.
			print $decomp;
			}
		}
	print "\n";
	}
