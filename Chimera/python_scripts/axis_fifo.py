import struct, sys
import array
from devices import fifo_devices

class AXIS_FIFO:
  """Class to allow writing to a AXIS-FIFO.
  For details about the driver, look at the axis-fifo
  driver in the drivers/staging directory of the linux kernel"""

  def __init__(self, device=None):
    self.devicename = device
    if device is not None:
      self.dev = open(device, "r+b")

  def __del__(self):
    if self.devicename is not None:
      self.dev.close()

  def write_axis_fifo(self, word, MSB_first=True):
    """Write a 4-byte word to the AXIS-FIFO.
    In this function, word can either be of type integer or a byte
    string.
    MSB_first is set such that the FPGA register will contain
    0x12345678 when word is 0x12345678
    without MSB_first set, the byte order is reversed and the register
    contains 0x78563412 instead."""
    if isinstance(word, int):
      word = struct.pack('>I', word)
    if MSB_first:
      word=word[::-1]
    if self.devicename is not None:
      self.dev.write(word)
      self.dev.flush()
    else:
      txt = ''
      invword=word[::-1]
      for char in invword:
        txt = txt + "{0:#04x}".format(ord(char))[2:]
      print('0x' + txt + '     written to FIFO')

def print_usage():
  print("Usage: axis_fifo.py device value")
  print("  where device is from the following list")
  print("  and value is a 4-byte hex-notation value")
  print("")
  print("  Example values:")
  print("    0x00000000")
  print("    0xFFFFFFFF")
  print("    0xDEADBEEF")
  print("    0x12AA8844")
  print("")
  print("  Devices:")
  devs = []
  for item in fifo_devices:
    devs.append(item)
  devs.sort(key=lambda v: v.upper())
  for item in devs:
    print("    {}".format(item))

if __name__ == "__main__":
  print(sys.argv[2])
  if  (len(sys.argv) != 3):
    print_usage()
  elif (not sys.argv[2].startswith("0x")):
    print("ERROR: Invalid value\n")
    print_usage()
  elif (not sys.argv[1] in fifo_devices):
    print("ERROR: Invalid device\n")
    print_usage()
  else:
    print("writing {}".format(sys.argv[2]))
    fifo = AXIS_FIFO(fifo_devices[sys.argv[1]])
    fifo.write_axis_fifo(sys.argv[2])
