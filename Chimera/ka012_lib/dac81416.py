from axis_fifo import AXIS_FIFO
import struct
import sys

class DAC81416:
  """Class to control DAC81416 in project KA012.
  Very simple class just used for testing.
  """

  def __init__(self, device=None):
    if device is None:
      self.fifo = AXIS_FIFO()
    else:
      self.fifo = AXIS_FIFO(device)
    #SPI config register
    #sets device in active mode
    #activates streaming mode
    self.fifo.write_axis_fifo("\x00\x03\x0A\x8C")
    #GEN config
    #activate internal/external ref
    # self.fifo.write_axis_fifo("\x00\x04\x3F\x00")
    self.fifo.write_axis_fifo("\x00\x04\x7F\x00")
    #BRDCONFIG - disable broadcast mode
    self.fifo.write_axis_fifo("\x00\x05\x00\x00")
    #SYNCCONFIG - activate LDAC
    self.fifo.write_axis_fifo("\x00\x06\xFF\xFF")
    #TOGGCONFIG0 - leave at default
    self.fifo.write_axis_fifo("\x00\x07\x00\x00")
    #TOGGCONFIG1 - leave at default
    self.fifo.write_axis_fifo("\x00\x08\x00\x00")
    #DACRANGE set to +-10V
    self.fifo.write_axis_fifo("\x00\x0A\xAA\xAA")
    #DACRANGE set to +-10V
    self.fifo.write_axis_fifo("\x00\x0B\xAA\xAA")
    #DACRANGE set to +-10V
    self.fifo.write_axis_fifo("\x00\x0C\xAA\xAA")
    #DACRANGE set to +-10V
    self.fifo.write_axis_fifo("\x00\x0D\xAA\xAA")
    #TRIGGER - leave at default
    self.fifo.write_axis_fifo("\x00\x0E\x00\x00")
    #power down control
    self.fifo.write_axis_fifo("\x00\x09\x00\x00")

    for channel in range(16):
	    self.set_DAC(channel, 256*128)

  def set_DAC(self, channel, value):
    assert channel>=0 and channel<=15, 'Invalid channel for DAC81416 in set_DAC'
    val = b"\x00" + struct.pack('B',channel+16) + struct.pack('>H', value)
    print(val)
    if self.fifo is not None:
      self.fifo.write_axis_fifo(val)

if __name__ == "__main__":
    from devices import fifo_devices
    print('setting dac0 0')
    dac = DAC81416(fifo_devices['DAC81416_0'])
    # print "0"
    valInt = int((float(sys.argv[1])+10)*256*256/20)
    dac.set_DAC(0, valInt)

    print('setting dac1 0')
    dac = DAC81416(fifo_devices['DAC81416_1'])
    # print "0"
    valInt = int((float(sys.argv[1])+10)*256*256/20)
    dac.set_DAC(0, valInt)
