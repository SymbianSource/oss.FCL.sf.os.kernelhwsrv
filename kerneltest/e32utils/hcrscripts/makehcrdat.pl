#!perl -w
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
# This simple script makes a binary HCR data file from a text input file
#
use strict;

use HCRdat;
use HCRrec;

package mhd;

#
# Find out what file the user is interested in..
# Make sure it's specified and exists.
#
$mhd::trace = 0;
$mhd::otrace = 0;

if (@ARGV < 2 || @ARGV > 4) {
    die "\nUsage: hcrmd.bat <source_textfile> <dest_datfile> [-i]\n";
    }

my $textfile = shift @ARGV;
my $datfile  = shift @ARGV;

my $do_create_image = 0;
my $opt_i = shift @ARGV;
#my $partid = 0x10000005;
if (defined($opt_i)) {
    $do_create_image = 1 if ($opt_i eq "-i");
    die "error: unknown command option\n" if ($opt_i ne "-i");
    #my $i_no = shift @ARGV;
    #$partid = hex($i_no) if (defined($i_no)); 
    #printf("partitionid: 0x%x\n", $partid)
    }


print "\n    HCR Binary Data File Generator, version v0.1\n";
print "    Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.\n\n";
print "-input: $textfile\n" if($mhd::trace);
print "-output: $datfile\n" if($mhd::trace);

die "error: Specifed source_textfile not found!" unless(-f $textfile);
#die "error: Specified dest_binfile '$datfile' already exists!" if(-e $datfile);

printf "\nReading input file... $textfile\n";

printf "-opening text file\n" if($mhd::trace);
my $tfh;
open($tfh, "<$textfile");

printf "-started conversion...\n"  if($mhd::trace);
my $datobj = HCRdat->new();
my $inrec = 0;
my $ln = 0;
my $recobj;

while (<$tfh>)
    {
    $ln++;
    if ($_ =~ '^\s*#') {
        printf "-comment\n" if($mhd::trace);
        }
    elsif ($_ =~ '^@') {
        die "error: Syntax error line $ln: New record started before previous one is closed" if($inrec > 0);
        printf "-start\n" if($mhd::trace);
        $inrec = 1;
        $recobj = HCRrec->new();
        }
    elsif ($_ =~ '^\.') {
        die "error: Syntax error line $ln: Record closed before a new record has been opened" if($inrec == 0); 
        printf "-end\n" if($mhd::trace);
        if ($recobj->IsValid()) {
            $datobj->AddSettingRecord($recobj);
            }
        else {
            die "error: Record after record " . $datobj->NumRecords() . " completed but not valid, missing or has =0  fields?\n";
            }
        $inrec = 0;
        }
    elsif ($_ =~ '^\s*$') {
        printf "-blank\n" if($mhd::trace);
        }
    elsif ($_ =~ '^\s*cuid:\s') {
        print "--cuid " if($mhd::trace);
        my @hwords = split(/\s+/,$_);
        die "error: 'cuid:' line incorrectly formed" if (scalar(@hwords) != 2); 

        $recobj->CUID($hwords[1]);
        printf("=0x%08x\n", $recobj->CUID()) if($mhd::trace);
        }
    elsif ($_ =~ '^\s*eid:\s') {
        print "--eid " if($mhd::trace);
        my @hwords = split(/\s+/,$_);
        die "error: 'eid:' line incorrectly formed" if (scalar(@hwords) != 2); 

        $recobj->EID($hwords[1]);
        print "=".($recobj->EID())."\n" if($mhd::trace);  
        }
    elsif ($_ =~ '^\s*type:\s') {
        print "--type " if($mhd::trace);
        my @hwords = split(/\s+/,$_);
        die "error: 'type:' line incorrectly formed" if (scalar(@hwords) != 2); 
        
        $recobj->Type($hwords[1]); 
        printf("=0x%08x (%s)\n", $recobj->Type(), $recobj->TypeName()) if($mhd::trace);
        }
    elsif ($_ =~ '^\s*flags:\s') {
        print "--flags " if($mhd::trace);
        my @hwords = split(/\s+/,$_);
        die "error: 'flags:' line incorrectly formed" if (scalar(@hwords) != 2);
        
        $recobj->Flags($hwords[1]);
        printf ("=0x%x\n", $recobj->Flags()) if($mhd::trace);  
        printf ("warning: flag length value greater than 2-bytes\n") if ($recobj->Flags() > 0xffff);
        }
    elsif ($_ =~ '^\s*intval:\s') {
        print "--intval " if($mhd::trace);
        my @hwords = split(/\s+/,$_);
        die "error: 'intval:' line incorrectly formed" if (scalar(@hwords) != 2); 

        $recobj->IntValue($hwords[1]);
        printf("=%d (0x%x)\n", $recobj->IntValue(), $recobj->IntValue()) if($mhd::trace);  
        }
    elsif ($_ =~ '^\s*hexval:\s') {
        print "--hexval " if($mhd::trace);
        my @hwords = split(/\s+/,$_);
        die "error: 'hexval:' line incorrectly formed" if (scalar(@hwords) != 2); 

        $recobj->HexValue($hwords[1]);
        printf("=%d (0x%x)\n", $recobj->IntValue(), $recobj->IntValue()) if($mhd::trace);  
        }
    elsif ($_ =~ '^\s*arrval:\s') {
        print "--arrval " if($mhd::trace);
        my @hwords = split(/\s+/,$_);
        die "error: 'arrval:' line incorrectly formed" if (scalar(@hwords) != 2); 
        
		print  $hwords[1]."\n" if ($mhd::trace);
        $recobj->ArrValue($hwords[1]);  
        }
    elsif ($_ =~ '^\s*strval:\s') {
        print "--strval " if($mhd::trace);
        my @hwords = split(/\"/,$_);
        die "error: 'strval:' line incorrectly formed" if (scalar(@hwords) != 3); 

        my $strval_size = $recobj->Length();
        $recobj->StrValue($hwords[1]);
        
        printf("=\"%s\"\n", substr($recobj->StrValue(), $strval_size)) if($mhd::trace);  
        }
    elsif ($_ =~ '^\s*binval:\s') {
        print "--binval " if($mhd::trace);
        my @hwords = split(/:/,$_);
        die "error: 'binval:' line incorrectly formed" if (scalar(@hwords) < 2); 

        my $binval_size = $recobj->Length();
        $recobj->BinValue($hwords[1]);
        
        my $binval_ref = $recobj->BinValue();
        my @binval = @$binval_ref;
        
        printf("(%d) =", $#binval+1) if($mhd::trace);
        my $uint16 = $binval_size;
        for (; $uint16 < @binval; $uint16++) {
            printf("%02x ", $binval[$uint16]) if($mhd::trace);
            } 

        print "\n" if($mhd::trace);
        }
    elsif ($_ =~ '') {
        }
    else {
        die "error: unknown line type '$_'" 
#        print $_  if($mhd::trace);
        }
    }

close $tfh;

printf "\nGenerating output file... $datfile\n";

printf "-creating binary data file\n"  if($mhd::otrace);
if ($datobj->WriteToFile($datfile.".tmp") != 0) {
    die "error: failed to write to dest_binfile";
    }
    
printf "-renaming file to temp file to $datfile\n"  if($mhd::otrace);
rename ($datfile.".tmp", $datfile);

printf "-file header written:\n"  if($mhd::otrace);
$datobj->ShowHeader() if($mhd::otrace);

if ($do_create_image) {
    my $imgfile = $datfile . ".img";
    print "\nGenerating partition image... ".  $imgfile . "\n";

    if ($datobj->WriteToImage($imgfile, $datfile) != 0) {
        die "error: failed to write to image file $imgfile";
        }
    }

print "\nDone.\n";
exit 0;



