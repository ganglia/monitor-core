#!/usr/bin/perl -w
#
# matapicos v2.2 - Vins Vilaplana <vins at terra dot es)
#
# Translated by Humberto Rossetti Baptista <humberto at baptista dot name)
# slight adjustments and code cleanup too :-)
#
# Changes:
#  - 2007/02/27 - knobi@knobisoft.de - Various changes:
#                    Add value-based chopping (-t value)
#                    Add analysis only mode (-a)
#                    Controll verbose/debug output using -d and -v
#                    Add -h help option
#                    Move to using the Getopt::Std package
#                    Use "strict" mode
#  - 2006/01/12 - vins@terra.es - "$!" takes other values in some perl interpreters (e.g. FreeBSD 4.11-R). Thanks to Atle Veka!
#

use strict;
use Getopt::Std;
my %opt=();
getopts("adhl:t:v",\%opt);

my (@dump,%exp,@cols,@dbak,%tot,%por);
my ($linea,$linbak,$lino,$cdo,$tresto,$tstamp,$a,$b,$c,$cont);
my $DEBUG = 0;
my $ANALYZE = 0;
my $VERBOSE = 0;

# Limit % for cutting. Any peak representing less than this % will be cut
my $LIMIT=0.6; # obs this is really %, so 0.6 means 0.6% (and not 0.006%!)

# Threshhold for cutting. Values above it will be chopped if "-t" is used
my $THRESH=1.01e300; # Just set it to a very high default

# Flag to indicate whether we are doing "binning" or threshold based chopping
my $BINNING=1;

if ($opt{h} || ($#ARGV < 0)) {
   print "REMOVESPIKES: Remove spikes from RRDtool databases.\n\n";
   print "Usage:\n";
   print "$0 -d -a [-l number] [-t maxval] name_of_database\n\n";
   print "Where:\n";
   print "  -d enables debug messages\n";
   print "  -a runs only the analysis phase of the script\n";
   print "  -h prints this message\n";
   print "  -l sets the % limit of spikes bin-based chopping (default: $LIMIT)\n";
   print "  -t sets the value above which records are chopped. Disabled by default.\n";
   print "     Enabling value-based chopping will disable bin-based chopping\n\n";
   print "  -v Verbose mode. Shows some information\n";
   print "  name_of_database is the rrd file to be treated.\n";
   exit;
}

if ($opt{d}) { 
   $DEBUG = 1;
   $VERBOSE = 1; 
   print "Enabling DEBUG mode\n";
}

if ($opt{a}) { 
   $ANALYZE = 1; 
   print "Running in ANALYZE mode\n";
}

if ($opt{v}) { 
   $VERBOSE = 1; 
   print "Running in VERBOSE mode\n";
}

if ($opt{l}) { 
   $LIMIT=$opt{l}; 
   print "Limit for bin-based chopping set to $LIMIT\n" if $VERBOSE;
}

if ($opt{t}) { 
   $THRESH=$opt{t}; 
   $BINNING=0;
   printf("Max Value set to %g, disabling bin-based chopping\n",$THRESH) if $VERBOSE;
}

# temporary filename:
# safer this way, so many users can run this script simultaneusly
my $tempfile="/tmp/matapicos.dump.$$"; 

###########################################################################
# Dump the rrd database to the temporary file (as XML)
system("rrdtool dump $ARGV[0] > $tempfile") == 0 or die "\n";

# Scan the XML dump checking the variations and exponent deviations
open(FICH,"<$tempfile") 
   || die "$0: Cannot open file $tempfile:\n $! - $@";

while (<FICH>) {
  chomp;
  $linea=$_;
  $cdo=0;
  if ($linea=~/^(.*)<row>/) { $tstamp=$1; }
  if ($linea=~/(<row>.*)$/) { $tresto=$1; }
  if (/<v>\s\d\.\d+e.(\d+)\s<\/v>/) {
    @dump = split(/<\/v>/, $tresto);
    for ($lino=0; $lino<=$#dump-1; $lino++) {   # scans DS's within each row 
      if ( $dump[$lino]=~/\d\.\d+e.(\d+)\s/ ) { # make sure it is a number (and not NaN)
        $a=substr("0$lino",-2).":".$1;
        $exp{$a}++;                             # store exponents
        $tot{substr("0$lino",-2)}++;            # and keep a per DS total
      }
    }
  }
}

close FICH;

###########################################################################
# Scan the hash to get the percentage variation of each value
foreach $lino (sort keys %exp) {
  ($a)=$lino=~/^(\d+)\:/;                      
  $por{$lino}=(100*$exp{$lino})/$tot{$a};
}

if ($DEBUG || $ANALYZE) { 
   # Dumps percentages for debugging purposes
   print "--percentages--\n";
   foreach $lino (sort keys %exp) {
     print $lino."--".$exp{$lino}."/";
     ($a)=$lino=~/^(\d+)\:/;
     print $tot{$a}." = ".$por{$lino}."%\n";
   }
   print "---------------\n\n";
   exit if $ANALYZE;
}


###########################################################################
# Open the XML dump, and create a new one removing the spikes:
open(FICH,"<$tempfile") || 
   die "$0: Cannot open $tempfile for reading: $!-$@";
open(FSAL,">$tempfile.xml")  || 
   die "$0: Cannot open $tempfile.xml for writing: $!-$@";

$linbak='';
$cont=0;
while (<FICH>) {
  chomp;
  $linea=$_;
  $cdo=0;
  if ($linea=~/^(.*)<row>/) { $tstamp=$1; }     # Grab timestamp
  if ($linea=~/(<row>.*)$/) { $tresto=$1; }     # grab rest-of-line :-)
  if (/<v>\s\d\.\d+e.(\d+)\s<\/v>/) {           # are there DS's?
    @dump=split(/<\/v>/, $tresto);              # split them
    if ($linbak ne '') {
      for ($lino=0;$lino<=$#dump-1;$lino++) {   # for each DS:
        if ($dump[$lino]=~/\d\.\d+e.(\d+)\s/) { # grab number (and not a NaN)
	  $c=$&;
          $a=$1*1;                              # and exponent
          $b=substr("0$lino",-2).":$1";         # calculate the max percentage of this DS
          if (($BINNING &&                      #
		($por{$b}< $LIMIT)) ||          # if this line represents less than $LIMIT
	      (!$BINNING &&			#
		($c > $THRESH))) {              # or the value is larger then $THRESH
            $linea=$tstamp.$linbak;             # we dump it
            $cdo=1;
            $tresto=$linbak;
          }
        }
      }
    }
    $linbak=$tresto;
    if ($cdo==1) { 
      print "Chopping peak at $tstamp\n" if $DEBUG;
      $cont++; }
  }
  
  print FSAL "$linea\n";
}
close FICH;
close FSAL;

###########################################################################
# Cleanup and move new file to the place of original one
# and original one gets backed up.
if ($cont == 0 && $VERBOSE) { print "No peaks found.!\n"; }
else {
  rename($ARGV[0],"$ARGV[0].old");
  $lino="rrdtool restore $tempfile.xml $ARGV[0]";
  system($lino);
  die "$0: Unable to execute the rrdtool restore on $ARGV[0] - $! - $@\n" if $? != 0;
}

# cleans up the files created
unlink("$tempfile");
unlink("$tempfile.xml");

