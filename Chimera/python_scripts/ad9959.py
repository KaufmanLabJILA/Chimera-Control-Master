from axis_fifo import AXIS_FIFO
from devices import fifo_devices
import struct

class AD9959:
  """Class to control AD9959 in project KA012.
  Very simple class just used for testing.
  """

  def __init__(self, device=None):
    if device is None:
      self.fifo = AXIS_FIFO()
    else:
      self.fifo = AXIS_FIFO(device)

    #Function Register 1 (FR1)
    self.fifo.write_axis_fifo("\x00\x81\x00\xD0")
    self.fifo.write_axis_fifo("\x00\xC1\x00\x00")

    # initialize PLL multipliers to 20 (500 MHz clock), set VCO gain high 
    with open(fifo_devices['AD9959_0'], "r+b") as character:
      writeToDDS(character,1,0xD0000000)

    with open(fifo_devices['AD9959_1'], "r+b") as character:
      writeToDDS(character,1,0xD0000000)

    with open(fifo_devices['AD9959_2'], "r+b") as character:
      writeToDDS(character,1,0xD0000000)


def writeToDDS(dds_char_dev, address, data):
  data_bytes = struct.pack('<I', data)
  print(struct.unpack('<I', data_bytes))
  words = []
  #instruction and D16-D31
  words.append(data_bytes[2])
  words.append(data_bytes[3])
  words.append(struct.pack('<B', address + 128))
  words.append("\x00")
  #IOU and D0-D15
  words.append(data_bytes[0])
  words.append(data_bytes[1])
  words.append(struct.pack('<B', 192 + 1))
  words.append("\x00")

  for word in words:
    dds_char_dev.write(word)
