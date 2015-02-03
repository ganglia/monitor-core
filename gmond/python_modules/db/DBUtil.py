"""
The MIT License

Copyright (c) 2008 Gilad Raphaelli <gilad@raphaelli.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

if using < python2.5, http://code.activestate.com/recipes/523034/ works as a
pure python collections.defaultdict substitute
"""

#from collections import defaultdict
try:
    from collections import defaultdict
except:
    class defaultdict(dict):
        def __init__(self, default_factory=None, *a, **kw):
            if (default_factory is not None and
                not hasattr(default_factory, '__call__')):
                raise TypeError('first argument must be callable')
            dict.__init__(self, *a, **kw)
            self.default_factory = default_factory
        def __getitem__(self, key):
            try:
                return dict.__getitem__(self, key)
            except KeyError:
                return self.__missing__(key)
        def __missing__(self, key):
            if self.default_factory is None:
                raise KeyError(key)
            self[key] = value = self.default_factory()
            return value
        def __reduce__(self):
            if self.default_factory is None:
                args = tuple()
            else:
                args = self.default_factory,
            return type(self), args, None, None, self.items()
        def copy(self):
            return self.__copy__()
        def __copy__(self):
            return type(self)(self.default_factory, self)
        def __deepcopy__(self, memo):
            import copy
            return type(self)(self.default_factory,
                              copy.deepcopy(self.items()))
        def __repr__(self):
            return 'defaultdict(%s, %s)' % (self.default_factory,
                                            dict.__repr__(self))

import MySQLdb

def is_hex(s):
    try:
        int(s, 16)
        return True
    except ValueError:
        return False

def longish(x):
        if len(x):
                try:
                        return long(x)
                except ValueError:
                        if(x.endswith(',')):
                           return longish(x[:-1])
                        if(is_hex(x.lower()) == True):
                           return hexlongish(x)
                        #print "X==(%s)(%s)(%s)" %(x, x[:-1],hexlongish(x)), sys.exc_info()[0]
                        return longish(x[:-1])
        else:
                raise ValueError

def hexlongish(x):
	if len(x):
		try:
			return long(str(x), 16)
		except ValueError:
			return longish(x[:-1])
	else:
		raise ValueError

def parse_innodb_status(innodb_status_raw, innodb_version="1.0"):
	def sumof(status):
		def new(*idxs):
			return sum(map(lambda x: longish(status[x]), idxs))
		return new

	innodb_status = defaultdict(int)
	innodb_status['active_transactions']
	individual_buffer_pool_info = False
	
	for line in innodb_status_raw:
		istatus = line.split()

		isum = sumof(istatus)

		# SEMAPHORES
		if "Mutex spin waits" in line:
			innodb_status['spin_waits'] += longish(istatus[3])
			innodb_status['spin_rounds'] += longish(istatus[5])
			innodb_status['os_waits'] += longish(istatus[8])

		elif "RW-shared spins" in line:
			if innodb_version == 1.0:
				innodb_status['spin_waits'] += isum(2,8)
				innodb_status['os_waits'] += isum(5,11)
			elif innodb_version >= 5.5:
				innodb_status['spin_waits'] += longish(istatus[2])
				innodb_status['os_waits'] += longish(istatus[7])

		elif "RW-excl spins" in line and innodb_version >= 5.5:
			innodb_status['spin_waits'] += longish(istatus[2])
			innodb_status['os_waits'] += longish(istatus[7])

		# TRANSACTIONS
		elif "Trx id counter" in line:
			if innodb_version >= 5.6:
				innodb_status['transactions'] += longish(istatus[3])
			elif innodb_version == 5.5:
				innodb_status['transactions'] += hexlongish(istatus[3])
			else:
				innodb_status['transactions'] += isum(3,4)

		elif "Purge done for trx" in line:
			if innodb_version >= 5.6:
				innodb_status['transactions_purged'] += longish(istatus[6])
			elif innodb_version == 5.5:
				innodb_status['transactions_purged'] += hexlongish(istatus[6])
			else:
				innodb_status['transactions_purged'] += isum(6,7)

		elif "History list length" in line:
			innodb_status['history_list'] = longish(istatus[3])

		elif "---TRANSACTION" in line and innodb_status['transactions']:
			innodb_status['current_transactions'] += 1
			if "ACTIVE" in line:
				innodb_status['active_transactions'] += 1

		elif "LOCK WAIT" in line and innodb_status['transactions']:
			innodb_status['locked_transactions'] += 1

		elif 'read views open inside' in line:
			innodb_status['read_views'] = longish(istatus[0])

		# FILE I/O
		elif 'OS file reads' in line:
			innodb_status['data_reads'] = longish(istatus[0])
			innodb_status['data_writes'] = longish(istatus[4])
			innodb_status['data_fsyncs'] = longish(istatus[8])

		elif 'Pending normal aio' in line:
			innodb_status['pending_normal_aio_reads'] = longish(istatus[4])
			innodb_status['pending_normal_aio_writes'] = longish(istatus[7])

		elif 'ibuf aio reads' in line:
			innodb_status['pending_ibuf_aio_reads'] = longish(istatus[3])
			innodb_status['pending_aio_log_ios'] = longish(istatus[6])
			innodb_status['pending_aio_sync_ios'] = longish(istatus[9])

		elif 'Pending flushes (fsync)' in line:
			innodb_status['pending_log_flushes'] = longish(istatus[4])
			innodb_status['pending_buffer_pool_flushes'] = longish(istatus[7])

		# INSERT BUFFER AND ADAPTIVE HASH INDEX
		elif 'merged recs' in line and innodb_version == 1.0:
			innodb_status['ibuf_inserts'] = longish(istatus[0])
			innodb_status['ibuf_merged'] = longish(istatus[2])
			innodb_status['ibuf_merges'] = longish(istatus[5])

		elif 'Ibuf: size' in line and innodb_version >= 5.5:
			innodb_status['ibuf_merges'] = longish(istatus[10])

		elif 'merged operations' in line and innodb_version >= 5.5:
			in_merged = 1

		elif 'delete mark' in line and 'in_merged' in vars() and innodb_version >= 5.5:
			innodb_status['ibuf_inserts'] = longish(istatus[1])
			innodb_status['ibuf_merged'] = 0
			del in_merged

		# LOG
		elif "log i/o's done" in line:
			innodb_status['log_writes'] = longish(istatus[0])

		elif "pending log writes" in line:
			innodb_status['pending_log_writes'] = longish(istatus[0])
			innodb_status['pending_chkp_writes'] = longish(istatus[4])
		
		elif "Log sequence number" in line:
			if innodb_version >= 5.5:
				innodb_status['log_bytes_written'] = longish(istatus[3])
			else:
				innodb_status['log_bytes_written'] = isum(3,4)
		
		elif "Log flushed up to" in line:
			if innodb_version >= 5.5:
				innodb_status['log_bytes_flushed'] = longish(istatus[4])
			else:
				innodb_status['log_bytes_flushed'] = isum(4,5)

		# BUFFER POOL AND MEMORY
		elif "INDIVIDUAL BUFFER POOL INFO" in line:
			# individual pools section.  We only want to record the totals 
			# rather than each individual pool clobbering the totals
			individual_buffer_pool_info = True

		elif "Buffer pool size, bytes" in line and not individual_buffer_pool_info:
			innodb_status['buffer_pool_pages_bytes'] = longish(istatus[4])

		elif "Buffer pool size" in line and not individual_buffer_pool_info:
			innodb_status['buffer_pool_pages_total'] = longish(istatus[3])
		
		elif "Free buffers" in line and not individual_buffer_pool_info:
			innodb_status['buffer_pool_pages_free'] = longish(istatus[2])
		
		elif "Database pages" in line and not individual_buffer_pool_info:
			innodb_status['buffer_pool_pages_data'] = longish(istatus[2])
		
		elif "Modified db pages" in line and not individual_buffer_pool_info:
			innodb_status['buffer_pool_pages_dirty'] = longish(istatus[3])
		
		elif "Pages read" in line and "ahead" not in line and not individual_buffer_pool_info:
				innodb_status['pages_read'] = longish(istatus[2])
				innodb_status['pages_created'] = longish(istatus[4])
				innodb_status['pages_written'] = longish(istatus[6])

		# ROW OPERATIONS
		elif 'Number of rows inserted' in line:
			innodb_status['rows_inserted'] = longish(istatus[4])
			innodb_status['rows_updated'] = longish(istatus[6])
			innodb_status['rows_deleted'] = longish(istatus[8])
			innodb_status['rows_read'] = longish(istatus[10])
		
		elif "queries inside InnoDB" in line:
			innodb_status['queries_inside'] = longish(istatus[0])
			innodb_status['queries_queued'] = longish(istatus[4])

	# Some more stats
	innodb_status['transactions_unpurged'] = innodb_status['transactions'] - innodb_status['transactions_purged']
	innodb_status['log_bytes_unflushed'] = innodb_status['log_bytes_written'] - innodb_status['log_bytes_flushed']

	return innodb_status
	
if __name__ == '__main__':
	from optparse import OptionParser

	parser = OptionParser()
	parser.add_option("-H", "--Host", dest="host", help="Host running mysql", default="localhost")
	parser.add_option("-u", "--user", dest="user", help="user to connect as", default="")
	parser.add_option("-p", "--password", dest="passwd", help="password", default="")
	parser.add_option("-S", "--socket", dest="unix_socket", help="unix_socket", default="")
	(options, args) = parser.parse_args()

	try:
		conn = MySQLdb.connect(user=options.user, host=options.host, passwd=options.passwd, unix_socket=options.socket)

		cursor = conn.cursor(MySQLdb.cursors.Cursor)
		cursor.execute("SHOW /*!50000 ENGINE*/ INNODB STATUS")
		innodb_status = parse_innodb_status(cursor.fetchone()[0].split('\n'))
		cursor.close()

		conn.close()
	except MySQLdb.OperationalError, (errno, errmsg):
		raise

