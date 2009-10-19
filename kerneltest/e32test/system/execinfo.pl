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

my $state=0;
my @execs;
my @fastexecs;
my @params;
my $npar;
my $fast;
my $swinum;
while (<STDIN>) {
	if ($state==0) {
		if (/^(.*?)__NAKED__(.*?)\((.*)\)\s*$/) {
			@params=split(',' , $3);
			$npar=scalar(@params);
			$state=1;
		}
	} elsif ($state==1) {
		if (/^\s*\{\s*$/) {
			$state=2;
		}
	} elsif ($state==2) {
		if (/^\s*asm\s*\(\s*\"\s*swi(.*?)(EExec\w+)/) {
			$swinum=$2;
			$fast=0;
			$state=3;
		} elsif (/^\s*asm\s*\(\s*\"\s*swi(.*?)(EFastExec\w+)/) {
			$swinum=$2;
			$fast=1;
			$state=3;
		}
	} elsif ($state==3) {
		if (/^\s*\}\s*$/) {
			$state=0;
			my %info;
			$info{'num'}=$swinum;
			$info{'npar'}=$npar;
			$info{'par1'}=$params[0] if ($npar>0);
			$info{'par2'}=$params[1] if ($npar>1);
			$info{'par3'}=$params[2] if ($npar>2);
			$info{'par4'}=$params[3] if ($npar>3);
			push @fastexecs, \%info if ($fast);
			push @execs, \%info unless ($fast);
		}
	}
}

my $nsx=scalar(@execs);
my $nfx=scalar(@fastexecs);

print "Number of fast execs = $nfx\n";
print "Number of slow execs = $nsx\n";

my $i=0;
print "\n\nstatic const SFastExecInfo[]=\n\t{\n";
foreach (@fastexecs) {
	my $infoRef=$_;
	print "\t\t{\n";
	print "\t\t$$infoRef{'num'},\n";
	print "\t\t$$infoRef{'npar'},\n";
	if ($$infoRef{'par1'}) {
		print "\t\t$$infoRef{'par1'},\n";
	} else {
		print "\t\tNO_PAR,\n";
	}
	if ($$infoRef{'par2'}) {
		print "\t\t$$infoRef{'par2'},\n";
	} else {
		print "\t\tNO_PAR,\n";
	}
	if ($$infoRef{'par3'}) {
		print "\t\t$$infoRef{'par3'},\n";
	} else {
		print "\t\tNO_PAR,\n";
	}
	if ($$infoRef{'par4'}) {
		print "\t\t$$infoRef{'par4'},\n";
	} else {
		print "\t\tNO_PAR\n";
	}
	if (++$i==$nfx) {
		print "\t\t}\n";
	} else {
		print "\t\t},\n";
	}
}
print"\t};\n";

$i=0;
print "\n\nstatic const SSlowExecInfo[]=\n\t{\n";
foreach (@execs) {
	my $infoRef=$_;
	print "\t\t{\n";
	print "\t\t$$infoRef{'num'},\n";
	print "\t\t$$infoRef{'npar'},\n";
	if ($$infoRef{'par1'}) {
		print "\t\t$$infoRef{'par1'},\n";
	} else {
		print "\t\tNO_PAR,\n";
	}
	if ($$infoRef{'par2'}) {
		print "\t\t$$infoRef{'par2'},\n";
	} else {
		print "\t\tNO_PAR,\n";
	}
	if ($$infoRef{'par3'}) {
		print "\t\t$$infoRef{'par3'},\n";
	} else {
		print "\t\tNO_PAR,\n";
	}
	if ($$infoRef{'par4'}) {
		print "\t\t$$infoRef{'par4'},\n";
	} else {
		print "\t\tNO_PAR\n";
	}
	if (++$i==$nsx) {
		print "\t\t}\n";
	} else {
		print "\t\t},\n";
	}
}
print"\t};\n";

foreach (@execs) {
	print "$$_{'num'}\n";
}

