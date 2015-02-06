# ## DESCRIPTION
# Redis plugin for Ganglia that exposes most of the counters in the
# Redis `INFO` command to Ganglia for all your graphing needs.  The
# metrics it comes with are pretty rudimentary but they get the job
# done.
#
# ## FILES
# ## THEME SONG
#
# The Arcade Fire - "Wake Up"
#
# ## SEE ALSO
# The Redis `INFO` command is described at <http://code.google.com/p/redis/wiki/InfoCommand>.
#
# The original blog post on this plugin is at
# <http://rcrowley.org/2010/06/24/redis-in-ganglia.html>.  Gil
# Raphaelli's MySQL plugin, on which this one is based can be found at
# <http://g.raphaelli.com/2009/1/5/ganglia-mysql-metrics>.

import socket
import time
#import logging

#logging.basicConfig(level=logging.DEBUG, format="%(asctime)s - %(name)s - %(levelname)s\t Thread-%(thread)d - %(message)s", filename='/tmp/gmond.log', filemode='w')
#logging.debug('starting up')


def metric_handler(name):

    # Update from Redis.  Don't thrash.
    if 15 < time.time() - metric_handler.timestamp:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((metric_handler.host, metric_handler.port))
        if metric_handler.auth is not None:
            s.send("*2\r\n$4\r\nAUTH\r\n$%d\r\n%s\r\n" % (len(metric_handler.auth), metric_handler.auth))
            result = s.recv(100)
            if not 'OK' in result:
                return 0
        s.send("*1\r\n$4\r\nINFO\r\n")
        #logging.debug("sent INFO")
        info = s.recv(4096)
        #logging.debug("rcvd INFO")
        if "$" != info[0]:
            return 0
        msglen = int(info[1:info.find("\n")])
        if 4096 < msglen:
            info += s.recv(msglen - 4096)
        metric_handler.info = {}
        try:
          for line in info.splitlines()[1:]:
              #logging.debug("line is %s done" % line)
              if "" == line:
                  continue
              if "#" == line[0]:
                  continue
              n, v = line.split(":")
              if n in metric_handler.descriptors:
                  if n == "master_sync_status":
                      v = 1 if v == 'up' else 0
                  if n == "db0":
                      v = v.split('=')[1].split(',')[0]
                  if n == "used_memory":
                      v = int(int(v) / 1000)
                  if n == "total_connections_received":
                      # first run, zero out and record total connections
                      if metric_handler.prev_total_connections == 0:
                          metric_handler.prev_total_connections = int(v)
                          v = 0
                      else:
                          # calculate connections per second
                          cps = (int(v) - metric_handler.prev_total_connections) / (time.time() - metric_handler.timestamp)
                          metric_handler.prev_total_connections = int(v)
                          v = cps
                  if n == "total_commands_processed":
                      # first run, zero out and record total commands
                      if metric_handler.prev_total_commands == 0:
                          metric_handler.prev_total_commands = int(v)
                          v = 0
                      else:
                          # calculate commands per second
                          cps = (int(v) - metric_handler.prev_total_commands) / (time.time() - metric_handler.timestamp)
                          metric_handler.prev_total_commands = int(v)
                          v = cps
                  #logging.debug("submittincg metric %s is %s" % (n, int(v)))
                  metric_handler.info[n] = int(v)  # TODO Use value_type.
        except Exception, e:
            #logging.debug("caught exception %s" % e)
            pass
        s.close()
        metric_handler.timestamp = time.time()

    #logging.debug("returning metric_handl: %s %s %s" % (metric_handler.info.get(name, 0), metric_handler.info, metric_handler))
    return metric_handler.info.get(name, 0)


def metric_init(params={}):
    metric_handler.host = params.get("host", "127.0.0.1")
    metric_handler.port = int(params.get("port", 6379))
    metric_handler.auth = params.get("auth", None)
    metric_handler.timestamp = 0
    metric_handler.prev_total_commands = 0
    metric_handler.prev_total_connections = 0
    metrics = {
        "connected_clients": {"units": "clients"},
        "connected_slaves": {"units": "slaves"},
        "blocked_clients": {"units": "clients"},
        "used_memory": {"units": "KB"},
        "rdb_changes_since_last_save": {"units": "changes"},
        "rdb_bgsave_in_progress": {"units": "yes/no"},
        "master_sync_in_progress": {"units": "yes/no"},
        "master_link_status": {"units": "yes/no"},
        #"aof_bgrewriteaof_in_progress": {"units": "yes/no"},
        "total_connections_received": {"units": "connections/sec"},
        "instantaneous_ops_per_sec": {"units": "ops"},
        "total_commands_processed": {"units": "commands/sec"},
        "expired_keys": {"units": "keys"},
        "pubsub_channels": {"units": "channels"},
        "pubsub_patterns": {"units": "patterns"},
        #"vm_enabled": {"units": "yes/no"},
        "master_last_io_seconds_ago": {"units": "seconds ago"},
        "db0": {"units": "keys"},
    }
    metric_handler.descriptors = {}
    for name, updates in metrics.iteritems():
        descriptor = {
            "name": name,
            "call_back": metric_handler,
            "time_max": 90,
            "value_type": "int",
            "units": "",
            "slope": "both",
            "format": "%d",
            "description": "http://code.google.com/p/redis/wiki/InfoCommand",
            "groups": "redis",
        }
        descriptor.update(updates)
        metric_handler.descriptors[name] = descriptor
    return metric_handler.descriptors.values()


def metric_cleanup():
    pass
