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
#
use strict;

#use Math::BigInt;

#
# Perl module to create and maintain feature manager data files.
# You can either set up the information programmatically or else load up
# information from a pre-existing feature data file and then modify it. You
# can also save the information to a file (in feature manager dataset format).
#
# This class maintains header information plus two arrays, one containing
# feature flag information and the other containing default supported range
# information. Those are themselves objects and have their own accessor
# methods.
#

package HCRdat;

use HCRrec;


#
# n e w
#
# Create a new HCRdat object. For example 'my $hd = HCRdat->new("filea");
#
sub new
{
	my $arg = shift;
	my $fn = shift;
	my $class = ref($arg) || $arg;
	my $self = {
			        fingerprint => "HCRf",  # 4 bytes wide.
					fileversion => 1,       # 2 bytes.
					fileflags => 0x0001,  # 2 bytes.
					numrecords => 0,      # 4 bytes. 
					lsdoffset => 0,       # 4 bytes. 
					lsdsize => 0,         # 4 bytes.
				    packprefix => "V",    # Changed with endian-ness.
					                      # Used to create binary strings.

					settingrecords => [],  # Array of objects
					lsd => [],             # Array of bytes
	           };
    bless $self, $class;
	return $self;
}


# Print to STDOUT the header information we have.
sub ShowHeader
{
	my $self = shift;
	return undef unless(ref($self));

	# Get header information..
	my $typefield = $self->TypeField();
	my $fileversion = $self->FileVersion();
	my $fileflags = $self->FileFlags();
	my $numrecords = $self->NumRecords();
	my $lsdoffset = $self->LsdOffset();
	my $lsdsize = $self->LsdSize();
	
	# Display it in English.
	print "  FINGERPRINTF: '$typefield'\n";
	print "  FILEVERSION: '$fileversion'\n";
	print "  FILEFLAGS: '$fileflags'\n";
	print "  NUMRECORDS: '$numrecords'\n";
	print "  LSDOFFSET: '$lsdoffset'\n";
    print "  LSDSIZE: '$lsdsize'\n";

	return(0);
}

# Get/Set the endian-ness we want. Changes the 'packprefix' member which is
# used in the creation of binary data.
sub Endian
{
	my $self = shift;
	return undef unless(ref($self));
	my $arg = shift;
	return $self->{endian} unless(defined($arg));
	if($arg =~ m/(LE|BE)/i)
	{
		my $endian = uc($1);
		$self->{endian} = $endian;
		# Used by 'pack' to generate binary strings.
		$self->{packprefix} = "V" if($endian eq "LE");
		$self->{packprefix} = "N" if($endian eq "BE");
	}
	return $self->{endian};
}

# This is the fingerprint.
sub TypeField
{
	my $self = shift;
	return undef unless(ref($self));
	my $arg = shift;
	$self->{fingerprint} = $arg if(defined($arg));
	return $self->{fingerprint};
}

sub FileVersion
{
	my $self = shift;
	return undef unless(ref($self));
	my $arg = shift;
	# Should we be testing for a numeric value?
	$self->{fileversion} = $arg if(defined($arg));
	return $self->{fileversion};
}

sub FileFlags
{
	my $self = shift;
	return undef unless(ref($self));
	my $arg = shift;
	$self->{fileflags} = $arg if(defined($arg));
	return $self->{fileflags};
}

# How many feature flag objects have we got?
sub NumRecords
{
	my $self = shift;
	return undef unless(ref($self));
	my $arg = shift;
	$self->{numrecords} += $arg if(defined($arg));
	return $self->{numrecords};
}


sub LsdOffset
{
	my $self = shift;
	return undef unless(ref($self));
	my $arg = shift;
	$self->{lsdoffset} = $arg if(defined($arg));
	return $self->{lsdoffset};
}

sub LsdSize
{
	my $self = shift;
	return undef unless(ref($self));
	my $arg = shift;
	$self->{lsdsize} = $arg if(defined($arg));
	return $self->{lsdsize};
}

# Create a binary string containing the header information for the
# feature manager data file based on the various fields in this object.
sub CreateBinaryHeader
{
	my $self = shift;
	return undef unless(ref($self));
	my $hdrstring;

	# Get the letter for packing information with 'pack' into a binary form.
	my $pack16 = lc($self->{packprefix});
	my $pack32 = uc($self->{packprefix});
	
	# Get header information..
	my $typefield = $self->TypeField();
	my $fileversion = $self->FileVersion();
	my $fileflags = $self->FileFlags();
	my $numrecords = $self->NumRecords();
	my $lsdoffset = $self->LsdOffset();
	my $lsdsize = $self->LsdSize();

	# Write the 'type' field out. This is 'feat'. Would this be different on
	# big-endian systems?
	$hdrstring = $typefield;

	# Now the file version number. A 16-bit value.. Will this cause trouble
	# if the shifted value is signed?
	$hdrstring .= pack($pack16 . "1", $fileversion);

	# Now the file flags. Another 16-bit value..
	$hdrstring .= pack($pack16 . "1", $fileflags);

	# Now the number of listed features - a 32-bit value.
	$hdrstring .= pack($pack32 . "1", $numrecords);

	# Now the number of listed features - a 32-bit value.
	$hdrstring .= pack($pack32 . "1", $lsdoffset);

	# Now the number of listed features - a 32-bit value.
	$hdrstring .= pack($pack32 . "1", $lsdsize);

	# Now the 3 reserved words
	$hdrstring .= pack($pack32 . "3", (0, 0, 0));

	return $hdrstring;
}

sub CreateImageHdr
{
	my $self = shift;
	return undef unless(ref($self));
	#my $partid = shift;
	#return -1 unless(defined($partid));

	# Add fingerprint, 1st reserved word and format version
	my $imghdr = pack "V4", (0x5F524348, 0x54524150, 0x00000000, 0x00000001);
	# Add space for image size, timestamp, 2nd reserved word
	$imghdr .= pack "V3", (0x00000000, time, 0x00000000);
	# Add space for payload checksum, HCR Payload constants: UID and 0x0 flags
    $imghdr .= pack "V3", (0x00000000, 0x10286AB8, 0x00000000);
    #Reserved space
    $imghdr .= pack "x216", (0x00000000);
 
    return $imghdr;
}

sub WriteToImage
{
	my $self = shift;
	return undef unless(ref($self));
	my $imgfile = shift;
	return -1 unless(defined($imgfile));
	my $datfile = shift;
	return -1 unless(defined($datfile));
	#my $partid = shift;
	#return -1 unless(defined($partid));
	my $rc = 0;
	
    open IMGFILE, "> $imgfile" or die "Couldn't open file '$imgfile' for writing.\n";
	binmode IMGFILE;
   
 	syswrite(IMGFILE, $self->CreateImageHdr(), 256);	
 	
    open DATFILE, "$datfile" or die "Couldn't open file '$datfile' for reading.\n";
	binmode DATFILE;
	# print FILE $self->BinaryContent();
	
    #my $wordsum = 0x1200000000;
    #my $wordsum = Math::BigInt->new("0x0220100123");
    #printf("test: %x\n", $wordsum->external();

	my $imgsize = 256;
	my $word;
	printf("-reading image:\n")  if ($mhd::otrace);
	while (sysread (DATFILE, $word, 4)) {
	    #printf ("%08x ",$word)  if ($mhd::otrace);
        my $iword = unpack("V" , $word);
	    printf ("%08x ",$iword)  if ($mhd::otrace);
        $rc = syswrite (IMGFILE, $word, 4);
        die "error: ($rc) failed to write datfile word into imgfile.\n" if ($rc != 4);
	    #$wordsum->badd($iword);
        $imgsize += 4;
	    print "\n" if (($mhd::otrace) && ($imgsize%16==0));
        }
    print "\n" if ($mhd::otrace);
    # ordsum: 0x". $wordsum ."\n" if ($mhd::otrace);
	my $checksum = 0x12345678;
	close DATFILE;
	
	printf("-image size: %d, checksum: 0x%08x", $imgsize, $checksum) if ($mhd::otrace);
	
	$rc = sysseek(IMGFILE, 16, 0);
	die "error: ($rc) failed to seek in image to write header.\n" if ($rc != 16);

	# Write out the image size	
	my $imginfo1 = pack "V1", ($imgsize);
	$rc = syswrite(IMGFILE, $imginfo1, 4);
	die "error: ($rc) failed to write image size/checksum to image header.\n" if ($rc != 4);

	$rc = sysseek(IMGFILE, 28, 0);
	die "error: ($rc) failed to seek in image to write header.\n" if ($rc != 28);
	
	# Write out the image checksum 
	my $imginfo2 = pack "V1", ($checksum);
	$rc = syswrite(IMGFILE, $imginfo2, 4);
	die "error: ($rc) failed to write image size/checksum to image header.\n" if ($rc != 4);
  
	close IMGFILE; 	
	
    return 0;
}

# Writes the binary file specified as an argument with the content of this
# and contained feature flag and dsr objects.
sub WriteToFile
{
	my $self = shift;
	return undef unless(ref($self));
	my $file = shift;
	return undef unless(defined($file));
    open FILE, "> $file" or die "Couldn't open file '$file' for writing.\n";
	binmode FILE;
	print FILE $self->BinaryContent();
	
	close FILE;
	return 0;
}


# Create the binary equivalent of the internal data and return it as a
# string.
sub BinaryContent
{
	my $self = shift;
	return undef unless(ref($self));

    # Get the feature flag entries.. This is an array reference.
	# For each one append the binary representation of the information
	# contained.
	my $records = "";
    my $lsd = "";
   	my $ffs_ref = $self->SettingRecords();
	my $ff;

    my $count = 0;
	foreach $ff (@$ffs_ref)
	{
	    $count++;
	    printf("-encoding record: %04d (0x%08x:%04d)\n", $count, $ff->CUID(), $ff->EID());
		$records .= $ff->GetRecHdrBinary(length ($lsd));
	    my $stype = $ff->Type();
    	if (($stype & 0xffff0000) && ($ff->Length() > 0)) {
		    $lsd .= $ff->GetRecLsdBinary();
            }
	}

    $self->LsdOffset(32+length ($records));     # header size 32
    $self->LsdSize(length ($lsd));
    
	my $header = $self->CreateBinaryHeader();

	return $header . $records . $lsd;
}

# Return a reference to the 'feature flags' array.
sub SettingRecords
{
	my $self = shift;
	return undef unless(ref($self));
	return $self->{settingrecords};
}

# Add a Feature Flag object. Perhaps there should be code to check if we
# already know about this feature flag. (i.e check the uid against the ones
# we have).
sub AddSettingRecord
{
	my $self = shift;
	return undef unless(ref($self));
	my $arg = shift;
	die "panic: method 'AddSettingRecord' requires a 'HCRrec' object as argument.\n"
   	    unless(ref($arg) eq  "HCRrec");
   	
	push @{$self->SettingRecords()}, $arg;
	$self->NumRecords(1);
	
    return 0;
}


1;

