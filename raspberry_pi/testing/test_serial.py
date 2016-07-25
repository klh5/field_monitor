import serial
import sys

try:
	serial_conn = serial.Serial("/dev/ttyAMA0", 115200)
except OSError as e:
	print "ERROR: Could not open serial port: %s" % (e)
	sys.exit(1)	

while(True):

	packet_in = serial_conn.readline()
	print packet_in
