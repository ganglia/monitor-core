#
# Copyright (C) 2010 Bernard Li <bernard@vanhpc.org>
#                    All Rights Reserved.
#
# This code is part of a perl module for ganglia.
#
# Contributors : Nicolas Brousse <nicolas@brousse.info>
#

use strict;
our @descriptors;
our $Random_Max = 50;
our $Constant_Value = 50;
use vars qw(@descriptors $Random_Max $Constant_Value);

sub Random_Numbers {
    my $i = int(rand($Random_Max));
    printf("Value from Random_Numbers : %d\n", $i);
    return $i;
}

sub Constant_Number {
    my $i = int($Constant_Value);
    printf("Value from Constant_Numbers : %d\n", $i);
    return $i;
}

sub metric_init {
    my $params = shift;

    if (exists($params->{'RandomMax'})) {
        $Random_Max = int($params->{'RandomMax'});
    }

    if (exists($params->{'ConstantValue'})) {
        $Constant_Value = int($params->{'ConstantValue'});
    }

    my %d1 = (
        'name' => 'PlRandom_Numbers',
        'call_back' => 'Random_Numbers',
        'time_max' => 90,
        'value_type' => 'uint',
        'units' => 'N',
        'slope' => 'both',
        'format' => '%u',
        'description' => 'Example module metric (random numbers)',
        'groups' => 'example,random'
    );
    my %d2 = (
        'name' => 'PlConstant_Number',
        'call_back' => 'Constant_Number',
        'time_max' => 90,
        'value_type' => 'uint',
        'units' => 'N',
        'slope' => 'zero',
        'format' => '%u',
        'description' => 'Example module metric (constant number)',
        'groups' => 'example'
    );

    push @descriptors, { %d1 };
    push @descriptors, { %d2 };
    return @descriptors;
}

sub metric_cleanup {
}

if ($ENV{_} !~ /gmond/) {
    my $params = {'RandomMax' => 500, 'ConstantValue' => 322};

    metric_init($params);
    foreach (@descriptors) {
        my $v = eval($_->{'call_back'});
        printf("value for %s is %u\n", $_->{'name'}, $v);
    }   
}
