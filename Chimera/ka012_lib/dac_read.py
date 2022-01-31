#!/usr/bin/python

if __name__ == "__main__":
	with open("/dev/axis_fifo_0x0000000080002000", "r+b") as character:
		print('Reading...')
		reading = character.read(4)
		print('Read {} bytes: {} {} {} {}'.format(len(reading), hex(ord(reading[0])), hex(ord(reading[1])), hex(ord(reading[2])), hex(ord(reading[3]))))
