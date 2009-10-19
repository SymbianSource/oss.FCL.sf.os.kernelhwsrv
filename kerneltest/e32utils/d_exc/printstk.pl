#! perl
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
#

if (@ARGV<1)
	{
#........1.........2.........3.........4.........5.........6.........7.....
	print <<USAGE_EOF;

Usage:
	printstk.pl d_exc_nnn [romimage.symbol]

Given the output of D_EXC, a file d_exc_nnn.txt and d_exc_nnn.stk, it 
uses the other information to try to put symbolic information against 
the stack image.

USAGE_EOF
	exit 1;
	}

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

my $RomBase = 0xF8000000;
my $RomLimit = 0xFFF00000;
add_object($RomBase,$RomLimit, "ROM");

# Handle a MAKSYM.LOG file for a ROM
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

# Handle MAP file for a non execute-in-place binary
#
sub read_map_symbols
	{
	my ($binary, $binbase)=@_;
	$binary =~ /([^\\]+)$/;
	my $basename=$1;
	if (not open MAPFILE, "$basename.map")
		{
		print "Can't open map file for \n$binary.map)\n";		
		return;
		}

		
	my @maplines;
	while (<MAPFILE>) 
		{
		push @maplines, $_;
		}
	close MAPFILE;
# See if we're dealing with the RVCT output
	if ($maplines[0] =~ /^ARM Linker/) 
		{
		# scroll down to the global symbols
		while ($_ = shift @maplines) 
			{
			if (/Global Symbols/) 
				{
				last;
				}
			}
		# .text gets linked at 0x00008000		
		$imgtext=hex(8000);#start of the text section during linking
		
		foreach (@maplines) 
			{
			# name address ignore size section
			if (/^\s*(.+)\s*(0x\S+)\s+[^\d]*(\d+)\s+(.*)$/) 
				{
				my $symbol  = $1;
				my $addr = hex($2);
				my $size = $3;
				if ($size > 0)#symbols of the 0 size contain some auxillary information, ignore them
					{
	            			add_object($addr-$imgtext+$binbase,#relocated address of the current symbol 
						$addr-$imgtext+$binbase+$size,#relocated address of the current symbol + size of the current symbol
						"$binary $symbol");
					}
				}
			}			      
		}
	else 
#we are dealing with GCC output
		{
		my $imgtext;
		
		# Find text section
		while (($_ = shift @maplines) && !(/^\.text\s+/)) 
			{
			}

		/^\.text\s+(\w+)\s+(\w+)/
			or die "ERROR: Can't get .text section info for \"$file\"\n";
		$imgtext=hex($1);#start of the text section during linking
		$binbase-=$imgtext;
		
		foreach (@maplines) 
			{
			if (/___CTOR_LIST__/)
				{
				last;	# end of text section
				}

			if (/^\s(\.text)?\s+(0x\w+)\s+(0x\w+)\s+(.*)$/io) 
				{		    
				$textlimit = hex($2)+$binbase+hex($3)-1;			
				next;
				}
					
			if (/^\s+(\w+)\s\s+([a-zA-Z_].+)/o) 			
				{				    
				my $addr = hex($1);
				my $symbol = $2;
				add_object($addr+$binbase,#relocated address of the current symbol
					$textlimit,#limit of the current object section
					"$binary $symbol");
				next;
				}
			}						
		}
#end of GCC output parsing		
	}

# Handle a matched pair of D_EXC output files (.txt and .stk)
#
sub read_d_exc
	{
	my ($name)=@_;

	$stackbase = 0;
	open D_EXC, "$name.txt" or die "Can't open $name.txt\n";

	binmode D_EXC;
	read D_EXC, $data, 16;
	close D_EXC;

	if ($data =~ /^(..)*.\0.\0/)
		{
		# Assuming Unicode
		close D_EXC;

		# Charconv won't convert STDIN or write to STDOUT
		# so we generate an intermediate UTF8 file 
		system "charconv -little -input=unicode $name.txt -output=utf8 $name.utf8.txt";

		open D_EXC, "$name.utf8.txt" or die "Can't open $name.utf8.txt\n";
		}
	else
		{
		# Assuming ASCII
		open D_EXC, "$name.txt" or die "Can't open $name.txt\n";
		}

	my $is_eka2_log = 0;

	while (my $line = <D_EXC>)
		{

		if ($line =~ /^EKA2 USER CRASH LOG$/)
			{
			$is_eka2_log = 1;
			next;
			}
	
		# code=1 PC=500f7ff8 FAR=00000042 FSR=e8820013
		
		if ($line =~ /^code=\d PC=(.{8})/)
			{
			$is_exc = 1;
			$fault_pc = hex($1);
			next;
			};

		# R13svc=81719fc0 R14svc=50031da0 SPSRsvc=60000010
	
		if ($line =~ /^R13svc=(.{8}) R14svc=(.{8}) SPSRsvc=(.{8})/)
			{
			$fault_lr = hex($2);
			next;
			}

		# r00=fffffff8 00000000 80000718 80000003

		if ($line =~ /^r(\d\d)=(.{8}) (.{8}) (.{8}) (.{8})/)
			{
			$registers{$1} = $line;
			if ($1 == 12)
				{
				$activesp = hex($3);
				$user_pc = hex($5);
				$user_lr = hex($4);
				}
			next;
			}

		# User Stack 03900000-03905ffb
		# EKA1 format deliberately broken (was /^Stack.*/) to catch version problems

		if ($line =~ /^User Stack (.{8})-(.{8})/)
			{
			$stackbase = hex($1);
			add_object($stackbase,hex($2), "Stack");
			next;
			}

		# fff00000-fff00fff C:\foo\bar.dll

		if ($line =~ /^(.{8})-(.{8}) (.+)/)
			{
			next if ($RomBase <= hex($1) && hex($1) < $RomLimit); # skip ROM XIP binaries
			add_object(hex($1), hex($2), $3);
			read_map_symbols($3, hex($1));
			}
		}
	close D_EXC;

	die "$name.txt is not a valid EKA2 crash log" unless $is_eka2_log;

	if ($stackbase == 0)
		{
		die "couldn't find stack information in $name.txt\n";
		}

	die "couldn't find stack pointer in  $name.txt\n" unless $activesp != 0;
	$activesp -= $stackbase;

	# Read in the binary dump of the stack

	open STACK, "$name.stk" or die "Can't open $name.stk\n";
	print "Stack Data from $name.stk\n";

	binmode STACK;
	while (read STACK, $data, 4)
		{
		unshift @stack, (unpack "V", $data);
		}
	$stackptr = 0;
	}

# Handle the captured text output from the Kernel debugger
#
sub read_debugger
	{
	my ($name)=@_;

	open DEBUGFILE, "$name" or die "Can't open $name\n";
	print "Kernel Debugger session from $name\n";

	# stuff which should be inferred from "$name"

	$stackbase = 0x81C00000;
	$stackmax  = 0x81C01DC0;
	$activesp = 0x81c01bc4-$stackbase;
	add_object($stackbase,0x81C01FFF, "Stack");

	while (my $line = <DEBUGFILE>)
		{
		if ($line =~ /^(\w{8}): ((\w\w ){16})/)
			{
			my $addr = hex($1);
			if ($addr < $stackbase || $addr > $stackmax)
				{
				next;
				}
			if (@stack == 0)
				{
				if ($addr != $stackbase)
					{
					printf "Missing stack data for %x-%x - fill with 0x29\n", $stackbase, $addr-1;
					@stack = (0x29292929) x (($addr-$stackbase)/4);
					}
				}
			unshift @stack, reverse (unpack "V4", (pack "H2"x16, (split / /,$2)));
			}
		}
		$stackptr = 0;
	}

read_d_exc(@ARGV[0]);
if (@ARGV>1)
	{
	read_rom_symbols(@ARGV[1]);
	}

# We've accumulated the ranges of objects indexed by start address,
# with a companion list of addresses subdivided by the leading byte
# Now sort them numerically...

sub numerically { $a <=> $b }
foreach my $key (keys %addresslist)
	{
	@{$addresslist{$key}} = sort numerically @{$addresslist{$key}};
	}

# Off we go, reading the stack!

sub skip_unused 
	{
	my $skipped=0;
	while (@stack)
		{
		my $word=(pop @stack);
		if ($word!=0x29292929)
			{ 
			push @stack, $word;
			last;
			}
		$skipped += 4;
		}
	$stackptr += $skipped;
	return $skipped;
	}

sub lookup_addr
{
	my ($word) = @_;

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
		my $data = pack "V", $word;
		$data =~ tr [\040-\177]/./c;
		return sprintf "%08x %4s  %s + 0x%x", $word, $data, $name, $word - $base;
		}
	return "";
}

sub match_addr
#
# Try matching one of the named areas in the addresslist
#
{
	my $word = (pop @stack);

	if ($word < 1024*1024)
		{
		push @stack, $word;
		return 0;
		}

	my $result = lookup_addr($word);
	if ($result ne "")
		{
		print "$result\n";
		$stackptr+=4;
		return 1;
		}
	push @stack, $word;
	return 0;
	}

sub match_tbuf8
#
# Try matching a TBuf8
#	0x3000LLLL 0x0000MMMM data
#	
	{
	if (scalar @stack <3)
		{
		return 0;	# too short
		}
	my $word = (pop @stack);
	my $maxlen = (pop @stack);
	
	my $len = $word & 0x0ffff;
	my $type = ($word >> 16) & 0x0ffff;
	if ( $type != 0x3000 || $maxlen <= $len || $maxlen > 4* scalar @stack 
		|| ($stackptr < $activesp && $stackptr + $maxlen + 8 > $activesp))
		{
		push @stack, $maxlen;
		push @stack, $word;
		return 0;		# wrong type, or invalid looking sizes, or out of date
		}

	printf "TBuf8<%d>, length %d\n", $maxlen, $len;
	$stackptr += 8;

	my $string="";
	while ($maxlen > 0)
		{
		$string .= pack "V", pop @stack;
		$maxlen -= 4;
		$stackptr += 4;
		}
	if ($len==0)
		{
		print "\n";
		return 1;
		}
	my $line = substr($string,0,$len);
	my @buf = unpack "C*", $line;
	$line =~ tr [\040-\177]/./c;
	printf "\n  %s", $line;
	while ($len > 0)
		{
		my $datalen = 16;
		if ($datalen > $len)
			{
			$datalen = $len;
			}
		$len -= $datalen;
		printf "\n  ";
		while ($datalen > 0)
			{
			my $char = shift @buf;
			printf "%02x ", $char;
			$datalen -= 1;
			}
		}
	printf "\n\n";
	return 1;
	}

# Skip the unused part of the stack

skip_unused;
printf "High watermark = %04x\n", $stackptr;

# process the interesting bit!

my $printed_current_sp = 0;
while (@stack)
	{
	if (!$printed_current_sp && $stackptr >= $activesp)
		{
		printf "\n >>>> current user stack pointer >>>>\n\n";

		print $registers{"00"};
		print $registers{"04"};
		print $registers{"08"};
		print $registers{"12"};

		if ($is_exc && $user_pc != $fault_pc)
			{
			print "\nWARNING: A kernel-side exception occured but this script\n";
			print "is currently limited to user stack analysis. Sorry.\n";
			my $result = lookup_addr($fault_pc);
			if ($result ne "")
				{
				print "Kernel PC = $result\n";
				}
			$result = lookup_addr($fault_lr);
			if ($result ne "")
				{
				print "Kernel LR = $result\n";
				}
			print "\n";
			}

		my $result = lookup_addr($user_pc);
		if ($result ne "")
			{
			print "User PC = $result\n";
			}
		$result = lookup_addr($user_lr);
		if ($result ne "")
			{
			print "User LR = $result\n";
			}
		printf "\n >>>> current user stack pointer >>>>\n\n";
		$printed_current_sp = 1;
		}

	printf "%04x  ", $stackptr;

	match_tbuf8() and next;
	match_addr() and next;

	$word = pop @stack;
	$data = pack "V", $word;
	$data =~ tr [\040-\177]/./c;
	printf "%08x %4s  ", $word, $data;
	$stackptr += 4;

	if ($word == 0x29292929)
		{
		$skipped = skip_unused;
		if ($skipped != 0)
			{
			printf "\n....";
			}
		printf "\n";
		next;
		}

	# Try matching $word against the known addresses of things
	printf "\n";
	}


