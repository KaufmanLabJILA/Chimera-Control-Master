from axis_fifo import AXIS_FIFO
from devices import fifo_devices
import struct

class GPIO_seq_point:
  def __init__(self, address, time, output):
    self.address = address
    self.time = time
    self.output = output

def write_point(fifo, point):
  #01XXAAAA TTTTTTTT DDDDDDDD
  fifo.write_axis_fifo("\x01\x00" + struct.pack('>H', point.address))
  fifo.write_axis_fifo(struct.pack('>I', point.time))
  fifo.write_axis_fifo(struct.pack('>I', point.output))

def program():
  fifo = AXIS_FIFO(fifo_devices['GPIO_seq'])
  points=[]
  points.append(GPIO_seq_point(address=0,time=   0,output=0xFFFFFFFF))
  points.append(GPIO_seq_point(address=1,time=10000,output=0x00000000))
  points.append(GPIO_seq_point(address=2,time=20000,output=0xFFFFFFFF))
  points.append(GPIO_seq_point(address=3,time=30000,output=0x00000000))
  points.append(GPIO_seq_point(address=4,time=   0,output=0x00000000))

  for point in points:
    write_point(fifo, point)

if __name__ == "__main__":
  program()
