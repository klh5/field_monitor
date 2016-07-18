#!/usr/bin/python

import serial
import time
import sys
import csv
import picamera
import os

"""
process_data.py	-	Python 2.7 script which runs on the Raspberry Pi. It takes the output from the receiver and saves it to a file 				for analysis later on.

			It also captures an image from the camera and saves it.
"""

#This is the name of the data output file. Change this to change where data gets saved to. It needs to be the full path to the file, including the file name.
data_file_name = "/home/pi/field_sensors/data_out.txt"

#This is the path where images should be saved. Images are automatically named with a timestamp, but you need to provide the path to where they should be saved. As with the data file this should be the full path.
image_file_name = "/home/pi/field_sensors/images"

#Packets are in this format:
#SENDER_ID NETWORK_ID VOLTAGE LIGHT_1...LIGHT_8 RSSI

#Expected length of packet, once it has been split into seperate readings
len_packet = 12 		

#Create a camera object
try:
	camera = picamera.PiCamera()
except PiCameraError:
	print "ERROR: Something went wrong with the camera"

	
#The receiver should be attached to a serial port. This code tries to open the port so that it can speak to the receiver
try:
	serial_conn = serial.Serial("/dev/ttyAMA0", 115200)
except OSError as e:
	print "ERROR: Could not open serial port: %s" % (e)
	sys.exit(1)	

while(True):
	
	#Read data from the serial port
	packet = serial_conn.readline()

	#Split the packet up by whitespace
	data = packet.split()
			
	#If the packet is a data packet, process it
	if len(data) == len_packet:
		
		#Fetch the time			
		date_time = time.strftime("%Y-%m-%d %H:%M:%S")

		#Print the time and the packet, for logging purposes
		print date_time +  " " + packet,
		
		#Write to the file name stored in file_name
		with open(file_name, "ab") as csv_file:
			file_writer = csv.writer(csv_file, delimiter = ",")
			file_writer.writerow([date_time, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]])
	
	elif "T" in packet:
		
		#Build camera image path	
		camera_path = image_file_name + "/" + packet + ".jpg"

		#Capture an image from the camera
		camera.capture(camera_path)
					
	#If the packet is not recognisable, just print it. Unrecognised packets should still be logged in case there is any garbled data
	else:
		print "Received unknown packet: %s" % (packet),

#Shut down the Raspberry Pi to save power
os.system("sudo shutdown -h now")



