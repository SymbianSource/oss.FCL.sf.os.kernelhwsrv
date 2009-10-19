# Copyright (c) 1995-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# e32\kernel\genexec.pl
# Generate user/kernel interface classes from single description file
# Usage:
# perl genexec.pl -i <input file> -e <enum.h> -u <user.h> -k <kernel.h>
# where:	<input file>	is the name of the description file
# <enum.h>		is the name of the generated header file containing the dispatch enumeration
# <user.h>		is the name of the generated header file containing user side interface
# <kernel.h>		is the name of the generated header file containing kernel side interface
# 
#

use Getopt::Long;
use Cwd;

#Getopt::Long::Configure("ignore_case");
my %opts=();
my $result = GetOptions (	\%opts,
							"input=s",
							"enum=s",
							"user=s",
							"kernel=s"
							);

# Bit 0 = argument permitted
# Bit 1 = argument mandatory
# Bit 2 = can be used with fast
# Bit 3 = can be used with slow
# Bit 4 = remember all arguments from repeated use of keyword

my %keywords = (
					'name'			=>	0x0f,
					'number'		=>	0x0f,
					'user'			=>	0x0f,
					'specialuser'	=>	0x0c,
					'kernel'		=>	0x0f,
					'specialkernel'	=>	0x0f,
					'export'		=>	0x0c,
					'return'		=>	0x0d,
					'arg1'			=>	0x0f,
					'arg2'			=>	0x0b,
					'arg3'			=>	0x0b,
					'arg4'			=>	0x0b,
					'karg1'			=>	0x0f,
					'karg2'			=>	0x0b,
					'karg3'			=>	0x0b,
					'karg4'			=>	0x0b,
					'unprotected'	=>	0x08,
					'lock'			=>	0x08,
					'nr'			=>	0x08,
					'handle'		=>	0x0b,
					'asm'			=>	0x0d,
					'hw'			=>	0x0c,
					'emulator'		=>	0x0c,
					'extended'		=>	0x0b,
					'ifdef'			=>	0x1f,
					'ifndef'		=>	0x1f
				);

my %keyword_synonyms = (
					'norelease'		=>	'nr',
					'noemulator'	=>	'hw'
				);

my @keyword_list = keys(%keywords);

my %objects =	(
					'thread'		=>	['EThread','DThread',1],
					'process'		=>	['EProcess','DProcess',1],
					'chunk'			=>	['EChunk','DChunk',1],
					'library'		=>	['ELibrary','DLibrary',1],
					'semaphore'		=>	['ESemaphore','DSemaphore',1],
					'mutex'			=>	['EMutex','DMutex',1],
					'timer'			=>	['ETimer','DTimer',1],
					'server'		=>	['EServer','DServer',1],
					'session'		=>	['ESession','DSession',1],
					'logicaldevice'	=>	['ELogicalDevice','DLogicalDevice',1],
					'physicaldevice'=>	['EPhysicalDevice','DPhysicalDevice',1],
					'logicalchannel'=>	['ELogicalChannel','DLogicalChannelBase',1],
					'changenotifier'=>	['EChangeNotifier','DChangeNotifier',1],
					'undertaker'	=>	['EUndertaker','DUndertaker',1],
					'msgqueue'		=>	['EMsgQueue','DMsgQueue',1],
					'property'		=>	['EPropertyRef','DPropertyRef',1],
					'condvar'		=>	['ECondVar','DCondVar',1],
					'shpool'		=>	['EShPool','DShPool',1],
					'shbuf'			=>	['EShBuf','DShBuf',1],
					'any'			=>	['','DObject',1],
					'ipcmessaged'	=>	['EIpcMessageD','RMessageK',0],
					'ipcmessage'	=>	['EIpcMessage','RMessageK',0],
					'ipcclient'		=>	['EIpcClient','DThread',0]
				);

my %object_synonyms = (
					'ldevice'		=>	'logicaldevice',
					'pdevice'		=>	'physicaldevice',
					'channel'		=>	'logicalchannel',
					'chnotifier'	=>	'changenotifier'
				);

my @object_list = keys(%objects);

my @fast;
my @slow;


my @input;
my $file;
my $line;
my $state = 0;
my $xref;
# need to add drive letter to path of execs.txt as cpp doesn't like
# absolute paths without a drive letter

my $drive = "";
if (($^O eq "" || $^O =~ /mswin32/i) && $opts{input} !~ /^[a-z]:/i)
{
$drive = substr(Cwd::cwd(),0,2);
}

open PIPE, "cpp -I- -I. -lang-c++ $drive$opts{input} | ";
while (<PIPE>)	{
	if (/^\s*\#\s+(\d+)\s+\"(.*?)\"/) {
		$line = $1;
		$file = $2;
		next;
	}
	if (/^\s*$/) {
		++$line;
		next;
	}
	if ($state == 0) {
		my %exec;
		if (/^\s*slow\s*\{\s*$/) {
			$exec{'type'} = 0;
			$exec{'file'} = $file;
			$exec{'line'} = $line;
			$xref = \%exec;
		} elsif (/^\s*fast\s*\{\s*$/) {
			$exec{'type'} = 1;
			$exec{'file'} = $file;
			$exec{'line'} = $line;
			$xref = \%exec;
		} else {
			die "Syntax error at $file line $line\n";
		}
		++$state;
	} elsif ($state == 1) {
		if (/^\s*}\s*$/) {
			if ($$xref{'type'}==0) {
				push @slow, $xref;
			} elsif ($$xref{'type'}==1) {
				push @fast, $xref;
			}
			undef $xref;
			$state = 0;
		} else {
			if (/^\s*(\w+)(.*?)$/) {
				my $keyword = $1;
				my $rest = $2;
				my $argument = -1;
				my $match_res = matchabbrev(\$keyword, \@keyword_list, \%keyword_synonyms);
				if ($match_res) {
					die "$match_res keyword $keyword at $file line $line\n";
				}
				my $flags = $keywords{$keyword};
				if ($$xref{'type'}==1 && ($flags&4)==0) {
					die "Keyword $keyword not permitted with FAST exec at $file line $line\n";
				}
				if ($$xref{'type'}==0 && ($flags&8)==0) {
					die "Keyword $keyword not permitted with SLOW exec at $file line $line\n";
				}
				if ($rest =~ /^\s*$/) {
					die "Missing argument at $file line $line\n" if ($flags & 2);
				} else {
					die "Invalid argument at $file line $line\n" unless ($flags & 1);
					if ($rest =~ /^\s*\=\s*(.*?)\s*$/) {
						$argument = $1;
					} else {
						die "Invalid argument syntax at $file line $line\n";
					}
				}
				if ($keyword eq 'hw') {
					$keyword = 'ifdef';
					$argument = '__EPOC32__';
					$flags = 0x1f;
				} elsif ($keyword eq 'emulator') {
					$keyword = 'ifndef';
					$argument = '__EPOC32__';
					$flags = 0x1f;
				}
				my $existing = $$xref{$keyword};
				if ($flags & 16) {
					if (!defined($existing)) {
						my @kw_args = ();
						$$xref{$keyword} = \@kw_args;
						$existing = \@kw_args;
					}
					push @$existing, $argument;
				} else {
					if (!defined($existing)) {
						$$xref{$keyword} = $argument;
					} else {
						die "Duplicate keyword $keyword at $file line $line\n";
					}
				}
			}
		}
	}
	++$line;
}
close PIPE;

checkexecs(\@fast, 'fast');
checkexecs(\@slow, 'slow');

my @ixfast = sortexecs(\@fast);
my @ixslow = sortexecs(\@slow);

my $fileheader = <<EOF;
//
// Copyright (c) 2005 Nokia Corporation and/or its subsidiary(-ies).All rights reserved.
//
// GENERATED FILE - DO NOT EDIT
//

/**
 * \@file
 * \@internalComponent
 */

EOF

my @enumfile;
push @enumfile, $fileheader;
genenum(\@ixfast, \@enumfile, "TFastExecDispatch");
genenum(\@ixslow, \@enumfile, "TExecDispatch");

my @userfile;
push @userfile, $fileheader;
push @userfile, "class Exec\n\t{\npublic\:\n";
genexecp(\@ixfast, \@userfile);
genexecp(\@ixslow, \@userfile);
push @userfile, "\t\};\n\n";
push @userfile, "#ifdef __GEN_USER_EXEC_CODE__\n";
genuser(\@ixfast, \@userfile);
genuser(\@ixslow, \@userfile);
push @userfile, "#endif\n";

my @kernelfile;
push @kernelfile, $fileheader;
push @kernelfile, "class ExecHandler\n\t{\npublic\:\n";
genexechp(\@ixfast, \@kernelfile);
genexechp(\@ixslow, \@kernelfile);
push @kernelfile, "\t\};\n\n";
push @kernelfile, "#ifdef __GEN_KERNEL_EXEC_CODE__\n";
genkernelfast(\@ixfast, \@kernelfile);
genkernelslow(\@ixslow, \@kernelfile);
push @kernelfile, "#endif\n";

if ($opts{enum}) {
	open FILE, ">$opts{enum}" or die "Can't open file $opts{enum} for write\n";
	print FILE @enumfile;
	close FILE;
}
if ($opts{user}) {
	open FILE, ">$opts{user}" or die "Can't open file $opts{user} for write\n";
	print FILE @userfile;
	close FILE;
}
if ($opts{kernel}) {
	open FILE, ">$opts{kernel}" or die "Can't open file $opts{kernel} for write\n";
	print FILE @kernelfile;
	close FILE;
}




# Match an input string against a list of strings with a list of synonyms.
# Any non-ambiguous abbreviation will be accepted.
# IN:	1st argument is input string
#		2nd argument is reference to match list
#		3rd argument is reference to synonym hash
# OUT:	If match found, return undef and replace first argument with unabbreviated match string
#		If no match found, return "Unknown"
#		If ambiguous input, return "Ambiguous"
#
sub matchabbrev($$$) {
	my ($inref, $lref, $sref)=@_;
	my @matches = grep(/^$$inref/i,@$lref);
	my $nmatches = scalar(@matches);
	my $synonym = 0;
	if ($nmatches==0) {
		@matches = grep(/^$$inref/i,keys(%$sref));
		$nmatches = scalar(@matches);
		$synonym = 1;
	}
	return "Unknown" if ($nmatches==0);
	if ($nmatches>1) {
		my @xmatches=grep(/^$$inref$/i,@matches);
		return "Ambiguous" if (scalar(@xmatches)!=1);
		$$inref=$xmatches[0];
	} else {
		$$inref=$matches[0];
	}
	$$inref = $sref->{$$inref} if ($synonym);
	return undef;
}


#
# Check a list of executive calls for bad or missing arguments
# and duplicate definitions.
# Assign executive dispatch numbers.
#
sub checkexecs($$) {
	my ($listref, $listname) = @_;
	my $next_num = 0;
	my @execnum;
	my @execnames;
	foreach(@$listref)	{
		my $xref = $_;
		my $n = $xref->{'number'};
		my $type = $xref->{'type'};
		if ($n) {
			if ($execnum[$n]) {
				die "Duplicate $listname exec number $n at $xref->{'file'} line $xref->{'line'}\n";
			}
			$execnum[$n] = 1;
		}
		my $nm = $xref->{'name'};
		if (!$nm) {
			die "Unnamed $listname exec at $xref->{'file'} line $xref->{'line'}\n";
		} elsif ($nm !~ /^\w+$/) {
			die "Invalid $listname exec name $nm at $xref->{'file'} line $xref->{'line'}\n";
		}
		if ( scalar(grep {$_ eq $nm} @execnames) ) {
			die "Duplicate $listname exec name $nm at $xref->{'file'} line $xref->{'line'}\n";
		}
		push @execnames, $nm;
		$xref->{'enumname'} = ($type==1) ? "EFastExec$nm" : "EExec$nm";
		my $u = $xref->{'user'};
		if ($u) {
			if ($u eq 'special') {
				undef $xref->{'userclass'};
				undef $xref->{'userfunc'};
				$xref->{'specialuser'} = -1;
			} elsif ($u =~ /^(\w+)\:\:(\w+)$/) {
				$xref->{'userclass'} = $1;
				$xref->{'userfunc'} = $u;
			} elsif ($u =~ /^\:\:(\w+)$/) {
				$xref->{'userclass'} = '';
				$xref->{'userfunc'} = $1;
			} elsif ($u =~ /^(\w+)$/) {
				$xref->{'userclass'} = $1;
				$xref->{'userfunc'} = "$1::$nm";
			}
		} else {
			$xref->{'userclass'} = 'Exec';
			$xref->{'userfunc'} = "Exec::$nm";
		}
		my $k = $xref->{'kernel'};
		if ($k) {
			if ($k eq 'special') {
				undef $xref->{'kernelclass'};
				undef $xref->{'kernelfunc'};
				if ($type!=1 || $n!=0) {
					die "special kernel only applicable to FAST EXEC 0\n";
				}
			} elsif ($k =~ /^(\w+)\:\:(\w+)$/) {
				$xref->{'kernelclass'} = $1;
				$xref->{'kernelfunc'} = $k;
			} elsif ($k =~ /^\:\:(\w+)$/) {
				$xref->{'kernelclass'} = '';
				$xref->{'kernelfunc'} = $1;
			} elsif ($k =~ /^(\w+)$/) {
				$xref->{'kernelclass'} = $1;
				$xref->{'kernelfunc'} = "$1::$nm";
			}
		} else {
			$xref->{'kernelclass'} = 'ExecHandler';
			$xref->{'kernelfunc'} = "ExecHandler::$nm";
		}
		$xref->{'karg1'} = $xref->{'arg1'} if (!defined ($xref->{'karg1'}));
		my $nargs = 0;
		$nargs=1 if (defined $xref->{'karg1'});
		if (!$xref->{'return'}) {
			$xref->{'return'} = 'void';
		}
		$xref->{'nargs'} = $nargs;
		my $spk = $xref->{'specialkernel'};
		if ($spk =~ /(\w+)(\:\:\w+)?/) {
			$xref->{'kerneldispatch'} = $spk;
		} elsif ($spk) {
			die "Invalid function name with SPECIALKERNEL at $xref->{'file'} line $xref->{'line'}\n";
		} else {
			$xref->{'kerneldispatch'} = $xref->{'kernelfunc'};
		}
		next if ($type == 1);		# nothing more to do for fast execs
		$xref->{'karg2'} = $xref->{'arg2'} if (!defined ($xref->{'karg2'}));
		$xref->{'karg3'} = $xref->{'arg3'} if (!defined ($xref->{'karg3'}));
		$xref->{'karg4'} = $xref->{'arg4'} if (!defined ($xref->{'karg4'}));
		my $h = $xref->{'handle'};
		if ($h) {
			my $matchres = matchabbrev(\$h, \@object_list, \%object_synonyms);
			if ($matchres) {
				die "$matchres object type at $xref->{'file'} line $xref->{'line'}\n";
			}
			my $obj_info = $objects{$h};
			my $add_one = $obj_info->[2];
			if ($h eq 'any') {
				$xref->{'handlep'} = "0";
			} else {
				if ($add_one) {
					$xref->{'handlep'} = "$obj_info->[0]+1";
				} else {
					$xref->{'handlep'} = "$obj_info->[0]";
				}
			}
			$xref->{'arg1'} = 'TInt';
			$xref->{'karg1'} = "$obj_info->[1]"."*";
			$xref->{'lock'} = -1;
		}
		$nargs=1 if (defined $xref->{'karg1'});
		$nargs=2 if (defined $xref->{'karg2'});
		$nargs=3 if (defined $xref->{'karg3'});
		$nargs=4 if (defined $xref->{'karg4'});
		$xref->{'nargs'} = $nargs;
		if ($xref->{'nr'} && !$xref->{'lock'}) {
			die "NR without LOCK/HANDLE at $xref->{'file'} line $xref->{'line'}\n";
		}
		if ($xref->{'unprotected'} && $xref->{'lock'}) {
			die "UNPROTECTED and LOCK/HANDLE incompatible at $xref->{'file'} line $xref->{'line'}\n";
		}
		my $ext = $xref->{'extended'};
		if ($ext) {
			if ( ($ext !~ /^\d$/) || $ext<2 || $ext2>8) {
				die "Invalid argument to EXTENDED at $xref->{'file'} line $xref->{'line'}\n";
			} elsif ($nargs<3) {
				die "Need >=3 normal arguments with EXTENDED at $xref->{'file'} line $xref->{'line'}\n";
			}
		}
	}
	foreach(@$listref)	{
		my $xref = $_;
		my $n = $xref->{'number'};
		unless ($n) {
			while ($execnum[$next_num]) {
				++$next_num;
			}
			$xref->{'number'} = $next_num++;
		}
	}
}


#
# Return a list of execs indexed by dispatch number
# Include gaps for nonexistent ones
#
sub sortexecs($) {
	my ($listref) = @_;
	my @indexed;
	my $xref;
	foreach $xref (@$listref) {
		my $n = $xref->{'number'};
		$indexed[$n] = $xref;
	}
	return @indexed;
}

#
# Generate enum definition
#
sub genenum($$$) {
	my ($srcref, $destref, $enumname) = @_;
	push @$destref, "\nenum $enumname\n\t\{\n";
	my $n;
	my $xref;
	for ($n=0; $n<scalar(@$srcref); ++$n) {
		$xref = $$srcref[$n];
		if ($xref) {
			push @$destref, "\t$xref->{'enumname'} = $n,\n";
		}
	}
	push @$destref, "\t\};\n";
}

#
# Output Exec class prototypes
#
sub genexecp($$) {
	my ($srcref, $destref) = @_;
	my $n;
	my $xref;
	for ($n=0; $n<scalar(@$srcref); ++$n) {
		$xref = $$srcref[$n];
		if ($xref && $xref->{'userclass'} eq 'Exec') {
			$xref->{'userfunc'} =~ /^Exec\:\:(\w+)$/;
			my $fdecl = "\t";
			$fdecl .= "IMPORT_C " if ($xref->{'export'});
			$fdecl .= "static $xref->{'return'} $1\(";
			$fdecl .= $xref->{'arg1'} if ($xref->{'arg1'});
			$fdecl .= ", $xref->{'arg2'}" if ($xref->{'arg2'});
			$fdecl .= ", $xref->{'arg3'}" if ($xref->{'arg3'});
			$fdecl .= ", $xref->{'arg4'}" if ($xref->{'arg4'});
			$fdecl .= "\);\n";
			push @$destref, $fdecl;
		}
	}
}

#
# Output ExecHandler class prototypes
#
sub genexechp($$) {
	my ($srcref, $destref) = @_;
	my $n;
	my $xref;
	for ($n=0; $n<scalar(@$srcref); ++$n) {
		$xref = $$srcref[$n];
		if ($xref && $xref->{'kernelclass'} eq 'ExecHandler') {
			$xref->{'kernelfunc'} =~ /^ExecHandler\:\:(\w+)$/;
			my $fdecl = "\tstatic $xref->{'return'} $1\(";
			$fdecl .= $xref->{'karg1'} if ($xref->{'karg1'});
			$fdecl .= ", $xref->{'karg2'}" if ($xref->{'karg2'});
			$fdecl .= ", $xref->{'karg3'}" if ($xref->{'karg3'});
			$fdecl .= ", $xref->{'karg4'}" if ($xref->{'karg4'});
			$fdecl .= "\);\n";
			push @$destref, $fdecl;
		}
	}
}


#
# Create conditional inclusion line
#
sub conditional($) {
	my ($xref) = @_;
	my $ifdefref = $xref->{'ifdef'};
	my $ifndefref = $xref->{'ifndef'};
	if (!$ifdefref and !$ifndefref) {
		return undef;
	}
	my $n = 0;
	my $cond = '#if';
	foreach $x (@$ifdefref) {
		$cond .= ' &&' if ($n);
		++$n;
		$cond .= " defined($x)";
	}
	foreach $x (@$ifndefref) {
		$cond .= ' &&' if ($n);
		++$n;
		$cond .= " !defined($x)";
	}
	return $cond;
}


#
# Output user-side code
#
sub genuser($$) {
	my ($srcref, $destref) = @_;
	my $n;
	my $xref;
	for ($n=0; $n<scalar(@$srcref); ++$n) {
		$xref = $$srcref[$n];
		if ($xref && !$xref->{'specialuser'}) {
			my $fdecl = "";
			my $endif = 0;
			my $cond = "";
			$cond = conditional($xref) unless ($xref->{'export'});
			if ($cond) {
				$fdecl .= "$cond\n";
				$endif = 1;
			}
			$fdecl .= "EXPORT_C " if ($xref->{'export'});
			$fdecl .= "__EXECDECL__ $xref->{'return'} $xref->{'userfunc'}\(";
			$fdecl .= $xref->{'arg1'} if ($xref->{'arg1'});
			$fdecl .= ", $xref->{'arg2'}" if ($xref->{'arg2'});
			$fdecl .= ", $xref->{'arg3'}" if ($xref->{'arg3'});
			$fdecl .= ", $xref->{'arg4'}" if ($xref->{'arg4'});
			$fdecl .= "\)\n\t\{\n";
			my $macro = ($xref->{'type'}==1) ? "FAST_EXEC" : "SLOW_EXEC";
			$macro.=chr(48+$xref->{'nargs'});
			$fdecl .= "\t$macro\($xref->{'enumname'}\);\n";
			$fdecl .= "\t\}\n";
			$fdecl .= "#endif\n" if ($endif);
			$fdecl .= "\n";
			push @$destref, $fdecl;
		}
	}
}


#
# Output fast exec dispatch table
#
sub genkernelfast($$) {
	my ($srcref, $destref) = @_;
	my $fast_count = scalar(@$srcref);
	push @$destref, "#define\tFAST_EXEC_COUNT\t\t$fast_count\n\n";
	push @$destref, "FAST_EXEC_BEGIN\n";
	push @$destref, "DECLARE_WORD(FAST_EXEC_COUNT)\n";
	my $n;
	my $xref;
	for ($n=0; $n<$fast_count; ++$n) {
		$xref = $$srcref[$n];
		my $endif=0;
		if ($xref && $xref->{'kernelfunc'}) {
			my $cond = conditional($xref);
			if ($cond) {
				push @$destref, "$cond\n";
				$endif = 1;
			}
			my $asm = $xref->{'asm'};
			if ($asm =~ /^\w+$/) {
				push @$destref, "#ifdef $asm\n";
			}
			if ($asm) {
				push @$destref, "DECLARE_ASM_FUNC(_asm_exec_$xref->{'name'}, $xref->{'kerneldispatch'})\n";
			} else {
				push @$destref, "DECLARE_FUNC($xref->{'kerneldispatch'})\n";
			}
			if ($asm =~ /^\w+$/) {
				push @$destref, "#else\n";
				push @$destref, "DECLARE_FUNC($xref->{'kerneldispatch'})\n";
				push @$destref, "#endif\n";
			}
			if ($endif) {
				push @$destref, "#else\nDECLARE_FAST_EXEC_INVALID\n#endif\n";
			}
		} elsif (!$xref || $xref->{'number'}!=0) {
			push @$destref, "DECLARE_FAST_EXEC_INVALID\n";
		}
	}
	push @$destref, "FAST_EXEC_END\n\n\n";
}



#
# Output slow exec dispatch table
#
sub genkernelslow($$) {
	my ($srcref, $destref) = @_;
	my $slow_count = scalar(@$srcref);
	push @$destref, "#define\tSLOW_EXEC_COUNT\t\t$slow_count\n\n";
	push @$destref, "SLOW_EXEC_BEGIN\n";
	push @$destref, "DECLARE_WORD(SLOW_EXEC_COUNT)\n";
	push @$destref, "DECLARE_INVALID_EXEC_HANDLER\n";
	push @$destref, "DECLARE_EXEC_PREPROCESS_HANDLER\n";
	my $n;
	my $xref;
	for ($n=0; $n<$slow_count; ++$n) {
		$xref = $$srcref[$n];
		my $endif=0;
		my $xflags="0";
		$xflags .= "|EF_C" if ($xref->{'lock'});
		$xflags .= "|EF_R" if ( ($xref->{'lock'}) && !($xref->{'nr'}) );
		if (defined $xref->{'handlep'}) {
			$xflags .= "|EF_P|($xref->{'handlep'})";
		}
		if ($xref->{'extended'}) {
			$xflags .= "|EF_A".chr($xref->{'extended'} + 48);
		}
		if ($xref && $xref->{'kernelfunc'}) {
			my $cond = conditional($xref);
			if ($cond) {
				push @$destref, "$cond\n";
				$endif = 1;
			}
			my $asm = $xref->{'asm'};
			if ($asm =~ /^\w+$/) {
				push @$destref, "#ifdef $asm\n";
			}
			if ($asm) {
				push @$destref, "DECLARE_FLAGS_ASMFUNC($xflags, _asm_exec_$xref->{'name'}, $xref->{'kerneldispatch'})\n";
			} else {
				push @$destref, "DECLARE_FLAGS_FUNC($xflags, $xref->{'kerneldispatch'})\n";
			}
			if ($asm =~ /^\w+$/) {
				push @$destref, "#else\n";
				push @$destref, "DECLARE_FLAGS_FUNC($xflags, $xref->{'kerneldispatch'})\n";
				push @$destref, "#endif\n";
			}
			if ($endif) {
				push @$destref, "#else\nDECLARE_SLOW_EXEC_INVALID\n#endif\n";
			}
		} else {
			push @$destref, "DECLARE_SLOW_EXEC_INVALID\n";
		}
	}
	push @$destref, "SLOW_EXEC_END\n\n\n";
}

