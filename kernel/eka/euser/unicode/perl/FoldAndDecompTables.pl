# Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# Creates C++ code describing how to decompose, compose and fold each character.
# Usage:
# perl -w FoldAndDecompTables.pl < <output-from-UnicodeMaxDecompose>
# Tables we want to create:
# A: Ordered list of non-excluded decompositions
# B: List of folded decompositions matching A
# C: List of decompositions not listed in A of length > 1
# D: List of folded decompositions matching C
# E: List of decompositions of length = 1 whose matching folded decompositions
# are of length > 1
# F: List of folded decompositions matching E
# G: List of decompositions of length = 1 with matching folded decompositions
# H: List of folded decompostions matching G
# I: List of folded decompositions that do not have matching decompositions
# J: List of decompositions (folding and otherwise) of length > 2
# K: Hash table mapping Unicode value to its folded decomposition value in the
# concatenated list B-D-F-H-I
# L: List of hash slots in K matching A (providing a mapping from non-excluded
# decompositions to Unicode value)
# [all lengths are of UTF16 strings]
# 
#

use strict;

#
# Hash table:
#

# Size of hashing table = 1 to the power $LgHashTableSize
my $LgHashTableSize = 12;

# Do not change these next two values!
my $HashTableSize = 1 << $LgHashTableSize;
my $HashTableBitmaskCpp = sprintf('0x%x', $HashTableSize - 1);

# Hashing function in Perl: Getting the initial search position
sub HashStart
	{
	return $_[0] & ($HashTableSize - 1);
	}
# How far to step through each time
sub HashStep
	{
	my ($code) = @_;
	$code *= $code >> $LgHashTableSize;
	return ($code * 2 + 1) & ($HashTableSize - 1);
	}

# Make sure input string is all hex numbers separated by single spaces with
# each hex number having 4 digits and decomposed into UTF16
sub Normalize
	{
	my ($string) = @_;
	if ($string =~ /^([0-9A-F]{4}( [0-9A-F]{4})*)?$/)
		{
		return $string;
		}
	my $norm = '';
	foreach my $elt (split(' ', $string))
		{
		if ($elt)
			{
			die "'$elt' is not a hex number"
				unless $elt =~ /[0-9a-fA-F]+/;
			$norm = $norm.' '
				unless $norm eq '';
			$elt = hex $elt;
			if ($elt < 0x10000)
				{
				$norm = $norm.(sprintf('%04X', $elt));
				}
			else
				{
				# Add a surrogate pair
				$norm = $norm.(sprintf('%04X %04X',
					($elt / 0x400) + 0xD7C0, ($elt % 0x400) + 0xDC00));
				}
			}
		}
	#print STDERR "'$string' normalized to '$norm'\n";
	return $norm;
	}

# First stage:
# Hash of Unicode values to normalised decomposition and folded strings
my %Decomp = ();
my %Folded = ();
# Mapping from decomposition->char, if not excluded
my %Composition = ();
# characters with non-excluded decompositions
my @IncludedDecomps = ();
# characters with long (>1 UTF16) excluded decompositions
my @LongExcludedDecomps = ();
# characters with singleton decompositions but long folds
my @ShortDecompsLongFolds = ();
# characters with singleton folds and singleton
my @ShortDecompsShortFolds = ();
# characters with singleton folds but no decomps
my @ShortFoldsOnly = ();

# A mapping from decompositions of length greater than two
# to the code that produced them.
my %VeryLongDecompositions = ();

# A list of characters containing all decompositions of length >2 as slices
my @VeryLongDecompData = ();
# Mapping from decomposition->index into VeryLongDecompData
my %VeryLongDecompMap = ();

# There will be a hash table mapping Unicode values to indices into the other
# tables. %Index maps the same thing in Perl.
my %Index = ();
# %HashTableEntryContents maps the table entries to the Unicode values they
# contain.
my %HashTableEntryContents = ();
# %HashTableEntry maps Unicode value to the entry in the hash table
my %HashTableEntry = ();

# Bind a unicode value to an index into the tables
sub AddHashValue
	{
	my ($unicode, $index) = @_;
	$Index{$unicode} = $index;
	my $pos = HashStart($unicode);
	my $step = HashStep($unicode);
	while (exists $HashTableEntryContents{$pos})
		{
		$pos += $step;
		if ($HashTableSize <= $pos)
			{
			$pos %= $HashTableSize;
			}
		}
	$HashTableEntryContents{$pos} = $unicode;
	$HashTableEntry{$unicode} = $pos;
	}

# Bind a whole array to the indices starting from that given as the first
# argument. Returns the index of the next slot to be filled.
sub AddListToHash
	{
	my ($index, @unicodes) = @_;
	while (@unicodes)
		{
		AddHashValue(shift @unicodes, $index);
		$index++;
		}
	return $index;
	}

# put the results of a read line into the data structures
sub AddCode
	{
	my ($code, $excluded, $decomposition, $folded) = @_;
	return if ($decomposition eq '' && $folded eq '');
	$Decomp{$code} = $decomposition;
	$Folded{$code} = $folded;

	if (!$excluded && $decomposition ne '')
		{
		push @IncludedDecomps, $code;
		$Composition{$decomposition} = $code;
		}
	elsif (4 < length $decomposition)
		{
		push @LongExcludedDecomps, $code;
		}
	elsif (4 < length $folded)
		{
		push @ShortDecompsLongFolds, $code;
		}
	elsif ($decomposition ne '')
		{
		push @ShortDecompsShortFolds, $code;
		}
	elsif ($folded ne '')
		{
		push @ShortFoldsOnly, $code;
		}

	$VeryLongDecompositions{$decomposition} = $code
		if (9 < length $decomposition);
	$VeryLongDecompositions{$folded} = $code
		if (9 < length $folded);
	}

if (scalar(@ARGV) != 0)
	{
	print (STDERR "Usage:\nperl -w FoldAndDecompTables.pl < <output-from-UnicodeMaxDecompose>\n");
	exit 1;
	}

my $lineNo = 0;
my $inBlock = 0;
while(<STDIN>)
	{
	$lineNo++;
	if (/^(1?[0-9a-fA-F]{4,5});([^;]*);.*symbian:(E?);[^;]*;([0-9a-fA-F \t]*);([0-9a-fA-F \t]*)[ \t]*$/i)
		{
		my $code = hex $1;
		my $description = $2;
		my $excluded = $3;
		my $decomposition = Normalize($4);
		my $folded = Normalize($5);

		die ("Value $1 too large to be Unicode at line $lineNo.")
			if (0x110000 <= $code);

		die("Normalisation failed with '$decomposition' at line $lineNo.")
			unless (length $decomposition) == 0 || (length $decomposition) % 5 == 4;
		die("Normalisation failed with '$folded' at line $lineNo.")
			unless (length $folded) == 0 || (length $folded) % 5 == 4;

		AddCode($code, $excluded, $decomposition, $folded);

		if ($description =~ /^<.*Last>$/i)
			{
			die("End of block without start at line $lineNo!")
				if !$inBlock;
			while ($inBlock <= $code)
				{
				AddCode($inBlock, $excluded, $decomposition, $folded);
				$inBlock++;
				}
			$inBlock = 0;
			}
		elsif ($description =~ /^<.*First>$/i)
			{
			die("Block within block at line $lineNo!")
				if $inBlock;
			$inBlock = $code + 1;
			}
		}
	elsif (!/^[ \t]*$/)
		{
		die("Did not understand line $lineNo.");
		}
	}

# We need to construct the data for the table of decompositions of length > 2.
foreach my $decomp (sort {length $::b <=> length $::a} keys %VeryLongDecompositions)
	{
	if (!exists $VeryLongDecompMap{$decomp})
		{
		# Does not already exist
		my $newPos = scalar @VeryLongDecompData;
		$VeryLongDecompMap{$decomp} = $newPos;
		foreach my $code (split(' ', $decomp))
			{
			push @VeryLongDecompData, $code;
			}
		while ($decomp =~ /^([0-9A-F]{4}( [0-9A-F]{4}){2,}) [0-9A-F]{4}$/)
			{
			$decomp = $1;
			$VeryLongDecompMap{$decomp} = $newPos;
			}
		}
	}

# We need to sort the codes for included decompositions into lexicographic
# order of their decompositions.
# This, luckily, is the same as sorting the strings that represent their
# decompositions in hex lexicographically.
@IncludedDecomps = sort {$Decomp{$::a} cmp $Decomp{$::b}} @IncludedDecomps;

print (STDERR 'Included: ', scalar(@IncludedDecomps), "\nLong: ", scalar(@LongExcludedDecomps));
print(STDERR "\nLongFolds: ", scalar(@ShortDecompsLongFolds), "\nShort: ", scalar(@ShortDecompsShortFolds));
print(STDERR "\nShortFoldsOnly: ", scalar(@ShortFoldsOnly), "\nTOTAL: ");
print STDERR (scalar(@IncludedDecomps) + scalar(@LongExcludedDecomps) + scalar(@ShortDecompsLongFolds) + scalar(@ShortDecompsShortFolds) + scalar(@ShortFoldsOnly));
print STDERR "\n";

# Analyse the hash table to find out the maximum and average time
# taken to find each ASCII character
my $maxAsciiTime = 0;
my $totalAsciiTime = 0;
my $mostDifficultCode = undef;
my $asciiFoundWithoutStepCount = 0;
for (32..126)
	{
	my $code = $_;
	my $pos = HashStart($code);
	my $step = HashStep($code);
	my $stepCount = 1;
	if ($HashTableEntry{$code})
		{
		my $posRequired = $HashTableEntry{$code};
		while ($pos != $posRequired)
			{
			$pos = ($pos + $step) % $HashTableSize;
			$stepCount++;
			}
		}
	$totalAsciiTime += $stepCount;
	if ($maxAsciiTime < $stepCount)
		{
		$maxAsciiTime = $stepCount;
		$mostDifficultCode = $code;
		}
	if ($stepCount == 1)
		{
		$asciiFoundWithoutStepCount++;
		}
	}
printf (STDERR "Average ASCII search: %f\n", $totalAsciiTime / 95);
printf (STDERR "Maximum ASCII search %d for %x: '%c'.\n", $maxAsciiTime, $mostDifficultCode, $mostDifficultCode);

# Now we populate the hash table
my $index = 0;

$index = AddListToHash($index, @IncludedDecomps);
my $hashIndexAfterIncludedDecomps = $index;
printf (STDERR "after IncludedDecomps index= %d\n", $hashIndexAfterIncludedDecomps);

$index = AddListToHash($index, @LongExcludedDecomps);
my $hashIndexAfterLongExcludeDecomps = $index;
printf (STDERR "after LongExcludedDecomps index= %d\n", $hashIndexAfterLongExcludeDecomps);

$index = AddListToHash($index, @ShortDecompsLongFolds);
my $hashIndexAfterShortDecompsLongFolds = $index;
printf (STDERR "after ShortDecompsLongFolds index= %d\n", $hashIndexAfterShortDecompsLongFolds);

$index = AddListToHash($index, @ShortDecompsShortFolds);
my $hashIndexAfterShortDecompsShortFolds = $index;
printf (STDERR "after ShortDecompsShortFolds index= %d\n", $hashIndexAfterShortDecompsShortFolds);

$index = AddListToHash($index, @ShortFoldsOnly);
my $hashIndexAfterShortFoldsOnly = $index;
printf (STDERR "after ShortFoldsOnly index= %d\n", $hashIndexAfterShortFoldsOnly);

#
# Output C++ File
#
my $totalBytes = 0;

print "// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).\n";
print "// All rights reserved.\n";
print "// This component and the accompanying materials are made available\n";
print "// under the terms of the License \"Eclipse Public License v1.0\"\n";
print "// which accompanies this distribution, and is available\n";
print "// at the URL \"http://www.eclipse.org/legal/epl-v10.html\".\n";
print "//\n";
print "// Initial Contributors:\n";
print "// Nokia Corporation - initial contribution.\n";
print "//\n";
print "// Contributors:\n";
print "//\n";
print "// Description:\n";
print "//\n";
print "// Fold and decomposition tables.\n";
print "//\n";
print "// These tables are linked in the following way:\n";
print "// KUnicodeToIndexHash is a hash table using double hashing for\n";
print "// conflict resolution. The functions DecompositionHashStart and\n";
print "// DecompositionHashStep give the start and step values for accessing\n";
print "// the table. The first probe is at DecompositionHashStart and each\n";
print "// subsequent probe is offset by DecompositionHashStep. Probes\n";
print "// continue until either 0 is found (indicating that the Unicode value\n";
print "// sought has no decompostion (i.e. decomposes to itself)) or a value\n";
print "// is found that has the sought Unicode value in its lower 20 bits.\n";
print "//\n";
print "// In this latter case, the upper 12 bits contain an index into\n";
print "// one of the following tables, according to the following rules:\n";
print "//\n";
print "// In the case of folding:\n";
print "// If the Index is less than the length of KNonSingletonFolds / 2,\n";
print "// it is an index into KNonSingletonFolds. If the Index is\n";
print "// greater than the length of KNonSingletonFolds / 2, then it is an\n";
print "// index into KSingletonFolds.\n";
print "//\n";
print "// In the case of decomposition:\n";
print "// If the Index is less than the length of KNonSingletonDecompositions / 2,\n";
print "// it is an index into KNonSingletonDecompositions. If the Index is\n";
print "// greater than the length of KNonSingletonDecompositions / 2, then it is an\n";
print "// index into KSingletonDecompositions.\n";
print "//\n";
print "// In summary:\n";
print "// Let Knsf be the length of KNonSingletonFolds / 2,\n";
print "// let Knsd be the length of KNonSingletonDecompositions / 2,\n";
print "// let Ksd be the length of KSingletonDecompositions and\n";
print "// let Ksf be the length of KSingletonFolds.\n";
print "// Now if you want to fold a character and you have found\n";
print "// its index 'i' from the KUnicodeToIndexHash, then;\n";
print "// if (i < Knsf) then look up\n";
print "//\t\tKNonSingletonFolds[i * 2] and KNonSingletonFolds[i * 2 + 1]\n";
print "// else if (Knsf <= i < Knsf + Ksf) look up KSingletonFolds[i - Knsf]\n";
print "// else there is no fold for this character.\n";
print "//\n";
print "// Or if you want to decompose the same character, then;\n";
print "// if (i < Knsd) then look up KNonSingletonDecompositions[i * 2]\n";
print "//\t\tand KNonSingletonDecompositions[i * 2 + 1]\n";
print "// else if (Knsd <= i < Knsd + Ksd) look up KSingletonDecompositions[i - Knsd]\n";
print "// else there is no decomposition for this character.\n";
print "//\n";
print "// Your index into KSingletonDecompositions or KSingletonFolds\n";
print "// yields a single value which is the decomposition or fold.\n";
print "//\n";
print "// The KNonSingletonFolds and KNonSingletonDecomposition\n";
print "// tables are made up of pairs of values. Each pair is either a pair\n";
print "// of Unicode values that constitute the fold or decomposition, or\n";
print "// the first value is KLongD and the second has its top 4 bits as the\n";
print "// length of the decomposition (or folded decomposition) minus 3,\n";
print "// and its bottom 12 bits as the index into KLongDecompositions\n";
print "// of where you can find this decomposition.\n";
print "//\n";
print "// KLongDecompositions simply contains UTF-16 (Unicode) for\n";
print "// all the decomposed and folded sequences longer than 4 bytes long.\n";
print "\n";
print "// Hash table mapping unicode values to indices into the other tables\n";
print "// in use = ".$hashIndexAfterShortFoldsOnly." entries\n";
print "const unsigned long KUnicodeToIndexHash[$HashTableSize] =\n\t{\n\t";
my @HashTableOutput;
for (0..($HashTableSize - 1))
	{
	my $v = 0;
	if (exists $HashTableEntryContents{$_})
		{
		$v = $HashTableEntryContents{$_};
		die ('Did not expect a Unicode value > 0xFFFFF')
			if 0xFFFFF < $v;
		$v |= ($Index{$v}) << 20;
		}
	push @HashTableOutput, sprintf('0x%08x', $v);
	$totalBytes += 4;
	}
print (shift @HashTableOutput);
my $valueCount = 0;
foreach my $v (@HashTableOutput)
	{
	print (((++$valueCount & 7) == 0)? ",\n\t" : ', ');
	print $v;
	}
print "\n\t};\n\n";
print "// Hash table access functions\n";
print "const int KDecompositionHashBitmask = $HashTableBitmaskCpp;\n\n";
print "inline int DecompositionHashStart(long a)\n";
print "\t{\n\treturn a & $HashTableBitmaskCpp;\n\t}\n\n";
print "inline int DecompositionHashStep(long a)\n";
print "\t{\n\ta *= a >> $LgHashTableSize;\n";
print "\treturn ((a<<1) + 1) & $HashTableBitmaskCpp;\n\t}\n\n";

print "// Table mapping KNonSingletonDecompositions to the hash table entry that\n";
print "// indexes it\n";
print "const unsigned short KCompositionMapping[] =\n\t{\n\t";
for (0..(scalar(@IncludedDecomps - 1)))
	{
	if ($_ != 0)
		{print (($_ & 7) == 0? ",\n\t" : ', ')}
	printf( '0x%04x', $HashTableEntry{$IncludedDecomps[$_]} );
	$totalBytes += 2;
	}
print "\n\t};\n\n";

print "// Table containing all the decomposition and folding strings longer\n";
print "// than 2 UTF16 characters\n";
print "const unsigned short KLongDecompositions[] =\n\t{\n\t0x";
for(0..(scalar(@VeryLongDecompData) - 1))
	{
	if ($_ != 0)
		{print (($_ & 7) == 0?",\n\t0x" : ', 0x')}
	print $VeryLongDecompData[$_];
	$totalBytes += 2;
	}
print "\n\t};\n\n";

print "// Table containing decompositions longer than one UTF16 character.\n";
print "// The top of the table contains all compositions, sorted lexicographically.\n";
print "// Any decompositions of length 2 are in the table as a pair of values,\n";
print "// decompositions longer than that are represented by a KLongD followed by\n";
print "// a value whose top four bits indicate the length of the decomposition minus\n";
print "// three and whose bottom 12 bits indicate an index into the KLongDecompositions\n";
print "// array where the decomposition starts.\n";
print "const long KLongD = 0;\n";
print "// sizeof/2 = ".$hashIndexAfterLongExcludeDecomps."\n";
print "const unsigned short KNonSingletonDecompositions[] =\n\t{\n\t";

sub PrintNonsingletonDecompTableEntry
	{
	my ($decomp) = @_;
	if (length $decomp < 10)
		{
		if ($decomp =~ /([0-9A-F]{4}) ([0-9A-F]{4})/)
			{
			print '0x'.$1.', 0x'.$2;
			}
		else
			{
			die("$decomp expected to be normalized and of length 1 or 2")
				if $decomp !~ /[0-9A-F]{4}/;
			print '0x'.$decomp.', 0xFFFF';
			}
		}
	else
		{
		printf ('KLongD, 0x%1X%03X', ((length $decomp) - 14)/5, $VeryLongDecompMap{$decomp});
		}
	}

{my $entryNo = 0;
foreach my $code (@IncludedDecomps)
	{
	if ($entryNo != 0)
		{print (($entryNo & 3) == 0?",\n\t" : ', ')}
	PrintNonsingletonDecompTableEntry($Decomp{$code});
	$entryNo++;
	$totalBytes += 4;
	}
foreach my $code (@LongExcludedDecomps)
	{
	print (($entryNo & 3) == 0?",\n\t" : ', ');
	PrintNonsingletonDecompTableEntry($Decomp{$code});
	$entryNo++;
	$totalBytes += 4;
	}
}
print "\n\t};\n\n";

print "// Table of folded decompositions which either have more than one UTF16, or\n";
print "// their normal decompositions have more than one UTF16\n";
print "// sizeof/2 = ".$hashIndexAfterShortDecompsLongFolds."\n";
print "const unsigned short KNonSingletonFolds[] =\n\t{\n\t";
{my $entryNo = 0;
foreach my $code (@IncludedDecomps)
	{
	if ($entryNo != 0)
		{print (($entryNo & 3) == 0?",\n\t" : ', ')}
	PrintNonsingletonDecompTableEntry($Folded{$code});
	$entryNo++;
	$totalBytes += 4;
	}
foreach my $code (@LongExcludedDecomps)
	{
	print (($entryNo & 3) == 0?",\n\t" : ', ');
	PrintNonsingletonDecompTableEntry($Folded{$code});
	$entryNo++;
	$totalBytes += 4;
	}
foreach my $code (@ShortDecompsLongFolds)
	{
	print (($entryNo & 3) == 0?",\n\t" : ', ');
	PrintNonsingletonDecompTableEntry($Folded{$code});
	$entryNo++;
	$totalBytes += 4;
	}
}
print "\n\t};\n\n";

print "// Table of singleton decompositions and characters with singleton folds\n";
print "// Note for Unicode 5.0:\n";
print "// Unicode 5.0 contains some non-BMP characters have non-BMP \"singleton\" folds.\n";
print "// As per the algorithm of this file, the non-BMP character should be stored in \n";
print "// this table. \"Unsigned short\" is not big enough to hold them. However, this \n";
print "// \"character\" information is not useful. So we just store 0xFFFF instead. \n";
print "// Please do check 0xFFFF when access this table. If meet 0xFFFF, that means \n";
print "// your character has no decomposition.\n";
print "// See the variable \"ShortDecompsLongFolds\" in FoldAndDecompTables.pl if you \n";
print "// want to know more.\n";
print "// sizeof = ".($hashIndexAfterShortDecompsShortFolds-$hashIndexAfterLongExcludeDecomps)."\n";
print "const unsigned short KSingletonDecompositions[] =\n\t{\n\t0x";
{my $entryNo = 0;
foreach my $code (@ShortDecompsLongFolds)
	{
	if ($entryNo != 0)
		{print (($entryNo & 7) == 0?",\n\t0x" : ', 0x')}
	if (exists $Decomp{$code} && $Decomp{$code} ne '')
		{
		print $Decomp{$code};
		}
	else
		{
		# Don't take these 0xFFFF as character.
		#printf ('%04X', $code);
		printf ("FFFF");
		}
	$entryNo++;
	$totalBytes += 4;
	}
foreach my $code (@ShortDecompsShortFolds)
	{
	if ($entryNo != 0)
		{print (($entryNo & 7) == 0?",\n\t0x" : ', 0x')}
	print $Decomp{$code};
	$entryNo++;
	$totalBytes += 4;
	}
}
print "\n\t};\n\n";

print "// Table of singleton folds\n";
print "// sizeof = ".($hashIndexAfterShortFoldsOnly-$hashIndexAfterShortDecompsLongFolds)."\n";
print "const unsigned short KSingletonFolds[] =\n\t{\n\t0x";
{my $entryNo = 0;
foreach my $code (@ShortDecompsShortFolds)
	{
	if ($entryNo != 0)
		{print (($entryNo & 7) == 0?",\n\t0x" : ', 0x')}
	print $Folded{$code};
	$entryNo++;
	$totalBytes += 4;
	}
foreach my $code (@ShortFoldsOnly)
	{
	print (($entryNo & 7) == 0?",\n\t0x" : ', 0x');
	print $Folded{$code};
	$entryNo++;
	$totalBytes += 4;
	}
}
print "\n\t};\n";

print "\n// Total size: $totalBytes bytes\n";
print STDERR $totalBytes, " bytes\n";
