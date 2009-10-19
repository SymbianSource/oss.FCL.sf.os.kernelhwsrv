#! usr\bin\perl
# Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

my $KFilename = $0;
$KFilename =~ s/\\romlaunch\.pl$/\\rom.pl/i;
my $KEnvironment="perl";
my $KLauncher="rom.bat";

# Main
	
if (!defined($ENV{EPOCROOT}))
	{
	# Need to set EPOCROOT
	
	my $devicepath=getDevicesPath();

	if (!defined($devicepath))
		{
		die "Please set EPOCROOT before launching $KLauncher. Alternatively, install\n".
		  "a the coresidency tools (tools stubs) and set a default device.\n";
		}

	push @INC, $devicepath;
	push @INC, $devicepath."/perllib";
	require CDevicesCLAccessor;

	$devicepath=~s/[^\/]+\/?$//; # Remove last path element
	my $deviceObject = New CDevicesCLAccessor($devicepath."/devices.xml");

	my $deviceName;

	if (defined($ENV{EPOCDEVICE}))
		{
		# Use EPOCDEVICE as default device
		$deviceName = $ENV{EPOCDEVICE};
		}
	elsif (($deviceObject->getDefaultDevice()) ne "")
		{
		# Use main default device
		$deviceName = $deviceObject->getDefaultDevice($deviceObject);
		}
	else
		{
		die "Please set a default device (using 'devices -setdefault') before using\n".
		  "$KLauncher. Alternatively, you can set EPOCROOT yourself.\n";
		}
	
	if ( ($deviceObject->isValidName($deviceName))
	  || ($deviceObject->isValidAlias($deviceName))
	  )
		{
		# Get path to the epoc32 tree from device
		my $epocroot = $deviceObject->getEpocRoot($deviceName);

		$epocroot =~ s/^[A-Za-z]://; # Remove leading drive letter

		# Ensure the correct slashes are present
		$epocroot =~ s/\//\\/g;
		if ($epocroot !~ /\\$/)
			{
			$epocroot = $epocroot."\\";
			}
		
		# Set EPOCROOT
		$ENV{EPOCROOT} = $epocroot;
		}
	else
		{
		die "'$deviceName' is not a recognised device name. If EPOCDEVICE is set, ensure\n".
		  "it is set to a valid device name.\n";
		}
	}

# Enclose arguments in quote marks if needed

my @args=@ARGV;
my $index=scalar(@args);

while ($index > 0)
	{
	$index--;

	if ($args[$index] =~ /\s/)
		{
		$args[$index] = '"'.$args[$index].'"';
		}
	}

# Call tool with arguments

system($KEnvironment." ".$KFilename." ".join(" ",@args));

# getDevicesPath()
#
# Discover the location of the devices API. They are expected to be found in an installed set of coresidency
# stubs, in the path.
#
# Parameters: None
#
# Returns: The path to the devices API (UNIX style path)
# 
# Dies: If no devices API can be found in the path. 
sub getDevicesPath()
	{
	my $devicepath = undef;
	my $paths = $ENV{PATH};
	$paths =~ s/\\/\//g;
	
	foreach my $path (split(";", $paths))
		{
		if (-e "$path/CDevicesCLAccessor.pm")
			{
			$devicepath=$path;
			}
		}
	
	return $devicepath;
	}
