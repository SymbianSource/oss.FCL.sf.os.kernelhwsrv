#!perl
# Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# This script filters and parses the contents of the performance tests results.
# @todo: 
# + make a temporary filtered file to work with it later 
# + search for all possible "TestId" 
# - print out time that the test has taken 
# Usage:
# filter_results.pl <test_log> [csv]
# <test_log> Raw test log filename
# csv        Will generate a set of comma-separated values
# 
#

use strict;

my @TestsIds;         #-- an array of test Ids  
my $currTestId;       #-- current TestId to work with
my $currTestStep;     #-- current test step number within current test

my @columnTitles;     #-- results columns titles
my $MaxColumns;       #-- maximal number of columns in the report table 
my $MaxRows;          #-- maximal number of rows in the report table   

my $inpFile;          #-- input file name
my $tmpFile = "filtered.tmp"; #-- temp. filtered file name  

my $searchPattern;
my ($matchLine, $matchLineNumber);

my $OutputMode = "Text";       # CSV or Text



#-----------------------------------------------------
#-- test tags that we expect to find in the input file
my $KTagPrefix  = "#~";

my $KTag_TestId     = "TestId=";
my $KTag_TestTitle  = "TestTitle_";
my $KTag_TestParam  = "TestParam_";  
my $KTag_TestRows   = "TestRows_";
my $KTag_TestColumns= "TestColumns_";
my $KTag_TS_Title   = "TS_Title_";
my $KTag_TS_Res     = "TS_Res_"; 
#-----------------------------------------------------

#-- input file
$inpFile = $ARGV[0];

if (lc($ARGV[1]) eq 'csv')
	{
	$OutputMode = "CSV";
	}

#-- filter input file by TestTag prefix, the result will be in $tmpFile 
FilterInputFile();

#-- now use filtered data as input
$inpFile =$tmpFile;

#-- find all TestId in the input file and put them into @TestsIds 
FindTestIds();
if(@TestsIds == 0)
{
    die "can't find $KTag_TestId tags in the input file!\n"
}

#-- process test results for each TestId
my $i;
foreach $i (@TestsIds)
{
    $currTestId = $i;
    
    ProcessTestResults();
}


unlink $tmpFile; 

exit;

sub TextPrintf
	{
	if ($OutputMode eq "Text")
		{
		printf @_;
		}
	}
sub TextPrint
	{
	if ($OutputMode eq "Text")
		{
		print @_;
		}
	}
sub CSVPrintf
	{
	if ($OutputMode eq "CSV")
		{
		printf @_;
		}
	}
sub CSVPrint
	{
	if ($OutputMode eq "CSV")
		{
		print @_;
		}
	}


#--------------------------------------------------------------------------------------------
#   Extract test results (by TestId) from the input file.  
#--------------------------------------------------------------------------------------------
sub ProcessTestResults
{
    my ($scratch0, $scratch1); 
    my ($i, $j);
    my ($CSVFields, $ExtraParams, $TimeUnit);
    $CSVFields = "TestId,TestStep,Param1,Param2,ExtraParams,Unit,Result";

    TextPrint("\n============================================================================\n\n");
    #-- extract and print test title
    $searchPattern=$KTag_TestTitle.$currTestId.":";
    FindPatternInFile();
    if($matchLine=~ /$searchPattern.(.*)/i)
    {
      $scratch0=$1; chomp($scratch0);
      TextPrint "Test $currTestId: ", $scratch0, "\n";
      CSVPrint "# Test $currTestId: ", $scratch0, "\n";
    } 
    else {die "can't find $searchPattern in the input file!\n"};
    
    #-- extract and print test parameters
    $searchPattern=$KTag_TestParam.$currTestId.":";
    FindPatternInFile();
    if($matchLine=~ /$searchPattern.(.*)/i)
    {
      $scratch0=$1; chomp($scratch0);
      TextPrint "Test parameters: ", $scratch0, "\n";
      # Turn parameters into comma-separated values
      foreach(split /,/, $scratch0)
      	{
	    if (/^\s*(.+)=(.+)\s*$/)
	    	{
		    if ($1 eq 'timeUnit')
		    	{
			    $TimeUnit = $2;
		    	}
		    else
		    	{
			    $ExtraParams .= ';' if ($ExtraParams) ne '';
			    $ExtraParams .= "$1=$2";
		    	}
  			}
  		}
    }
    else {die "can't find $searchPattern in the input file!\n"};
    
    TextPrint("\n============================================================================\n\n");
    
    #-- extract number of rows in the result tables
    $searchPattern=$KTag_TestRows.$currTestId.":";
    FindPatternInFile();
    if($matchLine=~ /$searchPattern.(.*)/i)
    {
      $MaxRows=$1; chomp($MaxRows);
      #TextPrint "Rows: $MaxRows\n";
    } 
    else {die "can't find $searchPattern in the input file!\n"};
    
    
    #-- extract test results columns description
    $searchPattern=$KTag_TestColumns."$currTestId:";
    FindPatternInFile();
    if($matchLine=~ /$searchPattern.(.*)/i)
    {
      $scratch0=$1; chomp($scratch0);
      #TextPrint "Test columns: ", $scratch0, "\n";
    
      @columnTitles = split(/,/,$scratch0);
      chomp(@columnTitles);
      @columnTitles=TrimStr(@columnTitles);
      
      $MaxColumns=shift(@columnTitles);
      #TextPrint "!!max columns: ", $MaxColumns, "\n";
    
      if($MaxColumns != @columnTitles)
        {die "Wrong $searchPattern value!, see input file, line:$matchLineNumber\n"};
    } 
    else {die "can't find $searchPattern in the input file!\n"};
    
    #-- test step loop. try to find all test steps within current test
    for($currTestStep=1; $currTestStep<999; $currTestStep++)
    {
      $searchPattern="$KTag_TS_Title$currTestId,$currTestStep:";
      FindPatternInFile();
      if($matchLine=~ /$searchPattern.(.*)/i)
      {
        $scratch0=$1; chomp($scratch0);
        TextPrint "\nTest step $currTestId,$currTestStep: ", $scratch0, "\n\n";
        CSVPrint "# Test step $currTestId,$currTestStep: ", $scratch0, "\n";
      } 
      else {last}; 
    
      #-- here we have current test id & test step number 
      
      #-- print out table columns headers
      for($i=0; $i<$MaxColumns; $i++)
      {
        TextPrintf "%12s", @columnTitles[$i];
      }
      $CSVFields =~ s/Param1/$columnTitles[0]/;
      CSVPrint "# $CSVFields\n";
      
      TextPrint("\n");
    
      #-- print out result table
      for($i=1; $i<=$MaxRows; $i++)
      {
	    my $rowcaption;
        for($j=1; $j<=$MaxColumns; $j++)
        {
            $searchPattern="$KTag_TS_Res$currTestId,$currTestStep,\\[$i,$j\\]";
            FindPatternInFile();
            if($matchLine=~ /$searchPattern.(.*)/i)
            {
              $scratch0=$1; chomp($scratch0);
              $scratch0 = TrimStr($scratch0);
              TextPrintf "%12s", "$scratch0";
              if ($j == 1)
              	{
	        	$rowcaption = $scratch0;
              	}
              else
              	{
              	CSVPrint $currTestId.",".$currTestStep.",".$rowcaption.",".$columnTitles[$j-1].",$ExtraParams,$TimeUnit,$scratch0\n";
          		}
            } 
            else {die "can't find $searchPattern in the input file!\n"};
                
        }
        
        TextPrint "\n";
      }
      TextPrint "\n";
      
    }

}#

exit;


#--------------------------------------------------------------------------------------------
#   Find all Test Id values in the input file and put them into @TestsIds array 
#--------------------------------------------------------------------------------------------
sub FindTestIds
{
  my $line;
  open(INP_FILE, "$inpFile") || die "Can't open input file $inpFile !";

  #-- read input file line by line, looking for pattern match
  while($line = <INP_FILE>)
  {
    if($line=~ /$KTag_TestId(\d*)/i)
    {#-- found a string matching pattern; put TestId to the array
       #print $line, $1."\n";
       push(@TestsIds, $1);
    }
  }

  close (INP_FILE);
}

#--------------------------------------------------------------------------------------------
#   Filters inputfile by the test tag prefix ($KTagPrefix)     
#   Filtered lines are placed to the ($tmpFile)  
#--------------------------------------------------------------------------------------------
sub FilterInputFile()
{
  my $line;
  open(INP_FILE, "$inpFile") || die "Can't open input file $inpFile !";
  open(TEMP_FILE, ">$tmpFile") || die "Can't open input file $tmpFile !";

  #-- read input file line by line, looking for pattern match
  while($line = <INP_FILE>)
  {  
    if($line=~ /$KTagPrefix(.*)/i)
    {#-- found a string with the prefix
      print TEMP_FILE $1."\n";;
    }
  }
  
  close (INP_FILE);
  close (TEMP_FILE);

}

#--------------------------------------------------------------------------------------------
#   Finds a firs occurence of a given pattern ($searchPattern) in the ($inpFile)  
#   and places the found line to the $matchLine. 
#   Also modifies $matchLineNumber 
#--------------------------------------------------------------------------------------------
sub FindPatternInFile
{
  my $line;
  open(INP_FILE, "$inpFile") || die "Can't open input file $inpFile !";

  #-- read input file line by line, looking for pattern match
  #-- not very efficient, may be optimised ? 
  $matchLineNumber=0;
  $matchLine="";
  while($line = <INP_FILE>)
  {
    $matchLineNumber++;

    if($line=~ /$searchPattern/i)
    {#-- found a string matching pattern; return
      chomp($line);
      #TextPrint $line."\n";
      $matchLine=$line;
      last;
    }
  }

  close (INP_FILE);
}


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

