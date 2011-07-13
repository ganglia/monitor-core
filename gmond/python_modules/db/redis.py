import socket
import time

def metric_handler(name):

    # Update from Redis.  Don't thrash.
    if 15 < time.time() - metric_handler.timestamp:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((metric_handler.host, metric_handler.port))
        s.send("INFO\n")
        info = s.recv(4096)
        if "$" != info[0]:
            return 0
        len = int(info[1:info.find("\n")])
        if 4096 < len:
            info += s.recv(len - 4096)
        metric_handler.info = {}
        for line in info.splitlines()[1:]:
            if "" == line:
                continue
            n, v = line.split(":")
            if n in metric_handler.descriptors:
                metric_handler.info[n] = int(v) # TODO Use value_type.
        s.close()
        metric_handler.timestamp = time.time()

    return metric_handler.info.get(name, 0)

def metric_init(params={}):
    metric_handler.host = params.get("host", "127.0.0.1")
    metric_handler.port = int(params.get("port", 6379))
    metric_handler.timestamp = 0
    metrics = {
        "connected_clients": {"units": "clients"},
        "connected_slaves": {"units": "slaves"},
        "blocked_clients": {"units": "clients"},
        "used_memory": {"units": "bytes"},
        "changes_since_last_save": {"units": "changes"},
        "bgsave_in_progress": {"units": "yes/no"},
        "bgrewriteaof_in_progress": {"units": "yes/no"},
        "total_connections_received": {
            "units": "connections",
            "slope": "positive",
        },
        "total_commands_processed": {
            "units": "commands",
            "slope": "positive",
        },
        "expired_keys": {"units": "keys"},
        "pubsub_channels": {"units": "channels"},
        "pubsub_patterns": {"units": "patterns"},
        "vm_enabled": {"units": "yes/no"},
        "master_last_io_seconds_ago": {"units": "seconds ago"},
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