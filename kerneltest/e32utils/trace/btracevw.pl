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

#!/usr/bin/perl

use File::Find;
use File::Spec::Functions;


	my $TraceFileName;
	
	my $PrintFlagFilePos = 0;
	my $PrintFlagHdrLen = 0;
	my $PrintFlagHdrFlags = 0;
	my $PrintFlagFormatString = 0;
	my $VerboseMode = 0;
	my $RawMode = 0;
	my $FormatIdIsSubCategory = 0;
	my $OutputSawDictionaryMode = 0;
	
	# for the category range 0-191, the format string is indexed by the category & subcategory
	%FormatTables = 
		(
		0 => 			# ERDebugPrintf
			{
			0 => "ThreadId %h, %s",
			},
	
		1 => 			# ERKernPrintf
			{
			0 => "ThreadId %h, %s",
			},

		3 =>			# EThreadIdentification
			{	
			0 => "ENanoThreadCreate, NThread %x",
			1 => "ENanoThreadDestroy, NThread %x",
			2 => "EThreadCreate, NThread %x, DProcess %x, name %s",
			3 => "EThreadDestroy, NThread %x, DProcess %x, Id %x",
			4 => "EThreadName, NThread %x, DProcess %x, name %s",
			5 => "EProcessName, NThread %x, DProcess %x, name %s",
			6 => "EThreadId, NThread %x, DProcess %x, Id %x",
			7 => "EProcessCreate, DProcess %x",
			8 => "EProcessDestroy, DProcess %x",
			},
		);

	my @typedefs;
	my @members;
	my %values	= (
#		UTF::KInitialClientFormat		=>	{type=>"TFormatId", size=>2, value=>512}
		KMaxTUint8						=> {type=>"TUint8", size=>1, value=>255},
		KMaxTUint16						=> {type=>"TUint16", size=>2, value=>65535}
	);
	my %macros;
	my @classes;
	my @enums;
	my %formatStrings;		# each enum may have it's own format string
	my %formatCategories;	# each enum may have it's own format category
	
	my %filescope;
	$filescope{file}=1;
	undef $filescope{name};	

	$filescope{typedefs}=\@typedefs;
	$filescope{members}=\@members;
	$filescope{values}=\%values;
	$filescope{macros} = \%macros;
	$filescope{FormatTables} = \%FormatTables;
	
	$filescope{classes} = \@classes;
	$filescope{enums} = \@enums;

	$filescope{formatStrings} =\%formatStrings;
	$filescope{formatCategories} = \%formatCategories;
	
		
		
	if (@ARGV == 0)
  		{
  		print "BTraceVw.pl \n";
  		print "An unsupported utility which extracts UTrace-style format-strings\n";
  		print "from header files & uses these to decode a BTrace output file\n";
  		print "Syntax : BTraceVw.pl [-v] [-r] [-sd] [-i <IncFilePath>] [<BTrace file>]\n";
  		print "where  : -v  = verbose mode\n";
  		print "       : -r  = raw output mode\n";
  		print "       : -sd = produce SAW trace viewer dictionary file\n";
  		print "       :       this file then needs to be merged into the 'com.symbian.analysis.trace.ui.prefs' file\n";
  		print "       :       located under the carbide workspace directory\n";
		print "\n";
  		
		print "e.g. (this decodes a trace file & produces a comma-separated output file) : \n";
		print "btracevw.pl -i /os/kernelhwsrv/userlibandfileserver/fileserver/inc/f32tracedef.h -i /os/kernelhwsrv/userlibandfileserver/fileserver/inc/utraceefsrv.h -i /os/kernelhwsrv/userlibandfileserver/fileserver/inc/utraceefile.h trace.utf >trace.csv\n";
		print "\n";
		print "e.g. (this overwrites the SAW dictioany file) : \n";
		print "btracevw.pl -sd -i /os/kernelhwsrv/userlibandfileserver/fileserver/inc/f32tracedef.h -i /os/kernelhwsrv/userlibandfileserver/fileserver/inc/utraceefsrv.h -i /os/kernelhwsrv/userlibandfileserver/fileserver/inc/utraceefile.h >com.symbian.analysis.trace.ui.prefs\n";
  		
		exit;
		}

	while (@ARGV > 0)
		{
		
		if ($ARGV[0] eq "-i")
	        {
	        shift @ARGV;
		    ($FilePath) = @ARGV;
	        shift @ARGV;

	        undef @incFiles;
		    @incFiles;
		
		    find sub { push @incFiles, $File::Find::name if m/\.h$/i;}, $FilePath ;
		    foreach $incFile (@incFiles)
		        {
				H2Trace($incFile, \%filescope);
		        }
	        }
		elsif ($ARGV[0] eq "-r")
	        {
		    $RawMode = 1;
   	        shift @ARGV;
	        }
		elsif ($ARGV[0] eq "-sd")
	        {
		    $OutputSawDictionaryMode = 1;
   	        shift @ARGV;
	        }
		elsif ($ARGV[0] eq "-v")
	        {
		    $VerboseMode = 1;
   	        shift @ARGV;
	        }
	    else
	    	{
			$TraceFileName = "$ARGV[0]";
	        shift @ARGV;
	    	}
        }
		
	if ($VerboseMode)
		{
		dump_scope(\%filescope);
		PrintFormatTables(\%FormatTables);
		}
	if ($OutputSawDictionaryMode)
		{
		OutputSawDictionary(\%FormatTables);
		}

    if (defined ($TraceFileName))
        {
        ReadTraceFile($RawMode);
        }

        
        
        
sub ReadTraceFile($)
    {
	(my $RawMode) = @_;
#	print "Trace file is $TraceFileName, RawMode $RawMode, VerboseMode $VerboseMode\n\n";

	open (LOGFILE, "<$TraceFileName") or die "Can't open $TraceFileName: $!\n";
	binmode (LOGFILE);

	my $val = 0;


	# enum TFlags from e32btrace.h
	$EHeader2Present	= 1<<0;
	$ETimestampPresent	= 1<<1;
	$ETimestamp2Present	= 1<<2;
	$EContextIdPresent	= 1<<3;
	$EPcPresent			= 1<<4;
	$EExtraPresent		= 1<<5;
	$ERecordTruncated	= 1<<6;
	$EMissingRecord		= 1<<7;
	
	# enum TFlags2 from e32btrace.h
	$EMultipartFlagMask	= 3<<0;
	$ECpuIdMask			= 0xfff<<20;

	# enum TMultiPart from e32btrace.h
	$EMultipartFirst	= 1;
	$EMultipartMiddle	= 2;
	$EMultipartLast		= 3;
	
	$EMaxBTraceDataArray = 80;
	
	# enum TCategory from e32btrace.h
	$EThreadIdentification = 3;
	
	# enum TThreadIdentification from e32btrace.h
	$EThreadCreate = 2;
	$EThreadName = 4;
	$EProcessName = 5;
	$EThreadId = 6;
	
	# Context Id bits from e32btrace.h
	$EContextIdMask = 0x00000003;
	$EContextIdThread = 0;
	$EContextIdFIQ = 0x1;
	$EContextIdIRQ = 0x2;
	$EContextIdIDFC = 0x3;

	# enum TClassificationRange from e32btraceu.h
	$EAllRangeFirst = 192;
	$EAllRangeLast = 222;

	%TCategoryIdToString = 
		(
		0 => "ERDebugPrintf",
		1 => "EKernPrintf",
		2 => "EPlatsecPrintf",
		3 => "EThreadIdentification",
		4 => "ECpuUsage",
        5 => "EKernPerfLog",
        6 => "EClientServer",
        7 => "ERequests",
        8 => "EChunks",
        9 => "ECodeSegs",
		10 => "EPaging",
		11 => "EThreadPriority",
		12 => "EPagingMedia",
		13 => "EKernelMemory",
		14 => "EHeap",
		15 => "EMetaTrace",
		16 => "ERamAllocator",
		17 => "EFastMutex",
		18 => "EProfiling", 
        19 => "EResourceManager",
        20 => "EResourceManagerUs",
		21 => "ERawEvent ",
		128 => "EPlatformSpecificFirst",
		191 => "EPlatformSpecificLast",
		192 => "ESymbianExtentionsFirst",

		# UTrace "ALL" range 
		192 => "EPanic",
		193 => "EError",
		194 => "EWarning", 
		195 => "EBorder", 
		196 => "EState", 
		197 => "EInternals", 
		198 => "EDump", 
		199 => "EFlow", 
		200 => "ESystemCharacteristicMetrics", 
		201 => "EAdhoc",

		253 => "ESymbianExtentionsLast",
		254 => "ETest1",
		255 => "ETest2",
		);


	%ProcessNames;
	%ThreadNames;
	%ThreadIds;
	
	
	# print column titles
	if ($PrintFlagFilePos) {printf "FilePos, ";}	# col #0
	if ($PrintFlagHdrLen) {	printf "Len, ";}		# col #1
	if ($PrintFlagHdrFlags) {printf "Flags, "; }	# col #2
	printf "Category, ";			# col #3
	printf "TimeStamp, ";			# col #4
	printf "Delta, ";				# col #5
	printf "context Id, ";			# col #6
	printf "PC, ";					# col #7
	printf "UID, ";					# col #8
	if ($PrintFlagFormatString){printf "Format string, ";}	# col #9
	printf "Formatted text, ";		# col #10
	print "\n\n";

	
	while (1)
		{
		my $pos = tell (LOGFILE);
		
		# print file pos (col #0)
		if ($PrintFlagFilePos){	printf ("0x%08X, ", $pos);}
		
		my $category;
		my $subCategory;
		my $multipartFlags = 0;
		my $recordData = "";
		my $recordLen;
		my $recordPos = 0;
		
		$recordLen = ReadRecord(LOGFILE, \$pos, \$recordData, \$category, \$subCategory, \$multipartFlags, $RawMode);
		if ($recordLen == -1)
			{last;}

			
		if (!$RawMode && ($multipartFlags == $EMultipartMiddle || $multipartFlags == $EMultipartLast))
			{next;}
					
#		print record contents
#		my $buf;
#					for (my $i=0; $i < $recordLen; $i+=4)
#						{
#		$buf.= sprintf ("%08X ", unpack("V", substr($recordData, $recordPos+$i, 4)));
#						}
#		printf "\n[$buf\n]";				


		# for UTrace "ALL" range, read UID 
		if ($category >= $EAllRangeFirst && $category <= $EAllRangeLast && 
			(!$RawMode) && $multipartFlags != $EMultipartMiddle && $multipartFlags != $EMultipartLast)
			{
			$uid = unpack("V", substr($recordData, $recordPos, 4));
			$recordPos+= 4;	

			# then read formatID			
			$FormatIdIsSubCategory = ($subCategory != 0) ? 1 : 0;
			if ($FormatIdIsSubCategory)
				{
				$formatId = $subCategory
				}
			else				
				{
				$formatId = unpack("V", substr($recordData, $recordPos, 4));
  				$recordPos+= 4;
				}
			}
		
					
		# print UID (col #8)
		printf "0x%08X, ", $uid;

			
		my $formatTable;
		my $formatString;
		if ($category >= $EAllRangeFirst && $category <= $EAllRangeLast)
			{
			$formatString = $FormatTables{$uid}{$formatId};
			}
		else
			{
			$formatString = $FormatTables{$category}{$subCategory};
			}


		# Get thread names
		if ($category == $EThreadIdentification)
			{
			if ($subCategory == $EProcessName)
				{
				my $process = unpack("V", substr($recordData, 4, 4));
				my $processName = substr($recordData, 8, $recordLen - 8);	
#				printf ("\nprocess [%08X] processName [$processName]\n", $process);
				$ProcessNames{$process} = $processName;
				}
			elsif ($subCategory == $EThreadCreate || $subCategory == $EThreadName)
				{
				my $thread = unpack("V", substr($recordData, 0, 4));
				my $process = unpack("V", substr($recordData, 4, 4));
				my $threadName = substr($recordData, 8, $recordLen - 8);	
#				printf ("\nprocess [%08X] thread [%08X] threadName [$threadName]\n", $process, $thread, $threadName);
				$ThreadNames{$thread} = $ProcessNames{$process} . "::" . $threadName;
				}
			elsif ($subCategory == $EThreadId)
				{
				my $thread = unpack("V", substr($recordData, 0, 4));
				my $process = unpack("V", substr($recordData, 4, 4));
				my $threadId = unpack("V", substr($recordData, 8, 4));
#				printf ("\nprocess [%08X] thread [%08X] threadId [%08X]\n", $process, $thread, $threadId);
				$ThreadIds{$thread} = $threadId;
				}
			}
			
			
		# print Format string (col #9)
		if ($PrintFlagFormatString)
			{
			my $formatStringWithoutCommas = $formatString;
			$formatStringWithoutCommas=~ s/,/ /g;
			printf "%s, ", $formatStringWithoutCommas;
			}

		my $formattedText;
		
		my $lenFormatString = length($formatString);
		if ($lenFormatString && !$RawMode && $multipartFlags != $EMultipartMiddle && $multipartFlags != $EMultipartLast)
			{
			for (my $i=0; $i<$lenFormatString; $i++)
				{
				my $c = (substr ($formatString, $i, 1));
#				printf "$c\n";
				if ($c eq "%")
					{
					undef my $fieldLen;
					$i++;
	        		$c = (substr ($formatString, $i, 1));
					if ($c eq "%")
						{
						$formattedText.= substr ($formatString, $i, 1);
						next;
						}
					if ($c eq "*")	## take length from buffer
						{
						$fieldLen = unpack("V", substr($recordData, $recordPos, 4));
						if ($fieldLen > $recordLen-$recordPos)
							{
							$formattedText.= "*** Invalid field length ***";
							last;
							}
						$recordPos+= 4;
						$i++;
		        		$c = (substr ($formatString, $i, 1));
						}
					if (lc $c eq "x" || $c eq "h")
						{
						if (defined $fieldLen)
							{
							if (($fieldLen & 3) == 0)
								{
								for (my $i=0; $i< $fieldLen; $i+= 4)
									{
									$formattedText.= sprintf ("%08X ", unpack("V", substr($recordData, $recordPos, 4)));
									$recordPos+= 4;
									}
								}
							else
								{
								for (my $i=0; $i< $fieldLen; $i++)
									{
									$formattedText.= sprintf ("%02X ", unpack("C", substr($recordData, $recordPos, 1)));
									$recordPos++;
									}
								}
							}
						else
							{
							$formattedText.= sprintf ("0x%08X", unpack("V", substr($recordData, $recordPos, 4)));
							$recordPos+= 4;
							}
						$recordPos = ($recordPos + 3) & ~3;
						next;
						}
					# display "%ld" as hex for now as don't know how to get perl to use or display a 64 decimal value
					elsif (lc $c eq "l" && substr ($formatString, $i+1, 1) eq "d")
						{
						$i++;
						my $loWord = unpack("V", substr($recordData, $recordPos, 4));
						$recordPos+= 4;
						my $hiWord = unpack("V", substr($recordData, $recordPos, 4));
						$recordPos+= 4;
						$formattedText.= sprintf ("0x%X:%08X", $hiWord, $loWord);
						}
					elsif (lc $c eq "l" && substr ($formatString, $i+1, 1) eq "x")
						{
						$i++;
						my $loWord = unpack("V", substr($recordData, $recordPos, 4));
						$recordPos+= 4;
						my $hiWord = unpack("V", substr($recordData, $recordPos, 4));
						$recordPos+= 4;
						$formattedText.= sprintf ("0x%X:%08X", $hiWord, $loWord);
						}
					elsif (lc $c eq "d")
						{
						$formattedText.= sprintf ("%d", unpack("V", substr($recordData, $recordPos, 4)));
						$recordPos+= 4;
						$recordPos = ($recordPos + 3) & ~3;
						next;
						}
					elsif ($c eq "s")
						{
						if (!defined $fieldLen) 
							{$fieldLen = $recordLen - $recordPos;}
						$formattedText.= substr($recordData, $recordPos, $fieldLen);
						$recordPos+= $fieldLen; 
						$recordPos = ($recordPos + 3) & ~3;
						next;
						}
					elsif ($c eq "S")
						{
						if (!defined $fieldLen) 
							{$fieldLen = $recordLen-$recordPos;}
						for (my $j=0; $j < $fieldLen; $j+=2)
							{
					        my $byte = unpack("c", substr ($recordData, $recordPos+$j, 1));
 							$formattedText.= sprintf ("%c", $byte);
							}
						$recordPos+= $fieldLen; 
						$recordPos = ($recordPos + 3) & ~3;
						next;
						}
					elsif ($c eq "c")
						{
				        my $byte = unpack("c", substr ($recordData, $recordPos, 1));
						$formattedText.= sprintf ("%c", $byte);
						}
					}
				else
					{
					$formattedText.= $c;
					}
				}
			}
		else	# no format string : print as hex
			{
			for (my $i=0; $i < $recordLen; $i+=4)
				{
				$formattedText.= sprintf ("%08X ", unpack("V", substr($recordData, $i, 4)));
				}
			$recordPos+= $recordLen; $recordLen = 0;
			
			}
		

		# print Formatted text (col #10)
		$formattedText=~ s/,/;/g;
		$formattedText=~ s/\r//g;
		$formattedText=~ s/\n/,/g;
		printf "%s", $formattedText;

		printf("\n");

		if ($len < 0 || $recordLen < 0)	{die "truncated file";}
  

		$pos+= ($len +3) & ~3;
		seek (LOGFILE, $pos, SEEK_SET) or die "truncated file";
		$i++;
		}

	close (LOGFILE);

	if ($VerboseMode)
		{
		print "*** Processes ***\n";
		for $id ( keys %ProcessNames )
			{
			printf ("process %08X ProcessName %s\n", $id, $ProcessNames{$id});
			}
		print "*** Thread ***\n";
		for $id ( keys %ThreadNames )
			{
			printf ("thread %08X ThreadName %s::%X\n", $id, $ThreadNames{$id}, $ThreadIds{$id});
			}
		}

    }

    
sub ReadSingleRecord
	{
	($fh, $data, $dataLen, $recordLen, $category, $subCategory, $multipartFlags, $extraN, $totalLen, $offset, $RawMode) = @_;	
	
	my $hdr;
	my $flags;
	my $header2;
	my $timestamp;
	my $timestamp2;
	my $contextId;
	my $programConter;	
	
	my $recordOffset = 0;
	
	$timestampLast;	
	my $timestampDelta = 0;	
	
	my $bytesRead = read($fh, $hdr, 4);
	
	
	if ($bytesRead < 4)	
		{return -1;}

	($$recordLen,$flags,$$category,$$subCategory) = unpack("CCCC", $hdr);
	$$dataLen = $$recordLen-4;
	
	if ($flags & $EHeader2Present)
		{$$multipartFlags = (ReadDword($fh) & $EMultipartFlagMask); $$dataLen-= 4}
	else
		{$$multipartFlags = 0;}
	if ($flags & $ETimestampPresent)
		{$timestamp = ReadDword($fh); $$dataLen-= 4;}
	if ($flags & $ETimestamp2Present)
		{$timestamp2 = ReadDword($fh); $$dataLen-= 4;}
	if ($flags & $EContextIdPresent)
		{$contextId = ReadDword($fh); $$dataLen-= 4;}
	if ($flags & $EPcPresent)
		{$programConter = ReadDword($fh); $$dataLen-= 4;}
	if ($flags & $EExtraPresent)
		{$$extraN = ReadDword($fh); $$dataLen-= 4;}
	if ($$multipartFlags != 0)
		{
		$$totalLen = ReadDword($fh);  $$dataLen-= 4;
		if ($$multipartFlags == $EMultipartMiddle || $$multipartFlags == $EMultipartLast)
			{$$offset = ReadDword($fh);  $$totalLen-= 4; $$dataLen-= 4;}
		}				

	$timestampDelta = $timestamp - $timestampLast;
	$timestampLast = $timestamp;

	read($fh, $$data, ($$dataLen + 3) & ~3);


	if ($RawMode || $$multipartFlags == $EMultipartFirst || $$multipartFlags == 0)
		{
		# print header len (col #1)
		if ($PrintFlagHdrLen){printf ("0x%02X, ", $$recordLen);}
	
		# print header flags (col #2)
		if ($PrintFlagHdrFlags)
			{
			printf ("%02X ", $flags);
			if ($flags & $EHeader2Present) {printf "EHeader2Present ";}
			if ($flags & $ETimestampPresent) {printf "ETimestampPresent ";}
			if ($flags & $ETimestamp2Present) {printf "ETimestamp2Present ";}
			if ($flags & $EContextIdPresent) {printf "EContextIdPresent ";}
			if ($flags & $EPcPresent) {printf "EPcPresent ";}
			if ($$multipartFlags != 0)
				{
				printf "EExtraPresent ";
				if ($$multipartFlags == $EMultipartFirst) {print "EMultipartFirst ";}
				elsif ($$multipartFlags == $EMultipartMiddle) {print "EMultipartMiddle ";}
				elsif ($$multipartFlags == $EMultipartLast) {print "EMultipartLast ";}
				printf ("ExtraN(0x%08X) ", $$extraN);
				}
			if ($flags & $ERecordTruncated) {printf "ERecordTruncated ";}
			if ($flags & $EMissingRecord) {printf "EMissingRecord ";}
			print ",";
			}
				
		# print category (col #3)
		printf "(%d;%d) $categoryString  , ", $$category, $$subCategory;
	
		# print timestamp(s) (col #4)
		printf "0x";
		if (defined $timestamp2) {printf "%08X : ", $timestamp2;}
		printf "%08X", $timestamp;
		printf ", ";;
	
		# print timestamp delta (col #5)
		printf "0x%08X, ", $timestampDelta;

		# print context Id (col #6)
		if (!$RawMode && defined $ThreadNames{$contextId})
			{
			printf ("%s::%X, ", $ThreadNames{$contextId}, $ThreadIds{$contextId});
			}
		else			
			{
			if ((($contextId & $EContextIdMask) == $EContextIdThread) || $RawMode)
				{printf "0x%08X, ", $contextId;}
			elsif (($contextId & $EContextIdMask) == $EContextIdFIQ)
				{printf "FIQ, ";}
			elsif (($contextId & $EContextIdMask) == $EContextIdIRQ)
				{printf "IRQ, ";}
			elsif (($contextId & $EContextIdMask) == $EContextIdIDFC)
				{printf "IDFC, ";}
			}
	
		# print Program Counter (col #7)
		printf "0x%08X, ", $programConter;
		}

		
	
	
#########################################################
#	my $hex;
#	for (my $i=0; $i < $$dataLen; $i+=4)
#		{
#		$hex.= sprintf ("%08X ", unpack("V", substr($$data, $i, 4)));
#		}
#	printf "\nadding [$hex]\n";
#########################################################
	return $bytesRead
	}

	      
sub ReadRecord 
	{
	($fh, $recordPos, $recordData, $category, $subCategory, $multipartFlags, $RawMode) = @_;
#	printf "CurrentPos %08X\n", $pos;



	seek ($fh, $$recordPos, SEEK_SET) or die "truncated file";
	my $recordLen;
	my $extraN;
	my $totalLen;
	my $offset;
	my $dataLen;
	my $data;
	my $bytesRead;
	
	
	$bytesRead = ReadSingleRecord($fh,  \$data, \$dataLen, \$recordLen, \$$category, \$$subCategory, \$$multipartFlags, \$extraN, \$totalLen, \$offset, $RawMode);

	if ($bytesRead == -1)	# eof ?
		{return -1; }
	$$recordPos+= ($recordLen +3) & ~3;
	
	$$recordData = $data;
    $offset = $dataLen;

	$offset-= 4;		# subtract 4 bytes for UID ?????????
    
    if ($RawMode || $$multipartFlags != $EMultipartFirst)
    	{return $dataLen;}

    $pos = $$recordPos;

	while (1)
		{
		
		# find next record, i.e. look for a record which matches $extraN 
		
		seek ($fh, $pos, SEEK_SET) or die "truncated file";

		my $recordLen;
		
		my $category;
		my $subCategory;
		my $multipartFlags;
		my $currentExtraN;
		my $currentOffset;
		
		my $totalLen;
		my $currentDataLen;
		my $data;
		$bytesRead = ReadSingleRecord($fh, \$data, \$currentDataLen, \$recordLen, \$category, \$subCategory, \$multipartFlags, \$currentExtraN, \$totalLen, \$currentOffset, $RawMode);
		if ($bytesRead == -1)	# eof ?
			{return -1; }
		$pos+= ($recordLen +3) & ~3;
		
#		printf "\npos %08X, Seaching for (extra %08X, offset %08X), found (extra %08X, offset %08X)\n",
#			$pos, $extraN, $offset, $currentExtraN, $currentOffset;

		if ($currentExtraN == $extraN && $currentOffset == $offset)
			{
			$$recordData.= $data;
			$offset+= $currentDataLen;
			$dataLen+= $currentDataLen;
			}
			
		if ($multipartFlags == $EMultipartLast)
			{last;}
		}
	
	return $dataLen;
	}	

sub ReadDword {
	(my $fh) = @_;
	my $buffer;

	$bytesRead = read($fh, $buffer, 4);
	if ($bytesRead < 4) 	{die "truncated file";}

	my $dword = unpack("V", $buffer);

	return $dword
	};

sub ReadByte {
	(my $fh) = @_;
	my $buffer;

	$bytesRead = read($fh, $buffer, 1);
	if ($bytesRead < 1) 	{die "truncated file";}

	my $byte = unpack("C", $buffer);

	return $byte
	};

    
	
sub PrintFormatTables($)
	{
	my ($formatTables) = @_;
		
	for $tableIndex ( sort keys %$formatTables )
		{
		printf ("SYMTraceFormatCategory %08X:\n", $tableIndex);
		for $formatId (sort keys %{ $$formatTables{$tableIndex} } )
			{
			printf ("%08X => %s\n", $formatId, $$formatTables{$tableIndex}{$formatId});
			}
			print "\n";
		}
	}
        


sub OutputSawDictionary($)
	{
	my ($formatTables) = @_;


	# SAW enums
	$EFieldTypeHexDump = 0;
	$EFieldTypeHex = 1;
	$EFieldTypeDecimal = 2;
	$EFieldTypeStringToEnd = 3;
	$EFieldTypeNullTerminatedString = 4;
	$EFieldTypeHexDumpToEnd = 5;
	$EFieldTypeUnicodeToEnd = 6;
	$EFieldTypeNullTerminatedUnicode = 7;
	$EFieldTypeCountedUnicode = 8;
	$EFieldTypeCountedHexDump = 9;
	$EFieldTypeCountedString = 10;

	my $moduleIds;	# string containg all UIDs separared by semi-colons
		
	for $tableIndex ( sort keys %$formatTables )
		{
		if ($tableIndex < 256)
			{
			next;
			}
		$moduleIds.= sprintf ("%08X;", $tableIndex);
		
		printf ("MODULEID_%08X_DESC=\n", $tableIndex);
		printf ("MODULEID_%08X_NAME=%08X\n", $tableIndex, $tableIndex);
		
		my $formatIds;
		$formatIds = sprintf ("MODULEID_%08X_FORMATIDS=", $tableIndex);
		
		for $formatId  (sort keys %{ $$formatTables{$tableIndex} } )
			{
			$formatIds.= sprintf ("%d;", $formatId);
			}
		printf ("$formatIds\n");
		
		
		for $formatId (sort keys %{ $$formatTables{$tableIndex} } )
			{
			my $fieldCount = 0;
			my $formatString = $$formatTables{$tableIndex}{$formatId};
			
#printf ("formatString = (%s)\n", $formatString);

			# format name is the first format string up until the first space or '%' character or end-of line ...
			$formatString=~ m/^[^%\s]*/;
			my $formatName = $&;
			
			# thow the format name away
			$formatString = $';
			
			# strip the leading space
			$formatString=~ s/\s*//;

			printf ("MODULEID_%08X_FORMATID_%d_NAME=%s\n", $tableIndex, $formatId, $formatName);
#printf ("MODULEID_%08X_FORMATID_%d_DESC=\n", $tableIndex, $formatId);

			my $lenFormatString = length($formatString);
			
			my $formattedText;
			my $fieldType = $EFieldTypeHex;
			my $fieldLen = 0;
			while (length($formatString))
				{
				my $c = (substr ($formatString, 0, 1));
#print ("[$formatString][$c]\n");				
				$formatString=~ s/.//;	# strip the leading space
				if ($c eq "%")
					{
#print "found %\n";							
					my $fieldLenSpecified = 0;
	        		$c = (substr ($formatString, 0, 1));
					$formatString=~ s/.//;	# discard char
#print "c2=$c\n";							
					if ($c eq "%")
						{
						$formattedText.= substr ($formatString, 0, 1);
						next;
						}
					if ($c eq "*")	## take length from buffer
						{
						$fieldLenSpecified = 1;
		        		$c = (substr ($formatString, 0, 1));
						$formatString=~ s/.//;	# discard char
						}
					if (lc $c eq "x" || $c eq "h")
						{
						## deal wilth $fieldLenSpecified
						if ($fieldLenSpecified)
							{
							$fieldType = $EFieldTypeCountedHexDump;
							$fieldLen = 0;
							}
						else
							{
							$fieldType = $EFieldTypeHex;
							$fieldLen = 4;
							}
						}
					elsif (lc $c eq "l" && substr ($formatString, 0, 1) eq "d")
						{
						$formatString=~ s/.//;	# discard char
						$fieldType = $EFieldTypeDecimal;
						$fieldLen = 8;
						}
					elsif (lc $c eq "l" && substr ($formatString, 0, 1) eq "x")
						{
						$formatString=~ s/.//;	# discard char
						$fieldType = $EFieldTypeHex;
						$fieldLen = 8;
						}
					elsif (lc $c eq "d")
						{
						$fieldType = $EFieldTypeDecimal;
						$fieldLen = 4;
						}
					elsif ($c eq "s")
						{
						## deal wilth $fieldLenSpecified
						if ($fieldLenSpecified)
							{
							$fieldType = $EFieldTypeCountedString;
							$fieldLen = 0;
							}
						else
							{
							$fieldType = $EFieldTypeStringToEnd;
							$fieldLen = 0;
							}
						}
					elsif ($c eq "S")
						{
						## deal wilth $fieldLenSpecified
						if ($fieldLenSpecified)
							{
							$fieldType = $EFieldTypeCountedUnicode;
							$fieldLen = 0;
							}
						else
							{
							$fieldType = EFieldTypeUnicodeToEnd;
							$fieldLen = 0;
							}
						}
					elsif ($c eq "c")
						{
						$fieldType = $EFieldTypeHex;
						$fieldLen = 1;
						}
					printf ("MODULEID_%08X_FORMATID_%d_FIELD_%d_NAME=%s\n", $tableIndex, $formatId, $fieldCount, $formattedText);
					printf ("MODULEID_%08X_FORMATID_%d_FIELD_%d_TYPE=%s\n", $tableIndex, $formatId, $fieldCount, $fieldType);
					if ($fieldLen > 0)
						{printf ("MODULEID_%08X_FORMATID_%d_FIELD_%d_LENGTH=%s\n", $tableIndex, $formatId, $fieldCount, $fieldLen);}
					$fieldCount++;
					$formattedText="";
					
					$formatString=~ s/\s//;	# strip the leading space
					}
				else
					{
#					if ($c eq ":") {$formattedText.= '\\'; }
					$formattedText.= $c;
					}
				}
			printf ("MODULEID_%08X_FORMATID_%d_FIELDS=%d\n", $tableIndex, $formatId, $fieldCount);
			
			}
		print "MODULEIDS=$moduleIds\n";
		}
	}
	
	
	
	
	
	
	        
        
sub H2Trace($$)
{
	%basictypes = (
		TInt8		=>	1,
		TUint8		=>	1,
		TInt16		=>	2,
		TUint16		=>	2,
		TInt32		=>	4,
		TUint32		=>	4,
		TInt		=>	4,
		TUint		=>	4,
		TBool		=>	4,
		TInt64		=>	8,
		TUint64		=>	8,
		TLinAddr	=>	4,
		TVersion	=>	4,
		TPde		=>	4,
		TPte		=>	4,
		TProcessPriority => 4,
		TFormatId	=>  2,
	);
	
	if (scalar(@_)!= 2) {
		die "perl h2trace.pl <input.h>\n";
	}
	my ($infile, $filescope) = @_;
	
	if ($VerboseMode)
		{print "\nOpening $infile\n";}
	
	open IN, $infile or die "Can't open $infile for input\n";
	my $in;
	while (<IN>) {
		$in.=$_;
	}
	close IN;
	
	# First remove any backslash-newline combinations
	$in =~ s/\\\n//gms;
	
	# Remove any character constants
	$in =~  s/\'(.?(${0})*?)\'//gms;
	
	# Strip comments beginning with //
	$in =~ s/\/\/(.*?)\n/\n/gms;    #//(.*?)\n
	
	# Strip comments (/* */) but leave doxygen comments (/** */)
	$in =~ s/\/\*[^*](.*?)\*\//\n/gms;  #/*(.*?)*/
	
	
	# Collapse whitespace into a single space or newline
	$in =~ s/\t/\ /gms;
	$in =~ s/\r/\ /gms;
	
	# Tokenize on non-identifier characters
	my @tokens0 = split(/(\W)/,$in);
	my @tokens;
	my $inString = 0;
	my $inComment = 0;
	my $string;
	foreach $t (@tokens0) {
		next if ($t eq "");
		next if (!$inString && ($t eq " " or $t eq ""));
		if ($inComment == 0) 
			{
			if ($t eq "/")
				{$inComment = 1;}
			}
		elsif ($inComment == 1) 
			{
			if ($t eq "*")
				{$inComment = 2;}
			else
				{$inComment = 0;}
			}
		elsif ($inComment == 2) 
			{
			if ($t eq "*")
				{$inComment = 3;}
			}
		elsif ($inComment == 3) 
			{
			if ($t eq "/")
				{
				$inComment = 0;
		        # if we were in a string, need to push previous '*'
		        if ($inString)
		          {
		          push @tokens, "*";
		          }
				$inString = 0;	# end of comment aborts a string
				$string = "";
				}
			else
				{$inComment = 2;}
			}
			
		if ($t eq "\"")
			{
			if (!$inString) 
				{
				$inString=1;
				next;
				}
			else
				{
				$inString=0;
				$t = $string;
				$string = "";
#				if ($VerboseMode) {print "string : [$t]\n";	}
				}
			}
			
		if ($inString)
			{
			$string.= $t;
			next;
			}
		push @tokens, $t;
	}
	
	my $CurrentTraceFormatString;
	my $CurrentTraceFormatCategory;
	# format Key as specified by the @TraceFormatCategory tag is either the current category 
	# or the current UID
	my $CurrentFormatTableKey;	
	
	
	my $line=1;
	parse_scope($filescope, \@tokens, \$line);

	#print $in;
	#print join (" ", @tokens);
}	# end of     H2Trace
	


	sub parse_scope($$$) {
		my ($scope, $tokens, $line) = @_;
		my $state = 1;
		
		my @classes;
		my $curr_offset=0;
		my $overall_align=0;
#		print ">parse_scope $scope->{name}\n";
		
		while (scalar(@$tokens))
			{
			my $t = shift @$tokens;
#			printf "t: [$t] [$$line]\n";
	    	if (!defined ($t)) {
	      		printf "undefined !";
	      		next;
	      	}
			if ($state>=-1 and $t eq "\n") {
				++$$line;
				$state=1;
				next;
			} elsif ($state==-1 and $t ne "\n") {
				next;
			} elsif ($state==-2 and $t ne ';') {
				next;
			}
			
			if ($state>0 and $t eq '#') {
				$t = shift @$tokens;
				if ($t eq 'define') {
					my $ident = shift @$tokens;
					my $defn = shift @$tokens;
					if ($defn ne '(') {	# don't do macros with parameters
#					print "MACRO: $ident :== $defn\n";
					$macros{$ident} = $defn;
					}
				}
				$state=-1;	# skip to next line
				next;
			}
			
			
			if (parse_doxygen($scope,$tokens, $line, $t) == 1)
				{next;}
	
			if ($t eq "namespace" ) {
				$state=0;
				my %cl;
				$cl{specifier}=$t;
				$cl{scope}=$scope;
				$cl{values}=$scope->{values};
				$cl{members}=\$scope->{members};
				$cl{typedefs}=\$scope->{typedefs};
				$cl{FormatTables}=$scope->{FormatTables};
				$cl{formatStrings} =$scope->{formatStrings};
				$cl{formatCategories} =$scope->{formatCategories};
				
				my $new_namespace = \%cl;
				my $n = get_token($scope,$tokens,$line);
				if ($n !~ /\w+/) {
					warn "Unnamed $t not supported at line $$line\n";
					return;
				}
				$new_namespace->{name}=$n;
				my @class_match = grep {$_->{name} eq $n} @classes;
				my $exists = scalar(@class_match);
				my $b = get_token($scope,$tokens,$line);
				if ($b eq ':') {
					die "Inheritance not supported at line $$line\n";
				} elsif ($b eq ';') {
					# forward declaration
					push @classes, $new_namespace unless ($exists);
					next;
				} elsif ($b ne '{') {
					warn "Syntax error#1 at line $$line\n";
					return;
				}
				if ($exists) {
					$new_namespace = $class_match[0];
					if ($new_namespace->{complete}) {
						warn "Duplicate definition of $cl{specifier} $n\n";
					}
				}
				push @classes, $new_namespace unless ($exists);
				parse_scope($new_namespace, $tokens, $line);
				next;
			}
			
			if ($t eq "struct" or $t eq "class" or $t eq "NONSHARABLE_CLASS") {
				next if ($state==0);
				$state=0;
				my %cl;
				$cl{specifier}=$t;
				$cl{scope}=$scope;
				my @members;
				my @typedefs;
				$cl{members}=\@members;
				$cl{typedefs}=\@typedefs;
				$cl{FormatTables}=$scope->{FormatTables};
				my $new_class = \%cl;
				my $n;

				if ($t eq "NONSHARABLE_CLASS")
					{
					my $b = get_token($scope,$tokens,$line);
					if ($b !~ /\(/) {die "Syntax error at line $$line\n";}
					$n = get_token($scope,$tokens,$line);
  				$b = get_token($scope,$tokens,$line);
					if ($b !~ /\)/) {die "Syntax error at line $$line\n";}
					}
				else					
					{
					$n = get_token($scope,$tokens,$line);
					}
								
				
				if ($n !~ /\w+/) {
					warn "Unnamed $t not supported at line $$line\n";
					return;
				}
				$new_class->{name}=$n;
				my @class_match = grep {$_->{name} eq $n} @classes;
				my $exists = scalar(@class_match);
				my $b = get_token($scope,$tokens,$line);
				#skip inheritance etc until we get to a '{' or \ ';'
				while ($b ne '{' && $b ne ';')
					{
			        $b = get_token($scope,$tokens,$line);
			        die "Syntax error#2 at line $$line\n" if  (!defined $b);
					}
				if ($b eq ';') {
					# forward declaration
					push @classes, $new_class unless ($exists);
					next;
				} 
				if ($exists) {
					$new_class = $class_match[0];
					if ($new_class->{complete}) {
						warn "Duplicate definition of $cl{specifier} $n\n";
					}
				}
				push @classes, $new_class unless ($exists);
				parse_scope($new_class, $tokens, $line);
				next;
			} elsif ($t eq "enum") {
				$state=0;
				my $n = get_token($scope,$tokens,$line);
				my $name="";
				if ($n =~ /\w+/) {
					$name = $n;
					$n = get_token($scope,$tokens,$line);
				}
				push @enums, $name;
				if ($n ne '{') {
					die "Syntax error#4 at line $$line\n";
				}
				parse_enum($scope, $tokens, $line, $name);
				next;
			} elsif ($t eq '}') {
				$state=0;
				if ($scope->{scope}) {
			        if ($scope->{specifier} eq "namespace")
			        	{
						$scope->{complete}=1;
#						print "Scope completed\n";
						last;
						}
					$t = get_token($scope,$tokens,$line);
					# skip to next ';'
					while (defined ($t) and $t ne ';')
						{$t = get_token($scope,$tokens,$line);}
					die "Syntax error#5 at line $$line\n" if ($t ne ';');
					$scope->{complete}=1;
#					print "Scope completed\n";
					last;
				}
				warn "Syntax error#5 at line $$line\n";
				return;
			}
			$state=0;
			if ($scope->{scope}) {
				if ($t eq "public" or $t eq "private" or $t eq "protected") {
					if (shift (@$tokens) eq ':') {
						next;	# ignore access specifiers
					}
				die "Syntax error#6 at line $$line\n";
				}
			}
			unshift @$tokens, $t;
			
			my @currdecl = parse_decl_def($scope, $tokens, $line);
#			print scalar (@currdecl), "\n";
			if ($t eq 'static') {
				next;	# skip static members
			}
			my $typedef;
			if ($t eq 'typedef') {
#			print "TYPEDEF\n";
				$typedef = 1;
				$t = shift @currdecl;
				$t = $currdecl[0];
			} else {
#			print "NOT TYPEDEF\n";
				$typedef = 0;
			}
#			print "$currdecl[0]\n";
			next if (scalar(@currdecl)==0);
			
			if ($t eq "const") {
				# check for constant declaration
#				print "CONST $currdecl[1] $currdecl[2] $currdecl[3]\n";
				my $ctype = lookup_type($scope, $currdecl[1]);
#				print "$ctype->{basic}    $ctype->{size}\n";
				if ($ctype->{basic} and $currdecl[2]=~/^\w+$/ and $currdecl[3] eq '=') {
					if ($typedef!=0) {
						die "Syntax error#7 at line $$line\n";
					}
					shift @currdecl;
					shift @currdecl;
					my $type = $ctype->{name};
					my $name;		#### = shift @currdecl;

					if ($scope->{name})
						{	
						$name = $scope->{name} . "::" . shift @currdecl;
						}
					else
						{
						$name = shift @currdecl;
						}
#					printf "[$name,$scope->{name}]";
					my $size = $ctype->{size};
					shift @currdecl;
					my $value = get_constant_expr($scope,\@currdecl,$line);
					$values{$name} = {type=>$type, size=>$size, value=>$value};
					next;
				}
			}
			
			
			
		}
	}
	
	sub get_token($$$) {
		my ($scope,$tokenlist,$line) = @_;
		while (scalar(@$tokenlist)) {
			my $t = shift @$tokenlist;
			return $t if (!defined($t));
			if (parse_doxygen($scope,$tokenlist, $line, $t) == 1)
				{next;}
			if ($t !~ /^[\s]*$/)
				{
				if ($$tokenlist[0] eq ":" and $$tokenlist[1] eq ":")
					{
					$t.= shift @$tokenlist;
					$t.= shift @$tokenlist;
					$t.= shift @$tokenlist;
#					print "Colon-separated token";
					}
				return $t
				}
			++$$line;
		}
  		return undef;
	}
	
	sub skip_qualifiers($) {
		my ($tokens) = @_;
		my $f=0;
		my %quals = (
			EXPORT_C => 1,
			IMPORT_C => 1,
			inline => 1,
			virtual => 0,
			const => 0,
			volatile => 0,
			static => 0,
			extern => 0,
			LOCAL_C => 0,
			LOCAL_D => 0,
			GLDEF_C => 0,
			GLREF_C => 0,
			GLDEF_D => 0,
			GLREF_D => 0
			);
		for (;;) {
			my $t = $$tokens[0];
			my $q = $quals{$t};
			last unless (defined ($q));
			$f |= $q;
			shift @$tokens;
		}
		return $f;
	}
	
	sub parse_indirection($) {
		my ($tokens) = @_;
		my $level = 0;
		for (;;) {
			my $t = $$tokens[0];
			if ($t eq '*') {
				++$level;
				shift @$tokens;
				next;
			}
			last if ($t ne "const" and $t ne "volatile");
			shift @$tokens;
		}
		return $level;
	}
	
	sub get_operand($$$) {
		my ($scope,$tokens,$line) = @_;
		my $t = get_token($scope,$tokens,$line);
		if ($t eq '-') {
			my $x = get_operand($scope,$tokens,$line);
			return -$x;
		} elsif ($t eq '+') {
			my $x = get_operand($scope,$tokens,$line);
			return $x;
		} elsif ($t eq '~') {
			my $x = get_operand($scope,$tokens,$line);
			return ~$x;
		} elsif ($t eq '!') {
			my $x = get_operand($scope,$tokens,$line);
			return $x ? 0 : 1;
		} elsif ($t eq '(') {
			my $x = get_constant_expr($scope,$tokens,$line);
			my $t = get_token($scope,$tokens,$line);
			if ($t ne ')') {
				warn "Missing ) at line $$line\n";
				return undefined;
			}
			return $x;
		} elsif ($t eq "sizeof") {
			my $ident = get_token($scope,$tokens,$line);
			if ($ident eq '(') {
				$ident = get_token($scope,$tokens,$line);
				my $cb = get_token($scope,$tokens,$line);
				if ($cb ne ')') {
					warn "Bad sizeof() syntax at line $$line\n";
					return undefined;
				}
			}
			$ident = look_through_macros($ident);
			if ($ident !~ /^\w+$/) {
				warn "Bad sizeof() syntax at line $$line\n";
				return undefined;
			}
			my $type = lookup_type($scope, $ident);
			if (!defined $type) {
				warn "Unrecognised type $ident at line $$line\n";
				return undefined;
			}
			if ($type->{basic}) {
				return $type->{size};
			} elsif ($type->{enum}) {
				return 4;
			} elsif ($type->{ptr}) {
				return 4;
			} elsif ($type->{fptr}) {
				return 4;
			}
			my $al = $type->{class}->{align};
			my $sz = $type->{class}->{size};
			return ($sz+$al-1)&~($al-1);
		}
		$t = look_through_macros($t);
		if ($t =~ /^0x/i) {
			return oct($t);
		} elsif ($t =~ /^\d/) {
			return $t;
		} elsif ($t =~ /^\w+$/) {
			my $x = lookup_value($scope,$t);
#			die "Unrecognised identifier '$t' at line $$line\n" unless defined($x);
			if (!defined($x)) {
				print "Unrecognised identifier '$t' at line $$line\n" ;
			}
			return $x;
		} elsif ($t =~ /^\w+::\w+$/) {
			my $x = lookup_value($scope,$t);
#			die "Unrecognised identifier '$t' at line $$line\n" unless defined($x);
			if (!defined($x)) {
				print "Unrecognised identifier '$t' at line $$line\n" ;
			}
			return $x;
		} else {
			warn "Syntax error#10 at line $$line\n";
			return undefined;
		}
	}
	
	sub look_through_macros($) {
		my ($ident) = @_;
		while ($ident and $macros{$ident}) {
			$ident = $macros{$ident};
		}
		return $ident;
	}
	
	sub lookup_value($$) {
		my ($scope,$ident) = @_;
		while ($scope) {
			my $vl = $scope->{values};
			if (defined($vl->{$ident})) {
				return $vl->{$ident}->{value};
			}
			$scope = $scope->{scope};
		}
		return undef();
	}
	
	sub lookup_type($$) {
		my ($scope,$ident) = @_;
		if ($basictypes{$ident}) {
			return {scope=>$scope, basic=>1, name=>$ident, size=>$basictypes{$ident} };
		}
		while ($scope) {
			if ($basictypes{$ident}) {
				return {scope=>$scope, basic=>1, name=>$ident, size=>$basictypes{$ident} };
			}
			my $el = $scope->{enums};
			my $cl = $scope->{classes};
			my $td = $scope->{typedefs};
			if (grep {$_ eq $ident} @$el) {
				return {scope=>$scope, enum=>1, name=>$ident, size=>4 };
			}
			my @match_class = (grep {$_->{name} eq $ident} @$cl);
			if (scalar(@match_class)) {
				return {scope=>$scope, class=>$match_class[0]};
			}
			my @match_td = (grep {$_->{name} eq $ident} @$td);
			if (scalar(@match_td)) {
				my $tdr = $match_td[0];
				my $cat = $tdr->{category};
				if ($cat eq 'basic' or $cat eq 'enum' or $cat eq 'class') {
					$ident = $tdr->{alias};
					next;
				} else {
					return { scope=>$scope, $cat=>1, $size=>$tdr->{size} };
				}
			}
			$scope = $scope->{scope};
		}
		return undef();
	}
	
	sub get_mult_expr($$$) {
		my ($scope,$tokens,$line) = @_;
		my $x = get_operand($scope,$tokens,$line);
		my $t;
		for (;;) {
			$t = get_token($scope,$tokens,$line);
			if ($t eq '*') {
				my $y = get_operand($scope,$tokens,$line);
				$x = $x * $y;
			} elsif ($t eq '/') {
				my $y = get_operand($scope,$tokens,$line);
				if ($y != 0)
					{$x = int($x / $y);}
			} elsif ($t eq '%') {
				my $y = get_operand($scope,$tokens,$line);
				if ($y != 0)
					{$x = int($x % $y);}
			} else {
				last;
			}
		}
		unshift @$tokens, $t;
		return $x;
	}
	
	sub get_add_expr($$$) {
		my ($scope,$tokens,$line) = @_;
		my $x = get_mult_expr($scope,$tokens,$line);
		my $t;
		for (;;) {
			$t = get_token($scope,$tokens,$line);
			if ($t eq '+') {
				my $y = get_mult_expr($scope,$tokens,$line);
				$x = $x + $y;
			} elsif ($t eq '-') {
				my $y = get_mult_expr($scope,$tokens,$line);
				$x = $x - $y;
			} else {
				last;
			}
		}
		unshift @$tokens, $t;
		return $x;
	}
	
	sub get_shift_expr($$$) {
		my ($scope,$tokens,$line) = @_;
		my $x = get_add_expr($scope,$tokens,$line);
		my $t, $t2;
		for (;;) {
			$t = get_token($scope,$tokens,$line);
			if ($t eq '<' or $t eq '>') {
				$t2 = get_token($scope,$tokens,$line);
				if ($t2 ne $t) {
					unshift @$tokens, $t2;
					last;
				}
			}
			if ($t eq '<') {
				my $y = get_add_expr($scope,$tokens,$line);
				$x = $x << $y;
			} elsif ($t eq '>') {
				my $y = get_add_expr($scope,$tokens,$line);
				$x = $x >> $y;
			} else {
				last;
			}
		}
		unshift @$tokens, $t;
		return $x;
	}
	
	sub get_and_expr($$$) {
		my ($scope,$tokens,$line) = @_;
		my $x = get_shift_expr($scope,$tokens,$line);
		my $t;
		for (;;) {
			$t = get_token($scope,$tokens,$line);
			if ($t eq '&') {
				my $y = get_shift_expr($scope,$tokens,$line);
				$x = $x & $y;
			} else {
				last;
			}
		}
		unshift @$tokens, $t;
		return $x;
	}
	
	sub get_xor_expr($$$) {
		my ($scope,$tokens,$line) = @_;
		my $x = get_and_expr($scope,$tokens,$line);
		my $t;
		for (;;) {
			$t = get_token($scope,$tokens,$line);
			if ($t eq '^') {
				my $y = get_and_expr($scope,$tokens,$line);
				$x = $x ^ $y;
			} else {
				last;
			}
		}
		unshift @$tokens, $t;
		return $x;
	}
	
	sub get_ior_expr($$$) {
		my ($scope,$tokens,$line) = @_;
		my $x = get_xor_expr($scope,$tokens,$line);
		my $t;
		for (;;) {
			$t = get_token($scope,$tokens,$line);
			if ($t eq '|') {
				my $y = get_xor_expr($scope,$tokens,$line);
				$x = $x | $y;
			} else {
				last;
			}
		}
		unshift @$tokens, $t;
		return $x;
	}
	
	sub get_constant_expr($$$) {
		my ($scope,$tokens,$line) = @_;
		my $x = get_ior_expr($scope,$tokens,$line);
		return $x;
	}
	
	sub parse_enum($$$$) {
		my ($scope,$tokens,$line,$enum_name) = @_;
		my $vl = $scope->{values};
		my $fstr = $scope->{formatStrings};
		my $fcat = $scope->{formatCategories};
		my $fmtTable = $scope->{FormatTables};
		
		my $x = 0;
		for (;;) {
			my $t = get_token($scope,$tokens,$line);
			last if ($t eq '}');
			if (!defined($t)) {
				die "Unexpected end of file #2 at line $$line\n";
			}
			
			if ($t eq '#') {
				next;
				}
			
			if ($t !~ /^\w+$/) {
				warn "Syntax error#11 at line $$line\n";
				next;
			}

			if ($scope->{name})
				{	
				$t = $scope->{name} . "::" . $t;
				}

			if (defined($vl->{$t})) {
				warn "Duplicate identifier [$t] at line $$line\n";
			}
			my $t2 = get_token($scope,$tokens,$line);
			if ($t2 eq ',') {
				$vl->{$t} = {type=>$enum_name, size=>4, value=>$x, enum=>1};
				$fstr->{$t} = $CurrentTraceFormatString; 
				$fcat->{$t} = $CurrentTraceFormatCategory; 
				if (defined $CurrentTraceFormatCategory && defined $CurrentTraceFormatString)
					{ $fmtTable->{$CurrentTraceFormatCategory}{$x} = $CurrentTraceFormatString; }
				undef $CurrentTraceFormatString;
				++$x;
			} elsif ($t2 eq '}') {
				$vl->{$t} = {type=>$enum_name, size=>4, value=>$x, enum=>1};
				$fstr->{$t} = $CurrentTraceFormatString; 
				$fcat->{$t} = $CurrentTraceFormatCategory; 
				if (defined $CurrentTraceFormatCategory && defined $CurrentTraceFormatString)
					{ $fmtTable->{$CurrentTraceFormatCategory}{$x} = $CurrentTraceFormatString; }
				undef $CurrentTraceFormatString;
				++$x;
				last;
			} elsif ($t2 eq '=') {
				$x = get_constant_expr($scope, $tokens, $line);
				$vl->{$t} = {type=>$enum_name, size=>4, value=>$x, enum=>1};
				$fstr->{$t} = $CurrentTraceFormatString; 
				$fcat->{$t} = $CurrentTraceFormatCategory;
				if (defined $CurrentTraceFormatCategory && defined $CurrentTraceFormatString)
					{ $fmtTable->{$CurrentTraceFormatCategory}{$x} = $CurrentTraceFormatString; }
				undef $CurrentTraceFormatString; 
				++$x;
				$t2 = get_token($scope,$tokens,$line);
				last if ($t2 eq '}');
				next if ($t2 eq ',');
				warn "Syntax error#12 at line $$line\n";
			} else {
				unshift @$tokens, $t2;
			}
		}
		my $t = get_token($scope,$tokens,$line);
		if ($t ne ';') {
			warn "Missing ; at line $$line\n";
		}
	}
	
	
	sub  parse_decl_def($$$) {
		my ($scope,$tokens,$line) = @_;
		my $level=0;
		my @decl;
		while ( scalar(@$tokens) ) {
			my $t = get_token($scope,$tokens, $line);
			if ( (!defined ($t) || $t eq ';') and ($level==0)) {
				return @decl;
			}
	
			if ($t eq "static")
				{
				next;
				}
	
			push @decl, $t;
			if ($t eq '{') {
				++$level;
			}
			if ($t eq '}') {
				if ($level==0) {
					warn "Syntax error#13 at line $$line\n";
					unshift @$tokens, $t;
					return @decl;
					
				}
				if (--$level==0) {
					return ();	# end of function definition reached
				}
			}
		}
		die "Unexpected end of file #3 at line $$line\n";
	}
	
	sub dump_scope($) {
		my ($scope) = @_;
		my $el = $scope->{enums};
		my $cl = $scope->{classes};
		my $vl = $scope->{values};
		my $fstr = $scope->{formatStrings};
		my $fcat = $scope->{formatCategories};
		print "SCOPE: $scope->{name}\n";
		if (scalar(@$el)) {
			print "\tenums:\n";
			foreach (@$el) {
				print "\t\t$_\n";
			}
		}
		if (scalar(keys(%$vl))) {
			print "\tvalues:\n";
			foreach $vname (keys(%$vl)) {
				my $v = $vl->{$vname};
				my $x = $v->{value};
				my $t = $v->{type};
				my $sz = $v->{size};
				my $fstring = $fstr->{$vname};
				my $fcategory = $fcat->{$vname};
				if ($v->{enum}) {
					printf ("\t\t$vname\=$x (enum $t) size=$sz fcat=[0x%x] fstr=[%s]\n", $fcategory,$fstring);
				} else {
					printf ("\t\t$vname\=$x (type $t) size=$sz fcat=[0x%x] fstr=[%s]\n", $fcategory, $fstring);
				}
			}
		}
		if ($scope->{scope}) {
			my $members = $scope->{members};
			foreach (@$members) {
				my $n = $_->{name};
				my $sz = $_->{size};
				my $off = $_->{offset};
				my $spc = $_->{spacing};
				if (defined $spc) {
					print "\t$n\[\]\: spacing $spc size $sz offset $off\n";
				} else {
					print "\t$n\: size $sz offset $off\n";
				}
			}
			print "\tOverall size : $scope->{size}\n";
			print "\tOverall align: $scope->{align}\n";
		}
		foreach $s (@$cl) {
			dump_scope($s);
		}
	}
	
	
	
		
	sub parse_doxygen($$$$) {
		my ($scope,$tokens,$line,$t) = @_;
	
		if ($t ne "/")
			{
			return 0;	# not a doxygen comment
			}
		if ($t eq "/") {
			$state=0;
			my $t2 = shift @$tokens;
			my $t3 = shift @$tokens;
	
			if ($t2 ne "*" || $t3 ne "*")
				{
				unshift @$tokens, $t3;
				unshift @$tokens, $t2;
				return 0;	# not a doxygen comment
				}
		}
#		printf "doxygen start on line %d\n", $$line;
		for (;;) {
			my $t = shift @$tokens;
			if (!defined($t)) 
					{
					warn "Unexpected end of file #4 at line $$line\n";	
					return
					}
			
			if ($t eq "\n"){++$$line };
			
			if ($t eq '*')
				{
				my $t2 = shift @$tokens;
				last if ($t2 eq '/');
				unshift @$tokens, $t2;
				}
			
			if ($t eq '@')
				{
				my $t2 = shift @$tokens;
				if ($t2 eq 'SYMTraceFormatString')
					{
					my $t3 = shift @$tokens;
#					if ($VerboseMode){print "SYMTraceFormatString = [$t3]\n";}
					$CurrentTraceFormatString = $t3;
					}
				if ($t2 eq 'SYMTraceFormatCategory')
					{
					$CurrentTraceFormatCategory = get_operand($scope,$tokens,$line);
#					if ($VerboseMode){printf ("SYMTraceFormatCategory = 0x%x\n", $CurrentTraceFormatCategory);}
					}
				else
					{
					unshift @$tokens, $t2;
					}
				}
	
		}
#		printf ("doxygen end  on line %d\n", $$line);
		return 1;	# is a doxygen comment
	}
	

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
                
