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

my @rootdirs;
open PIPE, "dir /b /ad \\ |";
while (<PIPE>) {
	my $dir=$_;
#	print;
	chomp $dir;
	unless (/^epoc32$/i) {
		$dir="\\".$dir."\\*.bld";
		push @rootdirs, $dir;
	}
}
close PIPE;
my %bldfiles;
my $dir;
foreach $dir (@rootdirs) {
#	print "$dir\n";
	open PIPE, "dir /s /b $dir 2>NUL |";
	while (<PIPE>) {
		my %bldfileprops;
		$bldfileprops{'fullname'}=lc $_;
		/\\(\w+)\.bld$/;
		my $name=lc $1;
		$bldfileprops{'name'}=$name;
		if (defined $bldfiles{$name}) {
			die "Duplicate build file name $name\n";
		}
		$bldfiles{$name}=\%bldfileprops;
	}
	close PIPE;
}

my $bld;
my @defaults;
my @compulsory;
foreach $bld (keys %bldfiles) {
	my $ref=$bldfiles{$bld};
	my $filename=$$ref{'fullname'};
	chomp $filename;
	my @options;
	my @components;
	open FILE, $filename or die "Could not open file $filename\n";
	while (<FILE>) {
		if (/^\!(\w+)$/) {
			$$ref{lc $1}=1;
		} elsif (/^\<option/i) {
			push @options, $_;
		} elsif (!/^\s*$/) {
			push @components, $_;
		}
	}
	close FILE;
	$$ref{'options'}=\@options;
	$$ref{'components'}=\@components;
	if (!$$ref{'explicit'}) {
		push @defaults, $bld;
	}
	if ($$ref{'compulsory'}) {
		if ($$ref{'explicit'}) {
			die "File $filename: can't be COMPULSORY and EXPLICIT\n";
		}
		push @compulsory, $bld;
	}
}

my @todo_temp;
my @omit;
my $i;
my $nargs=scalar(@ARGV);
if ($nargs==0) {
	@todo_temp=@defaults;
} else {
	@todo_temp=@compulsory;
	for ($i=0; $i<$nargs; ++$i) {
		my $name=lc($ARGV[$i]);
		if ($name eq '+') {
			foreach $bld (@defaults) {
				push @todo_temp, $bld if (!grep {$bld eq $_} @todo_temp);
			}
		} elsif ($name=~/^\-(\w+)$/) {
			if (defined $bldfiles{$1}) {
				my $ref=$bldfiles{$1};
				if ($$ref{'compulsory'}) {
					die "Cannot omit $1\n";
				}
				push @omit, $1 if (!grep {$1 eq $_} @omit);
			} else {
				die "Unrecognised build $1\n";
			}
		} elsif (defined $bldfiles{$name}) {
			push @todo_temp, $name if (!grep {$name eq $_} @todo_temp);
		} else {
			die "Unrecognised build $name\n";
		}
	}
}

#print join "\n",@todo_temp;
#print "\n";

my @todo;
foreach $bld (@todo_temp) {
	push @todo, $bld if (!grep {$bld eq $_} @omit);
}

#print join "\n",@todo;
#print "\n";

my @output1;
my @output2;
my $nvariants=0;
foreach $bld (@todo) {
	my $ref=$bldfiles{$bld};
	next if ($nargs==0 && $$ref{'explicit'});
	++$nvariants if (!$$ref{'incremental'});
	my $optref=$$ref{'options'};
	my $compref=$$ref{'components'};
	my $option;
	my $component;
	foreach $option (@$optref) {
		$option=~s/\s+/ /;
		if (!grep {$_ eq $option} @output1) {
			push @output1, $option;
		}
	}
	foreach $component (@$compref) {
		$component=~s/\s+/ /;
		if (!grep {$_ eq $component} @output2) {
			push @output2, $component;
		}
	}
}

if ($nvariants==0) {
	die "No variants specified for building\n";
}

@output1=sort @output1;
@output2=sort @output2;

my $output;
foreach (@output1) {
	$output.=$_;
}
$output.="\n";
foreach (@output2) {
	$output.=$_;
}
$output.="\n";

print "\n\n$output";

