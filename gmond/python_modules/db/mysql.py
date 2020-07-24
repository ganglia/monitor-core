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
"""

###  Changelog:
###    v1.0.1 - 2010-07-21
###       * Initial version
###
###    v1.0.2 - 2010-08-04
###       * Added system variables: max_connections and query_cache_size
###       * Modified some innodb status variables to become deltas
###
###    v1.0.3 - 2011-12-02
###       * Support custom UNIX sockets
###
###  Requires:
###       * yum install MySQL-python
###       * DBUtil.py

###  Copyright Jamie Isaacs. 2010
###  License to use, modify, and distribute under the GPL
###  http://www.gnu.org/licenses/gpl.txt

import time
import MySQLdb

from DBUtil import parse_innodb_status, defaultdict

import logging

descriptors = []

logging.basicConfig(level=logging.ERROR, format="%(asctime)s - %(name)s - %(levelname)s\t Thread-%(thread)d - %(message)s", filename='/tmp/mysqlstats.log', filemode='w')
logging.debug('starting up')

last_update = 0
mysql_conn_opts = {}
mysql_stats = {}
mysql_stats_last = {}
delta_per_second = False

REPORT_INNODB = True
REPORT_MASTER = True
REPORT_SLAVE = True

MAX_UPDATE_TIME = 15


def update_stats(get_innodb=True, get_main=True, get_subordinate=True):
    """

    """
    logging.debug('updating stats')
    global last_update
    global mysql_stats, mysql_stats_last

    cur_time = time.time()
    time_delta = cur_time - last_update
    if time_delta <= 0:
        #we went backward in time.
        logging.debug(" system clock set backwards, probably ntp")

    if cur_time - last_update < MAX_UPDATE_TIME:
        logging.debug(' wait ' + str(int(MAX_UPDATE_TIME - (cur_time - last_update))) + ' seconds')
        return True
    else:
        last_update = cur_time

    logging.debug('refreshing stats')
    mysql_stats = {}

    # Get info from DB
    try:
        conn = MySQLdb.connect(**mysql_conn_opts)

        cursor = conn.cursor(MySQLdb.cursors.DictCursor)
        cursor.execute("SELECT GET_LOCK('gmetric-mysql', 0) as ok")
        lock_stat = cursor.fetchone()
        cursor.close()

        if lock_stat['ok'] == 0:
            return False

        cursor = conn.cursor(MySQLdb.cursors.Cursor)
        cursor.execute("SHOW VARIABLES")
        #variables = dict(((k.lower(), v) for (k,v) in cursor))
        variables = {}
        for (k, v) in cursor:
            variables[k.lower()] = v
        cursor.close()

        cursor = conn.cursor(MySQLdb.cursors.Cursor)
        cursor.execute("SHOW /*!50002 GLOBAL */ STATUS")
        #global_status = dict(((k.lower(), v) for (k,v) in cursor))
        global_status = {}
        for (k, v) in cursor:
            global_status[k.lower()] = v
        cursor.close()

        cursor = conn.cursor(MySQLdb.cursors.Cursor)
        cursor.execute("SELECT PLUGIN_STATUS, PLUGIN_VERSION FROM `information_schema`.Plugins WHERE PLUGIN_NAME LIKE '%innodb%' AND PLUGIN_TYPE LIKE 'STORAGE ENGINE';")

        have_innodb = False
        innodb_version = 1.0
        row = cursor.fetchone()

        if row[0] == "ACTIVE":
            have_innodb = True
            innodb_version = row[1]
        cursor.close()

        # try not to fail ?
        get_innodb = get_innodb and have_innodb
        get_main = get_main and variables['log_bin'].lower() == 'on'

        innodb_status = defaultdict(int)
        if get_innodb:
            cursor = conn.cursor(MySQLdb.cursors.Cursor)
            cursor.execute("SHOW /*!50000 ENGINE*/ INNODB STATUS")
            innodb_status = parse_innodb_status(cursor.fetchone()[2].split('\n'), innodb_version)
            cursor.close()
            logging.debug('innodb_status: ' + str(innodb_status))

        main_logs = tuple
        if get_main:
            cursor = conn.cursor(MySQLdb.cursors.Cursor)
            cursor.execute("SHOW MASTER LOGS")
            main_logs = cursor.fetchall()
            cursor.close()

        subordinate_status = {}
        if get_subordinate:
            cursor = conn.cursor(MySQLdb.cursors.DictCursor)
            cursor.execute("SHOW SLAVE STATUS")
            res = cursor.fetchone()
            if res:
                for (k,v) in res.items():
                    subordinate_status[k.lower()] = v
            else:
                get_subordinate = False
            cursor.close()

        cursor = conn.cursor(MySQLdb.cursors.DictCursor)
        cursor.execute("SELECT RELEASE_LOCK('gmetric-mysql') as ok")
        cursor.close()

        conn.close()
    except MySQLdb.OperationalError, (errno, errmsg):
        logging.error('error updating stats')
        logging.error(errmsg)
        return False

    # process variables
    # http://dev.mysql.com/doc/refman/5.0/en/server-system-variables.html
    mysql_stats['version'] = variables['version']
    mysql_stats['max_connections'] = variables['max_connections']
    mysql_stats['query_cache_size'] = variables['query_cache_size']

    # process global status
    # http://dev.mysql.com/doc/refman/5.0/en/server-status-variables.html
    interesting_global_status_vars = (
        'aborted_clients',
        'aborted_connects',
        'binlog_cache_disk_use',
        'binlog_cache_use',
        'bytes_received',
        'bytes_sent',
        'com_delete',
        'com_delete_multi',
        'com_insert',
        'com_insert_select',
        'com_load',
        'com_replace',
        'com_replace_select',
        'com_select',
        'com_update',
        'com_update_multi',
        'connections',
        'created_tmp_disk_tables',
        'created_tmp_files',
        'created_tmp_tables',
        'key_reads',
        'key_read_requests',
        'key_writes',
        'key_write_requests',
        'max_used_connections',
        'open_files',
        'open_tables',
        'opened_tables',
        'qcache_free_blocks',
        'qcache_free_memory',
        'qcache_hits',
        'qcache_inserts',
        'qcache_lowmem_prunes',
        'qcache_not_cached',
        'qcache_queries_in_cache',
        'qcache_total_blocks',
        'questions',
        'select_full_join',
        'select_full_range_join',
        'select_range',
        'select_range_check',
        'select_scan',
        'subordinate_open_temp_tables',
        'subordinate_retried_transactions',
        'slow_launch_threads',
        'slow_queries',
        'sort_range',
        'sort_rows',
        'sort_scan',
        'table_locks_immediate',
        'table_locks_waited',
        'threads_cached',
        'threads_connected',
        'threads_created',
        'threads_running',
        'uptime',
    )

    non_delta = (
        'max_used_connections',
        'open_files',
        'open_tables',
        'qcache_free_blocks',
        'qcache_free_memory',
        'qcache_total_blocks',
        'subordinate_open_temp_tables',
        'threads_cached',
        'threads_connected',
        'threads_running',
        'uptime'
    )

    # don't put all of global_status in mysql_stats b/c it's so big
    for key in interesting_global_status_vars:
        if key in non_delta:
            mysql_stats[key] = global_status[key]
        else:
            # Calculate deltas for counters
            if time_delta <= 0:
                #systemclock was set backwards, nog updating values.. to smooth over the graphs
                pass
            elif key in mysql_stats_last:
                if delta_per_second:
                    mysql_stats[key] = (int(global_status[key]) - int(mysql_stats_last[key])) / time_delta
                else:
                    mysql_stats[key] = int(global_status[key]) - int(mysql_stats_last[key])
            else:
                mysql_stats[key] = float(0)

            mysql_stats_last[key] = global_status[key]

    mysql_stats['open_files_used'] = int(global_status['open_files']) / int(variables['open_files_limit'])

    innodb_delta = (
        'data_fsyncs',
        'data_reads',
        'data_writes',
        'log_writes'
    )

    # process innodb status
    if get_innodb:
        for istat in innodb_status:
            key = 'innodb_' + istat

            if istat in innodb_delta:
                # Calculate deltas for counters
                if time_delta <= 0:
                    #systemclock was set backwards, nog updating values.. to smooth over the graphs
                    pass
                elif key in mysql_stats_last:
                    if delta_per_second:
                        mysql_stats[key] = (int(innodb_status[istat]) - int(mysql_stats_last[key])) / time_delta
                    else:
                        mysql_stats[key] = int(innodb_status[istat]) - int(mysql_stats_last[key])
                else:
                    mysql_stats[key] = float(0)

                mysql_stats_last[key] = innodb_status[istat]

            else:
                mysql_stats[key] = innodb_status[istat]

    # process main logs
    if get_main:
        mysql_stats['binlog_count'] = len(main_logs)
        mysql_stats['binlog_space_current'] = main_logs[-1][1]
        #mysql_stats['binlog_space_total'] = sum((long(s[1]) for s in main_logs))
        mysql_stats['binlog_space_total'] = 0
        for s in main_logs:
            mysql_stats['binlog_space_total'] += int(s[1])
        mysql_stats['binlog_space_used'] = float(main_logs[-1][1]) / float(variables['max_binlog_size']) * 100

    # process subordinate status
    if get_subordinate:
        mysql_stats['subordinate_exec_main_log_pos'] = subordinate_status['exec_main_log_pos']
        #mysql_stats['subordinate_io'] = 1 if subordinate_status['subordinate_io_running'].lower() == "yes" else 0
        if subordinate_status['subordinate_io_running'].lower() == "yes":
            mysql_stats['subordinate_io'] = 1
        else:
            mysql_stats['subordinate_io'] = 0
        #mysql_stats['subordinate_sql'] = 1 if subordinate_status['subordinate_sql_running'].lower() =="yes" else 0
        if subordinate_status['subordinate_sql_running'].lower() == "yes":
            mysql_stats['subordinate_sql'] = 1
        else:
            mysql_stats['subordinate_sql'] = 0
        mysql_stats['subordinate_lag'] = subordinate_status['seconds_behind_main']
        mysql_stats['subordinate_relay_log_pos'] = subordinate_status['relay_log_pos']
        mysql_stats['subordinate_relay_log_space'] = subordinate_status['relay_log_space']

    logging.debug('success updating stats')
    logging.debug('mysql_stats: ' + str(mysql_stats))


def get_stat(name):
    logging.info("getting stat: %s" % name)
    global mysql_stats
    #logging.debug(mysql_stats)

    global REPORT_INNODB
    global REPORT_MASTER
    global REPORT_SLAVE

    ret = update_stats(REPORT_INNODB, REPORT_MASTER, REPORT_SLAVE)

    if ret:
        if name.startswith('mysql_'):
            label = name[6:]
        else:
            label = name

        logging.debug("fetching %s" % name)
        try:
            return mysql_stats[label]
        except:
            logging.error("failed to fetch %s" % name)
            return 0
    else:
        return 0

def metric_init(params):
    global descriptors
    global mysql_conn_opts
    global mysql_stats
    global delta_per_second

    global REPORT_INNODB
    global REPORT_MASTER
    global REPORT_SLAVE

    REPORT_INNODB = str(params.get('get_innodb', True)) == "True"
    REPORT_MASTER = str(params.get('get_main', True)) == "True"
    REPORT_SLAVE = str(params.get('get_subordinate', True)) == "True"

    logging.debug("init: " + str(params))

    mysql_conn_opts = dict(
        host = params.get('host', 'localhost'),
        user = params.get('user'),
        passwd = params.get('passwd'),
        port = int(params.get('port', 3306)),
        connect_timeout = int(params.get('timeout', 30)),
    )
    if params.get('unix_socket', '') != '':
        mysql_conn_opts['unix_socket'] = params.get('unix_socket')

    if params.get("delta_per_second", '') != '':
        delta_per_second = True

    main_stats_descriptions = {}
    innodb_stats_descriptions = {}
    subordinate_stats_descriptions = {}

    misc_stats_descriptions = dict(
        aborted_clients = {
            'description': 'The number of connections that were aborted because the client died without closing the connection properly',
            'value_type': 'float',
            'units': 'clients',
        },

        aborted_connects = {
            'description': 'The number of failed attempts to connect to the MySQL server',
            'value_type': 'float',
            'units': 'conns',
        },

        binlog_cache_disk_use = {
            'description': 'The number of transactions that used the temporary binary log cache but that exceeded the value of binlog_cache_size and used a temporary file to store statements from the transaction',
            'value_type': 'float',
            'units': 'txns',
        },

        binlog_cache_use = {
            'description': ' The number of transactions that used the temporary binary log cache',
            'value_type': 'float',
            'units': 'txns',
        },

        bytes_received = {
            'description': 'The number of bytes received from all clients',
            'value_type': 'float',
            'units': 'bytes',
        },

        bytes_sent = {
            'description': ' The number of bytes sent to all clients',
            'value_type': 'float',
            'units': 'bytes',
        },

        com_delete = {
            'description': 'The number of DELETE statements',
            'value_type': 'float',
            'units': 'stmts',
        },

        com_delete_multi = {
            'description': 'The number of multi-table DELETE statements',
            'value_type': 'float',
            'units': 'stmts',
        },

        com_insert = {
            'description': 'The number of INSERT statements',
            'value_type': 'float',
            'units': 'stmts',
        },

        com_insert_select = {
            'description': 'The number of INSERT ... SELECT statements',
            'value_type': 'float',
            'units': 'stmts',
        },

        com_load = {
            'description': 'The number of LOAD statements',
            'value_type': 'float',
            'units': 'stmts',
        },

        com_replace = {
            'description': 'The number of REPLACE statements',
            'value_type': 'float',
            'units': 'stmts',
        },

        com_replace_select = {
            'description': 'The number of REPLACE ... SELECT statements',
            'value_type': 'float',
            'units': 'stmts',
        },

        com_select = {
            'description': 'The number of SELECT statements',
            'value_type': 'float',
            'units': 'stmts',
        },

        com_update = {
            'description': 'The number of UPDATE statements',
            'value_type': 'float',
            'units': 'stmts',
        },

        com_update_multi = {
            'description': 'The number of multi-table UPDATE statements',
            'value_type': 'float',
            'units': 'stmts',
        },

        connections = {
            'description': 'The number of connection attempts (successful or not) to the MySQL server',
            'value_type': 'float',
            'units': 'conns',
        },

        created_tmp_disk_tables = {
            'description': 'The number of temporary tables on disk created automatically by the server while executing statements',
            'value_type': 'float',
            'units': 'tables',
        },

        created_tmp_files = {
            'description': 'The number of temporary files mysqld has created',
            'value_type': 'float',
            'units': 'files',
        },

        created_tmp_tables = {
            'description': 'The number of in-memory temporary tables created automatically by the server while executing statement',
            'value_type': 'float',
            'units': 'tables',
        },

        #TODO in graphs: key_read_cache_miss_rate = key_reads / key_read_requests

        key_read_requests = {
            'description': 'The number of requests to read a key block from the cache',
            'value_type': 'float',
            'units': 'reqs',
        },

        key_reads = {
            'description': 'The number of physical reads of a key block from disk',
            'value_type': 'float',
            'units': 'reads',
        },

        key_write_requests = {
            'description': 'The number of requests to write a key block to the cache',
            'value_type': 'float',
            'units': 'reqs',
        },

        key_writes = {
            'description': 'The number of physical writes of a key block to disk',
            'value_type': 'float',
            'units': 'writes',
        },

        max_used_connections = {
            'description': 'The maximum number of connections that have been in use simultaneously since the server started',
            'units': 'conns',
            'slope': 'both',
        },

        open_files = {
            'description': 'The number of files that are open',
            'units': 'files',
            'slope': 'both',
        },

        open_tables = {
            'description': 'The number of tables that are open',
            'units': 'tables',
            'slope': 'both',
        },

        # If Opened_tables is big, your table_cache value is probably too small.
        opened_tables = {
            'description': 'The number of tables that have been opened',
            'value_type': 'float',
            'units': 'tables',
        },

        qcache_free_blocks = {
            'description': 'The number of free memory blocks in the query cache',
            'units': 'blocks',
            'slope': 'both',
        },

        qcache_free_memory = {
            'description': 'The amount of free memory for the query cache',
            'units': 'bytes',
            'slope': 'both',
        },

        qcache_hits = {
            'description': 'The number of query cache hits',
            'value_type': 'float',
            'units': 'hits',
        },

        qcache_inserts = {
            'description': 'The number of queries added to the query cache',
            'value_type': 'float',
            'units': 'queries',
        },

        qcache_lowmem_prunes = {
            'description': 'The number of queries that were deleted from the query cache because of low memory',
            'value_type': 'float',
            'units': 'queries',
        },

        qcache_not_cached = {
            'description': 'The number of non-cached queries (not cacheable, or not cached due to the query_cache_type setting)',
            'value_type': 'float',
            'units': 'queries',
        },

        qcache_queries_in_cache = {
            'description': 'The number of queries registered in the query cache',
            'value_type': 'float',
            'units': 'queries',
        },

        qcache_total_blocks = {
            'description': 'The total number of blocks in the query cache',
            'units': 'blocks',
        },

        questions = {
            'description': 'The number of statements that clients have sent to the server',
            'value_type': 'float',
            'units': 'stmts',
        },

        # If this value is not 0, you should carefully check the indexes of your tables.
        select_full_join = {
            'description': 'The number of joins that perform table scans because they do not use indexes',
            'value_type': 'float',
            'units': 'joins',
        },

        select_full_range_join = {
            'description': 'The number of joins that used a range search on a reference table',
            'value_type': 'float',
            'units': 'joins',
        },

        select_range = {
            'description': 'The number of joins that used ranges on the first table',
            'value_type': 'float',
            'units': 'joins',
        },

        # If this is not 0, you should carefully check the indexes of your tables.
        select_range_check = {
            'description': 'The number of joins without keys that check for key usage after each row',
            'value_type': 'float',
            'units': 'joins',
        },

        select_scan = {
            'description': 'The number of joins that did a full scan of the first table',
            'value_type': 'float',
            'units': 'joins',
        },

        subordinate_open_temp_tables = {
            'description': 'The number of temporary tables that the subordinate SQL thread currently has open',
            'value_type': 'float',
            'units': 'tables',
            'slope': 'both',
        },

        subordinate_retried_transactions = {
            'description': 'The total number of times since startup that the replication subordinate SQL thread has retried transactions',
            'value_type': 'float',
            'units': 'count',
        },

        slow_launch_threads = {
            'description': 'The number of threads that have taken more than slow_launch_time seconds to create',
            'value_type': 'float',
            'units': 'threads',
        },

        slow_queries = {
            'description': 'The number of queries that have taken more than long_query_time seconds',
            'value_type': 'float',
            'units': 'queries',
        },

        sort_range = {
            'description': 'The number of sorts that were done using ranges',
            'value_type': 'float',
            'units': 'sorts',
        },

        sort_rows = {
            'description': 'The number of sorted rows',
            'value_type': 'float',
            'units': 'rows',
        },

        sort_scan = {
            'description': 'The number of sorts that were done by scanning the table',
            'value_type': 'float',
            'units': 'sorts',
        },

        table_locks_immediate = {
            'description': 'The number of times that a request for a table lock could be granted immediately',
            'value_type': 'float',
            'units': 'count',
        },

        # If this is high and you have performance problems, you should first optimize your queries, and then either split your table or tables or use replication.
        table_locks_waited = {
            'description': 'The number of times that a request for a table lock could not be granted immediately and a wait was needed',
            'value_type': 'float',
            'units': 'count',
        },

        threads_cached = {
            'description': 'The number of threads in the thread cache',
            'units': 'threads',
            'slope': 'both',
        },

        threads_connected = {
            'description': 'The number of currently open connections',
            'units': 'threads',
            'slope': 'both',
        },

        #TODO in graphs: The cache miss rate can be calculated as Threads_created/Connections

        # Threads_created is big, you may want to increase the thread_cache_size value.
        threads_created = {
            'description': 'The number of threads created to handle connections',
            'value_type': 'float',
            'units': 'threads',
        },

        threads_running = {
            'description': 'The number of threads that are not sleeping',
            'units': 'threads',
            'slope': 'both',
        },

        uptime = {
            'description': 'The number of seconds that the server has been up',
            'units': 'secs',
            'slope': 'both',
        },

        version = {
            'description': "MySQL Version",
            'value_type': 'string',
            'format': '%s',
        },

        max_connections = {
            'description': "The maximum permitted number of simultaneous client connections",
            'slope': 'zero',
        },

        query_cache_size = {
            'description': "The amount of memory allocated for caching query results",
            'slope': 'zero',
        }
    )

    if REPORT_MASTER:
        main_stats_descriptions = dict(
            binlog_count = {
                'description': "Number of binary logs",
                'units': 'logs',
                'slope': 'both',
            },

            binlog_space_current = {
                'description': "Size of current binary log",
                'units': 'bytes',
                'slope': 'both',
            },

            binlog_space_total = {
                'description': "Total space used by binary logs",
                'units': 'bytes',
                'slope': 'both',
            },

            binlog_space_used = {
                'description': "Current binary log size / max_binlog_size",
                'value_type': 'float',
                'units': 'percent',
                'slope': 'both',
            },
        )

    if REPORT_SLAVE:
        subordinate_stats_descriptions = dict(
            subordinate_exec_main_log_pos = {
                'description': "The position of the last event executed by the SQL thread from the main's binary log",
                'units': 'bytes',
                'slope': 'both',
            },

            subordinate_io = {
                'description': "Whether the I/O thread is started and has connected successfully to the main",
                'value_type': 'uint8',
                'units': 'True/False',
                'slope': 'both',
            },

            subordinate_lag = {
                'description': "Replication Lag",
                'units': 'secs',
                'slope': 'both',
            },

            subordinate_relay_log_pos = {
                'description': "The position up to which the SQL thread has read and executed in the current relay log",
                'units': 'bytes',
                'slope': 'both',
            },

            subordinate_sql = {
                'description': "Subordinate SQL Running",
                'value_type': 'uint8',
                'units': 'True/False',
                'slope': 'both',
            },
        )

    if REPORT_INNODB:
        innodb_stats_descriptions = dict(
            innodb_active_transactions = {
                'description': "Active InnoDB transactions",
                'value_type':'uint',
                'units': 'txns',
                'slope': 'both',
            },

            innodb_current_transactions = {
                'description': "Current InnoDB transactions",
                'value_type':'uint',
                'units': 'txns',
                'slope': 'both',
            },

            innodb_buffer_pool_pages_data = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'pages',
            },

            innodb_data_fsyncs = {
                'description': "The number of fsync() operations",
                'value_type':'float',
                'units': 'fsyncs',
            },

            innodb_data_reads = {
                'description': "The number of data reads",
                'value_type':'float',
                'units': 'reads',
            },

            innodb_data_writes = {
                'description': "The number of data writes",
                'value_type':'float',
                'units': 'writes',
            },

            innodb_buffer_pool_pages_free = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'pages',
                'slope': 'both',
            },

            innodb_history_list = {
                'description': "InnoDB",
                'units': 'length',
                'slope': 'both',
            },

            innodb_ibuf_inserts = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'inserts',
            },

            innodb_ibuf_merged = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'recs',
            },

            innodb_ibuf_merges = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'merges',
            },

            innodb_log_bytes_flushed = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'bytes',
            },

            innodb_log_bytes_unflushed = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'bytes',
                'slope': 'both',
            },

            innodb_log_bytes_written = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'bytes',
            },

            innodb_log_writes = {
                'description': "The number of physical writes to the log file",
                'value_type':'float',
                'units': 'writes',
            },

            innodb_buffer_pool_pages_dirty = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'pages',
                'slope': 'both',
            },

            innodb_os_waits = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'waits',
            },

            innodb_pages_created = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'pages',
            },

            innodb_pages_read = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'pages',
            },

            innodb_pages_written = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'pages',
            },

            innodb_pending_aio_log_ios = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'ops',
            },

            innodb_pending_aio_sync_ios = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'ops',
            },

            innodb_pending_buffer_pool_flushes = {
                'description': "The number of pending buffer pool page-flush requests",
                'value_type':'uint',
                'units': 'reqs',
                'slope': 'both',
            },

            innodb_pending_chkp_writes = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'writes',
            },

            innodb_pending_ibuf_aio_reads = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'reads',
            },

            innodb_pending_log_flushes = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'reqs',
            },

            innodb_pending_log_writes = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'writes',
            },

            innodb_pending_normal_aio_reads = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'reads',
            },

            innodb_pending_normal_aio_writes = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'writes',
            },

            innodb_buffer_pool_pages_bytes = {
                'description': "The total size of buffer pool, in bytes",
                'value_type':'uint',
                'units': 'bytes',
                'slope': 'both',
            },

            innodb_buffer_pool_pages_total = {
                'description': "The total size of buffer pool, in pages",
                'value_type':'uint',
                'units': 'pages',
                'slope': 'both',
            },

            innodb_queries_inside = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'queries',
            },

            innodb_queries_queued = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'queries',
                'slope': 'both',
            },

            innodb_read_views = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'views',
            },

            innodb_rows_deleted = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'rows',
            },

            innodb_rows_inserted = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'rows',
            },

            innodb_rows_read = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'rows',
            },

            innodb_rows_updated = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'rows',
            },

            innodb_spin_rounds = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'spins',
                'slope': 'both',
            },

            innodb_spin_waits = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'spins',
                'slope': 'both',
            },

            innodb_transactions = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'txns',
            },

            innodb_transactions_purged = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'txns',
            },

            innodb_transactions_unpurged = {
                'description': "InnoDB",
                'value_type':'uint',
                'units': 'txns',
            },
        )

    update_stats(REPORT_INNODB, REPORT_MASTER, REPORT_SLAVE)

    time.sleep(MAX_UPDATE_TIME)
    update_stats(REPORT_INNODB, REPORT_MASTER, REPORT_SLAVE)

    for stats_descriptions in (innodb_stats_descriptions, main_stats_descriptions, misc_stats_descriptions, subordinate_stats_descriptions):
        for label in stats_descriptions:
            if mysql_stats.has_key(label):
                format = '%u'
                if stats_descriptions[label].has_key('value_type'):
                    if stats_descriptions[label]['value_type'] == "float":
                        format = '%f'

                d = {
                    'name': 'mysql_' + label,
                    'call_back': get_stat,
                    'time_max': 60,
                    'value_type': "uint",
                    'units': "",
                    'slope': "both",
                    'format': format,
                    'description': "http://search.mysql.com/search?q=" + label,
                    'groups': 'mysql',
                }

                d.update(stats_descriptions[label])

                descriptors.append(d)

            else:
                logging.error("skipped " + label)

    #logging.debug(str(descriptors))
    return descriptors


def metric_cleanup():
    logging.shutdown()
    # pass

if __name__ == '__main__':
    from optparse import OptionParser
    import os

    logging.debug('running from cmd line')
    parser = OptionParser()
    parser.add_option("-H", "--Host", dest="host", help="Host running mysql", default="localhost")
    parser.add_option("-u", "--user", dest="user", help="user to connect as", default="")
    parser.add_option("-p", "--password", dest="passwd", help="password", default="")
    parser.add_option("-P", "--port", dest="port", help="port", default=3306, type="int")
    parser.add_option("-S", "--socket", dest="unix_socket", help="unix_socket", default="")
    parser.add_option("--no-innodb", dest="get_innodb", action="store_false", default=True)
    parser.add_option("--no-main", dest="get_main", action="store_false", default=True)
    parser.add_option("--no-subordinate", dest="get_subordinate", action="store_false", default=True)
    parser.add_option("-b", "--gmetric-bin", dest="gmetric_bin", help="path to gmetric binary", default="/usr/bin/gmetric")
    parser.add_option("-c", "--gmond-conf", dest="gmond_conf", help="path to gmond.conf", default="/etc/ganglia/gmond.conf")
    parser.add_option("-g", "--gmetric", dest="gmetric", help="submit via gmetric", action="store_true", default=False)
    parser.add_option("-q", "--quiet", dest="quiet", action="store_true", default=False)

    (options, args) = parser.parse_args()

    metric_init({
        'host': options.host,
        'passwd': options.passwd,
        'user': options.user,
        'port': options.port,
        'get_innodb': options.get_innodb,
        'get_main': options.get_main,
        'get_subordinate': options.get_subordinate,
        'unix_socket': options.unix_socket,
    })

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
