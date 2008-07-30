#!/usr/bin/perl -w
#
# Simple script to read ganglia's rrds and modify some of their configuration values.
#  - Uses the tune and resize commands.
#
# Written by:  Jason A. Smith <smithj4@bnl.gov>
#

# Modules to use:
use RRDs;  # Round Robin Database perl module (shared version) - from rrdtool package.
use RRDp;  # Round Robin Database perl module (piped version) - from rrdtool package.
use Cwd;
use Data::Dumper;
use File::Basename;
use Getopt::Long;
use English;
use strict;

# Get the process name from the script file:
my $process_name = basename $0;

# Define some useful variables:
my $rrd_dir = '/var/lib/ganglia/rrds';
my $rrdtool_path = '/usr/bin/rrdtool';
my $heartbeat = 0;
my $grow = 0;
my $shrink = 0;
my $dump = 0;
my $restore = 0;
my $verbose = 0;
my $debug = 0;
my $num_files = 0;

# Get the command line options:
&print_usage if $#ARGV < 0;
$Getopt::Long::ignorecase = 0;  # Need this because I have two short options, same letter, different case.
GetOptions('r|rrds=s'      => \$rrd_dir,
           'H|heartbeat=i' => \$heartbeat,
	   'g|grow=i'      => \$grow,
	   's|shrink=i'    => \$shrink,
	   'D|dump'        => \$dump,
	   'R|restore'     => \$restore,
	   'v|verbose'     => \$verbose,
           'd|debug'       => \$debug,
           'h|help'        => \&print_usage,
          ) or &print_usage;

# Recursively loop over ganglia's rrd directory, reading all directory and rrd files:
my $start = time;
chdir("/tmp");  # Let the rrdtool child process work in /tmp.
my $pid = RRDp::start $rrdtool_path;
# Catch and report errors instead of aborting (not present in older versions of rrdtool):
$RRDp::error_mode = 'catch' if defined $RRDp::error_mode;
chdir("$rrd_dir") or die "$process_name: Error: Directory doesn't exist: $rrd_dir";
&process_dir($rrd_dir);
my $status = RRDp::end;
my $time = time - $start;  $time = 1 if $time == 0;
my ($usertime, $systemtime, $realtime) =  ($RRDp::user, $RRDp::sys, $RRDp::real);

# Print final stats:
warn sprintf "\n$process_name: Processed %d rrd files in %d seconds (%.1f f/s)\n", $num_files, $time, $num_files/$time;
warn sprintf "  - RRDp(status=%s): user=%.3f, sys=%.3f, real=%.3f (%.1f%% cpu)\n\n", $status, $usertime, $systemtime, $realtime, 100*($usertime+$systemtime)/$realtime if $realtime;

# Exit:
exit $status;

# Function to read all directory entries, testing them for files & directories and processing them accordingly:
sub process_dir {
  my ($dir) = @_;
  
  my $cwd = getcwd;
  warn "$process_name: Reading directory: $cwd ....\n";
  foreach my $entry (glob("*")) {
    if (-d $entry) {
      chdir("$entry");
      &process_dir($entry);
      chdir("..");
    } elsif (-f $entry) {
      &process_file("$cwd/$entry");
    }
  }
}

# Function to process a given file:
sub process_file {
  my ($file) = @_;
  my $error;
  
  # Who owns the file (if resizing the file then I have to move the file & change the ownership back):
  my ($mode, $uid, $gid) = (stat($file))[2,4,5];
  
  # Dump the binary rrd file to xml if asked to:
  if ($dump and $file =~ /\.rrd$/) {
    my($filename, $directory, $suffix) = fileparse($file, '.rrd');
    my $xml = $directory . $filename . '.xml';
    warn "$process_name: Dumping binary rrd file: $file to xml file: $xml\n" if $verbose;
    if ($RRDs::VERSION >= 1.2013) {
      RRDs::dump($file, $xml);
      $error = RRDs::error;
    } else {
      # In old versions of rrdtool, dump would write directly to stdout:
      #  - can't use the preferred RRDs, will have to use RRDp instead.
      my $cmd = "dump \"$file\"";
      RRDp::cmd($cmd);
      # Old versions of rrdtool also abort when the RRDp command fails!
      #  - can catch the abort with eval but then the next command will fail with a broken pipe unless RRDp is restarted.
      my $answer = RRDp::read;  # Returns a pointer (scalar reference) containing the output of the command.
      # In new versions of rrdtool (not sure which version this was introduced though), errors can be caught:
      if ($RRDp::error) {
	$error = $RRDp::error;
      } elsif ($answer) {
	# If it succeeds then it returns a reference to the actual XML dump:
	if ($$answer =~ /^<!-- Round Robin Database Dump -->/) {
	  # Save it to the xml file manually:
	  open(XML, ">$xml") or warn "ERROR opening xml file: $xml - $!";
	  my $ret = print XML $$answer;
	  $error = "Error writing to xml file: $xml - $!" if not $ret;
	  close XML;
	} else {
	  $error = "Unexpected result: $$answer";
	}
      } else {
	$error = 'rrdtool dump produced no output.';
      }
    }
    my ($size) = (stat($xml))[7];
    if ($error or not $size) {
      warn "ERROR while trying to dump $file to $xml - $error";
    } else {
      chown $uid, $gid, $xml if $uid or $gid;  # It is assumed that a sysadmin is running this script as root.
      chmod $mode, $xml;
    }
  }
  
  # Restore the binary rrd file from an xml dump if asked to:
  if ($restore and $file =~ /\.xml$/) {
    my($filename, $directory, $suffix) = fileparse($file, '.xml');
    my $rrd = $directory . $filename . '.rrd';
    warn "$process_name: Restoring binary rrd file: $rrd from xml file: $file\n" if $verbose;
    RRDs::restore($file, $rrd);
    $error = RRDs::error;
    if ($error) {
      warn "ERROR while trying to restore $rrd from $file - $error";
    } else {
      chown $uid, $gid, $rrd if $uid or $gid;  # It is assumed that a sysadmin is running this script as root.
      chmod $mode, $rrd;
    }
    # For below, just in case ($file is assumed to be the rrd file):
    $file = $rrd;
  }
  
  # The rest of this is only for rrd files:
  if ($file =~ /\.rrd$/) {
    
    # Read the rrd header and other useful information:
    warn "$process_name: Reading rrd file: $file\n" if $debug;
    my $info = RRDs::info($file);
    $error = RRDs::error;
    warn "ERROR while reading $file: $error" if $error;
    print "$file: ", Data::Dumper->Dump([$info], ['Info']) if $debug;
    my $num_rra = 0;  # Maximum index number of the RRAs.
    foreach my $key (keys %$info) {
      if ($key =~ /rra\[(\d+)\]/) {
	$num_rra = $1 if $1 > $num_rra;
      }
    }
    my ($start, $step, $names, $data) = RRDs::fetch($file, 'AVERAGE');
    $error = RRDs::error;
    warn "ERROR while reading $file: $error" if $error;
    if ($debug) {
      print "Start:       ", scalar localtime($start), " ($start)\n";
      print "Step size:   $step seconds\n";
      print "DS names:    ", join (", ", @$names)."\n";
      print "Data points: ", $#$data + 1, "\n";
    }
    
    # Set the heartbeat if asked to:
    if ($heartbeat) {
      foreach my $n (@$names) {
	warn "$process_name: Updating heartbeat for DS: $file:$n to $heartbeat\n" if $verbose;
	RRDs::tune($file, "--heartbeat=$n:$heartbeat");
	$error = RRDs::error;
	warn "ERROR while trying to tune $file:$n - $error" if $error;
      }
    }
    
    # Resize the rrds if asked to (have to use the pipe module - resize doesn't exist in the shared version):
    if ($grow or $shrink) {
      my $action = $grow ? 'GROW' : 'SHRINK';
      my $amount = $grow ? $grow : $shrink;
      foreach my $n (0..$num_rra) {
	my $cmd = "resize \"$file\" $n $action $amount";
	warn sprintf "$process_name: %sING: %s:RRA[%d] by %d rows.\n", $action, $file, $n, $amount if $verbose;
	RRDp::cmd($cmd);
	my $answer = RRDp::read;  # Returns nothing.
	warn "$process_name: Renaming: /tmp/resize.rrd --> $file\n" if $verbose;
	# Note: don't use perl's rename function because it doesn't work across filesystems!
	system("mv /tmp/resize.rrd $file");
	chown $uid, $gid, $file if $uid or $gid;
      }
    }
  }
  $num_files++;
}

# Print usage function:
sub print_usage {
  print STDERR <<EndOfUsage;

Usage: $process_name [-Options]

 Options:
  -r|--rrds dir		Location of rrds to read (Default: $rrd_dir).
  -H|--heartbeat #	Set heartbeat interval to # (Default: unchanged).
  -g|--grow #		Add # rows to all RRAs in rrds (Default: unchanged).
  -s|--shrink #		Remove # rows from all RRAs in rrds (Default: unchanged).
  -D|--dump		Dump the binary rrd files to xml.
  -R|--restore		Restore the binary rrd files from an earlier xml dump.
  -v|--verbose		Enable verbose mode (explicitly print all actions).
  -d|--debug		Enable debug mode (more detailed messages written).
  -h|--help		Print this help message.

Simple script to read ganglia's rrds and modify some of their configuration
values.  It can also dump & restore rrd files, useful when moving ganglia
to a new architecture.

EndOfUsage
  
  exit 0;
}

#
# End file.
#
