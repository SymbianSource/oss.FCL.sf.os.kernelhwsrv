@rem
@rem Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
@rem All rights reserved.
@rem This component and the accompanying materials are made available
@rem under the terms of the License "Eclipse Public License v1.0"
@rem which accompanies this distribution, and is available
@rem at the URL "http://www.eclipse.org/legal/epl-v10.html".
@rem
@rem Initial Contributors:
@rem Nokia Corporation - initial contribution.
@rem
@rem Contributors:
@rem
@rem Description:
@rem

@perl -x check.bat
@goto end
#!perl

require Cwd;
$cwd=Cwd::getcwd();
#print "cwd: ",$cwd,"\n";
$cwd =~ s/\//\\/;
$cwd =~ /^(\w:)?([\\|\/]\S+[\\|\/])/isx;
$drive=$1;
$prjroot=$2;
#print "drive ",$drive,"\n";
#print "project root: ",$prjroot,"\n";
@cpps=();
@mmps=();
@mmpflags=();
%sources=();
%sourceusers=();
ls ($prjroot);
#print "CPP files:\n";
#foreach $entry (@cpps)
#	{
#	print $entry,"\n";
#	}
#print "MMP files:\n";
$count=0;
$countz=0;
$countsrc=0;
foreach $entry (@mmps)
	{
#	print $entry,"\n";
	$srclist=$sources{$entry};
	$mmpflags[$count]=0;
	++$count;
	++$countz if (scalar(@$srclist)==0);
	$countsrc += scalar(@$srclist);
	foreach $src (@$srclist)
		{
		$uselist=$sourceusers{$src};
		$sourceusers{$src}=$uselist." ".$entry;
#		print "\t",$src,"\n";
		}
	}
$hashsize=scalar(%sources);
#print $hashsize;
#print "Count=",$count,"\n";
#print "CountZ=",$countz,"\n";
#print "CountSrc=",$countsrc,"\n";

foreach $entry (@cpps)
	{
	$uses=$sourceusers{$entry};
	print "File ",$entry," not referenced by any .MMP file\n" unless $uses;
	}

parsebldinf();

$i=0;
foreach $entry (@mmps)
	{
	next if $mmpflags[$i++];
	print "\n",$entry,".MMP does not appear in BLD.INF\n";
	print "\t Source files:\n";
	$srclist=$sources{$entry};
	foreach $src (@$srclist)
		{
		print "\t",$src,"\n";
		}
	}
end;

sub ls
	{
	my $dir=@_[0];
	$dir =~ s/\//\\/;
#	print $dir,"\n";
	opendir(DIR,$dir);
	my @dir=readdir(DIR);
	closedir(DIR);
	my $entry;
	my $fullentry;
	foreach $entry (@dir)
		{
		$entry=~s/\//\\/;
		next if (($entry eq ".") or ($entry eq ".."));
		$fullentry=$dir.$entry;
		if (-d $fullentry)
			{
			ls($fullentry."\\");
			}
		else
			{
			push @cpps, uc($fullentry) if ($entry =~ /\S+\.cpp/isx);
			if ($entry =~ /(\S+)\.mmp/isx)
				{
				push @mmps, uc($1);
				parsemmp($fullentry,\%sources);
				}
#			print $fullentry,"\n";
			}
		}
	}

sub parsemmp
	{
	my $mmpfile=@_[0];
	my $array=@_[1];
	my $project;
	my $subproject;
#	print "ParseMMP ",$mmpfile,"\n";
	open(MMP,$mmpfile);
	my @sources=();
	while(<MMP>)
		{
		if (/^PROJECT\s+(\S+)/isx)
			{
			$project=uc($1);
#			print "Project ",$project,"\n";
			}
		elsif (/^SUBPROJECT\s+(\S+)/isx)
			{
			$subproject=uc($1);
#			print "Subproject ",$subproject,"\n";
			}
		elsif (/^source\s*(.+)/isx)
			{
#			print $1;
			my $list=$1;
			while($list)
				{
				$list =~ /^(\S+)\s*(.*)/isx;
				push @sources, "\\".$project."\\".$subproject."\\".uc($1);
				$list=$2;
#				print "\t Source ",$1,"\n";
				}
			}
		}
	close(MMP);
	$mmpfile =~ /\S*\\(\S+)\.mmp/isx;
	$mmpfileshort=uc($1);
	$$array{$mmpfileshort}=\@sources;
	}

sub parsebldinf
	{
	my $reachedmmps=0;
	open(BLDINF,"bld.inf");
	while (<BLDINF>)
		{
#		print;
		if ($reachedmmps==0)
			{
			$reachedmmps=1 if (/PRJ_MMPFILES|PRJ_TESTMMPFILES/isx);
			next;
			}
		/(\S+)\s*.*/isx;
		my $mmpfile=uc($1);
#		print "****************",$mmpfile,"\n";
		my $mmp;
		my $i=0;
		foreach $mmp (@mmps)
			{
			$mmpflags[$i]=1 if ($mmp eq $mmpfile);
			++$i;
			}
		}
	close(BLDINF);
	}

__END__
:end
