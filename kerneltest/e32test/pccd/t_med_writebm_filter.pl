#!perl
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

#--------------------------------------------------------------------------------------------
#   This is a filter script for MMC/SD cards benchmark test     t_med_writebm
#   Produces csv file from given input
#
#   usage:  filter1.pl <log file>
#
#   See t_med_writebm.cpp for more details.
#--------------------------------------------------------------------------------------------

use strict;


my $inpFile;    #-- input file name
my $line;
my $printHdr;   #-- 1 if we need to print a header

#-----------------------------------------------------
#-- test tags that we expect to find in the input file
#my $KTagPrefix  = "~#";
#-----------------------------------------------------

#-- input file
$inpFile = $ARGV[0];

$printHdr = 1;

#-- process imput file
open(INP_FILE, "$inpFile") || die "Can't open input file $inpFile !\n";

  while($line = <INP_FILE>)
  {
    if($line=~ /~#pos:(\d*):(\d*).*time:(\d*)/i)
    {#-- found a string matching pattern; return
      if($printHdr)
      {
        print "W1_pos,W2_pos,time_us\n";
        $printHdr = 0;
      }

      print "$1,$2,$3\n";
      #chomp($line);
      #print $line."\n";
      #$matchLine=$line;
      #last;
    }
    elsif($line=~ /~#Local Drive:(\d*)/i)
    {
        print("local drive=$1; ");
    }
    elsif($line=~ /~#MediaPos:(\d*):(\d*)/i)
    {
        print("Media pos=$1:$2; ");
    }
    elsif($line=~ /~#WinNum:(\d*)/i)
    {
        print("Windows=$1; ");
    }
    elsif($line=~ /~#NumWrites:(\d*)/i)
    {
        print("NumWrites=$1\n");
    }
    elsif($line=~ /~#Window(\d).*sz:(\d*).*posInc:(.\d*)/i)
    {
        print("Window$1: size=$2; PosInc=$3\n");
    }

  }


close (INP_FILE);

#---

exit;


#--------------------------------------------------------------------------------------------
# Strip spaces from the beginning & end of the string
#--------------------------------------------------------------------------------------------
sub TrimStr 
{
    my @out = @_;
    for (@out) 
    {
        s/^\s+//;
        s/\s+$//;
    }
    return wantarray ? @out : $out[0];
}

