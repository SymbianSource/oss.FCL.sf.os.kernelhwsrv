#!/usr/bin/perl -w
# Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
# USBinterop2
# 
# AUTHOR: Steve Elliott
# DESCRIPTION: This is a simple utility to format a specified drive to the FAT format
# 
#
# @ 2008

use Getopt::Long;
use Pod::Usage;

my $drive = "";
my $help = 0;
my $logfile = "";

# Error codes
my $KErrNone = 0;			# No error - Test passed
my $KErrLogFile = -1;		# Can't open log file

my %opts = ( 'drive=s' 	=> \$drive,
	     'help!' 	=> \$help,
	     'logfile=s'=> \$logfile);

GetOptions(%opts) || pod2usage(2);
pod2usage(-exitval => 1, -verbose => 2) if $help;

# we're starting
LogPrint("FAT format of device ".$drive.":\n");

open (INPUTPIPE,"| format ".$drive.": /FS:FAT") or exit $KErrLogFile;

close (INPUTPIPE) or exit $KErrLogFile;

LogPrint("Format complete\n");

# we're done, exit
print "\n* Done\n";

exit $KErrNone;

sub LogPrint
	{
	if ($logfile ne "")
		{
		open LOGFILE, ">>$logfile" or exit $KErrLogFile;
		print LOGFILE @_;
		close LOGFILE;
		}
	print @_;
	}

######################################################################

__END__

=head1 NAME

formatdrive.pl - Formats a drive

=head1 SYNOPSIS

usage:   formatdrive.pl [options]

=head1 OPTIONS

=over 0

=item --drive=<drive location>

The path to the drive to format.

=item --help=<file>

Displays this help.

=back

=head1 DESCRIPTION

This is a simple utility to format a specified drive to the FAT format

=cut
