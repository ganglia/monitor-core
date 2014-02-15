###  This script reports disk stat metrics to ganglia.
###
###  Notes:
###    This script exposes values in /proc/diskstats and calculates
###    various statistics based on the Linux kernel 2.6. To find
###    more information on these values, look in the Linux kernel
###    documentation for "I/O statistics fields".
###
###    By default, the script would monitor any entry listed under
###    /proc/diskstats that is not containing a number. Override it by passing
###    a list of devices in the 'devices' parameter.
###
###    This script has the option of explicitly setting which devices
###    to check using the "devices" option in your configuration. If
###    you set this value, it will invalidate the MIN_DISK_SIZE and
###    IGNORE_DEV options described below. This enables you to
###    monitor specific partitions instead of the entire device.
###    Example value: "sda1 sda2".
###    Example value: "sda sdb sdc".
###
###    This script also checks for a minimum disk size in order to
###    only measure interesting devices by default.
###    [Can be overriden if "devices" is set]
###
###    This script looks for disks to check in /proc/partitions while
###    ignoring any devices present in IGNORE_DEV by default.
###    [Can be overriden if "devices" is set]
###
###  Changelog:
###    v1.0.1 - 2010-07-22
###       * Initial version
###
###    v1.0.2 - 2010-08-03
###       * Modified reads_per_sec to not calculate per second delta.
###         This enables us to generate a better graph by stacking
###         reads/writes with reads/writes merged.
###

###  Copyright Jamie Isaacs. 2010
###  License to use, modify, and distribute under the GPL
###  http://www.gnu.org/licenses/gpl.txt

import time
import subprocess
import traceback
import logging
import os
import stat

descriptors = []

logging.basicConfig(level=logging.ERROR, format="%(asctime)s - %(name)s - %(levelname)s\t Thread-%(thread)d - %(message)s")
logging.debug('starting up')

last_update = 0
cur_time = 0
stats = {}
last_val = {}

MAX_UPDATE_TIME = 15
BYTES_PER_SECTOR = 512

# 5 GB
MIN_DISK_SIZE = 5242880
# Set to None to trigger disk discovery under /proc/diskstats
# Pass a 'devices' parameter to explicitly list disks to monitor
DEVICES = None
IGNORE_DEV = 'dm-|loop|drbd'

PARTITIONS_FILE = '/proc/partitions'
DISKSTATS_FILE = '/proc/diskstats'
DMDIR = '/dev/mapper'
device_mapper = ''

PARTITIONS = []
dmnames = []
dm2pair = {}
pair2dm = {}
devnames = []
dev2pair = {}
pair2dev = {}

def build_dmblock_major_minor_tables():
	"""Returns
	1) a table of filenames that are all device mapper block special files
	2) a dict mapping each device mapper name to (major,minor)
	3) a dict mapping each (major,minor) pair to a table of devce mapper names"""

	names = []
	name2pair = {}
	pair2name = {}
	mapper_entries = []

	mapper_entries = os.listdir(DMDIR)
	for n in mapper_entries:
		s = os.lstat(DMDIR + '/' + n)
		if stat.S_ISBLK(s[stat.ST_MODE]):
			names.append(n)
			maj = str(os.major(s.st_rdev))
			min = str(os.minor(s.st_rdev))
			name2pair[n] = (maj, min)
			pair2name[(maj, min)] = n

	logging.debug('grabbed dmsetup device info')
	logging.debug('dmsetup devices: ' + str(name2pair))

	return (names, name2pair, pair2name)

def build_block_major_minor_tables():
	"""Returns
	1) a table of filenames that are all block special files
	2) a dict mapping each dev name to (major,minor)
	3) a dict mapping each (major,minor) pair to a table of dev names"""
	dnames = []
	d2p = {}
	p2d = {}

	# Get values from diskstats file
	p = subprocess.Popen("awk '{print $1,$2, $3}' " + DISKSTATS_FILE, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()

	logging.debug('grabbed diskstat device info')
	logging.debug('diskstat devices: ' + str(out))

	for n in out.split('\n'):
		if n:
			[maj, min, name] = n.split()
			dnames.append(name)
			d2p[name] = (maj, min)
			p2d[(maj, min)] = name

	return (dnames, d2p, p2d)

def get_devname(dev):
	"""Returns
	device mapper name converted to dev name"""

	(maj,min) = dm2pair[dev]
	name = pair2dev[(maj,min)]
	return name

def list_dmnames():
	"""Returns
	string of device names associated with device mapper names"""

	global dmnames
	global dm2pair
	global pair2dm
	global devnames
	global dev2pair
	global pair2dev
	devlist = ''

	dmnames, dm2pair, pair2dm =  build_dmblock_major_minor_tables()
	logging.debug('dmnames: ' + str(dmnames))

	devnames, dev2pair, pair2dev =  build_block_major_minor_tables()
	logging.debug('devnames: ' + str(dmnames))

	for d in dmnames:
		devlist = devlist + ' ' + str(d)

	logging.debug('devlist: ' + str(devlist))

	return devlist

def get_partitions():
	logging.debug('getting partitions')
	global PARTITIONS

	if DEVICES is not None:
		# Explicit device list has been set
		logging.debug(' DEVICES has already been set')
		out = DEVICES

	else:	
		# Load partitions
		awk_cmd = "awk 'NR > 1 && $0 !~ /" + IGNORE_DEV + "/ && $4 !~ /[0-9]$/ {ORS=\" \"; print $4}' "
		p = subprocess.Popen(awk_cmd + PARTITIONS_FILE, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		out, err = p.communicate()
		logging.debug('  result: ' + out)
		
		if p.returncode:
			logging.warning('failed getting partitions')
			return p.returncode

	for dev in out.split():
		if DEVICES is not None:
			# Explicit device list has been set
			PARTITIONS.append(dev)
		else:		
			# Load disk block size
			f = open('/sys/block/' + dev + '/size', 'r')
			c = f.read()
			f.close()

			# Make sure device is large enough to collect stats
			if (int(c) * BYTES_PER_SECTOR / 1024) > MIN_DISK_SIZE:
				PARTITIONS.append(dev)
			else:
				logging.debug(' ignoring ' + dev + ' due to size constraints')

	logging.debug('success getting partitions')
	return 0


###########################################################################
# This is the order of metrics in /proc/diskstats
# 0 major         Major number
# 1 minor         Minor number
# 2 blocks        Blocks
# 3 name          Name
# 4 reads         This is the total number of reads completed successfully.
# 5 merge_read    Reads and writes which are adjacent to each other may be merged for
#               efficiency.  Thus two 4K reads may become one 8K read before it is
#               ultimately handed to the disk, and so it will be counted (and queued)
#               as only one I/O.  This field lets you know how often this was done.
# 6 s_read        This is the total number of sectors read successfully.
# 7 ms_read       This is the total number of milliseconds spent by all reads.
# 8 writes        This is the total number of writes completed successfully.
# 9 merge_write   Reads and writes which are adjacent to each other may be merged for
#               efficiency.  Thus two 4K reads may become one 8K read before it is
#               ultimately handed to the disk, and so it will be counted (and queued)
#               as only one I/O.  This field lets you know how often this was done.
# 10 s_write       This is the total number of sectors written successfully.
# 11 ms_write      This is the total number of milliseconds spent by all writes.
# 12 ios           The only field that should go to zero. Incremented as requests are
#               given to appropriate request_queue_t and decremented as they finish.
# 13 ms_io         This field is increases so long as field 9 is nonzero.
# 14 ms_weighted   This field is incremented at each I/O start, I/O completion, I/O
###########################################################################
def update_stats():
	logging.debug('updating stats')
	global last_update, stats, last_val, cur_time
	global MAX_UPDATE_TIME
	
	cur_time = time.time()

	if cur_time - last_update < MAX_UPDATE_TIME:
		logging.debug(' wait ' + str(int(MAX_UPDATE_TIME - (cur_time - last_update))) + ' seconds')
		return True

	#####
	# Update diskstats
	stats = {}

	if not PARTITIONS:
		part = get_partitions()	
		if part:
			# Fail if return is non-zero
			logging.warning('error getting partitions')
			return False

	# Get values for each disk device
	for dev in PARTITIONS:
		logging.debug(" dev: " + dev)

		# Setup storage lists
		if not dev in stats:
			stats[dev] = {}
		if not dev in last_val:
			last_val[dev] = {}

		# Convert from dmname to devname for use by awk
		if device_mapper == 'true':
			olddev = dev
			dev = get_devname(dev)

		# Get values from diskstats file
		p = subprocess.Popen("awk -v dev=" + dev + " '$3 == dev' " + DISKSTATS_FILE, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		out, err = p.communicate()
		
		vals = out.split()
		logging.debug('  vals: ' + str(vals))

 		# Reset back to orignal dev name
		if device_mapper == 'true':
			dev = olddev

		get_diff(dev, 'reads',  int(vals[3]))
		get_diff(dev, 'writes', int(vals[7]))

		get_diff(dev, 'reads_merged',  int(vals[4]))
		get_diff(dev, 'writes_merged', int(vals[8]))

		get_delta(dev, 'read_bytes_per_sec',  int(vals[5]), float(BYTES_PER_SECTOR) )
		get_delta(dev, 'write_bytes_per_sec', int(vals[9]), float(BYTES_PER_SECTOR) )

		get_delta(dev, 'read_time',  float(vals[6]), 0.001 )
		get_delta(dev, 'write_time', float(vals[10]), 0.001 )

		get_diff(dev, 'io_time', float(vals[12]), 0.001)
		get_percent_time(dev, 'percent_io_time', float(stats[dev]['io_time']))
		get_delta(dev, 'weighted_io_time', float(vals[13]), 0.001)


	logging.debug('success refreshing stats')
	logging.debug('stats: ' + str(stats))
	logging.debug('last_val: ' + str(last_val))

	last_update = cur_time
	return True

def get_delta(dev, key, val, convert=1):
	logging.debug(' get_delta for ' + dev +  '_' + key)
	global stats, last_val

	if convert == 0:
		logging.warning(' convert is zero!')

	interval = cur_time - last_update

	if key in last_val[dev] and interval > 0:

		if val < last_val[dev][key]:
			logging.debug('  fixing int32 wrap')
			val += 4294967296

		stats[dev][key] = (val - last_val[dev][key]) * float(convert) / float(interval)
	else:
		stats[dev][key] = 0

	last_val[dev][key] = int(val)

def get_percent_time(dev, key, val):
	logging.debug(' get_percent_time for ' + dev +  '_' + key)
	global stats, last_val

	interval = cur_time - last_update

	if interval > 0:
		stats[dev][key] = (val / interval) * 100
	else:
		stats[dev][key] = 0

def get_diff(dev, key, val, convert=1):
	logging.debug(' get_diff for ' + dev + '_' + key)
	global stats, last_val

	if key in last_val[dev]:
		stats[dev][key] = (val - last_val[dev][key]) * float(convert)
	else:
		stats[dev][key] = 0

        # If for some reason we have a negative diff we should assume counters reset
        # and should set it back to 0
	if stats[dev][key] < 0:
	  stats[dev][key] = 0


	last_val[dev][key] = val

def get_stat(name):
	logging.debug(' getting stat: ' + name)
	global stats

	ret = update_stats()

	if ret:
		if name.startswith('diskstat_'):
			fir = name.find('_')
			sec = name.find('_', fir + 1)

			dev = name[fir+1:sec]
			label = name[sec+1:]

			try:
				return stats[dev][label]
			except:
				logging.warning('failed to fetch [' + dev + '] ' + name)
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
	global descriptors, device_mapper
	global MIN_DISK_SIZE, DEVICES, IGNORE_DEV

	# Use params.get here to assure function via gmond
	if params.get('device-mapper') == 'true':
		devices = list_dmnames()
		DEVICES = devices
		IGNORE_DEV = 'loop|drbd'
 		device_mapper = 'true'
		logging.debug('dm block devices: ' + str(devices))
	else:
		DEVICES = params.get('devices')

	logging.debug('init: ' + str(params))

	time_max = 60

	descriptions = dict(
		reads = {
			'units': 'reads',
			'description': 'The number of reads completed'},

		reads_merged = {
			'units': 'reads',
			'description': 'The number of reads merged. Reads which are adjacent to each other may be merged for efficiency. Multiple reads may become one before it is handed to the disk, and it will be counted (and queued) as only one I/O.'},

		read_bytes_per_sec = {
			'units': 'bytes/sec',
			'description': 'The number of bytes read per second'},

		read_time = {
			'units': 's',
			'description': 'The time in seconds spent reading'},

		writes = {
			'units': 'writes',
			'description': 'The number of writes completed'},

		writes_merged = {
			'units': 'writes',
			'description': 'The number of writes merged. Writes which are adjacent to each other may be merged for efficiency. Multiple writes may become one before it is handed to the disk, and it will be counted (and queued) as only one I/O.'},

		write_bytes_per_sec = {
			'units': 'bytes/sec',
			'description': 'The number of bbytes written per second'},

		write_time = {
			'units': 's',
			'description': 'The time in seconds spent writing'},

		io_time = {
			'units': 's',
			'description': 'The time in seconds spent in I/O operations'},

		percent_io_time = {
			'units': 'percent',
			'value_type': 'float',
			'format': '%f',
			'description': 'The percent of disk time spent on I/O operations'},

		weighted_io_time = {
			'units': 's',
			'description': 'The weighted time in seconds spend in I/O operations. This measures each I/O start, I/O completion, I/O merge, or read of these stats by the number of I/O operations in progress times the number of seconds spent doing I/O.'}
	)

	update_stats()

	for label in descriptions:
		for dev in PARTITIONS: 
			if stats[dev].has_key(label):

				d = {
					'name': 'diskstat_' + dev + '_' + label,
					'call_back': get_stat,
					'time_max': time_max,
					'value_type': 'float',
					'units': '',
					'slope': 'both',
					'format': '%f',
					'description': label,
					'groups': 'diskstat'
				}

				# Apply metric customizations from descriptions
				d.update(descriptions[label])	

				descriptors.append(d)
			else:
				logging.error("skipped " + label)

	#logging.debug('descriptors: ' + str(descriptors))

	# For command line testing
	#time.sleep(MAX_UPDATE_TIME)
	#update_stats()

	return descriptors

def metric_cleanup():
	logging.shutdown()
	# pass

if __name__ == '__main__':
	from optparse import OptionParser
	import os

	logging.debug('running from cmd line')
	parser = OptionParser()
	parser.add_option('-d', '--devices', dest='devices', default=None, help='devices to explicitly check')
	parser.add_option('-b', '--gmetric-bin', dest='gmetric_bin', default='/usr/bin/gmetric', help='path to gmetric binary')
	parser.add_option('-c', '--gmond-conf', dest='gmond_conf', default='/etc/ganglia/gmond.conf', help='path to gmond.conf')
	parser.add_option('-g', '--gmetric', dest='gmetric', action='store_true', default=False, help='submit via gmetric')
	parser.add_option('-m', '--device-mapper', dest='device_mapper', action='store_true', default=False, help='utilize all device mapper devices if set')
	parser.add_option('-q', '--quiet', dest='quiet', action='store_true', default=False)

	(options, args) = parser.parse_args()

	if options.device_mapper:
		metric_init({
			'device-mapper': 'true',
		})
	else:
		metric_init({
			'devices': options.devices,
		})

	while True:
		for d in descriptors:
			v = d['call_back'](d['name'])
			if not options.quiet:
				print ' %s: %s %s [%s]' % (d['name'], v, d['units'], d['description'])
	
			if options.gmetric:
				if d['value_type'] == 'uint':
					value_type = 'uint32'
				else:
					value_type = d['value_type']
	
				cmd = "%s --conf=%s --value='%s' --units='%s' --type='%s' --name='%s' --slope='%s'" % \
					(options.gmetric_bin, options.gmond_conf, v, d['units'], value_type, d['name'], d['slope'])
				os.system(cmd)
				
		print 'Sleeping 15 seconds'
		time.sleep(15)


