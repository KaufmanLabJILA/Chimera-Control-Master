#!/usr/bin/python

import struct

def writeToDDS(dds_char_dev, address, data):
  data_bytes = struct.pack('<I', data)
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
    character.write(word)

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
  with open("/dev/axis_fifo_0x0000000080005000", "r+b") as character:
    # ~ writeWords(character)
    writeToDDS(character,4,0x271DE698) # (80 MHz: 0x28F5C378) (76.45 MHZ: 0x271DE698)

  print("writing to 6000")
  with open("/dev/axis_fifo_0x0000000080006000", "r+b") as character:
    # ~ writeWords(character)
    writeToDDS(character,4,0x3AE145D0) # 115 MHz

  print("writing to 7000")
  with open("/dev/axis_fifo_0x0000000080007000", "r+b") as character:
    # ~ writeWords(character)
    writeToDDS(character,4,0x23D70898) # 70 MHz
