import socket
import sys

class zynq_tcp_client:
    def __init__(self):
        self.server_address = ("10.10.0.2", 8080)
        # Create a TCP/IP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    def connect(self):
        # print('connecting to {} port {}'.format(self.server_address[0], self.server_address[1]))
        self.sock.connect(self.server_address)

    def triggerGigamoogWithTweezersOn(self):
        try:
            dev_m = b'DIOseq_2'
            dev_m = dev_m.ljust(64, b'\0')
            # print('sending "{}"'.format(dev_m.decode()))
            self.sock.sendall(dev_m)

            bytes_m = b't000186A0_b0000000000000180'
            bytes_m = bytes_m.ljust(28, b'\0')
            # print('sending "{}"'.format(bytes_m.decode()))
            self.sock.sendall(bytes_m)

            bytes_m = b't000196A0_b0000000000000100'
            bytes_m = bytes_m.ljust(28, b'\0')
            # print('sending "{}"'.format(bytes_m.decode()))
            self.sock.sendall(bytes_m)

            end_m = b'end_0'
            end_m = end_m.ljust(64, b'\0')
            # print('sending "{}"'.format(end_m.decode()))
            self.sock.sendall(end_m)

            trigger_m = b'trigger'
            trigger_m = trigger_m.ljust(64, b'\0')
            # print('sending "{}"'.format(trigger_m.decode()))
            self.sock.sendall(trigger_m)
        except:
            print('write failed')

    def disconnect(self):

        # print('closing socket')
        self.sock.close()

if __name__ == "__main__":
    client = zynq_tcp_client()
    client.connect()
    client.triggerGigamoogWithTweezersOn()
    client.disconnect()
