import struct

def getWord(bytes):
  return bytes[3] + bytes[2] + bytes[1] + bytes[0]

def getSeqDACWords(point):
  address, time, chan, start, steps, incr =  point.address, point.time, point.chan, point.start, point.steps, point.incr
  time_bytes = struct.pack('>I', time)
  chan_bytes = struct.pack('>I', chan)
  start_bytes = struct.pack('>I', start)
  steps_bytes = struct.pack('>I', steps)
  incr_bytes = struct.pack('>I', incr)

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
