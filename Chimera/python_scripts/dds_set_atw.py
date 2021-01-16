#!/usr/bin/python

from axis_fifo import AXIS_FIFO
from devices import fifo_devices
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
    dds_char_dev.write(word)

if __name__ == "__main__":
  # device = fifo_devices['AD9959_0']
  # fifo = AXIS_FIFO(device)

  # fifo.write_axis_fifo("\x00\x86\x28\xF5") #(76.45 MHZ: 0x271DE698)  (80 MHz: 0x28F5C378)
  # fifo.write_axis_fifo("\x00\xC1\xC3\x78")

  with open("/dev/axis_fifo_0x0000000080005000", "r+b") as character:
    # ~ writeWords(character)
    writeToDDS(character,6,0x20080000) #
