import socket
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = ("10.10.0.2", 8080)
print >>sys.stderr, 'starting up on %s port %s' % server_address
sock.bind(server_address)

# Listen for incoming connections
sock.listen(1)

while True:
    # Wait for a connection
    print >>sys.stderr, 'waiting for a connection'
    connection, client_address = sock.accept()

    try:
	    print >>sys.stderr, 'connection from', client_address

	    # Receive the data in small chunks and retransmit it
	    while True:
	        data = connection.recv(4)
	        print >>sys.stderr, 'received "%s"' % data
	        if data:
	            for byte in range(4):
					print 'byte ', byte, ':', format(int(binascii.hexlify(byte_buf[ii*12 + jj*4 + byte]), 16), '08b')
	        else:
	            print >>sys.stderr, 'no more data from', client_address
	            break
            
    finally:
        # Clean up the connection
        connection.close()