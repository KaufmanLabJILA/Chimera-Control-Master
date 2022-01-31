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