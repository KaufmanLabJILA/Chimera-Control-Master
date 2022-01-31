from axis_fifo import AXIS_FIFO
from devices import fifo_devices
import struct

fifo = AXIS_FIFO(fifo_devices['AD9959_0'])

#Function Register 1 (FR1)
fifo.write_axis_fifo("\x00\x80\x00\x00")
fifo.write_axis_fifo("\x00\xC0\x00\x00")


