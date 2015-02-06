###  This script reports process metrics to ganglia.
###
###  Notes:
###    This script exposes values for CPU and memory utilization
###    for running processes. You can retrieve the process ID from
###    either providing a pidfile or an awk regular expression.
###    Using a pidfile is the most efficient and direct method.
###
###    When using a regular expression, keep in mind that there is
###    a chance for a false positive. This script will help to avoid
###    these by only returning parent processes. This means that
###    the results are limited to processes where ppid = 1.
###
###    This script also comes with the ability to test your regular
###    expressions via command line arguments "-t".
###
###  Testing:
###   -- This is a correct examples of how to monitor apache.
###
###      $ python procstat.py -p httpd -v '/var/run/httpd.pid' -t
###       Testing httpd: /var/run/httpd.pid
###       Processes in this group:
###       PID, ARGS
###       11058 /usr/sbin/httpd
###       8817 /usr/sbin/httpd
###       9000 /usr/sbin/httpd
###       9001 /usr/sbin/httpd
###
###       waiting 2 seconds
###       procstat_httpd_mem: 202076 KB [The total memory utilization]
###       procstat_httpd_cpu: 0.3 percent [The total percent CPU utilization]
###
###   -- This example shows a regex that returns no processes with a
###      ppid of 1.
###
###      $ python procstat.py -p test -v 'wrong' -t
###       Testing test: wrong
###       failed getting pgid: no process returned
###      ps -Ao pid,ppid,pgid,args | awk 'wrong && $2 == 1 && !/awk/ && !/procstat\.py/ {print $0}'
###
###   -- This example shows a regex that returns more than one process
###      with a ppid of 1.
###
###      $ python procstat.py -p test -v '/mingetty/' -t
###       Testing test: /mingetty/
###       failed getting pgid: more than 1 result returned
###      ps -Ao pid,ppid,pgid,args | awk '/mingetty/ && $2 == 1 && !/awk/ && !/procstat\.py/ {print $0}'
###       7313     1  7313 /sbin/mingetty tty1
###       7314     1  7314 /sbin/mingetty tty2
###       7315     1  7315 /sbin/mingetty tty3
###       7316     1  7316 /sbin/mingetty tty4
###       7317     1  7317 /sbin/mingetty tty5
###       7318     1  7318 /sbin/mingetty tty6
###
###  Command Line Example:
###    $ python procstat.py -p httpd,opennms,splunk,splunk-web \
###    -v '/var/run/httpd.pid','/opt/opennms/logs/daemon/opennms.pid','/splunkd.*start/','/twistd.*SplunkWeb/'
###
###     procstat_httpd_mem: 202068 KB [The total memory utilization]
###     procstat_splunk_mem: 497848 KB [The total memory utilization]
###     procstat_splunk-web_mem: 32636 KB [The total memory utilization]
###     procstat_opennms_mem: 623112 KB [The total memory utilization]
###     procstat_httpd_cpu: 0.3 percent [The total percent CPU utilization]
###     procstat_splunk_cpu: 0.6 percent [The total percent CPU utilization]
###     procstat_splunk-web_cpu: 0.1 percent [The total percent CPU utilization]
###     procstat_opennms_cpu: 7.1 percent [The total percent CPU utilization]
###
###  Example Values:
###    httpd:      /var/run/httpd.pid or \/usr\/sbin\/httpd
###    mysqld:     /var/run/mysqld/mysqld.pid or /\/usr\/bin\/mysqld_safe/
###    postgresql: /var/run/postmaster.[port].pid or /\/usr\/bin\/postmaster.*[port]/
###    splunk:     /splunkd.*start/
###    splunk-web: /twistd.*SplunkWeb/
###    opennms:    /opt/opennms/logs/daemon/opennms.pid or java.*Dopennms
###    netflow:    /java.*NetFlow/
###    postfix:    /var/spool/postfix/pid/master.pid or /\/usr\/libexec\/postfix\/master/
###
###  Error Tests:
###    python procstat.py -p test-more,test-none,test-pidfail -v '/java/','/javaw/','java.pid' -t
###
###  Changelog:
###    v1.0.1 - 2010-07-23
###      * Initial version
###
###    v1.1.0 - 2010-07-28
###      * Modified the process regex search to find the parent
###        process and then find all processes with the same process
###        group ID (pgid). "ps" is only used for regex searching on
###        the initial lookup for the parent pid (ppid). Now all
###        subsequent calls use /proc/[pid]/stat for CPU jiffies, and
###        /proc/[pid]/statm for memory rss.
###      * Added testing switch "-t" to help troubleshoot a regex
###      * Added display switches "-s" and "-m" to format the output
###        of /proc/[pid]/stat and /proc/[pid]/statm
###

###  Copyright Jamie Isaacs. 2010
###  License to use, modify, and distribute under the GPL
###  http://www.gnu.org/licenses/gpl.txt

import time
import subprocess
import traceback, sys
import os.path
import glob
import logging

descriptors = []

logging.basicConfig(level=logging.ERROR, format="%(asctime)s - %(name)s - %(levelname)s\t Thread-%(thread)d - %(message)s", filename='/tmp/gmond.log', filemode='w')
logging.debug('starting up')

last_update = 0
stats = {}
last_val = {}
pgid_list = {}

MAX_UPDATE_TIME = 15

# clock ticks per second... jiffies (HZ)
JIFFIES_PER_SEC = os.sysconf('SC_CLK_TCK')

PAGE_SIZE=os.sysconf('SC_PAGE_SIZE')

PROCESSES = {}

def readCpu(pid):
	try:
		stat = file('/proc/' + pid + '/stat', 'rt').readline().split()
		#logging.debug(' stat (' + pid + '): ' + str(stat))
		utime = int(stat[13])
		stime = int(stat[14])
		cutime = int(stat[15])
		cstime = int(stat[16])
		return (utime + stime + cutime + cstime)
	except:
		logging.warning('failed to get (' + str(pid) + ') stats')
		return 0

def get_pgid(proc):
	logging.debug('getting pgid for process: ' + proc)
	ERROR = 0

	if pgid_list.has_key(proc) and os.path.exists('/proc/' + pgid_list[proc][0]):
		return pgid_list[proc]

	val = PROCESSES[proc]
	# Is this a pidfile? Last 4 chars are .pid
	if '.pid' in val[-4:]:
		if os.path.exists(val):
			logging.debug(' pidfile found')
			ppid = file(val, 'rt').readline().strip()
			pgid = file('/proc/' + ppid + '/stat', 'rt').readline().split()[4]
		else:
			raise Exception('pidfile (' + val + ') does not exist')

	else:
		# This is a regex, lets search for it
		regex = PROCESSES[proc]
		cmd = "ps -Ao pid,ppid,pgid,args | awk '" + regex + " && $2 == 1 && !/awk/ && !/procstat\.py/ {print $0}'"
		p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		out, err = p.communicate()

		if p.returncode:
			raise Exception('failed executing ps\n' + cmd + '\n' + err)

		result = out.strip().split('\n')
		logging.debug(' result: ' + str(result))

		if len(result) > 1:
			raise Exception('more than 1 result returned\n' + cmd + '\n' + out.strip())

		if result[0] in '':
			raise Exception('no process returned\n' + cmd)

		res = result[0].split()
		ppid = res[0]
		pgid = res[2]

	if os.path.exists('/proc/' + ppid):
		logging.debug(' ppid: ' + ppid + ' pgid: ' + pgid)
		return (ppid, pgid)
	else:
		return ERROR

def get_pgroup(ppid, pgid):
	'''Return a list of pids having the same pgid, with the first in the list being the parent pid.'''
	logging.debug('getting pids for ppid/pgid: ' + ppid + '/' + pgid)

	try:
		# Get all processes in this group
		p_list = []
		for stat_file in glob.glob('/proc/[1-9]*/stat'):
			stat = file(stat_file, 'rt').readline().split()
			if stat[4] == pgid:
				p_list.append(stat[0])

		# place pid at the top of the list
		p_list.remove(ppid)
		p_list.insert(0, ppid)

		logging.debug('p_list: ' + str(p_list))

		return p_list

	except:
		logging.warning('failed getting pids')

def get_rss(pids):
	logging.debug('getting rss for pids')

	rss = 0
	for p in pids:
		try:
			statm = open('/proc/' + p + '/statm', 'rt').readline().split()
			#logging.debug(' statm (' + p + '): ' + str(statm))
		except:
			# Process finished, ignore this mem usage
			logging.warning(' failed getting statm for pid: ' + p)
			continue

		rss += int(statm[1])

	rss *= PAGE_SIZE
	return rss

def test(params):
	global PROCESSES, MAX_UPDATE_TIME

	MAX_UPDATE_TIME = 2

	logging.debug('testing processes: ' + str(params))

	PROCESSES = params

	for proc,val in PROCESSES.items():
		print('')
		print(' Testing ' + proc + ': ' + val) 

		try:
			(ppid, pgid) = get_pgid(proc)
		except Exception, e:
			print(' failed getting pgid: ' + str(e))
			continue

		pids = get_pgroup(ppid, pgid)

		print(' Processes in this group: ')
		print(' PID, ARGS')
		for pid in pids:
			# Read from binary file containing command line arguments
			args = file('/proc/' + pid + '/cmdline', 'rt').readline().replace('\0', ' ')
			print(' ' + pid + ' ' + args)

	logging.debug('success testing')

def update_stats():
	logging.debug('updating stats')
	global last_update, stats, last_val
	
	cur_time = time.time()

	if cur_time - last_update < MAX_UPDATE_TIME:
		logging.debug(' wait ' + str(int(MAX_UPDATE_TIME - (cur_time - last_update))) + ' seconds')
		return True
	else:
		last_update = cur_time

	for proc,val in PROCESSES.items():
		logging.debug(' updating for ' + proc)

		# setup storage lists
		if not proc in stats:
			stats[proc] = {}
		if not proc in last_val:
			last_val[proc] = {}

		#####
		# Update CPU utilization
		try:
			(ppid, pgid) = get_pgid(proc)
		except Exception, e:
			logging.warning(' failed getting pgid: ' + str(e))
			stats[proc]['cpu'] = 0.0
			stats[proc]['mem'] = 0
			continue

		# save for later
		pgid_list[proc] = (ppid, pgid)

		pids = get_pgroup(ppid, pgid)

		cpu_time = time.time()
		proc_time = 0
		for p in pids:
			proc_time += readCpu(p)
		
		logging.debug(' proc_time: ' + str(proc_time) + ' cpu_time: ' + str(cpu_time))

		# do we have an old value to calculate with?
		if 'cpu_time' in last_val[proc]:
			logging.debug(' last_val: ' + str(last_val[proc]))
			logging.debug(' calc: 100 * ' + str(proc_time - last_val[proc]['proc_time']) + ' / ' + str(cpu_time - last_val[proc]['cpu_time']) + ' * ' + str(JIFFIES_PER_SEC))
			stats[proc]['cpu'] = 100 * (proc_time - last_val[proc]['proc_time']) / float((cpu_time - last_val[proc]['cpu_time']) * JIFFIES_PER_SEC)

			logging.debug(' calc: ' + str(stats[proc]['cpu']))
		else:
			stats[proc]['cpu'] = 0.0

		last_val[proc]['cpu_time'] = cpu_time
		last_val[proc]['proc_time'] = proc_time

		#####
		# Update Mem utilization	
		rss = get_rss(pids)
		stats[proc]['mem'] = rss
	
	logging.debug('success refreshing stats')
	logging.debug('stats: ' + str(stats))

	return True

def get_stat(name):
	logging.debug('getting stat: ' + name)

	ret = update_stats()

	if ret:
		if name.startswith('procstat_'):
			fir = name.find('_')
			sec = name.find('_', fir + 1)

			proc = name[fir+1:sec]
			label = name[sec+1:]

			try:
				return stats[proc][label]
			except:
				logging.warning('failed to fetch [' + proc + '] ' + name)
				return 0
		else:
			label = name

		try:
			return stats[label]
		except:
			logging.warning('failed to fetch ' + name)
			return 0
	else:
		return 0

def metric_init(params):
	global descriptors
	global PROCESSES

	logging.debug('init: ' + str(params))

	PROCESSES = params

	#for proc,regex in PROCESSES.items():
		
	update_stats()

	descriptions = dict(
		cpu = {
			'units': 'percent',
			'value_type': 'float',
			'format': '%.1f',
			'description': 'The total percent CPU utilization'},

		mem = {
			'units': 'B',
			'description': 'The total memory utilization'}
	)

	time_max = 60
	for label in descriptions:
		for proc in PROCESSES:
			if stats[proc].has_key(label):

				d = {
					'name': 'procstat_' + proc + '_' + label,
					'call_back': get_stat,
					'time_max': time_max,
					'value_type': 'uint',
					'units': '',
					'slope': 'both',
					'format': '%u',
					'description': label,
					'groups': 'procstat'
				}

				# Apply metric customizations from descriptions
				d.update(descriptions[label])

				descriptors.append(d)

			else:
				logging.error("skipped " + proc + '_' + label)

	#logging.debug('descriptors: ' + str(descriptors))

	return descriptors

def display_proc_stat(pid):
	try:
		stat = file('/proc/' + pid + '/stat', 'rt').readline().split()

		fields = [
			'pid', 'comm', 'state', 'ppid', 'pgrp', 'session',
			'tty_nr', 'tty_pgrp', 'flags', 'min_flt', 'cmin_flt', 'maj_flt',
			'cmaj_flt', 'utime', 'stime', 'cutime', 'cstime', 'priority',
			'nice', 'num_threads', 'it_real_value', 'start_time', 'vsize', 'rss',
			'rlim', 'start_code', 'end_code', 'start_stack', 'esp', 'eip',
			'pending', 'blocked', 'sigign', 'sigcatch', 'wchan', 'nswap',
			'cnswap', 'exit_signal', 'processor', 'rt_priority', 'policy'
		]

		# Display them
		i = 0
		for f in fields:
			print '%15s: %s' % (f, stat[i])
			i += 1

	except:
		print('failed to get /proc/' + pid + '/stat')
		print(traceback.print_exc(file=sys.stdout))

def display_proc_statm(pid):
	try:
		statm = file('/proc/' + pid + '/statm', 'rt').readline().split()

		fields = [
			'size', 'rss', 'share', 'trs', 'drs', 'lrs' ,'dt' 
		]

		# Display them
		i = 0
		for f in fields:
			print '%15s: %s' % (f, statm[i])
			i += 1

	except:
		print('failed to get /proc/' + pid + '/statm')
		print(traceback.print_exc(file=sys.stdout))

def metric_cleanup():
	logging.shutdown()
	# pass

if __name__ == '__main__':
	from optparse import OptionParser
	import os

	logging.debug('running from cmd line')
	parser = OptionParser()
	parser.add_option('-p', '--processes', dest='processes', default='', help='processes to explicitly check')
	parser.add_option('-v', '--value', dest='value', default='', help='regex or pidfile for each processes')
	parser.add_option('-s', '--stat', dest='stat', default='', help='display the /proc/[pid]/stat file for this pid')
	parser.add_option('-m', '--statm', dest='statm', default='', help='display the /proc/[pid]/statm file for this pid')
	parser.add_option('-b', '--gmetric-bin', dest='gmetric_bin', default='/usr/bin/gmetric', help='path to gmetric binary')
	parser.add_option('-c', '--gmond-conf', dest='gmond_conf', default='/etc/ganglia/gmond.conf', help='path to gmond.conf')
	parser.add_option('-g', '--gmetric', dest='gmetric', action='store_true', default=False, help='submit via gmetric')
	parser.add_option('-q', '--quiet', dest='quiet', action='store_true', default=False)
	parser.add_option('-t', '--test', dest='test', action='store_true', default=False, help='test the regex list')

	(options, args) = parser.parse_args()

	if options.stat != '':
		display_proc_stat(options.stat)
		sys.exit(0)
	elif options.statm != '':
		display_proc_statm(options.statm)
		sys.exit(0)

	_procs = options.processes.split(',')
	_val = options.value.split(',')
	params = {}
	i = 0
	for proc in _procs:
		params[proc] = _val[i]
		i += 1
	
	if options.test:
		test(params)
		update_stats()

		print('')
		print(' waiting ' + str(MAX_UPDATE_TIME) + ' seconds')
		time.sleep(MAX_UPDATE_TIME)

	metric_init(params)

	for d in descriptors:
		v = d['call_back'](d['name'])
		if not options.quiet:
			print ' %s: %s %s [%s]' % (d['name'], d['format'] % v, d['units'], d['description'])

		if options.gmetric:
			if d['value_type'] == 'uint':
				value_type = 'uint32'
			else:
				value_type = d['value_type']

			cmd = "%s --conf=%s --value='%s' --units='%s' --type='%s' --name='%s' --slope='%s'" % \
				(options.gmetric_bin, options.gmond_conf, v, d['units'], value_type, d['name'], d['slope'])
			os.system(cmd)


