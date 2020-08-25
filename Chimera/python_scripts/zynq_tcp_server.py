import socket
import errno
import sys
import binascii

import sequencer
from axis_fifo import AXIS_FIFO
from devices import fifo_devices
from devices import gpio_devices
from axi_gpio import AXI_GPIO

from dac81416 import DAC81416

class zynq_tcp_server:
	def __init__(self):
		self.seq = sequencer.sequencer()
		self.chimeraInterface()

	#Function that reads commands from Chimera and passes them to the axis-fifo interface of the Zynq FPGA
	def chimeraInterface(self):
		# Create a TCP/IP socket
		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

		# Bind the socket to the port
		server_address = ("10.10.0.2", 8080)
		print 'server starting up on %s port %s' % server_address
		sock.bind(server_address)

		# Listen for incoming connections
		sock.listen(1)

		while True:
		    # Wait for a connection
		    print 'waiting for a connection'
		    connection, client_address = sock.accept()

		    try:
			    print 'connection from', client_address

			    # Receive the data in small chunks and retransmit it
			    while True:
			        data = connection.recv(64)
			        print 'received "%s"' % data
			        print(len(data))
			        if data:
			            self.writeDevice(connection, data)
			            # connection.sendall(data)
			        else:
			            print 'no more data from', client_address
			            break
		    except socket.error as error:
		    	print error
		    	break

		connection.close()

	def writeDevice(self, conn, data):
		data_split = data.split('_')
		dev = data_split[0]
		print 'dev = ', dev
		if (dev == 'DIOseq'):
			self.writeDIOseq(conn, data_split)
		elif (dev == 'DAC0seq'):
			self.writeDACseq(conn, data_split)
		else:
			print 'no device selected'

	def writeDIOseq(self, conn, data_split):
		num_snapshots = int(data_split[1].strip('\0'))
		print 'num_bytes = ', 4*3*num_snapshots
		byte_buf = self.socket_read(conn, 4*3*num_snapshots) #each byte buffer snapshot consists of 3 sets of 4 bytes
		print hex(ord(byte_buf[0]))
		for ii in range(num_snapshots):
			print '\n', 'snapshot', ii
			for jj in range(3):
				print 'block', jj
				for byte in range(4):
					print 'byte ', byte, ':', format(int(binascii.hexlify(byte_buf[ii*12 + jj*4 + byte]), 16), '08b')
				self.seq.write_dio(byte_buf[ii*12 + jj*4 : ii*12 + jj*4 + 4])

	def socket_read(self, conn, expected):
	    """Read expected number of bytes from sock
	    Will repeatedly call recv until all expected data is received
	    """
	    buffer = b''
	    while len(buffer) < expected:
	        buffer += conn.recv(expected - len(buffer))
	    return buffer

if __name__ == "__main__":
	server = zynq_tcp_server()