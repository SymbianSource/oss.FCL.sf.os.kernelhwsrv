@rem
@rem Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

@perl -x romlaunch.bat %*
@goto end

#! usr\bin\perl

my $KFilename="rom.bat";

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
	
	if (defined($devicepath))
		{
		return $devicepath;
		}
	else
		{
		die "The '$KFilename' launcher cannot be used without the tools coresidency stubs.\n".
		  "Alternatively, please set EPOCROOT before calling '$KFilename' directly.\n";
		}
	}

# Main
	
use lib getDevicesPath();
use lib getDevicesPath()."/perllib";
use CDevicesCLAccessor;

my $devicepath=getDevicesPath();
$devicepath=~s/[^\/]+\/?$//; # Remove last path element
my $deviceObject = New CDevicesCLAccessor($devicepath."/devices.xml");

if (!defined($ENV{EPOCROOT}))
	{
	# Need to set EPOCROOT
	
	my $deviceName;

	if (defined($EHV{EPOCDEVICE}))
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
		  "the '$KFilename' launcher. Alternatively, set EPOCROOT and run\n".
		  "'$KFilename' directly\n";
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
		die "'$deviceName' is not a recognised device name.\n";
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

system($KFilename." ".join(" ",@args));

__END__
:end
