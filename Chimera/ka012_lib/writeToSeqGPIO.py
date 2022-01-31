import struct

def writeToSeqGPIO(dev, point):
  address, time, banka, bankb =  point.address, point.time, point.outputA, point.outputB
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
  words.append(banka_bytes[0])
  words.append(banka_bytes[1])
  words.append(banka_bytes[2])
  words.append(banka_bytes[3])
  
  #address for bank B
  address_bytes = struct.pack('>I', address + 0xC000)
  words.append(address_bytes[3])
  words.append(address_bytes[2])
  words.append(address_bytes[1])
  words.append(address_bytes[0])
  #time
  words.append(bankb_bytes[0])
  words.append(bankb_bytes[1])
  words.append(bankb_bytes[2])
  words.append(bankb_bytes[3])

  i = 0

  print 'writing to seqGPIO'
  for word in words:
    print(ord(word))
    i=i+1
    if(i==4):
      print("\n")
      i=0
    dev.write(word)
