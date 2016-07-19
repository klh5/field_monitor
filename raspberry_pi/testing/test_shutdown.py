import os
import picamera
import time
import netifaces

output_file = open("/home/pi/scripts/output.txt", "ab") 

output_file.write("Booted\n")
output_file.write("Setting up camera...\n")

try:
	camera = picamera.PiCamera()

	output_file.write("Camera setup done. Taking picture...\n")

	timestamp = time.strftime("%Y-%m-%d_%H:%M:%S")
	
	image_file_name = "/home/pi/scripts/" + timestamp + ".jpg"

	camera.capture(image_file_name)

	output_file.write("Shutting down now...\n")

	#Check for an ethernet connection
	addr = netifaces.ifaddresses("eth0")

	#If the ethernet connection has an IP address assigned, the Pi is communicating with the computer, so don't shut down
	if not netifaces.AF_INET in addr:

		#Shut down the Raspberry Pi to save power
		os.system("sudo shutdown -h now")

except picamera.PiCameraError:
	print "ERROR: Something went wrong with the camera\n"




