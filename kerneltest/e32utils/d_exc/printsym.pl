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
# Perl script to decode ROM symbols
#
# Usage: perl printsym.pl symbolfile
#
# Converts various forms of text from STDIN and write to stdout

use strict;

add_object(0xF8000000,0xFFF00000, "ROM");

die "Usage: printsym.pl romsymbolfile\n" unless @ARGV;

my %addresslist;
my %address;

read_rom_symbols($ARGV[0]);
shift;

## need to add more file types here... especially .map files


# We've accumulated the ranges of objects indexed by start address,
# with a companion list of addresses subdivided by the leading byte
# Now sort them numerically...

sub numerically { $a <=> $b }
foreach my $key (keys %addresslist)
{
	@{$addresslist{$key}} = sort numerically @{$addresslist{$key}};
}

# read lines from STDIN and decode them

print "Please enter data to be decoded\n";

while (my $line=<STDIN>)
	{
	next if ($line =~ /^\s+$/);		# skip blank lines
	print "\n";
	if ($line =~ /(?:^|\s)(([0-9A-Fa-f]{2} ){4,})/)	# pairs of hex digits separated by spaces = hex dump?
		{
		hexbytes($1);
		print "\n";
		next;
		}
	if ($line =~ /[0-9A-Fa-f]{8}\s+/)	# groups of hex words
		{
		hexwords($line);
		print "\n";
		next;
		}
	print "???\n";
	}

#############################################################################

sub add_object
	{
	my ($base, $max, $name) = @_;
	$address{$base} = [ $base, $max, $name ];
	my $key=$base>>20;
	my $maxkey=$max>>20;
	while ($key <= $maxkey)		# allowing for objects that span the boundary
		{
		push @{$addresslist{$key}}, $base;
		$key+=1;
		}
	}

sub match_addr
#
# Try matching one of the named areas in the addresslist
#
	{
	my ($word) = @_;

	if ($word < 1024*1024)
		{
		return 0;
		}

	# Optimization - try looking up the address directly

	my $base;
	my $max;
	my $name;
	if(defined $address{$word}) {
		($base, $max, $name) = @{$address{$word}};
	}
	if (!(defined $base))
		{
		my $key=$word>>20;
		my $regionbase;
		foreach $base (@{$addresslist{$key}})
			{
			if ($base <= $word)
				{
				$regionbase = $base;
				next;
				}
			if ($base > $word)
				{
				last;
				}
			}
		if(defined $regionbase)
			{
			($base, $max, $name) = @{$address{$regionbase}};
			}
		}
	if (defined $base && defined $max && $base <= $word && $max >= $word)
		{
		printf "%s + 0x%x", $name, $word - $base;
		return 1;
		}
	return 0;
	}

# Handle a MAKSYM.LOG file for a ROM
#
# NB. Wanted to do 
#
#   open ROMIMAGE, "cxxfilt <$romimage |" or open ROMIMAGE, $romimage or die
#
# but this uses "/bin/sh cxxfilt <$romimage" which works up to the point where the
# shell can't load cxxfilt.
#
sub read_rom_symbols
	{
	my ($romimage)=@_;
	open ROMSYMBOLS, $romimage or print "Can't open $romimage\n" and return;

	my $a;
	my $b;
	while (my $line = <ROMSYMBOLS>)
		{
		if(!($line =~ /^[0-9A-Fa-f]{8}/))
			{
			next;
			}
		# 8 bytes for the address
		
		$a = substr $line,0,8;
		if(!($a =~ /[0-9A-Fa-f]{8}/))
			{
			next;
			}
		# 4 bytes for the length
		$b = substr $line,12,4;
		if(!($b =~ /[0-9A-Fa-f]{4}/))
			{
			next;
			}
		# rest of line is symbol
		my $symbol = substr $line,20;
		chomp $symbol;

		my $base=hex($a);
		my $length=hex($b);
		if ($base < 0x50000000) 
			{
			next;	# skip this line
			}
		if ($length==0xffffffff)
			{
			$length=100;	# MAKSYM bug? choose a rational length
			}
		add_object($base, $base+$length-1, $symbol);
		}
	print "ROM Symbols from $romimage\n";
	}

sub dumpword
	{
	my ($word) = @_;
	my $data = pack "V", @_[0];
	$data =~ tr [\040-\177]/./c;
	printf "= %08x %4s  ", $word, $data;
	match_addr($word);
	printf "\n";
	}

sub hexbytes
	{
	my @bytes = split /\s+/, @_[0];
	my $wordcount = @bytes/4;
	map { dumpword($_) } (unpack "V$wordcount", (pack "H2"x($wordcount*4), @bytes));
	}
sub hexwords
	{
	my @words = grep /[0-9A-Fa-f]{8}/, split( /[^0-9A-Fa-f]+/, @_[0]);
	my $wordcount = @words;
	map { dumpword($_) } (unpack "N$wordcount", (pack "H8"x($wordcount), @words));
	}

