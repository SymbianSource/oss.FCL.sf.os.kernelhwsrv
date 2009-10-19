# Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# hal\halcfg.pl
# Check arguments and open files
# 
#

my $nargs=scalar(@ARGV);
my $xmode=0;
if ($nargs==0) {
	usage();
}
elsif ($nargs!=3 and $nargs!=4) {
	die "Invalid number of arguments.  Run with no arguments for help on usage.\n";
	exit;
}
my $arg=0;
if ($nargs==4) {
	if ($ARGV[$arg] eq '-x') {
		$xmode=1;
	} elsif ($ARGV[$arg] eq '-s') {
		$xmode=2;
	} elsif ($ARGV[$arg] eq '-h') {
		$xmode=3;
	} else {
		usage();
		die "Invalid 1st argument $ARGV[0]\n";
	}
	++$arg;
}
my $headerFileName=$ARGV[$arg++];
my @hdr = &read_file_strip_comments($headerFileName);

my $inputFileName=$ARGV[$arg++];
my @input = &read_file_strip_comments($inputFileName);

my $outputFileName=$ARGV[$arg++];

#
# Parse the header file
#
my $line=0;
my $state=0;
my $enumName;
my %enumValues=();
my $nextValue=0;
my $bitmask=0;
my %enumList=();
foreach (@hdr) {
	++$line;
	next if (/^\s*\#/);		# ignore preprocessor directives
	next if (/^\s*\/\//);	# ignore comments starting with //
	next if (/^\s*$/);		# ignore blank lines
	if ($state==0) {
		if (/^\s*class\s+HALData\s*$/) {
			$state=1;
		}
		if (/^\s*class\s+HALData\s*{/) {
			$state=2;
		}
		next;
	}
	if ($state==1) {
		if (/^\s*{/) {
			$state=2;
		}
		next;
	}
	if ($state==2) {
		if (/^\s*};/) {
			$state=3;
			last;
		}
		if (/^\s*enum\s+(\w+)(.*)/) {
			$enumName=$1;
			%enumValues=();
			$nextValue=0;
			$bitmask=0;
			if ($2=~/^\s+{/) {
				$state=5;
			} else {
				$state=4;
			}
		}
		if (/^\s*bitmask\s+(\w+)(.*)/) {
			$enumName=$1;
			%enumValues=();
			$nextValue=0;
			$bitmask=1;
			if ($2=~/^\s+{/) {
				$state=5;
			} else {
				$state=4;
			}
		}
		next;
	}
	if ($state==4) {
		if (/^\s*{/) {
			$state=5;
		}
		next;
	}
	if ($state==5) {
		if (/^\s*(\w+)(.*)/) {
			my $tag=$1;
			my $val=0;
			my $rest=$2;
#			print "$tag\n$rest\n";
			if ($rest=~/^\s*,/) {
				$val=$nextValue;
			} elsif ($rest=~/^\s*\=\s*(\d\w*)\s*\,/) {
				$val=$1;
				unless ($val=~/^\d*$/) {
					if ($val=~/^0x(\w+)$/i) {
						$val=hex $1;
					} else {
						undef $val;
					}
				}
			} elsif ($rest=~/^\s*$/) {	# ignore last one
				next;
			} else {
				undef $val;
			}
			unless (defined $val) {
				die "Syntax error at line $line in file $headerFileName\n";
			}
			$nextValue=$val+1;
			$enumValues{$tag}=$val;
#			print "$tag=$val\n";
		} elsif (/^\s*};/) {
			$state=2;
			if ($bitmask) {
				$enumValues{'__bitmask__'}=-1;
			}
			my %temp=%enumValues;
			$enumList{$enumName}=\%temp;
		}
		next;
	}
}
if ($state!=3) {
	die "Unexpected end of file in $headerFileName\n";

}

#	my @keys=keys %enumList;
#	foreach(@keys) {
#		print "enum $_\n\t{\n";
#		my $ref=$enumList{$_};
#		my @tags=keys(%$ref);
#		foreach(@tags) {
#			my $value=$$ref{$_};
#			print "\t$_=$value\n";
#		}
#		print "\t};\n";
#	}

#
# Build a list of properties for each attribute
#
my $attribref=$enumList{'TAttribute'};
unless ($attribref) {
	die "No TAttribute enum defined\n";
}
my @attribs=keys %$attribref;
my %attribList;
foreach (@attribs) {
	my %properties;
	$properties{'name'}=$_;
	$properties{'ordinal'}=$$attribref{$_};
	my $enum=$_;
	$enum=~s/^E/T/;			# change initial E to T
	if (defined $enumList{$enum}) {
		my $enumRef=$enumList{$enum};
		$properties{'enum'}=$enumRef;
		if (defined $$enumRef{'__bitmask__'}) {
			$properties{'bitmask'}=1;
		}
	}
	$attribList{$_}=\%properties;
}

my $attpropref=$enumList{'TAttributeProperty'};
my %PropTable;
if ($xmode) {
	unless ($attpropref) {
		die "No TAttributeProperty enum defined\n";
	}

	my @attprops=keys(%$attpropref);
	foreach (@attprops) {
		if (/^E(\w+)$/) {
			my $propname=lc $1;
			$PropTable{$propname}='HAL::'.$_;
		} else {
			die "Invalid attribute property $_\n";
		}
	}
}
my @PropTableKeys=keys %PropTable;


#
# Parse the input file
#
$line=0;
foreach (@input) {
	++$line;
	next if (/^\s*\/\//);	# ignore comments starting with //
	next if (/^\s*$/);		# ignore blank lines
	if (/^\s*(\w+)\s*(.*)/) {
		my $attrib=$1;
		my $rest=$2;
		my $propRef=$attribList{$attrib};
		unless (defined $propRef) {
			die "Unrecognised attribute at line $line in file $inputFileName\n";
		}
#		print "$rest\n";
		if ($rest=~/^\:\s*(.*)/) {	# attribute properties follow
			if (!$xmode) {
				die "Line $line: Properties not permitted without -x option\n";
			}
			$rest=$1;
#			print "$rest\n";
			while ($rest=~/^(\w+)\s*(.*)/) {
				my $prop=lc $1;
				$rest=$2;
#				print "$rest\n";
				my $error=matchabbrev(\$prop, \@PropTableKeys);
				if ($error) {
					die "$error property $prop at line $line in file $inputFileName\n";
				}
				$$propRef{$prop}=1;
				if ($rest=~/^,\s*(.*)/) {
					$rest=$1;
				} elsif ($rest=~/^=\s*(.*)/) {
					$rest=$1;
					last;
				} else {
					die "Syntax error at line $line in file $inputFileName\n";
				}
			}
		} elsif ($rest=~/^=\s*(.*)/) {
			$rest=$1				# attribute value follows
		} else {
			die "Invalid attribute specification at line $line in file $inputFileName\n";
		}
#		print "$rest\n";
		if ($xmode) {
#			print "$rest\n";
			if ($rest=~/^((\w|:)+)/) {
				$$propRef{'value'}=$1;
			} else {
				die "Invalid function name $rest at line $line in file $inputFileName\n";
			}
		} elsif (defined $$propRef{'bitmask'}) {		# bitmask value
			my $enumRef=$$propRef{'enum'};
			my @keys=keys %$enumRef;
			my $val=0;
			while ($rest=~/^(\w+)\s*(.*)/) {
				my $bitmaskKey=$1;
				$rest=$2;
				if ($bitmaskKey eq '0' or lc($bitmaskKey) eq 'none') {
					last if ($val==0);
					die "Inconsistent bit mask values at line $line in file $inputFileName\n";
				}
				my $error=matchabbrev(\$bitmaskKey,\@keys);
				if ($error) {
					die "$error bit value $bitmaskKey at line $line in file $inputFileName\n";
				}
				$val |= $$enumRef{$bitmaskKey};
				if ($rest=~/^\+\s*(.*)/) {
					$rest=$1;
				} elsif ($rest=~/^\s*$/) {
					last;
				} else {
					die "Syntax error at line $line in file $inputFileName\n";
				}
			}
			$$propRef{'value'}=$val;
		} elsif (defined $$propRef{'enum'} and $rest!~/^\d/) {	# enum value
			my $enumRef=$$propRef{'enum'};
			my @keys=keys %$enumRef;
			if ($rest=~/^(\w+)\s*$/) {
				my $enumKey=$1;
				my $error=matchabbrev(\$enumKey,\@keys);
				if ($error) {
					die "$error enumeration value $enumKey at line $line in file $inputFileName\n";
				}
				$$propRef{'value'}=$$enumRef{$enumKey};
			} else {
				die "Invalid enum value $rest at line $line in file $inputFileName\n";
			}
		} elsif ($rest=~/^(\-?\d\w*)\s*$/) {		# numeric value (decimal or hex) with optional -ve sign
			my $val=$1;
			unless ($val=~/^(\-?\d)\d*$/) {         # First character should be an optional -ve sign followed by only digits
				if ($val=~/^(\-?)0x(\w+)$/i) {       # First character should be an optional -ve sign followed by only alphanumerics 
					my $sign=$1;
					$val=hex $2;
					if ($sign  eq '-') {
					  $val=-$val;
					} 
				} else {
					undef $val;
				}
			}
			unless (defined $val) {
				die "Invalid attribute value $1 at line $line in file $inputFileName\n";
			}
			$$propRef{'value'}=$val;
		} else {								# invalid
			die "Invalid attribute value at line $line in file $inputFileName\n";
		}
	} else {
		die "Unrecognised attribute at line $line in file $inputFileName\n";
	}
}

#	foreach (@attribs) {
#		my $propRef=$attribList{$_};
#		if (defined $$propRef{'value'}) {
#			print "Attribute $_:\n";
#			my @keys=keys %$propRef;
#			foreach (@keys) {
#				print "\t$_=$$propRef{$_}\n";
#			}
#		}
#	}

#
# Sort attributes by ordinal
#
my @AttribsByOrdinal;
foreach (@attribs) {
	my $propRef=$attribList{$_};
	my $ordinal=$$propRef{'ordinal'};
	$AttribsByOrdinal[$ordinal]=$propRef;
}
my $nAttribs=scalar(@AttribsByOrdinal);

#
# Generate the output file
#

open OUT, ">$outputFileName" or die "Cannot open output file $outputFileName\n";

#	binmode OUT;
#	my $i=0;
#	while ($i<$nAttribs) {
#		my $value=0x80000000;
#		my $propRef=$AttribsByOrdinal[$i];
#		if (defined $$propRef{'value'}) {
#			$value=$$propRef{'value'};
#		}
#		++$i;
#		my $b3=($value>>24)&0xff;
#		my $b2=($value>>16)&0xff;
#		my $b1=($value>>8)&0xff;
#		my $b0=$value&0xff;
#		my $b0123=chr $b0;
#		$b0123.=chr $b1;
#		$b0123.=chr $b2;
#		$b0123.=chr $b3;
#		my $writeres=syswrite OUT, $b0123, 4;
#		if ($writeres != 4) {
#			die "Error writing output file $outputFileName\n";
#		}
#	}
#	exit;

my @splitName=split /(:|\\)/, $outputFileName;
my $rootName=pop @splitName;
my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime;
#print "\n$hour:$min:$sec $mday-$mon-$year wday=$wday, yday=$yday, isdst=$isdst\n\n";
$year+=1900;

print OUT "// $rootName\n";
print OUT "//\n";
print OUT "// Copyright (c) 1999-$year Nokia Corporation and/or its subsidiary(-ies).";
print OUT "// All rights reserved.\n";
print OUT "//\n";
if ($xmode!=2) {
	print OUT "// GENERATED FILE - DO NOT EDIT\n";
	print OUT "//\n";
}
print OUT "\n";
print OUT "#include <kernel/hal_int.h>\n";
if ($xmode==0) {
	print OUT <<EOF;
#ifdef __CW32__
// CodeWarrior requires EXPORT_C on the definiton here, as well as the declaration (in hal_int.h)
EXPORT_C const TInt HalInternal::InitialValue[]=
#else
EXPORT_D const TInt HalInternal::InitialValue[]=
#endif
	{
EOF

	my $i=0;
	while ($i<$nAttribs) {
		my $propRef=$AttribsByOrdinal[$i];
		++$i;
		my $separator=($i<$nAttribs)?',':'';
		my $name=$$propRef{'name'};
		my $value=0;
		if (defined $$propRef{'value'}) {
			$value=$$propRef{'value'};
		}
		print OUT "\t$value$separator\t\t// $name\n";
	}
	print OUT "\t};\n";
} elsif ($xmode==1) {
	print OUT "\n";
	my $i=0;
	while ($i<$nAttribs) {
		my $propRef=$AttribsByOrdinal[$i];
		++$i;
		my $name=$$propRef{'name'};
		my $imp=$$propRef{'value'};
		if ($imp=~/^\s*0\s*$/) {
			undef $imp;
		}
		if (defined $imp) {
			print OUT "GLREF_C TInt $imp(TInt, TInt, TBool, TAny*);\t// $name\n";
		}
	}
	print OUT "\n";
	print OUT "const TUint8 HalInternal::Properties[]=\n";
	print OUT "\t{\n";
	$i=0;
	while ($i<$nAttribs) {
		my $propRef=$AttribsByOrdinal[$i];
		++$i;
		my $separator=($i<$nAttribs)?',':'';
		my $properties="";
		my $name=$$propRef{'name'};
		if (defined $$propRef{'value'}) {
			$properties=$PropTable{'valid'};
			my @keys=keys(%$propRef);
			foreach (@keys) {
				my $pConst=$PropTable{$_};
				if (defined $pConst) {
					$properties.="|$pConst";
				}
			}
		}
		if ($properties=~/^\s*$/) {
			$properties='0';
		}
		print OUT "\t$properties$separator\t\t// $name\n";
	}
	print OUT "\t};\n";
	print OUT "\n#if 0\n";
	print OUT "const TInt HalInternal::Offset[]=\n";
	print OUT "\t{\n";
	my $memOffset=0;
	$i=0;
	while ($i<$nAttribs) {
		my $propRef=$AttribsByOrdinal[$i];
		++$i;
		my $separator=($i<$nAttribs)?',':'';
		my $name=$$propRef{'name'};
		my $imp=$$propRef{'value'};
		if ($imp=~/^\s*0\s*$/) {
			print OUT "\t$memOffset$separator\t\t// $name\n";
			$memOffset+=4;
		} else {
			print OUT "\t-1$separator\t\t// $name\n";
		}
	}
	print OUT "\t};\n";
	print OUT "\n#endif\n";
	print OUT "const TInt HalInternal::HalDataSize=$memOffset;\n";
	print OUT "\n";
	print OUT "const THalImplementation HalInternal::Implementation[]=\n";
	print OUT "\t{\n";
	$i=0;
	while ($i<$nAttribs) {
		my $propRef=$AttribsByOrdinal[$i];
		++$i;
		my $separator=($i<$nAttribs)?',':'';
		my $name=$$propRef{'name'};
		my $imp=$$propRef{'value'};
		if (!defined $imp or $imp=~/^\s*0\s*$/) {
			$imp='NULL';
		}
		print OUT "\t$imp$separator\t\t// $name\n";
	}
	print OUT "\t};\n";
} elsif ($xmode==2) {
	print OUT "\n";
	$i=0;
	while ($i<$nAttribs) {
		my $propRef=$AttribsByOrdinal[$i];
		++$i;
		my $name=$$propRef{'name'};
		my $imp=$$propRef{'value'};
		if ($imp=~/^\s*0\s*$/) {
			undef $imp;
		}
		my $setarg=' /*aSet*/';
		if (defined $$propRef{'settable'}) {
			$setarg=' aSet';
		}
		if (defined $imp) {
			print OUT "// $name\n";
			print OUT "TInt $imp(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool$setarg, TAny* aInOut)\n";
			print OUT "\t{\n";
			print OUT "\treturn KErrNone;\n";
			print OUT "\t}\n";
			print OUT "\n";
		}
	}
	print OUT "\n";
} elsif ($xmode==3) {
	my $hdrprot='__'.uc $rootName.'__';
	$hdrprot=~s/\./_/;
	print OUT "\n";
	print OUT "#ifndef $hdrprot\n";
	print OUT "#define $hdrprot\n";
	$i=0;
	while ($i<$nAttribs) {
		my $propRef=$AttribsByOrdinal[$i];
		++$i;
		my $name=$$propRef{'name'};
		my $imp=$$propRef{'value'};
		if ($imp=~/^\s*0\s*$/) {
			undef $imp;
		}
		if (defined $imp) {
			print OUT "GLREF_C TInt $imp(TInt, TInt, TBool, TAny*);\t// $name\n";
		}
	}
	print OUT "\n";
	print OUT "#endif\n";
}

print OUT "\n";

exit;

# END OF MAIN

sub matchabbrev($$) {
	my ($inref, $lref)=@_;
	my @matches=grep(/$$inref/i,@$lref);
	my $nmatches=scalar(@matches);
	if ($nmatches==0) {
		return "Unknown";
	} elsif ($nmatches>1) {
		my @xmatches=grep(/^$$inref$/i,@matches);
		if (scalar(@xmatches)!=1) {
			return "Ambiguous";
		} else {
			$$inref=$xmatches[0];
			return undef;
		}
	} else {
		$$inref=$matches[0];
		return undef;
	}
}

sub read_file_strip_comments($) {
	my ($filename) = @_;
	open IN, $filename or die "Cannot read file $filename\n";
	my $in;
	while (<IN>) {
		$in .= $_;
	}
	close IN;

	# Strip comments
	$in =~ s/\/\*(.*?)\*\//\n/gms;
	$in =~ s/\/\/(.*?)\n/\n/gms;

	return split(/(\n)/, $in);
}

sub usage() {
	print
		"\n",
		"halcfg.pl is a perl script that is used by the build system to generate the\n",
		"C++ source files from the Config and Values files.\n",
		"\n",
		"There are four modes in which the Perl script halcfg.pl can be used.\n",
		"\n",
		"\"perl halcfg.pl hal_data.h values.hda values.cpp\"\n",
		"\n",
		"takes the values.hda text file and generates a table of initial values for\n",
		"items stored by the HAL.\n",
		"\n",
		"\"perl halcfg.pl -x hal_data.h config.hcf config.cpp\"\n",
		"\n",
		"generates three tables:\n",
		"\n",
		"  - the properties, i.e. whether valid and/or writable, for each item\n",
		"\n",
		"  - the offset of each item within the DllGlobal block\n",
		"\n",
		"  - the function implementing each item, for derived attributes. ie. those\n",
		"    attributes that are not simply stored by the HAL.\n",
		"\n",
		"\"perl halcfg.pl -s hal_data.h config.hcf source.cpp\"\n",
		"\n",
		"generates a source file containing skeleton code for the implementation of the\n",
		"accessor function for each derived attribute\n",
		"\n",
		"\"perl halcfg.pl -h hal_data.h config.hcf header.h\"\n",
		"\n",
		"generates a header file containing prototypes for the accessor functions for\n",
		"each derived attribute\n",
		"\n",
		"Note that the header file hal_data.h containing the attributes and values used\n",
		"by the HAL is passed on all calls to the perl script.\n";

	exit;
}
