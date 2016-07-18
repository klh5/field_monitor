import os
import picamera
import time

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

	os.system("sudo shutdown -h now")

except PiCameraError:
	print "ERROR: Something went wrong with the camera\n"




