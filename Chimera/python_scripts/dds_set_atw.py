#!/usr/bin/python

import struct

def writeToDDS(dds_char_dev, address, data):
  data_bytes = struct.pack('<I', data)
  words = []
  #instruction and D0-D23
  words.append("\x77")
  words.append("\x00")
  words.append("\x00")
  words.append("\x06")

  for word in words:
    character.write(word)

if __name__ == "__main__":
  print("writing to 5000")
  with open("/dev/axis_fifo_0x0000000080005000", "r+b") as character:
    # ~ writeWords(character)
    writeToDDS(character,6,0x33) #
