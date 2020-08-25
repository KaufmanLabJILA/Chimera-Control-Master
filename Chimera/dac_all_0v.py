#!/usr/bin/python

with open("/dev/axis_fifo_0x0000000080002000", "r+b") as character:
    writewords = []
    #SPI config register
    writewords.append("\x84\x0A\x03\x00")
    #GEN config
    writewords.append("\x00\x01\x04\x00")
    #power down control
    writewords.append("\x00\x00\x09\x00")
    #DACRANGE
    writewords.append("\xAA\xAA\x0A\x00")
    #DACRANGE
    writewords.append("\xAA\xAA\x0B\x00")
    #DACRANGE
    writewords.append("\xAA\xAA\x0C\x00")
    #DACRANGE
    writewords.append("\xAA\xAA\x0D\x00")
    #broadcast register
    writewords.append("\x00\x80\x0F\x00")

    for word in writewords:
      character.write(word)
      print('Reading...')
      reading = character.read(4)
      print('Read {} bytes: {} {} {} {}'.format(len(reading), hex(ord(reading[0])), hex(ord(reading[1])), hex(ord(reading[2])), hex(ord(reading[3]))))
