import socket
import sys

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect the socket to the port where the server is listening
server_address = ("10.10.0.2", 8080)
print('connecting to {} port {}'.format(server_address[0], server_address[1]))
sock.connect(server_address)

try:
    
    # Send data
    message = b'This is the message.  It will be repeated.'
    print('sending "{}"'.format(message.decode()))
    sock.sendall(message)

    # Look for the response
    amount_received = 0
    amount_expected = len(message)
    
    while amount_received < amount_expected:
        data = sock.recv(80)
        amount_received += len(data)
        print('received "{}"'.format(data))

finally:
    print('closing socket')
    sock.close()