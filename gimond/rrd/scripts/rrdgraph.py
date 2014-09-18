#!/usr/bin/python2

import sys
import rrdtool

def main():
	rrdtool.graph( "sent.png", "--start", "-1d", "--vertical-label=Sent",
	"DEF:all=sent.rrd:all:AVERAGE",
	"DEF:rrdtool=sent.rrd:rrdtool:AVERAGE",
	"DEF:rrdcached=sent.rrd:rrdcached:AVERAGE",
	"DEF:graphite=sent.rrd:graphite:AVERAGE",
	"DEF:memcached=sent.rrd:memcached:AVERAGE",
	"DEF:riemann=sent.rrd:riemann:AVERAGE",
	"LINE1:all#CC0000:All",
	"LINE2:rrdtool#FF3300:RRDtool",
	"LINE3:rrdcached#FFFF00:RRDcached",
	"LINE4:graphite#00CC00:Graphite",
	"LINE5:memcached#0000FF:Memcahed",
	"LINE6:riemann#CC0099:Riemann\\r")
	rrdtool.graph( "sent_time.png", "--start", "-1d", "--vertical-label=Sent Time",
	"DEF:all=sent_time.rrd:all:AVERAGE",
	"DEF:rrdtool=sent_time.rrd:rrdtool:AVERAGE",
	"DEF:rrdcached=sent_time.rrd:rrdcached:AVERAGE",
	"DEF:graphite=sent_time.rrd:graphite:AVERAGE",
	"DEF:memcached=sent_time.rrd:memcached:AVERAGE",
	"DEF:riemann=sent_time.rrd:riemann:AVERAGE",
	"LINE1:all#CC0000:All",
	"LINE2:rrdtool#FF3300:RRDtool",
	"LINE3:rrdcached#FFFF00:RRDcached",
	"LINE4:graphite#00CC00:Graphite",
	"LINE5:memcached#0000FF:Memcahed",
	"LINE6:riemann#CC0099:Riemann\\r")
	rrdtool.graph( "poll.png", "--start", "-1d", "--vertical-label=Polls",
	"DEF:ok=poll.rrd:ok:AVERAGE",
	"DEF:failed=poll.rrd:failed:AVERAGE",
	"DEF:misses=poll.rrd:misses:AVERAGE",
	"LINE1:ok#CC0000:All",
	"LINE2:failed#FF3300:RRDtool",
	"LINE3:misses#FFFF00:RRDcached\\r")
	rrdtool.graph( "poll_time.png", "--start", "-1d", "--vertical-label=Polls Time",
	"DEF:ok=poll_time.rrd:ok:AVERAGE",
	"DEF:failed=poll_time.rrd:failed:AVERAGE",
	"LINE1:ok#CC0000:Ok",
	"LINE2:failed#FF3300:Failed\\r")
	rrdtool.graph( "sumrz.png", "--start", "-1d", "--vertical-label=Summarizations",
	"DEF:cluster=sumrz.rrd:cluster:AVERAGE",
	"DEF:root=sumrz.rrd:root:AVERAGE",
	"LINE1:cluster#CC0000:Cluster",
	"LINE2:root#FF3300:Root\\r")
	rrdtool.graph( "sumrz_time.png", "--start", "-1d", "--vertical-label=Summarizations Time",
	"DEF:all=sumrz_time.rrd:all:AVERAGE",
	"LINE1:all#CC0000:All\\r")
	rrdtool.graph( "requests.png", "--start", "-1d", "--vertical-label=Requests",
	"DEF:all=requests.rrd:all:AVERAGE",
	"DEF:interactive=requests.rrd:interactive:AVERAGE",
	"DEF:xml=requests.rrd:xml:AVERAGE",
	"LINE1:all#CC0000:All",
	"LINE2:interactive#FF3300:Interactive",
	"LINE3:xml#FFFF00:Xml\\r")
	rrdtool.graph( "requests_time.png", "--start", "-1d", "--vertical-label=Requests Time",
	"DEF:all=requests_time.rrd:all:AVERAGE",
	"DEF:interactive=requests_time.rrd:interactive:AVERAGE",
	"DEF:xml=requests_time.rrd:xml:AVERAGE",
	"LINE1:all#CC0000:All",
	"LINE2:interactive#FF3300:Interactive",
	"LINE3:xml#FFFF00:Xml\\r")
	rrdtool.graph( "cpu.png", "--start", "-1d", "--vertical-label=Cpu",
	"DEF:user=cpu.rrd:user:AVERAGE",
	"DEF:nice=cpu.rrd:nice:AVERAGE",
	"DEF:system=cpu.rrd:system:AVERAGE",
	"DEF:idle=cpu.rrd:idle:AVERAGE",
	"LINE1:user#CC0000:User",
	"LINE2:nice#FF3300:Nice",
	"LINE3:system#FFFF00:System",
	"LINE4:idle#00CC00:Idle\\r")
	rrdtool.graph( "received.png", "--start", "-1d", "--vertical-label=Received",
	"DEF:metrics=received.rrd:metrics:AVERAGE",
	"LINE1:metrics#CC0000:Metrics\\r")


if __name__ == "__main__":
	main()
