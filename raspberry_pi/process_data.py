#!/usr/bin/python

import serial
import time
import sys
import csv
import picamera
import os
import netifaces

"""
process_data.py	-	Python 2.7 script which runs on the Raspberry Pi. It takes the output from the receiver and saves it to a file for 		analysis later on.

									It also captures an image from the camera and saves it.
"""

#Packets are in this format:
#SENDER_ID NETWORK_ID VOLTAGE LIGHT_1...LIGHT_8 RSSI

#This is the name of the data output file. Change this to change where data gets saved to. It needs to be the full path to the file, including the file name.
data_file_name = "/home/pi/data/data.csv"

#This is the path where images should be saved. Images are automatically named with a timestamp, but you need to provide the path to where they should be saved. As with the data file this should be the full path.
image_file_name = "/home/pi/data/images/"

#This is the path to the log file. Change this to change where general output and errors are logged to.
log_file = open("/home/pi/data/log.txt", "ab") 

#Set up a timestamp variable. The controller will send a timestamp before data packets start arriving. If no timestamp is received, data wont be recorded.
timestamp = None

#Expected length of packet, once it has been split into seperate readings
len_packet = 12 		

#Check for an ethernet connection
addr = netifaces.ifaddresses("eth0")

try:

	#Try and set up the serial connection to the controller.
	serial_conn = serial.Serial("/dev/ttyAMA0", 115200)

	while(True):
	
		#Read data from the serial port
		packet = serial_conn.readline()

		#Split the packet up by whitespace
		data = packet.split()
			
		#If the packet is a data packet, process it
		if len(data) == len_packet:

			#Only record data to CSV file if there is a time to stamp it with
			if timestamp is not None:
		
				#Write to the file name stored in file_name
				with open(data_file_name, "ab") as csv_file:
					file_writer = csv.writer(csv_file, delimiter = ",")
					file_writer.writerow([timestamp, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]])
	
		#"T" indicates a timestamp packet. 
		elif "T" in packet:
			
			#Tidy up the timestamp and save it (we don't need to keep the "T" in file names)
			timestamp = packet.rstrip().replace("T", "")

		#"E" indicates the end of data transmission. The controller will only send this after data has been received from all loggers
		elif "E" in packet:
		
			#Only record an image if there is a time to stamp it with
			if timestamp is not None:

				#Try and create a camera object
				camera = picamera.PiCamera()
		
				#Build camera image path	
				camera_path = image_file_name + "/" + timestamp + ".jpg"

				#Capture an image from the camera
				camera.capture(camera_path)

				#Close the camera object
				camera.close()

				break

		#If the packet is not recognisable, just print it. Unrecognised packets should still be logged in case there is any garbled data
		else:
			log_unknown = "Received unknown packet: " + packet + "\n"
			log_file.write(log_unknown)

#No serial connection was made
except OSError:

	log_file.write("Could not open serial connection.\n")

#The camera couldn't be set up
except picamera.PiCameraError:

	log_file.write("Could not set up camera.\n")

#This happens whether anything goes wrong or not
finally:

	#If the ethernet connection has an IP address assigned, the Pi is communicating with the computer, so don't shut down
	if not netifaces.AF_INET in addr:

		#Shut down the Raspberry Pi to save power
		os.system("sudo shutdown -h now")





