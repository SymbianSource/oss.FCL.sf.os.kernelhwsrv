#
# Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - initial contribution.
#
# Contributors:
#
# Description: 
# Case Equivalence
# Given the unicode data file, work out the case equivalence classes
# i.e. the equivalence classes for the transitive closure of ~ defined as
# follows:
# a~b if Uppercase(a) == b || Lowercase(a) == b || Titlecase(a) == b
# Usage: perl CaseEquivalence <UnicodeData.txt
#

use strict;
my @Name = ();
my @Upper = ();
my @Lower = ();
my @Title = ();
# $DecompositionValue[$code] is undefined if $code has no decomposition
# sequence, if it has a single value decomposition sequence, then this is it,
# if it has a longer sequence, the value is -1
my @DecompositionValue = ();
# 1 for each code that has a differently-cased version,
# 2 for each code that is a lower-case version of something else.
my %Codes = ();
my %CaseClass = ();

# Command-line options
my $OptionOutputTrie = 1;
my $OptionOutputForwardMapping = 0;
my $OptionOutputReverseMapping = 0;
my $OptionIgnoreOneToOneReverseMappings = 0;
my $OptionIncludeExtraMappings = 1;

foreach my $optionString (@ARGV)
	{
	if ($optionString =~ m![/-]o[tfrm]!)
		{
		$OptionOutputTrie = 0;
		my $option = substr($optionString, 2, 1);
		if ($option eq 'f')
			{
			$OptionOutputForwardMapping = 1;
			}
		elsif ($option eq 'r')
			{
			$OptionOutputReverseMapping = 1;
			}
		elsif ($option eq 'm')
			{
			$OptionOutputReverseMapping = 1;
			$OptionIgnoreOneToOneReverseMappings = 1;
			}
		else
			{
			$OptionOutputTrie = 1;
			}
		}
	elsif ($optionString =~ m![/-]s!)
		{
		$OptionIncludeExtraMappings = 0;
		}
	else
		{
		print STDERR "Usage: perl CaseEquivalence [-o<mapping>] [-s]\nusing standard input and output streams.\n";
		print STDERR "<mapping> is one of:\nt: output C++ code giving a trie for folding case. Each trie level is 4 bits.\n";
		print STDERR "f: Give a list of all codes that need mapping and what they map to.\n";
		print STDERR "r: Give a list of all codes are mapped to and what maps to them.\n";
		print STDERR "m: Give a list of all codes are mapped to by more than one code.\n";
		print STDERR "\nOmitting the -s option adds the following case-equivalence:\nSpace = Non-breaking space\n";
		exit;
		}
	}

# set a code as being part of a non-unitary case-equivalence class.
sub add
	{
	my ($addition) = @_;
	if (!$Codes{$addition})
		{
		$Codes{$addition} = 1;
		}
	}

# make a code point to its final case varient
sub chaseDown
	{
	my ($codeVal) = @_;
	my $class = $codeVal;
	while ($CaseClass{$class})
		{
		$class = $CaseClass{$class};
		}
	$CaseClass{$codeVal} = $class unless $codeVal == $class;
	return $class;
	}

# link two codes together as being part of the same case-equivalence class
sub makeEquivalent
	{
	my ($left, $right) = @_;
	if (!$left || !$right)
		{
		return;
		}
	$left = chaseDown($left);
	$right = chaseDown($right);
	if ($Codes{$left} < $Codes{$right})
		{
		$CaseClass{$left} = $right;
		return;
		}
	if ($Codes{$right} < $Codes{$left})
		{
		$CaseClass{$right} = $left;
		return;
		}
	if ($left < $right)
		{
		$CaseClass{$right} = $left;
		return;
		}
	if ($right < $left)
		{
		$CaseClass{$left} = $right;
		return;
		}
	# $left == $right.. do nothing
	return;
	}

# Link possibly unmentioned codes together. The first one is considered lower-case
sub addEquivalenceClass
	{
	my ($lower, @rest) = @_;
	$Codes{$lower} = 2;
	foreach my $one (@rest)
		{
		$Codes{$one} = 1;
		makeEquivalent($lower, $one);
		}
	}

# Firstly we read in the data
while(<STDIN>)
	{
	my @line = split('#', $_, 1);
	my @fields = split(/;/, $line[0]);
	my @decomposition = split(' ', $fields[5]);
	if (1 < scalar(@fields))
		{
		my $codeVal = hex($fields[0]);
		# if the character has a non-compatibility decomposition sequence, record this fact.
		if (0 < scalar(@decomposition))
			{
			my $decompositionType = "";
			if ($decomposition[0] =~ m/<[a-zA-Z0-9]+>/)
				{
				$decompositionType = shift @decomposition;
				}
			if ($decompositionType !~ m/compat/i)
				{
				$DecompositionValue[$codeVal] = scalar(@decomposition) == 1? hex($decomposition[0]) : -1;
				}
			}
		$Name[$codeVal] = $fields[1];
		my $upperval = $fields[12];
		my $lowerval = $fields[13];
		my $titleval = $fields[14];

		# strip whitespace from the end of the string
		$titleval =~ s/\s+$//;
		if ($upperval)
			{
			$upperval = hex($upperval);
			$Upper[$codeVal] = $upperval;
			add $codeVal;
			add $upperval;
			}
		if ($titleval)
			{
			$titleval = hex($titleval);
			$Title[$codeVal] = $titleval;
			add $codeVal;
			add $titleval;
			}
		if ($lowerval)
			{
			$lowerval = hex($lowerval);
			$Lower[$codeVal] = $lowerval;
			add $codeVal;
			$Codes{$lowerval} = 2;
			}
		}
	}

# Remove all codes that decompose to a sequence
foreach my $codeVal (keys(%Codes))
	{
	my $current = $DecompositionValue[$codeVal];
	while ($current && 0 < $current)
		{
		$current = $DecompositionValue[$current];
		}
	if ($current && $current == -1)
		{
		delete $Codes{$codeVal};
		}
	}

# Next we form the equivalence classes.
if ($OptionIncludeExtraMappings)
	{
	# space = non-breaking space
	addEquivalenceClass(0x20, 0xA0);
	}
# We try to end up with everything being equivalent to a lower case letter
foreach my $codeVal (keys(%Codes))
	{
	makeEquivalent($codeVal, $Lower[$codeVal]);
	makeEquivalent($codeVal, $Upper[$codeVal]);
	makeEquivalent($codeVal, $Title[$codeVal]);
	}

# Next we chase each pointer in CaseClass down to its final result
foreach my $codeVal (keys(%CaseClass))
	{
	chaseDown($codeVal);
	}

# Now output the results in order, and collect the raw data
my @Offset = ();
my $oldCodeCount = 0;
foreach my $codeVal (sort {$a <=> $b} keys(%CaseClass))
	{
	my $class = $CaseClass{$codeVal};
	my $offset = $class - $codeVal;
	if ($OptionOutputForwardMapping)
		{
		printf "%x %d\t\t%s => %s\n", $codeVal, $offset, $Name[$codeVal], $Name[$class];
		}
	while ($oldCodeCount != $codeVal)
		{
		$Offset[$oldCodeCount] = 0;
		$oldCodeCount++;
		}
	$oldCodeCount++;
	$Offset[$codeVal] = $offset;
	}

if ($OptionOutputReverseMapping)
	{
	my %ReverseMapping = ();
	foreach my $codeVal (keys(%CaseClass))
		{
		my $mapsTo = $CaseClass{$codeVal};
		if (!$ReverseMapping{$mapsTo})
			{
			$ReverseMapping{$mapsTo} = [$codeVal];
			}
		else
			{
			push (@{ $ReverseMapping{$mapsTo} }, $codeVal);
			}
		}
	foreach my $mapVal (sort {$a <=> $b} keys(%ReverseMapping))
		{
		next if ($OptionIgnoreOneToOneReverseMappings && scalar(@{$ReverseMapping{$mapVal}}) == 1);
		printf("%x: %s <=", $mapVal, $Name[$mapVal]);
		my $firstTime = 1;
		foreach my $val ( @{ $ReverseMapping{$mapVal} } )
			{
			if (!$firstTime)
				{
				print ',';
				}
			$firstTime = 0;
			printf(" %s:%x", $Name[$val], $val);
			}
		print "\n";
		}
	}

# does the array 2 match array 1? Match the shorter array against the prefix of
# the other array
sub arraysMatch
	{
	my ($left, $right, $leftpos) = @_;
	my $last = scalar(@$left) - $leftpos;
	if (scalar(@$right) < $last)
		{
		$last = scalar(@$right);
		}
	my $pos = 0;
	while ($pos < $last)
		{
		if ($$left[$pos + $leftpos] != $$right[$pos])
			{
			return 0;
			}
		$pos++;
		}
	return 1;
	}

# find a match for array 2 in array 1, allowing values past the end of array 1
# to match anything in array 1
sub findMatch
	{
	my ($candidate, $term) = @_;
	my $pos = 0;
	while (!arraysMatch($candidate, $term, $pos))
		{
		$pos++;
		}
	return $pos;
	}

# add the data in array 2 to array 1, returning the position they went in.
sub addArray
	{
	my ($candidate, $addition) = @_;
	my $pos = findMatch($candidate, $addition);
	# add any required on to the end of the candidate block
	my $last = $pos + scalar(@$addition);
	my $additionPos = scalar(@$candidate) - $pos;
	while ($pos + $additionPos < $last)
		{
		$$candidate[$pos + $additionPos] = $$addition[$additionPos];
		$additionPos++;
		}
	return $pos;
	}

# create data block 1 and indices 2 from data 3 and block size 4
sub createTrieLevel
	{
	my ($data, $indices, $input, $blockSize) = @_;
	my $block = 0;
	while ($block * $blockSize < scalar(@$input))
		{
		my $start = $block * $blockSize;
		my $end = $start + $blockSize;
		my $currentBlockSize = $blockSize;
		if (scalar(@$input) < $end)
			{
			$end = scalar(@$input);
			$currentBlockSize = $end - $start;
			}
		my @currentBlock = @$input[$start..($end - 1)];
		while ($currentBlockSize != $blockSize)
			{
			$currentBlock[$currentBlockSize] = 0;
			$currentBlockSize++;
			}
		$$indices[$block] = addArray($data, \@currentBlock);
		$block++;
		}
	}

sub OutputArray
	{
	my $index = 0;
	my $firstTime = 1;
	while ($index != scalar(@_))
		{
		if (!$firstTime)
			{
			if ($index % 8)
				{
				print ', ';
				}
			else
				{
				print ",\n\t";
				}
			}
		else
			{
			print "\t";
			$firstTime = 0;
			}
		print($_[$index]);
		$index++;
		}
	print "\n";
	}

if ($OptionOutputTrie)
	{
	my @Trie0 = ();
	my @Index0 = ();
	my @Trie1 = ();
	my @Index1 = ();
	my @Trie2 = ();
	my @Index2 = ();
	createTrieLevel(\@Trie0, \@Index0, \@Offset, 16);
	createTrieLevel(\@Trie1, \@Index1, \@Index0, 16);
	createTrieLevel(\@Trie2, \@Index2, \@Index1, 16);
	print "// Use the bits from 12 up from your character to index CaseFoldTable0.\n";
	print "// Use the result of this plus bits 8-11 to index CaseFoldTable1.\n";
	print "// Use the result of this plus bits 4-7 to index CaseFoldTable2.\n";
	print "// Use the result of this plus bits 0-3 to index CaseFoldTable3.\n";
	print "// Add the result of this to your character to fold it.\n\n";
	print "static const short CaseFoldTable3[] =\n\t{\n";
	OutputArray(@Trie0);
	print "\t};\n\nstatic const unsigned short CaseFoldTable2[] =\n\t{\n";
	OutputArray(@Trie1);
	print "\t};\n\nstatic const unsigned char CaseFoldTable1[] =\n\t{\n";
	OutputArray(@Trie2);
	print "\t};\n\nstatic const unsigned char CaseFoldTable0[] =\n\t{\n";
	OutputArray(@Index2);
	print "\t};\n";
	}
