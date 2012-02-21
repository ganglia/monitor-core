import re
import time

PARAMS = {}

METRICS = {
    'time' : 0,
    'data' : {}
}

tcpext_file = "/proc/net/netstat"
snmp_file = "/proc/net/snmp"

LAST_METRICS = dict(METRICS)
METRICS_CACHE_MAX = 5

stats_pos = {}

stats_pos['tcpext'] = {
    'syncookiessent' : 1,
    'syncookiesrecv' : 2,
    'syncookiesfailed' : 3,
    'embryonicrsts' : 4,
    'prunecalled' : 5,
    'rcvpruned' : 6,
    'ofopruned' : 7,
    'outofwindowicmps' : 8,
    'lockdroppedicmps' : 9,
    'arpfilter' : 10,
    'tw' : 11,
    'twrecycled' : 12,
    'twkilled' : 13,
    'pawspassive' : 14,
    'pawsactive' : 15,
    'pawsestab' : 16,
    'delayedacks' : 17,
    'delayedacklocked' : 18,
    'delayedacklost' : 19,
    'listenoverflows' : 20,
    'listendrops' : 21,
    'tcpprequeued' : 22,
    'tcpdirectcopyfrombacklog' : 23,
    'tcpdirectcopyfromprequeue' : 24,
    'tcpprequeuedropped' : 25,
    'tcphphits' : 26,
    'tcphphitstouser' : 27,
    'tcppureacks' : 28,
    'tcphpacks' : 29,
    'tcprenorecovery' : 30,
    'tcpsackrecovery' : 31,
    'tcpsackreneging' : 32,
    'tcpfackreorder' : 33,
    'tcpsackreorder' : 34,
    'tcprenoreorder' : 35,
    'tcptsreorder' : 36,
    'tcpfullundo' : 37,
    'tcppartialundo' : 38,
    'tcpdsackundo' : 39,
    'tcplossundo' : 40,
    'tcploss' : 41,
    'tcplostretransmit' : 42,
    'tcprenofailures' : 43,
    'tcpsackfailures' : 44,
    'tcplossfailures' : 45,
    'tcpfastretrans' : 46,
    'tcpforwardretrans' : 47,
    'tcpslowstartretrans' : 48,
    'tcptimeouts' : 49,
    'tcprenorecoveryfail' : 50,
    'tcpsackrecoveryfail' : 51,
    'tcpschedulerfailed' : 52,
    'tcprcvcollapsed' : 53,
    'tcpdsackoldsent' : 54,
    'tcpdsackofosent' : 55,
    'tcpdsackrecv' : 56,
    'tcpdsackoforecv' : 57,
    'tcpabortonsyn' : 58,
    'tcpabortondata' : 59,
    'tcpabortonclose' : 60,
    'tcpabortonmemory' : 61,
    'tcpabortontimeout' : 62,
    'tcpabortonlinger' : 63,
    'tcpabortfailed' : 64,
    'tcpmemorypressures' : 65,
    'tcpsackdiscard' : 66,
    'tcpdsackignoredold' : 67,
    'tcpdsackignorednoundo' : 68,
    'tcpspuriousrtos' : 69,
    'tcpmd5notfound' : 70,
    'tcpmd5unexpected' : 71,
    'tcpsackshifted' : 72,
    'tcpsackmerged' : 73,
    'tcpsackshiftfallback' : 74,
    'tcpbacklogdrop' : 75,
    'tcpminttldrop' : 76,
    'tcpdeferacceptdrop' : 77
}

stats_pos['ip'] = {
    'inreceives': 3,
    'inhdrerrors': 4,
    'inaddrerrors': 5,
    'forwdatagrams': 6,
    'inunknownprotos': 7,
    'indiscards': 8,
    'indelivers': 9,
    'outrequests': 10,
    'outdiscards': 11,
    'outnoroutes': 12,
    'reasmtimeout': 13,
    'reasmreqds': 14,
    'reasmoks': 15,
    'reasmfails': 16,
    'fragoks': 17,
    'fragfails': 18,
    'fragcreates': 19
}
 
stats_pos['tcp'] = {
    'activeopens': 5,
    'passiveopens': 6,
    'attemptfails': 7,
    'estabresets': 8,
    'currestab': 9,
    'insegs': 10,
    'outsegs': 11,
    'retranssegs': 12,
    'inerrs': 13,
    'outrsts': 14
}

stats_pos['udp'] = {
    'indatagrams': 1,
    'noports': 2,
    'inerrors': 3,
    'outdatagrams': 4,
    'rcvbuferrors': 5,
    'sndbuferrors': 6,
}

stats_pos['icmp'] = {
    'inmsgs': 1,
    'inerrors': 2,
    'indestunreachs': 3,
    'intimeexcds': 4,
    'inparmprobs': 5,
    'insrcquenchs': 6,
    'inredirects': 7,
    'inechos': 8,
    'inechoreps': 9,
    'intimestamps': 10,
    'intimestampreps': 11,
    'inaddrmasks': 12,
    'inaddrmaskreps': 13,
    'outmsgs': 14,
    'outerrors': 15,
    'outdestunreachs': 16,
    'outtimeexcds': 17,
    'outparmprobs': 18,
    'outsrcquenchs': 19,
    'outredirects': 20,
    'outechos': 21,
    'outechoreps': 22,
    'outtimestamps': 23,
    'outtimestampreps': 24,
    'outaddrmasks': 25,
    'outaddrmaskreps': 26
}

def get_metrics():
    """Return all metrics"""

    global METRICS, LAST_METRICS

    if (time.time() - METRICS['time']) > METRICS_CACHE_MAX:

        try:
            mfile = open(tcpext_file, 'r')

        except IOError:
            return 0

        # convert to dict
        metrics = {}
        for line in mfile:
            if re.match("TcpExt: [0-9]", line):
                metrics = re.split("\s+", line)

        mfile.close()

        # update cache
        LAST_METRICS = dict(METRICS)
        METRICS = {
            'time': time.time(),
            'tcpext': metrics
        }

        ######################################################################
        # Let's get stats from /proc/net/snmp
        ######################################################################
        try:
            mfile = open(snmp_file, 'r')

        except IOError:
            return 0

        for line in mfile:
            if re.match("Udp: [0-9]", line):
                METRICS['udp'] = re.split("\s+", line)
            if re.match("Ip: [0-9]", line):
                METRICS['ip'] = re.split("\s+", line)
            if re.match("Tcp: [0-9]", line):
                METRICS['tcp'] = re.split("\s+", line)

        mfile.close()

    return [METRICS, LAST_METRICS]

def get_delta(name):
    """Return change over time for the requested metric"""

    # get metrics
    [curr_metrics, last_metrics] = get_metrics()

    parts = name.split("_")
    group = parts[0]
    index = stats_pos[group][parts[1]]

    try:
        delta = (float(curr_metrics[group][index]) - float(last_metrics[group][index])) /(curr_metrics['time'] - last_metrics['time'])
        if delta < 0:
            print name + " is less 0"
            delta = 0
    except KeyError:
        delta = 0.0

    return delta

def get_tcploss_percentage(name):

    # get metrics
    [curr_metrics, last_metrics] = get_metrics()

    index_outsegs = stats_pos["tcp"]["outsegs"]
    index_insegs = stats_pos["tcp"]["insegs"]
    index_loss = stats_pos["tcpext"]["tcploss"]

    try:
        pct = 100 * (float(curr_metrics['tcpext'][index_loss]) - float(last_metrics['tcpext'][index_loss])) / (float(curr_metrics['tcp'][index_outsegs]) +  float(curr_metrics['tcp'][index_insegs]) - float(last_metrics['tcp'][index_insegs]) - float(last_metrics['tcp'][index_outsegs]))
        if pct < 0:
            print name + " is less 0"
            pct = 0
    except (KeyError, ZeroDivisionError):
        pct = 0.0

    return pct

def get_retrans_percentage(name):

    # get metrics
    [curr_metrics, last_metrics] = get_metrics()

    index_outsegs = stats_pos["tcp"]["outsegs"]
    index_insegs = stats_pos["tcp"]["insegs"]
    index_retrans = stats_pos["tcp"]["retranssegs"]

    try:
        pct = 100 * (float(curr_metrics['tcp'][index_retrans]) - float(last_metrics['tcp'][index_retrans])) / (float(curr_metrics['tcp'][index_outsegs]) +  float(curr_metrics['tcp'][index_insegs]) - float(last_metrics['tcp'][index_insegs]) - float(last_metrics['tcp'][index_outsegs]))
        if pct < 0:
            print name + " is less 0"
            pct = 0
    except (KeyError, ZeroDivisionError):
        pct = 0.0

    return pct

def create_desc(skel, prop):
    d = skel.copy()
    for k, v in prop.iteritems():
        d[k] = v
    return d

def metric_init(params):
    global descriptors

    descriptors = []

    Desc_Skel = {
        'name'        : 'XXX',
        'call_back'   : get_delta,
        'time_max'    : 60,
        'value_type'  : 'float',
        'format'      : '%.5f',
        'units'       : 'count/s',
        'slope'       : 'both', # zero|positive|negative|both
        'description' : 'XXX',
        'groups'      : 'XXX',
    }

    for group in stats_pos:
        for item in stats_pos[group]:
            descriptors.append(create_desc(Desc_Skel, {
                "name"       : group + "_" + item,
                "description": item,
                'groups'     : group
            }))

    descriptors.append(create_desc(Desc_Skel, {
        "name"       : "tcpext_" + "tcploss_percentage",
        "call_back"  : get_tcploss_percentage,
        "description": "TCP percentage loss, tcploss / insegs + outsegs",
        "units"      : "pct",
        'groups'     : 'tcpext'
    }))

    descriptors.append(create_desc(Desc_Skel, {
        "name"       : "tcp_" + "retrans_percentage",
        "call_back"  : get_retrans_percentage,
        "description": "TCP retrans percentage, retranssegs / insegs + outsegs",
        "units"      : "pct",
        'groups'     : 'tcp'
    }))

    return descriptors

def metric_cleanup():
    '''Clean up the metric module.'''
    pass

#This code is for debugging and unit testing
if __name__ == '__main__':
    descriptors = metric_init(PARAMS)
    while True:
        for d in descriptors:
            v = d['call_back'](d['name'])
            print '%s = %s' % (d['name'],  v)
        print 'Sleeping 15 seconds'
        time.sleep(15)
