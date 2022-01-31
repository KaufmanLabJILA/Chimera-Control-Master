import struct

def getWord(bytes):
  return bytes[3] + bytes[2] + bytes[1] + bytes[0]

def getBankWord(bytes):
  return bytes[0] + bytes[1] + bytes[2] + bytes[3]

def getSeqGPIOWords(point):
  address, time, banka, bankb =  point.address, point.time, point.outputA, point.outputB
  time_bytes = struct.pack('>I', time)
  banka_bytes = struct.pack('>I', banka)
  bankb_bytes = struct.pack('>I', bankb)

  words = []
  #address for time
  address_bytes = struct.pack('>I', address + 0x4000)
  words.append([])
  words[0].append(getWord(address_bytes))
  #time
  words.append([])
  words[1].append(getWord(time_bytes))
  
  #address for bank A
  address_bytes = struct.pack('>I', address + 0x8000)
  words.append([])
  words[2].append(getWord(address_bytes))
  #bank a data
  words.append([])
  words[3].append(getBankWord(banka_bytes))

  
  #address for bank B
  address_bytes = struct.pack('>I', address + 0xC000)
  words.append([])
  words[4].append(getWord(address_bytes))
  #bank b data
  words.append([])
  words[5].append(getBankWord(bankb_bytes))

  return words

  # i = 0

  # print 'writing to seqGPIO'
  # for word in words:
  #   print(ord(word))
  #   i=i+1
  #   if(i==4):
  #     print("\n")
  #     i=0
  #   dev.write(word)
