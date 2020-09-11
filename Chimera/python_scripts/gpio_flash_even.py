#!/usr/bin/python

import struct
from time import sleep

def writeToSeqGPIO(dds_char_dev, address, time, banka, bankb):
  time_bytes = struct.pack('>I', time)
  banka_bytes = struct.pack('>I', banka)
  bankb_bytes = struct.pack('>I', bankb)

  words = []
  #address for time
  address_bytes = struct.pack('>I', address + 0x4000)
  words.append(address_bytes[3])
  words.append(address_bytes[2])
  words.append(address_bytes[1])
  words.append(address_bytes[0])
  #time
  words.append(time_bytes[3])
  words.append(time_bytes[2])
  words.append(time_bytes[1])
  words.append(time_bytes[0])
  
  #address for bank A
  address_bytes = struct.pack('>I', address + 0x8000)
  words.append(address_bytes[3])
  words.append(address_bytes[2])
  words.append(address_bytes[1])
  words.append(address_bytes[0])
  #bank a data
  words.append(banka_bytes[3])
  words.append(banka_bytes[2])
  words.append(banka_bytes[1])
  words.append(banka_bytes[0])
  
  #address for bank B
  address_bytes = struct.pack('>I', address + 0xC000)
  words.append(address_bytes[3])
  words.append(address_bytes[2])
  words.append(address_bytes[1])
  words.append(address_bytes[0])
  #time
  words.append(bankb_bytes[3])
  words.append(bankb_bytes[2])
  words.append(bankb_bytes[1])
  words.append(bankb_bytes[0])

  i = 0

  for word in words:
    print(ord(word))
    i=i+1
    if(i==4):
      print("\n")
      i=0
    dds_char_dev.write(word)

if __name__ == "__main__":
  print("writing to 4000")
  with open("/dev/axis_fifo_0x0000000080004000", "r+b") as character:
    #note that this sequence leaves all signals high when complete.
    #that makes it easy to verify that all outputs are OK for testing
    #high
    writeToSeqGPIO(character,0x0000,0x00000000,0x55555555,0x55555555)
    sleep(0.05)
    #low after 1 us
    writeToSeqGPIO(character,0x0001,0x00000064,0x00000000,0x00000000)
    sleep(0.05)
    #high another us later
    writeToSeqGPIO(character,0x0002,0x000000C8,0x55555555,0x55555555)
    sleep(0.05)
    #terminate sequence
    writeToSeqGPIO(character,0x0003,0x00000000,0x00000000,0x00000000)
    sleep(0.05)
