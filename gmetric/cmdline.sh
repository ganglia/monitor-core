#!/usr/bin/gengetopt --input
# See http://www.gnu.org/software/gengetopt/gengetopt.html for details

package "gmetric"
purpose "The Ganglia Metric Client (gmetric) announces a metric
on the list of defined send channels defined in a configuration file"

option "conf" c "The configuration file to use for finding send channels" string default="@sysconfdir@/gmond.conf" no
option "name" n "Name of the metric" string no
option "value" v "Value of the metric" string no
option "type" t "Either string|int8|uint8|int16|uint16|int32|uint32|float|double" string no
option "units" u "Unit of measure for the value e.g. Kilobytes, Celcius" string default="" no
option "slope" s "Either zero|positive|negative|both|derivative" string default="both"  no
option "tmax" x "The maximum time in seconds between gmetric calls" int default="60" no
option "dmax" d "The lifetime in seconds of this metric" int default="0" no
option "group" g "Group of the metric" string no
option "cluster" C "Cluster of the metric" string no
option "desc" D "Description of the metric" string no
option "title" T "Title of the metric" string no
option "spoof" S "IP address and name of host/device (colon separated) we are spoofing" string default="" no
option "heartbeat" H "spoof a heartbeat message (use with spoof option)" no

#option "mcast_channel" c "Multicast channel to send/receive on" string default="239.2.11.71" no
#option "mcast_port" p "Multicast port to send/receive on" int default="8649" no
#option "mcast_if" i "Network interface to multicast on e.g. 'eth1'" string default="kernel decides" no
#option "mcast_ttl" l "Multicast Time-To-Live (TTL)" int default="1" no

#Usage (a little tutorial)
#
#   The  command  line  options,  which  have  to  be handled by gengetopt
#   generated  function,  are  specified  in  a  file (typically with .ggo
#   extension). This file consist in lines of sentences with the following
#   formats:
#
#package <packname>
#version <version>
#option <long> <short> <desc> <argtype> {default="<default value>"} <required>
#option <long> <short> <desc> flag      <onoff>
#option <long> <short> <desc> no
#
#   Where:
#
#   packname
#     Double quoted string.
#
#   version
#     Double  quoted  string.
#
#   purpose
#     What  the  program  does  (even  on more than one line), it will be
#     printed with the help. Double  quoted  string.
#
#   long
#     The  long  option,  a  double  quoted string with  upper and  lower
#     case   chars,   digits,  '-' and '.'.  No spaces allowed.  The name
#     of  the   variables  generated  to store arguments are long options
#     converted  to  be legal C variable names.  This means, '.'  and '-'
#     are   both  replaced  by  '_'. '_arg' is appended, or '_flag' for a
#     flag.
#
#   short
#     The  short  option,  a   single   upper  or  lower  case char, or a
#     digit.  If  a  '-' is specified, then no short option is considered
#     for  the  long  option  (thus long options with no associated short
#     options are allowed).
#
#   desc
#     Double   quoted  string  with  upper  and lower case chars, digits,
#     '-', '.' and spaces. First character must not be a space.
#
#   argtype
#     string, int, short, long, float, double, longdouble or longlong.
#
#   default
#     an optional default value for the option.  The value must always be
#     specified as a double quoted string.
#
#   required
#     yes or no.
#
#   onoff
#     on  or  off. This is the state of the flag when the program starts.
#     If user specifies the option, the flag toggles.
#
#   The  third  type  of  option is used when the option does not take any
#   argument. It must not be required.
#
#   Comments  begins with '#' in any place of the line and ends in the end
#   of line.
#
#   Here's an example of such a file (the file is called sample1.ggo)
#
#   # file sample1.ggo
#   option  "str-opt"     s "A string option"      string     no
#   option  "my-opt"      m "Another integer option"      int     no
#   option  "int-opt"     i "A int option"         int        yes
#   option  "flag-opt"    - "A flag option"        flag       off
#   option  "funct-opt"   F "A function option"    no
#   option  "long-opt"    - "A long option"        long       no
#   option  "def-opt"     - "A string option with default" string  default="Hello" no
