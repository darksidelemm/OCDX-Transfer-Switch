#	OCDX Relay Control GUI
#	Mark Jessop 2016 <vk5qi@rfhead.net>
#
#	On Windows,Installing Anaconda Python (http://continuum.io/downloads) will give you the necessary dependencies.
# 	Otherwise, you need PyQt4.


import sys, os, datetime, httplib
from PyQt4 import QtGui, QtCore

# IP Address of the ESP8266. 
# In this case it's getting an IP via DHCP.
esp_host = "10.106.14.100"

# Relay state array
# Currently only have controls for 3 relays.
# These correspond to pins 4, 13 and 12 on the "ESP8266 Thing" PCB.
relay_states = [0,0,0]
antenna_state = -1

# Transfer Switch State


# PyQtGraph Window Setup
app = QtGui.QApplication([])


# Create Widgets
# Text field to put some basic info into.
relayDescriptionLabel = QtGui.QLabel("Waiting for first update...")
relayDescriptionLabel.setWordWrap(True)
# Buttons
antenna0btn = QtGui.QPushButton("Antenna A")
antenna1btn = QtGui.QPushButton("Antenna B")
relay2btn = QtGui.QPushButton("Spare Relay")
# Make all buttons default to red.
antenna0btn.setStyleSheet("background-color: red");
antenna1btn.setStyleSheet("background-color: red");
relay2btn.setStyleSheet("background-color: red");


# Update the button colors based on the relay state array, as defined above.
def updateButtons():
	if antenna_state == 0:
		antenna0btn.setStyleSheet("background-color: green")
		antenna1btn.setStyleSheet("background-color: red")
	elif antenna_state == 1:
		antenna0btn.setStyleSheet("background-color: red")
		antenna1btn.setStyleSheet("background-color: green")
	else:
		antenna0btn.setStyleSheet("background-color: red")
		antenna1btn.setStyleSheet("background-color: red")

	if relay_states[2] == 0:
		relay2btn.setStyleSheet("background-color: red")
	else:
		relay2btn.setStyleSheet("background-color: green")

# Attempt to read the status of the relays and RSSI from the ESP8266 board.
def read_status():
	global antenna_state, relay_states
	try:
		conn = httplib.HTTPConnection(esp_host,timeout=3)
		conn.request("GET","/read")
		data = conn.getresponse().read()
		relayDescriptionLabel.setText(data)
		conn.close()
		relay_data = data.split("\n")[2]
		antenna_data = data.split("\n")[3]
		rssi_data = data.split("\n")[4]
		analog_data = data.split("\n")[5]
		if "RELAYS" in relay_data:
			relay_data_states = relay_data.split(",")
			relay_states[0] = int(relay_data_states[1])
			relay_states[1] = int(relay_data_states[2])
			relay_states[2] = int(relay_data_states[3])

		if "ANTENNA" in antenna_data:
			antenna_state = int(antenna_data.split(",")[1])

		updateButtons()
	except:
		# TODO: Make this a bit more descriptive...
		relayDescriptionLabel.setText("Communication Error!")

# Toggle the state of a relay.
def toggle_state(relay_number):
	curr_state = relay_states[relay_number]
	new_state = 0 if curr_state else 1 # Invert state

	try:
		request_str = "/relay%d/%d" % (relay_number,new_state)
		conn = httplib.HTTPConnection(esp_host,timeout=1)
		conn.request("GET",request_str)
		data = conn.getresponse().read()
		relayDescriptionLabel.setText(data)
		conn.close()
		if "STATE" in data:
			relay_states[relay_number] = new_state
			return
	except:
		relayDescriptionLabel.setText("Communication Error!")

# Select a different antenna state
def select_antenna(antenna_number):

	try:
		request_str = "/antenna/%d" % (antenna_number)
		conn = httplib.HTTPConnection(esp_host,timeout=1)
		conn.request("GET",request_str)
		data = conn.getresponse().read()
		relayDescriptionLabel.setText(data)
		conn.close()
		if "ANTENNA" in data:
			antenna_state = antenna_number
			return
	except:
		relayDescriptionLabel.setText("Communication Error!")

# Callback functions for each of the GUI buttons.
def antenna0btnClicked():
	select_antenna(0)
	read_status()

antenna0btn.clicked.connect(antenna0btnClicked)

def antenna1btnClicked():
	select_antenna(1)
	read_status()

antenna1btn.clicked.connect(antenna1btnClicked)

def relay2btnClicked():
	toggle_state(2)
	read_status()

relay2btn.clicked.connect(relay2btnClicked)

# Create and Lay-out window
win = QtGui.QWidget()
win.resize(500,200)
win.show()
win.setWindowTitle("OCDX 40m Antenna Controller")
layout = QtGui.QGridLayout()
win.setLayout(layout)
# Add Widgets
layout.addWidget(antenna0btn,0,0)
layout.addWidget(antenna1btn,0,1)
layout.addWidget(relay2btn,0,2)
layout.addWidget(relayDescriptionLabel,1,0,1,3)

# Start a timer to attempt to read the remote station status every 5 seconds.
timer = QtCore.QTimer()
timer.timeout.connect(read_status)
timer.start(5000)

## Start Qt event loop unless running in interactive mode or using pyside.
if __name__ == '__main__':
	if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
		QtGui.QApplication.instance().exec_()


