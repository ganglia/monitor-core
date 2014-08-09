#!/usr/bin/python2

import socket
import json
import sys
import rrdtool

def main():
	print "Update settings:\n\tip:", sys.argv[1], "\n\tport:", sys.argv[2]
	ip = sys.argv[1]
	port = int(sys.argv[2])
	bufsize = 4096
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((ip, port))
	s.send("/status\r\n")
	data = s.recv(bufsize)
	s.close()
	# Making the Json: remove the header -> turn into a dictionary
	jsondata = json.loads(data.split('close\r\n\r\n')[1])
	# End
	# Updating rrd
	rrdtool.update('sent.rrd','N:' +
	`jsondata["metrics"]["sent"]["all"]["num"]` + ':' +
	`jsondata["metrics"]["sent"]["rrdtool"]["num"]` + ':' +
	`jsondata["metrics"]["sent"]["rrdcached"]["num"]` + ':' +
	`jsondata["metrics"]["sent"]["graphite"]["num"]` + ':' +
	`jsondata["metrics"]["sent"]["memcached"]["num"]` + ':' +
	`jsondata["metrics"]["sent"]["riemann"]["num"]`)
	rrdtool.update('sent_time.rrd','N:' +
	`jsondata["metrics"]["sent"]["all"]["totalMillis"]` + ':' +
	`jsondata["metrics"]["sent"]["rrdtool"]["totalMillis"]` + ':' +
	`jsondata["metrics"]["sent"]["rrdcached"]["totalMillis"]` + ':' +
	`jsondata["metrics"]["sent"]["graphite"]["totalMillis"]` + ':' +
	`jsondata["metrics"]["sent"]["memcached"]["totalMillis"]` + ':' +
	`jsondata["metrics"]["sent"]["riemann"]["totalMillis"]`)
	rrdtool.update('poll.rrd','N:' +
	`jsondata["metrics"]["polls"]["ok"]["num"]` + ':' +
	`jsondata["metrics"]["polls"]["failed"]["num"]` + ':' +
	`jsondata["metrics"]["polls"]["misses"]`)
	rrdtool.update('poll_time.rrd','N:' +
	`jsondata["metrics"]["polls"]["ok"]["totalMillis"]` + ':' +
	`jsondata["metrics"]["polls"]["failed"]["totalMillis"]`)
	rrdtool.update('sumrz.rrd','N:' +
	`jsondata["metrics"]["summarize"]["cluster"]` + ':' +
	`jsondata["metrics"]["summarize"]["root"]`)
	rrdtool.update('sumrz_time.rrd','N:' +
	`jsondata["metrics"]["summarize"]["totalMillis"]`)
	rrdtool.update('requests.rrd','N:' +
	`jsondata["metrics"]["requests"]["all"]["num"]` + ':' +
	`jsondata["metrics"]["requests"]["interactive"]["num"]` + ':' +
	`jsondata["metrics"]["requests"]["xml"]["num"]`)
	rrdtool.update('requests_time.rrd','N:' +
	`jsondata["metrics"]["requests"]["all"]["totalMillis"]` + ':' +
	`jsondata["metrics"]["requests"]["interactive"]["totalMillis"]` + ':' +
	`jsondata["metrics"]["requests"]["xml"]["totalMillis"]`)
	rrdtool.update('cpu.rrd','N:' +
	`jsondata["metrics"]["cpu"]["cpu_user"]` + ':' +
	`jsondata["metrics"]["cpu"]["cpu_nice"]` + ':' +
	`jsondata["metrics"]["cpu"]["cpu_system"]` + ':' +
	`jsondata["metrics"]["cpu"]["cpu_idle"]`)
	rrdtool.update('received.rrd','N:' +
	`jsondata["metrics"]["received"]["all"]`)
	# End updating rrd


if __name__ == "__main__":
	if len(sys.argv) < 3:
		print "Problem with arguments.\nUse:", sys.argv[0], "<ip> <port>"
	else:
		main()