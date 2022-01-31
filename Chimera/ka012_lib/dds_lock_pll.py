#!/usr/bin/python

import struct

def writeToDDS(dds_char_dev, address, data):
  data_bytes = struct.pack('<I', data)
  # print(struct.unpack('<I', data_bytes))
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

def dds_lock_pll():
  print("writing to 5000")
  with open("/dev/axis_fifo_0x0000000080005000", "r+b") as character:
    writeToDDS(character,1,0xD0000000)#PLL mult 20, VCO gain high (for 25MHz crystal, 500 MHz clock)

  print("writing to 6000")
  with open("/dev/axis_fifo_0x0000000080006000", "r+b") as character:
    writeToDDS(character,1,0xD0000000)#PLL mult 20, VCO gain high (for 25MHz crystal, 500 MHz clock)

  print("writing to 7000")
  with open("/dev/axis_fifo_0x0000000080007000", "r+b") as character:
    writeToDDS(character,1,0xD0000000)#PLL mult 20, VCO gain high (for 25MHz crystal, 500 MHz clock)

if __name__ == "__main__":
  print("writing to 5000")
  with open("/dev/axis_fifo_0x0000000080005000", "r+b") as character:
    writeToDDS(character,1,0xD0000000)#PLL mult 20, VCO gain high (for 25MHz crystal, 500 MHz clock)

  print("writing to 6000")
  with open("/dev/axis_fifo_0x0000000080006000", "r+b") as character:
    writeToDDS(character,1,0xD0000000)#PLL mult 20, VCO gain high (for 25MHz crystal, 500 MHz clock)

  print("writing to 7000")
  with open("/dev/axis_fifo_0x0000000080007000", "r+b") as character:
    writeToDDS(character,1,0xD0000000)#PLL mult 20, VCO gain high (for 25MHz crystal, 500 MHz clock)
