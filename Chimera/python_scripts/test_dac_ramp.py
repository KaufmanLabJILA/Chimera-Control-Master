from axis_fifo import AXIS_FIFO
from devices import fifo_devices

from axi_gpio import AXI_GPIO
from devices import gpio_devices

from dac81416 import DAC81416
import struct
import soft_trigger
import reset_all

from getSeqGPIOWords import getSeqGPIOWords

class GPIO_seq_point:
  def __init__(self, address, time, outputA, outputB):
    self.address = address
    self.time = time
    self.outputA = outputA
    self.outputB = outputB

class DAC_seq_point:
  def __init__(self, address, time, start, steps, incr, chan):
    self.address = address
    self.time = time
    self.start = start
    self.steps = steps
    self.incr = incr
    self.chan = chan

class DAC_ramp_tester:
  def __init__(self, device, device_seq0, device_seq1, main_seq):
    self.dac = DAC81416(device) # initialize DAC
    self.gpio2 = AXI_GPIO(gpio_devices['axi_gpio_2'])
    self.fifo_dac_seq0 = AXIS_FIFO(device_seq0)
    self.fifo_dac_seq1 = AXIS_FIFO(device_seq1)
    self.fifo_main_seq = AXIS_FIFO(main_seq)

  def write_point(self, fifo, point):
    #01XXAAAA TTTTTTTT DDDDDDDD DDDDDDDD
    #phase acc shifts by 12 bit => 4096
    #acc_start   <= gpio_in(63 downto 48);
    #acc_steps   <= gpio_in(47 downto 32);
    #acc_incr    <= gpio_in(31 downto  4);
    #acc_chan    <= to_integer(unsigned(gpio_in( 3 downto  0)));

    fifo.write_axis_fifo("\x01\x00" + struct.pack('>H', point.address))
    fifo.write_axis_fifo(struct.pack('>I', point.time))
    fifo.write_axis_fifo(struct.pack('>I', point.start*256*256 + point.steps))
    fifo.write_axis_fifo(struct.pack('>I', point.incr*16+point.chan))

  def reset_DAC(self):
    self.dac = DAC81416(device) # initialize DAC

  def mod_enable(self):
    self.gpio2.set_bit(0, channel=1)

  def mod_disable(self):
    self.gpio2.clear_bit(0, channel=1)

  def mod_report(self):
    print(self.gpio2.read_axi_gpio(channel=1))

  def dac_seq_write_points(self):
    points0=[]
    points1=[]
    #these ramps should complete in just under 64 ms
    #points.append(DAC_seq_point(address=0,time=0,start=256*128,steps=20000,incr=4096,chan=0))
    points0.append(DAC_seq_point(address=0,time=1,start=256*128,steps=10000,incr=50,chan=0))
    # points0.append(DAC_seq_point(address=2,time=160000,start=256*134,steps=0,incr=0,chan=0))
    points0.append(DAC_seq_point(address=1, time=0,start=0,steps=0,incr=0,chan=0))

    # points1.append(DAC_seq_point(address=0,time=1,start=256*200,steps=1,incr=0,chan=0))
    # points1.append(DAC_seq_point(address=1,time=500000,start=256*128,steps=1,incr=0,chan=0))
    # points1.append(DAC_seq_point(address=2, time=0,start=0,steps=0,incr=0,chan=0))

    for point in points0:
      self.write_point(self.fifo_dac_seq0, point)
    for point in points1:
      self.write_point(self.fifo_dac_seq1, point)

  def dio_seq_write_points(self):
    points=[]
    points.append(GPIO_seq_point(address=0,time=1,outputA=0x00000001,outputB=0x00000001))
    points.append(GPIO_seq_point(address=1,time=20000,outputA=0x00000000,outputB=0x00000000))
    points.append(GPIO_seq_point(address=2,time=40000,outputA=0x00000001,outputB=0x00000001))
    points.append(GPIO_seq_point(address=3,time=6400000,outputA=0x00000000,outputB=0x00000000))
    points.append(GPIO_seq_point(address=4,time=0,outputA=0x00000000,outputB=0x00000000))

    for point in points:
      print "add: ", point.address
      print "time: ", point.time
      print "outputA: ", point.outputA
      print "outputB: ", point.outputB

    # with open("/dev/axis_fifo_0x0000000080004000", "r+b") as character:
    for point in points:
        # writeToSeqGPIO(character, point)
      seqWords = getSeqGPIOWords(point)
      for word in  seqWords:
        # print word
        self.fifo_main_seq.write_axis_fifo(word[0], MSB_first=False)

def program(tester):
  # tester.fifo_dac_seq.write_axis_fifo("\x00\x0A\x01\x01")
  tester.dac_seq_write_points()
  tester.dio_seq_write_points()
  
  # ~ print('Next, we need to enable modulation')
  # ~ print('  tester.mod_enable()')
  # ~ print('Now, we can use the software trigger')
  # ~ print('  trigger()')
  # ~ print('All AXI peripherals can be reset, note this does not disable modulation')
  # ~ print('  reset()')
  # ~ print('Finally, don\'t forget to disable modulation again')
  # ~ print('  tester.mod_disable()')

if __name__ == "__main__":
  
  tester = DAC_ramp_tester(fifo_devices['DAC81416_0'], fifo_devices['DAC81416_0_seq'], fifo_devices['DAC81416_1_seq'], fifo_devices['GPIO_seq'])
  program(tester)
  tester.mod_enable()
  soft_trigger.trigger()
  # ~ reset_all.reset()
  # ~ tester.mod_disable()
