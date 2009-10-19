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
# e32utils\trace\btrace_syslock.pl
# Process runtests btrace log file to determine the maximum time the system
# lock was held for.
# Example commands to generate a runtests btrace log file with system lock 
# tracing analysis when running e32test.auto.bat and 10MB btrace buffer:
# 1 - btrace -f4,17 -m1 -b10480
# 2 - runtests e32test.auto.bat -a OR use 
# btrace -a
# after running what ever is being tested
# Syntax:
# perl btrace_syslock.pl <inputfile> [<symbolfile> [<maxsymbols]]
# If <maxsymbols> is given, this is how many of the last (i.e. slowest)
# results to look up in the symbol file.  Defaults to 1.
# 
#

use strict;

# Unbuffer stderr
my $oldfh = select(STDERR); $| = 1; select($oldfh);

print STDERR "\nTHIS TOOL IS UNOFFICIAL, UNSUPPORTED AND SUBJECT TO CHANGE WITHOUT NOTICE!\n\n";


my $usage = "$0: usage: perl $0 <logfile> [<symbolfile> [<maxsymbols>]]\n";

my ($infile, $symbolfile, $howmany) = @ARGV;

die($usage) if !($infile && -f $infile) || ($symbolfile && ! -f $symbolfile);

my $symbols = new SymbolTable($symbolfile);

open(my $in, "<", $infile) || die("$0: $infile: $!\n");

my @wanted = ();

my $lockinfo;
my $testnames = {};
my $bufferfulls = 0;
my $berror = 0;

while (<$in>)
	{
	if (/BTRACE BUFFER IS FULL/)
		{
			$berror = 1;
			next;
		}
	if (/^RUNTESTS: Test/)
		{
			$testnames->{$lockinfo} = $_ if $lockinfo;
			$berror = 0;
			next;
		}
	if (/^<FM/ && /System lock/i)
		{
		push(@wanted, $_);
		$lockinfo = $_;
		$bufferfulls++ if $berror;
		}
	}

close($in);

@wanted = sort @wanted;

if (ref($symbols))
	{
	$howmany ||= 1;
	$howmany = 1 if $howmany < 1;
	$howmany = scalar(@wanted) if $howmany > scalar(@wanted);

	print @wanted[0 .. $#wanted - $howmany];

	for my $line (@wanted[$#wanted - $howmany + 1 .. $#wanted])
		{
		print "\n", $line;

#             MaxTime  AveTime HeldCount MaxPC    MaxTimestamp  TraceId Name
# <FM000000>       71        7    104631 f8023ddc    300257957 640005a4 'Sys
#
		my @fields = split(" ", $line);
		my $maxpc = $fields[4];

		print "                                       ",
		    $symbols->lookup($maxpc);

		print "                                       ",
		    $testnames->{$line} if $testnames->{$line};
		}
	}
else
	{
	print @wanted;
	}

printf(STDERR "%d buffer %s found\n", $bufferfulls,
    $bufferfulls == 1 ? "overflow was" : "overflows were") if $bufferfulls;


# ========================================================================
#
package SymbolTable;

sub new
{
	my ($proto, $filename) = @_;

	return undef if ! $filename;

	my @symbols;

	open(my $in, "<", $filename) || die("$0: $filename: $!\n");

	print STDERR "Loading symbols...";

	while (<$in>)
		{
		# f800c040    0000    btrace_fiq   k_entry_.o(.emb_text)
		if (/^[0-9a-f]{8}\s/i)	# Have a symbol table entry
			{
			# Ensure the address is in lowercase
			$_ = lc(substr($_, 0, 8)) . substr($_, 8);
			push(@symbols, $_);
			}
		}

	close($in);

	my $symbols = [sort @symbols];

	my $class = ref($proto) || $proto;

	bless($symbols, $class);

	print STDERR " done\n";

	return $symbols;
}

# lookup() is an implementation of the binary search algorithm below,
# retrieved from wikipedia on 10/9/07
#
#  BinarySearch(A[0..N-1], value) {
#       low = 0
#       high = N - 1
#       while (low <= high) {
#           mid = (low + high) / 2
#           if (A[mid] > value)
#               high = mid - 1
#           else if (A[mid] < value)
#               low = mid + 1
#           else
#               return mid
#       }
#       return not_found
#   }
#
sub lookup
{
	my ($symbols, $addr) = @_;

	return "BAD ADDRESS $addr\n" unless $addr =~ /^[0-9a-f]{8}$/i;
	$addr = lc($addr);

	my ($low, $high) = (0, $#$symbols);

	while ($low <= $high)
		{
		my $mid = int(($low + $high) / 2);
		my $mid_value = substr($symbols->[$mid], 0, 8);
## print "low: $low, high: $high, mid: $mid, mid_value: $mid_value\n";
		if ($mid_value gt $addr)
			{
			$high = $mid - 1;
			}
		elsif ($mid_value lt $addr)
			{
			$low = $mid + 1;
			}
		else
			{
			# Found an exact match
			return($symbols->[$mid]);
			}
		}

	# We didn't find an exact match.  We want the largest value that is
	# less than the input address.  This will be the value at either
	# $low or $high.

	return $symbols->[$low] if $low <= $#$symbols &&
	    $symbols->[$low] lt $addr;
	return "NO SYMBOL FOUND\n" if $high < 0;
	return $symbols->[$high] if $symbols->[$high] lt $addr;
	return "THIS SHOULDN'T HAPPEN\n";
}
