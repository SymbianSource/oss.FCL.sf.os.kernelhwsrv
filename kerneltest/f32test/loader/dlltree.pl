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
# \f32test\loader\dlltree.pl
# Generate header and build files for one or more trees
# of DLLs and EXEs according to a description file.
# 
#

require Cwd;

my $dllbasename='dllt';
my $dllext='.dll';
my $exebasename='exet';
my $exeext='.exe';
my $mmpext='.mmp';
my $defext='.def';
my $libext='.lib';

my $source0ext='.cpp';
my $source1ext='.cia';
my $xippath="sys\\bin\\";
my $nonxippath="sys\\bin\\";

my $hostpath="\\Epoc32\\Release\\##MAIN##\\##BUILD##\\";

my $flag_value_exe=1;
my $flag_value_fixed=2;
my $flag_value_data=4;
my $flag_value_xip=8;
my $flag_value_dll_in_cycle=16;
my $flag_value_data_in_tree=32;
my $flag_value_xip_data_in_tree=64;
my $flag_value_exports=128;
my $flag_value_pagedcode=256;
my $flag_value_unpagedcode=512;
my $flag_value_idrive=1024;
my $flag_value_vdrive=2048;
my $flag_value_bytepair=4096;
my $flag_value_uncompressed=8192;
my $flag_value_targetonly=16384;
my $flag_value_pageddata=32768;
my $flag_value_unpageddata=65536;

my $sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst;
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = gmtime;
my $copy_end=$year+1900;

my $argc=scalar(@ARGV);

# check whether its raptor specific  
my $is_raptor = 0;
if ($ARGV[$argc-1] eq "raptor") # change "raptor" to something you want.
{
	pop(@ARGV);
	$is_raptor = 1;
	$argc--;
}

($argc==1 or $argc==2 or $argc==3) or die "Usage: perl dlltree.pl <filename> <dir> [-allowbad]\n";
my $infile=$ARGV[0];
open IN, $infile or die "Cannot open input file $infile\n";
my @in;
while (<IN>)	{
	push @in, $_;
}
close IN;
my $destdir='';
if ($argc>=2) {
	$destdir=$ARGV[1];
	unless (-d $destdir) {
		mkdir $destdir, 0777 or die "Can't create directory $destdir\n";
	}
	chdir $destdir or die "Can't chdir to directory $destdir\n";
}
my $allowbad;
if ($argc==3) {
	if ($ARGV[2] eq '-allowbad') {
		$allowbad = 1;
	}
}
$cwd=Cwd::getcwd();
$destpath=$cwd;
$destpath=~s/\//\\/g;
if ($destpath=~/^\w\:(.*)/) {
	$destpath=$1;
}
unless ($destpath=~/^(.*?)\\$/) {
	$destpath.='\\';
}
#print "$cwd\n";
#print "$destpath\n";

my @modules;
my @testcases;
my $line=0;
my $modnum=0;
my $state=0;
my $concat;
foreach (@in)	{
	++$line;
	next if (/^\s*$/ or /^\s*\#/);
	if ($state==0)	{
		if (/^\!TESTCASES/) {
			$state=1;
			next;
		}
		my @moddesc=split;
		my $modname=shift @moddesc;
		if (grep {lc($$_{name}) eq lc($modname)} @modules) {
			die "Duplicate module specification at line $line\n";
		}
		my %module;
		$module{name}=$modname;
		$module{num}=$modnum;
		$module{exports}=1;
		$module{nclients}=0;	# number of modules which import from this one
		while (scalar(@moddesc)) {
			my $att=shift @moddesc;
			if ($att=~/^\:$/) {
				last;
			} elsif ($att=~/^X$/i) {
				$module{exe}=1;
			} elsif ($att=~/^F$/i) {
				$module{fixed}=1;
			} elsif ($att=~/^D$/i) {
				$module{data}=1;
			} elsif ($att=~/^P$/i) {
				$module{pagedcode}=1;
			} elsif ($att=~/^PD$/i) {
				$module{pageddata}=1;
			} elsif ($att=~/^B$/i) {
				$module{bytepair}=1;
			} elsif ($att=~/^U$/i) {
				$module{uncompressed}=1;
			} elsif ($att=~/^I$/i) {
				$module{idrive}=1;
			} elsif ($att=~/^V$/i) {
				$module{vdrive}=1;
			} elsif ($att=~/^N$/i) {
				$module{unpagedcode}=1;
			} elsif ($att=~/^T$/i) {
				$module{targetonly}=1;
			} elsif ($att=~/^R(.*?)$/i) {
				$module{xip}=1;
				my $attp=$1;
				if ($attp=~/^\,(.*?)$/) {
					$module{attpname}=$1;
				} elsif (!($attp=~/^\s*$/)) {
					die "Garbage following R attribute at line $line\n";
				}
			}
		}
		if ($module{fixed} && !$module{exe}) {
			die "Can't have FIXED without EXE at line $line\n";
		}
		if ($module{pagedcode} && $module{xip}) {
			die "Can't have PAGEDCODE with XIP at line $line\n";
		}

		# makmake won't support paged binaries which aren't bytepair or uncompressed.
		# However, P can be specified without them in dlltree.txt.  This program generates
		# an MMP file with neither flag.  The flag is added when the files are copied to
		# internal media by T_LDRTST.  (It would be pointless to copy the files to removable
		# media and then edit them because removable media doesn't support paging.)

		my $bothCodePageFlags = $module{pagedcode} && $module{unpagedcode};
		if ($bothCodePageFlags && !$module{idrive}) {
			die "Can't have PAGEDCODE and UNPAGEDCODE without INTERNAL DRIVE at line $line\n";
		}
		if ($module{unpagedcode} && $module{xip}) {
			die "Can't have UNPAGEDCODE with XIP at line $line\n";
		}
		if ($module{pageddata} && !$module{exe}) {
			die "Can't have PAGEDDATA without EXE at line $line\n";
		}
		if ($module{idrive} && $module{xip}) {
			die "Can't have INTERNAL DRIVE with XIP at line $line\n";
		}
		if ($module{vdrive} && $module{xip}) {
			die "Can't have REMOVABLE DRIVE with XIP at line $line\n";
		}
		if ($module{idrive} && $module{vdrive}) {
			die "Can't have INTERNAL DRIVE with REMOVABLE DRIVE at line $line\n";
		}
		if ($module{bytepair} && $module{uncompressed}) {
			die "Can't have BYTEPAIR with UNCOMPRESSED at line $line\n";
		}
		if ($module{exe}) {
			if ($module{attpname}) {
				warn "Attach process with EXE ignored at line $line\n";
				delete $module{attpname};
			}
			$module{basefilename}=$exebasename.$modnum;
			$module{filename}=$module{basefilename}.$exeext;
			$module{source0}=$exebasename.$source0ext;
			$module{source1}="";
			$module{trgtype}='exexp';
			$module{lib}=$module{basefilename}.$libext;
		} else {
			$module{basefilename}=$dllbasename.$modnum;
			$module{filename}=$module{basefilename}.$dllext;
			$module{source0}=$dllbasename.$source0ext;
			$module{source1}=$dllbasename.$source1ext;
			$module{trgtype}='dll';
			$module{lib}=$module{basefilename}.$libext;
		}
		$module{depnames}=\@moddesc;	# dependency names are left over
		$module{line}=$line;
		$module{mark}=0;
		push @modules, \%module;
		++$modnum;
	} elsif ($state<=2) {
		if ($state==1) {
			$concat="";
			die "Syntax error at line $line\n" unless (/^(\w+)\:/);
		}
		if (/^(.*?)\\$/) {
			$concat.=$1;
			$concat.=' ';
			$state=2;
			next;
		} else {
			$concat.=$_;
			$state=1;
		}
		my @modlist=split(/\s+/, $concat);
		my $tcname=shift @modlist;
		$tcname=~/^(.*?)\:$/ or die "???\n";
		$tcname=$1;
		my %tc;
		$tc{name}=$tcname;
		$tc{mod_names}=\@modlist;
		push @testcases, \%tc;
	}
}
foreach $modref (@modules) {
	my $moddepref=$$modref{depnames};
	my @deps;
	foreach (@$moddepref) {
		my $depname=lc($_);
		my @match=grep {lc($$_{name}) eq $depname} @modules;
		if (scalar(@match)==0) {
			die "Unknown dependent module $depname at line $$modref{line}\n";
		}
		my $depref=$match[0];
		if ($$modref{xip} and !$$depref{xip}) {
			die "Illegal dependency: $$modref{name} (XIP) on $$depref{name} (non-XIP)\n";
		}
		push @deps, $$depref{num};
		++$$depref{nclients};
	}
	$$modref{deps}=\@deps;
	my $attpname=$$modref{attpname};
	if ($attpname) {
		my @match=grep {lc($$_{name}) eq lc($attpname)} @modules;
		if (scalar(@match)==0) {
			die "Unknown attach process $attpname at line $$modref{line}\n";
		}
		my $attpref=$match[0];
		if (!$$attpref{exe}) {
			die "Specified attach process is not EXE at line $$modref{line}\n";
		} elsif (!$$attpref{xip}) {
			die "Specified attach process is not XIP at line $$modref{line}\n";
		}
		$$modref{attp}=$attpref;
	}
}
foreach $tcref (@testcases) {
	my $modlistref=$$tcref{mod_names};
	my @modnums;
	foreach $modname (@$modlistref) {
		next if ($modname=~/^\s*$/);
		my @match=grep {lc($$_{name}) eq lc($modname)} @modules;
		if (scalar(@match)==0) {
			die "Unknown module $modname in test case $$tcref{name}\n";
		}
		push @modnums, $match[0]->{num};
	}
	$$tcref{modules}=\@modnums;
	$$tcref{count}=scalar(@modnums);
}
my $next_mark=0;
foreach $modref (@modules) {
	if ($$modref{nclients}==0 and $$modref{exe}) {
		# EXE with no exports
		$$modref{exports}=0;
		$$modref{trgtype}='exe';
	}
	++$next_mark;
	my $modnum=$$modref{num};
	my @tcdeps;
	calc_tc(\@tcdeps, \@modules, $modref, $next_mark);
	$$modref{tcdeps}=\@tcdeps;
	if (grep {$_==$modnum} @tcdeps) {
		$$modref{cycle}=1;
	}
	my @exes=grep {$modules[$_]->{exe}} @tcdeps;
	my $nexes=scalar(@exes);

	unless ($allowbad) {
		if ($nexes>1) {
			die "Module $$modref{name} links to more than one EXE\n";
		} elsif ($nexes==1) {
			my $exeref=$modules[$exes[0]];
			$$modref{linkexe}=$exeref;
			if ($$modref{exe}) {
				if ($$exeref{num}!=$modnum) {
					die "EXE $modref{name} links to another EXE\n";
				}
			} else {
				if ($$modref{attp} and $$exeref{num}!=$modref->{attp}->{num}) {
					die "DLL $$modref{name} ($modnum) incompatible attach process\n";
				}
				if (!$$modref{attp}) {
					$$modref{attp}=$exeref;
				}
			}
		}
	}
	if ($$modref{exe}) {
		foreach $depnum (@tcdeps) {
			my $depref=$modules[$depnum];
			my $depattpref=$depref->{attp};
			if ($depattpref and $depattpref->{num}!=$modnum) {
				die "DEP DLL $$depref{name} ($$depref{num}) incompatible attach process\n";
			}
#			if (!$depattpref) {
#				$$depref{attp}=$modref;
#			}
		}
	}
}
foreach $modref (@modules) {
	my @total_deps;
	my $tcdepref=$$modref{tcdeps};
	push @total_deps, @$tcdepref;
	my $linkexe=$$modref{linkexe};
	if ($linkexe) {
		my $exetcdepref=$$linkexe{tcdeps};
		push @total_deps, @$exetcdepref;
	}
	push @total_deps, $$modref{num};
	my $data=scalar(grep {$modules[$_]->{data}} @total_deps);
	if ($data!=0) {
		$$modref{dataintree}=1;
	}
	my $xipdata=scalar(grep {$modules[$_]->{data} and $modules[$_]->{xip}} @total_deps);
	if ($xipdata!=0) {
		$$modref{xipdataintree}=1;
	}
}

#foreach $modref (@modules) {
#	print "Module $$modref{num}:\n";
#	print "\tName:     $$modref{name}\n";
#	print "\tFilename: $$modref{filename}\n";
#	my $depref=$$modref{deps};
#	my $ndeps=scalar(@$depref);
#	print "\t#Deps:    $ndeps\n";
#	print "\tDeps:     ",join(',',@$depref),"\n";
#}

my @bldinf;
my $bldname='dlltree.inf';
push @bldinf, "// $destpath$bldname\n";
push @bldinf, "//\n";
push @bldinf, "// Copyright (c) 2000-$copy_end Symbian Ltd. All rights reserved.\n";
push @bldinf, "//\n";
push @bldinf, "\n";
push @bldinf, "PRJ_PLATFORMS\n";
push @bldinf, "BASEUSERDEFAULT\n";
push @bldinf, "\n";
push @bldinf, "PRJ_TESTMMPFILES\n";
push @bldinf, "\n";

my @dlltree;
my $dlltreename='dlltree.h';
push @dlltree, "// $destpath$dlltreename\n";
push @dlltree, "//\n";
push @dlltree, "// Copyright (c) 2000-$copy_end Symbian Ltd. All rights reserved.\n";
push @dlltree, "//\n";
push @dlltree, "\n";
push @dlltree, "#ifndef __DLLTREE_H__\n";
push @dlltree, "#define __DLLTREE_H__\n";
push @dlltree, "#include <e32std.h>\n";
push @dlltree, "\n";
push @dlltree, "class MDllList;\n";
push @dlltree, "\n";

my $ibyname='ldrtest.iby';
my @iby;

foreach $modref (@modules) {
	my @mmp;
	my $num=$$modref{num};
	my $mmpname=$$modref{basefilename}.$mmpext;
	my $defname=$$modref{basefilename}.$defext;
	my $depsref=$$modref{deps};
	my $ndeps=scalar(@$depsref);
	push @mmp, "// $destpath$mmpname\n";
	push @mmp, "//\n";
	push @mmp, "// Copyright (c) 2000-$copy_end Symbian Ltd. All rights reserved.\n";
	push @mmp, "//\n";
	push @mmp, "// Generated from $$modref{name}\n";
	push @mmp, "\n";
	push @mmp, "macro             __DLLNUM$num\n";
	push @mmp, "target            $$modref{filename}\n";
	push @mmp, "targettype        $$modref{trgtype}\n";
	push @mmp, "sourcepath        .\n";
	push @mmp, "source            $$modref{source0} $$modref{source1}\n";
	push @mmp, "library           euser.lib efsrv.lib\n";
	push @mmp, "Capability		NONE\n";
	foreach (@$depsref) {
		my $depref=$modules[$_];
		push @mmp, "library           $$depref{lib}\n";
	}
	if ($$modref{exports}) {
		push @mmp, "deffile           ./$defname\n";
		push @mmp, "nostrictdef\n";
	}
	push @mmp, "OS_LAYER_SYSTEMINCLUDE_SYMBIAN   \n";
	push @mmp, "systeminclude     ../../../e32test/mmu   \n";
	push @mmp, "userinclude       . \n";
	if ($$modref{fixed}) {
		push @mmp, "epocfixedprocess\n";
	}

	# if both paged flags are set or the compression is not pageable then print neither.
	# T_LDRTST will add the required flags when it copies the file to internal media.

	my $bothCodePageFlags = $$modref{pagedcode} && $$modref{unpagedcode};
	my $pageableCompression = $$modref{bytepair} || $$modref{uncompressed};
	if (!$bothCodePageFlags && $pageableCompression) {
		push @mmp, "pagedcode\n" if $$modref{pagedcode};
		push @mmp, "unpagedcode\n" if $$modref{unpagedcode};
	}

	if ($$modref{exe}) {
		# make exes unpageddata by default
		if (!$$modref{pageddata}) {
			$$modref{unpageddata}=1;
		}
		if ($$modref{pageddata}) {
			push @mmp, "pageddata\n";
		}
		if ($$modref{unpageddata}) {
			push @mmp, "unpageddata\n";
		}
	}

	if ($$modref{bytepair}) {
		push @mmp, "bytepaircompresstarget\n";
	}

	if ($$modref{uncompressed}) {
		push @mmp, "nocompresstarget\n";
	}

	if ($$modref{data} && !$$modref{exe}) {
		push @mmp, "epocallowdlldata\n";
	}
	push @mmp, sprintf("uid               0x00000000 0x%08x\n", $num+256);
	push @mmp, "SMPSAFE\n";
	$$modref{mmp}=\@mmp;

	if ($$modref{exports}) {
		my @def;
		push @def, "EXPORTS\n";
		if ($$modref{exe}) {
			push @def, "\tRegisterConstructorCall @ 1 NONAME\n";
			push @def, "\tRegisterInitCall @ 2 NONAME\n";
			push @def, "\tRegisterDestructorCall @ 3 NONAME\n";
		} else {
			push @def, "\tInit$num @ 1 NONAME\n";
			push @def, "\tChkC$num @ 2 NONAME\n";
			push @def, "\tBlkI$num @ 3 NONAME\n";
			push @def, "\tGetGeneration @ 4 NONAME\n";
			push @def, "\tRBlkI$num @ 5 NONAME\n";
			push @def, "\tSetCloseLib @ 6 NONAME\n";
		}
		$$modref{def}=\@def;
	}

	push @bldinf, "$$modref{basefilename}\t\tsupport\n";

	if ($num==0) {
		push @dlltree, "#if defined(__DLLNUM$num)\n";
	} else {
		push @dlltree, "#elif defined(__DLLNUM$num)\n";
	}
	push @dlltree, "#define DLLNUM               $num\n";
	if ($$modref{exe}) {
		push @dlltree, "#define EXENUM               $num\n";
		push @dlltree, "_LIT(KServerName, \"$$modref{name}\");\n";
	}
	push @dlltree, "#define INITFUNC             Init$num\n";
	push @dlltree, "#define CHKCFUNC             ChkC$num\n";
	push @dlltree, "#define BLKIFUNC             BlkI$num\n";
	push @dlltree, "#define RBLKIFUNC            RBlkI$num\n";
	push @dlltree, "#define CHKDEPS(r)           (\\\n";
	foreach (@$depsref) {
		my $depref=$modules[$_];
		unless ($$depref{exe}) {
			my $func="ChkC$_";
			push @dlltree, "\t((r)=$func())!=0 ||\\\n";
		}
	}
	push @dlltree, "\t((r)=0)!=0 )\n";
	push @dlltree, "#define INITDEPS(r,l)        (\\\n";
	foreach (@$depsref) {
		my $depref=$modules[$_];
		unless ($$depref{exe}) {
			my $func="Init$_";
			push @dlltree, "\t((r)=$func(l))!=0 ||\\\n";
		}
	}
	push @dlltree, "\t((r)=0)!=0 )\n";
	my $link_to_exe;
	push @dlltree, "#define RBLKIFUNC_DEPS(i,g)  {\\\n";
	foreach (@$depsref) {
		my $depref=$modules[$_];
		if ($$depref{exe}) {
			$link_to_exe=1;
		} else {
			my $func="RBlkI$_";
			push @dlltree, "\t(i)=$func(i,g);\\\n";
		}
	}
	push @dlltree, "\t}\n";
	if ($link_to_exe) {
		push @dlltree, "#define __DLL_LINK_TO_EXE\n";
	}
	if ($$modref{cycle}) {
		push @dlltree, "#define __DLL_IN_CYCLE\n";
	}
	if ($$modref{data}) {
		push @dlltree, "#define __MODULE_HAS_DATA\n";
	}
	if ($$modref{exports}) {
		push @dlltree, "#define __MODULE_EXPORT\t\tEXPORT_C\n";
		push @dlltree, "#define __MODULE_IMPORT\t\tIMPORT_C\n";
	} else {
		push @dlltree, "#define __MODULE_EXPORT\n";
		push @dlltree, "#define __MODULE_IMPORT\n";
	}
	foreach (@$depsref) {
		my $depref=$modules[$_];
		if ($$depref{exe}) {
		} else {
			push @dlltree, "extern \"C\" IMPORT_C TInt Init$_(MDllList&);\n";
			push @dlltree, "extern \"C\" IMPORT_C TInt ChkC$_();\n";
			push @dlltree, "extern \"C\" IMPORT_C TInt RBlkI$_(TInt, TInt);\n";
		}
	}
	my $hostfullpathname=$hostpath.$$modref{filename};
	if ($$modref{xip}) {
		my $epocfullpathname=$xippath.$$modref{filename};
		my $flags;
		if ($$modref{attp}) {
			$flags='process '.$modref->{attp}->{filename};
		}
		push @iby, "file=$hostfullpathname\t\t$epocfullpathname\t\t$flags\n";
	} else {
		my $epocfullpathname=$nonxippath.$$modref{filename};
		push @iby, "data=$hostfullpathname\t\t$epocfullpathname\n";
	}
}

#push @dlltree, "#else\n";
#push @dlltree, "#error No __DLLNUM macro defined\n";
push @dlltree, "#endif\n";
push @dlltree, "\n";

my $module_count=scalar(@modules);
push @dlltree, "const TInt KNumModules=$module_count;\n";
push @dlltree, "\n";
push @dlltree, "#ifdef __INCLUDE_DEPENDENCY_GRAPH\n";
push @dlltree, "static const TText* const ModuleName[KNumModules] =\n";
push @dlltree, "\t\{\n";
foreach $modref (@modules) {
	my $string="\t(const TText*)L\"$$modref{name}\"";
	unless ($$modref{num}==$module_count-1) {
		$string.=',';
	}
	push @dlltree, $string;
	my $pad=41-length($string);
	$pad=($pad+3)/4;
	push @dlltree, "\t"x$pad, "\/*", $$modref{num}, "*\/\n";
}
push @dlltree, "\t\};\n";
push @dlltree, "\n";
push @dlltree, "#define MODULE_NAME(n)	TPtrC(ModuleName[n])\n";
push @dlltree, "\n";
push @dlltree, "static const TText* const ModuleFileName[KNumModules] =\n";
push @dlltree, "\t\{\n";
foreach $modref (@modules) {
	if ($$modref{idrive} || $$modref{vdrive}) {
		my $fn = $$modref{filename};
		my $used_nxip_path = "?:\\$nonxippath$fn";
		$used_nxip_path =~ s/\\/\\\\/g;		# double backslashes
		substr($used_nxip_path,0,1) = $$modref{idrive} ? "0" : "1";
		push @dlltree, "\t(const TText*)L\"$used_nxip_path\"";
	} else {
		push @dlltree, "\t(const TText*)L\"$$modref{filename}\"";
	}
	if ($$modref{num}==$module_count-1) {
		push @dlltree, "\n";
	} else {
		push @dlltree, ",\n";
	}
}
push @dlltree, "\t\};\n";
push @dlltree, "\n";
push @dlltree, "#define MODULE_FILENAME(n)	TPtrC(ModuleFileName[n])\n";
push @dlltree, "\n";
foreach $modref (@modules) {
	my $modnum=$$modref{num};
	my $tcdepsref=$$modref{tcdeps};
	my $numdeps=scalar(@$tcdepsref);
	push @dlltree, "static const TInt Module$modnum","Deps[] =\n";
	push @dlltree, "\t\{$numdeps";
	if ($numdeps) {
		push @dlltree, ",", join(',',@$tcdepsref)
	}
	push @dlltree, "\};\n";
}
push @dlltree, "static const TInt* const ModuleDependencies[KNumModules] =\n";
push @dlltree, "\t\{\n";
foreach $modref (@modules) {
	my $modnum=$$modref{num};
	push @dlltree, "\tModule$modnum","Deps";
	if ($$modref{num}==$module_count-1) {
		push @dlltree, "\n";
	} else {
		push @dlltree, ",\n";
	}
}
push @dlltree, "\t\};\n";
push @dlltree, "\n";
push @dlltree, sprintf "const TInt KModuleFlagExe=0x%04x;\n", $flag_value_exe;
push @dlltree, sprintf "const TInt KModuleFlagFixed=0x%04x;\n", $flag_value_fixed;
push @dlltree, sprintf "const TInt KModuleFlagData=0x%04x;\n", $flag_value_data;
#push @dlltree, "#ifdef __EPOC32__\n";
push @dlltree, sprintf "const TInt KModuleFlagXIP=0x%04x;\n", $flag_value_xip;
push @dlltree, sprintf "const TInt KModuleFlagPagedCode=0x%04x;\n", $flag_value_pagedcode;
push @dlltree, sprintf "const TInt KModuleFlagUnpagedCode=0x%04x;\n", $flag_value_unpagedcode;
push @dlltree, sprintf "const TInt KModuleFlagIDrive=0x%04x;\n", $flag_value_idrive;
push @dlltree, sprintf "const TInt KModuleFlagVDrive=0x%04x;\n", $flag_value_vdrive;
push @dlltree, sprintf "const TInt KModuleFlagBytePair=0x%04x;\n", $flag_value_bytepair;
push @dlltree, sprintf "const TInt KModuleFlagUncompressed=0x%04x;\n", $flag_value_uncompressed;
#push @dlltree, "#else\n";
#push @dlltree, sprintf "const TInt KModuleFlagXIP=0x%04x;\n", 0;	# no XIPs on emulator
#push @dlltree, "#endif\n";
push @dlltree, sprintf "const TInt KModuleFlagDllInCycle=0x%04x;\n", $flag_value_dll_in_cycle;
push @dlltree, sprintf "const TInt KModuleFlagDataInTree=0x%04x;\n", $flag_value_data_in_tree;
#push @dlltree, "#ifdef __EPOC32__\n";
push @dlltree, sprintf "const TInt KModuleFlagXIPDataInTree=0x%04x;\n", $flag_value_xip_data_in_tree;
#push @dlltree, "#else\n";
#push @dlltree, sprintf "const TInt KModuleFlagXIPDataInTree=0x%04x;\n", 0;
#push @dlltree, "#endif\n";
push @dlltree, sprintf "const TInt KModuleFlagExports=0x%04x;\n", $flag_value_exports;
push @dlltree, sprintf "const TInt KModuleFlagTargetOnly=0x%04x;\n", $flag_value_targetonly;
push @dlltree, sprintf "const TInt KModuleFlagPagedData=0x%04x;\n", $flag_value_pageddata;
push @dlltree, sprintf "const TInt KModuleFlagUnpagedData=0x%04x;\n", $flag_value_unpageddata;
push @dlltree, "static const TInt ModuleFlags[KNumModules] =\n";
push @dlltree, "\t\{\n";
foreach $modref (@modules) {
	my $flags=0;
	my @flagNames = ();

	push @flagNames, "KModuleFlagExe" if ($$modref{exe});
	push @flagNames, "KModuleFlagFixed" if ($$modref{fixed});
	push @flagNames, "KModuleFlagData" if ($$modref{data});
	push @flagNames, "KModuleFlagXIP" if ($$modref{xip});
	push @flagNames, "KModuleFlagPagedCode" if ($$modref{pagedcode});
	push @flagNames, "KModuleFlagUnpagedCode" if ($$modref{unpagedcode});
	push @flagNames, "KModuleFlagPagedData" if ($$modref{pageddata});
	push @flagNames, "KModuleFlagUnpagedData" if (!$$modref{pageddata});
	push @flagNames, "KModuleFlagIDrive" if ($$modref{idrive});
	push @flagNames, "KModuleFlagVDrive" if ($$modref{vdrive});
	push @flagNames, "KModuleFlagBytePair" if ($$modref{bytepair});
	push @flagNames, "KModuleFlagDllInCycle" if ($$modref{cycle});
	push @flagNames, "KModuleFlagDataInTree" if ($$modref{dataintree});
	push @flagNames, "KModuleFlagXIPDataInTree" if ($$modref{xipdataintree});
	push @flagNames, "KModuleFlagExports" if ($$modref{exports});
	push @flagNames, "KModuleFlagUncompressed" if ($$modref{uncompressed});
	push @flagNames, "KModuleFlagTargetOnly" if ($$modref{targetonly});

	@flagNames = qw(0) if (scalar(@flagNames) == 0);
	my $flagString = "/\* " . $$modref{num} . " \*/\t" . join(' | ', @flagNames);
	unless ($$modref{num}==$module_count-1) {
		$flagString.=',';
	}
	push @dlltree, $flagString . "\n";
}
push @dlltree, "\t\};\n";
push @dlltree, "\n";
foreach $modref (@modules) {
	my $modnum=$$modref{num};
	my @rblki;
	++$next_mark;
	calc_rblki(\@rblki, \@modules, $modref, $next_mark);
	my $rblki_count=scalar(@rblki);
	my $rblki_sum=0;
	foreach (@rblki) {
		$rblki_sum += $_;
	}
	push @dlltree, "static const TInt Module$modnum","RBlkIParams[2] = \{ $rblki_count, $rblki_sum \};\n";
}
push @dlltree, "\n";
push @dlltree, "static const TInt* const ModuleRBlkIParams[KNumModules] =\n";
push @dlltree, "\t\{\n";
foreach $modref (@modules) {
	my $modnum=$$modref{num};
	push @dlltree, "\tModule$modnum","RBlkIParams";
	if ($$modref{num}==$module_count-1) {
		push @dlltree, "\n";
	} else {
		push @dlltree, ",\n";
	}
}
push @dlltree, "\t\};\n";
push @dlltree, "\n";
foreach $modref (@modules) {
	my $modnum=$$modref{num};
	my $mod_attp=$modnum;
	my $mod_linkp=$modnum;
	unless ($$modref{exe}) {
		$mod_attp = ($$modref{attp}) ? ($modref->{attp}->{num}) : -1;
		$mod_linkp = ($$modref{linkexe}) ? ($modref->{linkexe}->{num}) : -1;
	}
	push @dlltree, "static const TInt Module$modnum","ExeInfo[2] = \{ $mod_attp, $mod_linkp \};\n";
}
push @dlltree, "\n";
push @dlltree, "static const TInt* const ModuleExeInfo[KNumModules] =\n";
push @dlltree, "\t\{\n";
foreach $modref (@modules) {
	my $modnum=$$modref{num};
	push @dlltree, "\tModule$modnum","ExeInfo";
	if ($$modref{num}==$module_count-1) {
		push @dlltree, "\n";
	} else {
		push @dlltree, ",\n";
	}
}
push @dlltree, "\t\};\n";

foreach $tcref (@testcases) {
	push @dlltree, "\n";
	my $modlistref=$$tcref{modules};
	my $count=$$tcref{count};
	my $arraysize=$count+1;
	push @dlltree, "static const TInt TC_$$tcref{name}\[$arraysize\]=\n\t\{\n";
	if ($count==0) {
		push @dlltree, "\t$count\n";
	} else {
		push @dlltree, "\t$count\,\n";
	}
	foreach (@$modlistref) {
		--$count;
		if ($count==0) {
			push @dlltree, "\t$_\n";
		} else {
			push @dlltree, "\t$_\,\n";
		}
	}
	push @dlltree, "\t\};\n";
}

push @dlltree, "#endif\n";
push @dlltree, "\n";
push @dlltree, "#endif\n";

foreach $modref (@modules) {
	my $mmpname=$$modref{basefilename}.$mmpext;
	my $mmpref=$$modref{mmp};
	open OUT, ">$mmpname" or die "Could not open $mmpname for output\n";
	print OUT @$mmpref;
	close OUT;
	if ($$modref{exports}) {
		my $defname=$$modref{basefilename}.$defext;
		my $defref=$$modref{def};
		open OUT, ">$defname" or die "Could not open $defname for output\n";
		print OUT @$defref;
		close OUT;
	}
}
if (!$allowbad) {
	push @bldinf, "t_ldrtst\n";
}
open OUT, ">$bldname" or die "Could not open $bldname for output\n";
print OUT @bldinf;
close OUT;
open OUT, ">$dlltreename" or die "Could not open $dlltreename for output\n";
print OUT @dlltree;
close OUT;

my $testbatch ='';
if($is_raptor) {
	$testbatch="$ENV{EPOCROOT}\\epoc32\\data\\z\\test\\gen\\##MAIN##.auto.bat";
}
else {
	$testbatch = "$ENV{EPOCROOT}epoc32\\build";
	$destpath =~ s/\//\\/go;
	$testbatch.="\\" unless ($destpath =~ /^\\/);
	$testbatch.=$destpath;
	$testbatch.="##MAIN##.auto.bat";
	}
if (!$allowbad){
	push @iby, "data=$testbatch\t\ttest\\loader.auto.bat\n";	
}
open OUT, ">$ibyname" or die "Could not open $ibyname for output\n";
print OUT @iby;
close OUT;

#
# Accumulate list of dependency numbers
# 1st arg = \output list
# 2nd arg = \module list
# 3rd arg = \module to start from
# 4th arg = mark value to use
#
sub calc_tc($$$$) {
	my ($out, $mods, $modref, $mark)=@_;
	my $depsref=$$modref{deps};
	foreach $dep (@$depsref) {
		my $depref=$$mods[$dep];
		if ($$depref{mark} != $mark) {
			$$depref{mark}=$mark;
			unless ($$depref{exe}) {
				calc_tc($out, $mods, $depref, $mark);
			}
			push @$out, $dep;
		}
	}
}

#
# Accumulate RBlkI parameters
# 1st arg = \output list
# 2nd arg = \module list
# 3rd arg = \module to start from
# 4th arg = mark value to use
#
sub calc_rblki($$$$) {
	my ($out, $mods, $modref, $mark)=@_;
	if ($$modref{mark} != $mark) {
		$$modref{mark}=$mark;
		if (!$$modref{exe} and $$modref{data}) {
			push @$out, $$modref{num};
			my $depsref=$$modref{deps};
			foreach $dep (@$depsref) {
				calc_rblki($out, $mods, $$mods[$dep], $mark);
			}
		}
	}
}



