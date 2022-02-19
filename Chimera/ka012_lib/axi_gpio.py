import struct, sys
import binascii
from devices import gpio_devices
from mmap import mmap

class AXI_GPIO:
  """Class to allow writing to and reading from a AXI-GPIO.
  For more information see PG144 for AXI PGIO v2.0.
  
  The following register space is used:
  Offset    Name        Description
  0x0000    GPIO_DATA   Channel 1 AXI GPIO Data Register
  0x0004    GPIO_TRI    Channel 1 AXI GPIO 3-state Control Register
  0x0008    GPIO2_DATA  Channel 2 AXI GPIO Data Register
  0x000C    GPIO2_TRI   Channel 2 AXI GPIO 3-state Control Register
  
  No interrupts are used for now.
  """

  def __init__(self, device=None):
    self.devicebase = device
    region_size = 0xF
    if device is not None:
      self.dev = open("/dev/mem", "r+b")
      self.mem = mmap(self.dev.fileno(), region_size, offset=self.devicebase)

  def __del__(self):
    if self.devicebase is not None:
      self.dev.close()

  def write_axi_gpio(self, word, channel=1, MSB_first=True):
    """Write a 4-byte word to the AXI-GPIO on channel 1 (default) or 2.
    In this function, word can either be of type integer or a byte
    string.
    MSB_first is set such that the GPIO register will contain
    0x12345678 when word is 0x12345678
    without MSB_first set, the byte order is reversed and the register
    contains 0x78563412 instead."""
    assert channel==1 or channel==2, 'Invalid channel for AXI_GPIO'
    if isinstance(word, int):
      word = struct.pack('>I', word)
    if self.devicebase is not None:
      if MSB_first:
        word=word[::-1]
      self.mem.seek(0+8*(channel-1))
      self.mem.write(word)
    else:
      print('{0:#010x}     written to GPIO'.format(struct.unpack('>I',word)[0]))

  def set_bit(self, bit_number, channel=1, MSB_first=True):
    reg=self.read_axi_gpio(channel=channel)
    data_set=struct.unpack('>I', reg)[0]        #get int
    data_set=data_set | 1<<bit_number           #set bit
    self.write_axi_gpio(data_set,channel=channel)

  def clear_bit(self, bit_number, channel=1, MSB_first=True):
    reg=self.read_axi_gpio(channel=channel)
    data_clear=struct.unpack('>I', reg)[0]      #get int
    data_clear=data_clear & (~(1<<bit_number))  #clear bit
    self.write_axi_gpio(data_clear,channel=channel)

  def read_axi_gpio(self, channel=1, MSB_first=True):
    """Read a 4-byte word from the AXI-GPIO on channel 1 (default) or 2."""
    assert channel==1 or channel==2, 'Invalid channel for AXI_GPIO'
    if self.devicebase is not None:
      self.mem.seek(0+8*(channel-1))
      data = self.mem.read(4)
      if MSB_first:
        data=data[::-1] #reverse order
      return data
    else:
      print('AXI_GPIO: reading from dummy device - returning 0')
      return struct.pack('4B',0,0,0,0)

def print_usage():
  print("Usage: axi_gpio.py device channel value rw")
  print("  where device is from the following list")
  print("  and channel is either 1 or 2")
  print("  and value is a 4-byte hex-notation value")
  print("    it is ignored for read operations")
  print("  and rw is either r for a read or w for a write")
  print("")
  print("  Example values:")
  print("    0x00000000")
  print("    0xFFFFFFFF")
  print("    0xDEADBEEF")
  print("    0x12AA8844")
  print("")
  print("  Devices:")
  devs = []
  for item in gpio_devices:
    devs.append(item)
  devs.sort(key=lambda v: v.upper())
  for item in devs:
    print("    {}".format(item))

if __name__ == "__main__":
  if  (len(sys.argv) != 5):
    print("ERROR: Incorrect number of arguments\n")
    print_usage()
  elif not (sys.argv[4]=='r' or sys.argv[4]=='w'):
    print("ERROR: Invalid read/write flag\n")
    print_usage()
  elif (not sys.argv[3].startswith("0x")):
    print("ERROR: Invalid value\n")
    print_usage()
  elif not (sys.argv[2]=='1' or sys.argv[2]=='2'):
    print("ERROR: Invalid channel\n")
    print_usage()
  elif (not sys.argv[1] in gpio_devices):
    print("ERROR: Invalid device\n")
    print_usage()
  else:
    
    gpio = AXI_GPIO(gpio_devices[sys.argv[1]])
    if sys.argv[4]=='r':
      print("0x{}".format(binascii.hexlify(gpio.read_axi_gpio(channel=int(sys.argv[2])))))
    else:
      print("writing {}".format(sys.argv[3]))
      gpio.write_axi_gpio(struct.pack('>I', int(sys.argv[3], 0)),channel=int(sys.argv[2]))
