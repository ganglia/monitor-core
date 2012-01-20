#!/usr/bin/gengetopt --input
# See http://www.gnu.org/software/gengetopt/gengetopt.html for details

package "gmetad"
purpose "The Ganglia Meta Daemon (gmetad) collects information from
multiple gmond or gmetad data sources, saves the information to local
round-robin databases, and exports XML which is the concatentation of
all data sources"

option "conf" c "Location of gmetad configuration file" string default="@sysconfdir@/gmetad.conf" no
option "debug" d "Debug level. If greater than zero, daemon will stay in foreground." int default="0" no
option "pid-file" p "Write process-id to file" string no

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
