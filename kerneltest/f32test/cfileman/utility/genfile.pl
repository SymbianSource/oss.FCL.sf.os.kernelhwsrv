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

my $file=@ARGV[0];
my $newfile=@ARGV[1];
open TMP, $file;
my $line;
my @data;
my $n=0;
my $start=0;
while (my $line = <TMP>)
{
   if (($start == 0) && ($line =~ /^.*SYMTestCaseID.*/i))
   {
      $start=1;
   }
   if ($start)
   {
     push @data,$line;
   
     if ($line =~ /^.*SYMTestStatus\s*Implemented.*/i)
     {
         push @data, "int dummy$n(){}";
         push @data,"\n";
         $n++;
         $start = 0;
         next;
     }
   }
}

close TMP;

open DUMP, ">$newfile";
print "* Writing $newfile\n";
my $line;
foreach $line (@data)
{
	print DUMP $line;
}
close DUMP;
