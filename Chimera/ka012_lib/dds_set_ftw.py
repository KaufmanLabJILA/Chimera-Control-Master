#!/usr/bin/python

import struct

def writeToDDS(dds_char_dev, address, data):
  data_bytes = struct.pack('<I', data)
  for byte in data_bytes:
    print struct.unpack('<B', byte)
  words = []
  #instruction and D16-D31
  words.append(data_bytes[2])
  words.append(data_bytes[3])
  words.append(struct.pack('<B', 128 + address))
  words.append("\x00")
  #IOU and D0-D15
  words.append(data_bytes[0])
  words.append(data_bytes[1])
  words.append(struct.pack('<B', 192 + 1))
  words.append("\x00")

  for word in words:
    # print(struct.unpack('<B', word))
    dds_char_dev.write(word)

def writeWords(character):
  words = []
  #instruction and D16-D31
  words.append("\x2A\x2A\x84\x00")
  #IOU and D0-D15
  words.append("\x2A\x2A\xC1\x00")

  for word in words:
    character.write(word)
    #print(word)

if __name__ == "__main__":
  print("writing to 5000")
  with open("/dev/axis_fifo_0x0000000080005000", "wb") as character:
    # ~ writeWords(character)
    # writeToDDS(character,0,0x20000000)
    writeToDDS(character,4,0x23D70898)# (80 MHz: 0x28F5C378) (76.45 MHZ: 0x271DE698)

    # writeToDDS(character,0,0x40000000)
    # writeToDDS(character,4,0x371DE698)
    # writeToDDS(character,0,0x00000000)

  print("writing to 6000")
  with open("/dev/axis_fifo_0x0000000080006000", "wb") as character:
    # ~ writeWords(character)
    writeToDDS(character,4,0x3AE145D0) # 115 MHz

  print("writing to 7000")
  with open("/dev/axis_fifo_0x0000000080007000", "wb") as character:
    # ~ writeWords(character)
    writeToDDS(character,4,0x23D70898) # 70 MHz
