from axis_fifo import AXIS_FIFO
from devices import fifo_devices
import struct
import binascii

def writeToDDS(dds_char_dev, address, data):
  data_bytes = struct.pack('<I', data)
  # print(struct.unpack('<I', data_bytes))
  words = []
  #instruction and D16-D31, D31 is MSB zzp
  words.append(data_bytes[2])
  words.append(data_bytes[3])
  words.append(struct.pack('<B', address + 128))
  words.append(b'\x00')
  #IOU and D0-D15
  words.append(data_bytes[0])
  words.append(data_bytes[1])
  words.append(struct.pack('<B', 192 + 1))
  words.append(b'\x00')
  print '0x'+binascii.hexlify(''.join(words[3:None:-1])), \
        ' 0x'+binascii.hexlify(''.join(words[-1:3:-1]))
  for word in words:
    dds_char_dev.write(word)

class AD9959:
  """Class to control AD9959 in project KA012.
  Very simple class just used for testing.
  """

  def __init__(self, device=None):
    self.device = device
    if device is None:
      self.fifo = AXIS_FIFO()
    else:
      self.fifo = AXIS_FIFO(device)

  def set_DDS(self, channel, freq, amp = None):
    assert channel>=0 and channel<=3, 'Invalid channel for AD9959 in set_DDS'
    with open(self.device, "r+b") as character:
      # ~ writeWords(character)
      freqConv = int((2**32 - 1)*freq/500)
      if amp is None:
        amp=10
      ampConv  = (1<<(12+8)) | (int((2**10-1)*amp/10)<<8)
      # chanConv = int(2**channel)
      chanConv = 1<<channel<<28 #(2**channel)*268435456
      writeToDDS(character,0,chanConv)
      writeToDDS(character,4,freqConv)
      writeToDDS(character,6,ampConv)
      writeToDDS(character,0,0x00000000)





if __name__ == "__main__":
  from dds_lock_pll import dds_lock_pll 
  dds = AD9959(fifo_devices['AD9959_0'])
  freq = 85
  chan = 1
  dds_lock_pll()
  dds.set_DDS(chan, freq)
