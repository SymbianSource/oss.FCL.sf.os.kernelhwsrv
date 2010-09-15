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

#
# A simple class to manage feature flags for a feature set data file.
#
package HCRrec;

my %typemap = (
    Int32 =>       0x00000001,
    Int16 =>       0x00000002,    
    Int8 =>        0x00000004,
    Bool =>        0x00000008,    
    UInt32 =>      0x00000010,
    UInt16 =>      0x00000020,    
    UInt8 =>       0x00000040,
    LinAddr =>     0x00000100,
    BinData =>     0x00010000,
    Text8 =>       0x00020000,    
	ArrayInt32 =>  0x00040000,
	ArrayUInt32 => 0x00080000,
    Int64 =>       0x01000000,
    UInt64 =>      0x02000000,    
);
my %maptype = reverse %typemap;
my %lsdtype2packmap = (
    0x00010000 => "C",
    0x00020000 => "a",    
    0x01000000 => "C",
    0x02000000 => "C",    
);

# Create a feature flag object.
sub new
{
	my $arg = shift;
	my $class = ref($arg) || $arg;

	my $self = {
			     cuid => 0,              # 4 bytes
			     eid => 0,               # 4 bytes
			     type => 0,              # 4 bytes
			     flagword => 0x0000,     # 2 bytes 
                 valueset => 0,
                           
			     intvalue => 0,           # 4 bytes
			     strvalue => "",          # array of chars
			     binvalue => [],          # array of bytes
			     arrvalue => [],		  # array of 4 byte integers
			     
   				 endian => "LE",
			   };
 
	bless $self, $class;
	return $self;
}

sub Endian
{
	my $self = shift;
	return undef unless(ref($self));
	my $arg = shift;
	return $self->{endian} unless(defined($arg) and $arg =~ m/(^BE$|^LE$)/i);
	$self->{endian} = lc($1);
	return $self->{endian};
}

# Return a twelve byte string 'feature flag' information.
sub GetRecHdrBinary
{
	my $self = shift;
	return undef unless(ref($self));
	
	my $lsd_size = shift;
	
	my $stype = $self->Type(); 
	my @hdrarr = ( $self->CUID(), $self->EID(), $stype, $self->Flags(),
                $self->SizeInBytes() );
    
	# Decide whether we want big or little endian output.
	# According to the documentation, 'V', 'N' are GUARANTEED to be 32-bit.
	my $packstring;
	if($self->Endian() eq "BE") {
	    $packstring = "N3n2N";
        }
    else {
        $packstring = "V3v2V"; # Little endian.
        }
        
    #
    # Could add range checks here for 8-bit and 16-bit types.
    # However would stop negative test cases from being generated.
    # Do it later.
    #
    
    if ($stype & 0xffff) {
        print "Writing integer\n" if ($mhd::otrace);
        push @hdrarr, $self->IntValue();
        }
    
    if ($stype & 0xffff0000) {
        if ($self->Length() > 0) {
            print "Writing offset: " . $lsd_size . "\n" if ($mhd::otrace);
            push @hdrarr, $lsd_size;
            }
        else {
            print "Writing null offset: 0\n" if ($mhd::otrace);
            push @hdrarr, 0;            
            }
        }

	my $hdr_string = pack $packstring, @hdrarr;
	
	return $hdr_string;
}

# Return a twelve byte string 'feature flag' information.
# Assumes Little Endian output!
sub GetRecLsdBinary
{
	my $self = shift;
	return undef unless(ref($self));
	
    my $value = "";
    my $valuelen = $self->Length();
    my $vallen = $valuelen;
    #print "vallen before:" . $vallen . "\n";
    $vallen = ($valuelen+3)&0xfffc if ($valuelen%4) ;
    #print "vallen after:" . $vallen . "\n";
	my $valtype = $self->{type};

    # String
    if ($valtype & 0x00020000) {
	    my $packstr = $lsdtype2packmap{$valtype} . $vallen;
	    printf ("packstr:%s\n", $packstr) if($mhd::otrace);
        printf ("strvalue:%s\n", $self->{strvalue}) if($mhd::otrace);
        $value = pack $packstr,  $self->{strvalue} ;
        }
    # Binary Data
    elsif ($valtype & 0x00010000) {
        for (my $c=0;  $c < $valuelen; $c++) {
            my $byte = $self->{binvalue}[$c];
            $value .= pack $lsdtype2packmap{$valtype}, $byte;
            $vallen--;     
        }
        while ($vallen > 0) {
            $value .= pack "C", ( 0x00 );
            $vallen--;
            }
    }
    # 64bit quantity
    elsif ($valtype & 0x03000000) {
        die "error: 64 bit integer missing hex binvalues\n" if (! exists $self->{binvalue}[7]);
        $value  = pack $lsdtype2packmap{$valtype}, $self->{binvalue}[0];
        $value  .= pack $lsdtype2packmap{$valtype}, $self->{binvalue}[1];
        $value  .= pack $lsdtype2packmap{$valtype}, $self->{binvalue}[2];
        $value  .= pack $lsdtype2packmap{$valtype}, $self->{binvalue}[3];
        $value  .= pack $lsdtype2packmap{$valtype}, $self->{binvalue}[4];
        $value  .= pack $lsdtype2packmap{$valtype}, $self->{binvalue}[5];
        $value  .= pack $lsdtype2packmap{$valtype}, $self->{binvalue}[6];
        $value  .= pack $lsdtype2packmap{$valtype}, $self->{binvalue}[7];
        }
    # array of 32bit quantity
    elsif ($valtype & 0x000C0000) {
        for (my $c=0;  $c < $valuelen; $c++) {
            my $int = $self->{arrvalue}[$c];
            $value .= pack "V", $int;
            $vallen--;     
            }
	}    
    else {
        die "panic: proramming error!!";
    }
    
	return $value;
	}

# A single 32-bit number.
sub CUID
{
	my $self = shift;
	return undef unless(ref($self));
	my $uid = shift;
	return $self->{cuid} unless(defined($uid));
	my $uidv = hex($uid);
	$self->{cuid} = $uidv;
	return $uidv;
}

# A single 32-bit number.
sub EID
{
	my $self = shift;
	return undef unless(ref($self));
	my $id = shift;
	return $self->{eid} unless(defined($id));
	my $idv = int($id);
	$self->{eid} = $idv;
	return $idv;
}

sub Type
{
	my $self = shift;
	return undef unless(ref($self));
	my $type = shift;
	return $self->{type} unless(defined($type));
	my $enum = $typemap{$type};
	#print "--->Defined\n" if (defined $enum);
	#print "--->NOT Defined\n" if (! defined $enum);
	die "error: unknown setting type found in input file\n" if (! defined $enum);
   	$self->{type} = $enum;
	return $enum;
}

sub TypeName
{
	my $self = shift;
	return undef unless(ref($self));
	return "Undefined Type" if (! exists $maptype{$self->{type}});
	return $maptype{$self->{type}};
}

sub Flags
{
	my $self = shift;
	return undef unless(ref($self));
	my $flags = shift;
	return $self->{flagword} unless(defined($flags));
	my $vf = hex($flags);
	$self->{flagword} = $vf;
	return $vf;
}

sub Length
{
	my $self = shift;
	return undef unless(ref($self));
	my $len = shift;
	die "panic: Length() does not take an argument!\n" if (defined($len));
	
	my $length = 0;
	if ($self->{type} & 0x00020000) {
        $length = length ($self->{strvalue});
        }
    elsif ($self->{type} & 0x03010000) {
	    my $array_ref = $self->{binvalue};
	    my @array = @$array_ref;
	    $length = $#array+1;
	    }
    elsif ($self->{type} & 0x000C0000) {
	    my $array_ref = $self->{arrvalue};
	    my @array = @$array_ref;
	    $length = $#array+1;
	    #printf ("arrval length %d %d\n",  length ($self->{arrval}), $length);
	    }
	else {
	    $length = 0;
        }
	return $length;	
}

sub SizeInBytes
{
	my $self = shift;
	return undef unless(ref($self));
	my $len = shift;
	die "panic: Length() does not take an argument!\n" if (defined($len));
	
	my $size = 0;
	if ($self->{type} & 0x00020000) {
        $size = length ($self->{strvalue});
        }
    elsif ($self->{type} & 0x03010000) {
	    my $array_ref = $self->{binvalue};
	    my @array = @$array_ref;
	    $size = $#array+1;
	    }
    elsif ($self->{type} & 0x000C0000) {
	    my $array_ref = $self->{arrvalue};
	    my @array = @$array_ref;
	    $size = ($#array+1)*4;
	    #printf ("arrval length %d %d\n",  length ($self->{arrval}), $length);
	    }
	else {
	    $size = 0;
        }
	return $size;	
}

sub IsValid
{
	my $self = shift;
	return undef unless(ref($self));

    if (($self->{cuid} == 0) || ($self->{eid} == 0) ||
        ($self->{type} == 0) || ($self->{flagword} != 0) ||
        ($self->IsValueSet() == 0)) {
        return 0;
        }    
    
    #Record valid if we reach here
    return 1;    
}

sub IsValueSet
{
	my $self = shift;
	return undef unless(ref($self));
	return $self->{valueset};
}

sub MarkValueSet
{
	my $self = shift;
	return undef unless(ref($self));
	$self->{valueset} = 1;
}

sub IntValue
{
	my $self = shift;
	return undef unless(ref($self));
	my $value = shift;
	if (defined($value)) {
        my $int = int($value);
        $self->{intvalue} = $int;
        $self->MarkValueSet();
        }
	return $self->{intvalue};
}

sub HexValue
{
	my $self = shift;
	return undef unless(ref($self));
	my $value = shift;
	return $self->{intvalue} unless(defined($value));
	my $int = hex($value);
	$self->{intvalue} = $int;
	$self->MarkValueSet();
    return $int;
}

sub StrValue
{
	my $self = shift;
	return undef unless(ref($self));
	my $value = shift;
	return $self->{strvalue} unless(defined($value));
	#printf ("strlen before %d\n", length ($self->{strvalue}));	
    $self->{strvalue} .= $value;
	#printf ("strlen after %d\n",  length ($self->{strvalue}));
	$self->MarkValueSet();
    return $value;
}

sub ArrValue
{
	my $self = shift;
	return undef unless(ref($self));
	my $value = shift;

	return $self->{arrvalue} unless(defined($value));

    my $int = int($value);
	my $index = $self->Length();

	$self->{arrvalue}[$index] = $int; # Increments the array size as well as appending item
	$index*=4; 

	printf ("warning: array value larger than HCR maximum (512 bytes): %d\n", $index) if ($index > 512);    
	$self->MarkValueSet();

    return $self->{arrvalue};
}

sub BinValue
{
	my $self = shift;
	return undef unless(ref($self));
	my $value = shift;
	
	return $self->{binvalue} unless(defined($value));

    my @hwords = split(/\s/,$value);
    shift @hwords if ($hwords[0] eq "");
    my $hwordslen = scalar(@hwords);  

    #printf("(len:%d)(0:%04x 1:%04x last:%04x)\n", $hwordslen, hex($hwords[0]), hex($hwords[1]), hex($hwords[$hwordslen-1])) if ($mhd::trace);
    
    my $index = $self->Length();
	#printf ("binlen before %d\n", $index);
         
    #print "Index: " . $index . "\n";
    foreach my $word (@hwords) {
        if (length ($word) == 2) {
	        $self->{binvalue}[$index] = hex($word);
            }
        else {
            die "error: hexadecimal value '$word' too short/large for 8-bit integer\n";
            }


	   #$self->{binvalue}[$index] = $mint;
	   #printf("%d: %04x\n", $count, $self->{binvalue}[$count]);
       $index++;	  
	   }
	  

	#printf ("binlen after %d\n", $index);
            
    printf ("warning: binary value larger than HCR maximum (512 bytes): %d\n", $index) if ($index > 512);
    $self->MarkValueSet();            
	return $self->{binvalue};
}


# ###########################################################################

1;

