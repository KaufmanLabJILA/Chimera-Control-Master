from axis_fifo import AXIS_FIFO
from devices import fifo_devices

from axi_gpio import AXI_GPIO
from devices import gpio_devices

from dac81416 import DAC81416
import struct
import soft_trigger
import reset_all

from getSeqGPIOWords import getSeqGPIOWords
from time import sleep

class GPIO_seq_point:
  def __init__(self, address, time, outputA, outputB):
    self.address = address
    self.time = time
    self.outputA = outputA
    self.outputB = outputB

class DAC_seq_point:
  def __init__(self, address, time, start, incr, chan, clr_incr=0):
    assert (address >= 0),"invalid address!"
    assert (address <= 1023),"invalid address!"
    assert (time >= 0),"invalid time!"
    assert (time <= 65536*65536-1),"invalid time!"
    assert (clr_incr >= 0),"invalid clr_incr!"
    assert (clr_incr <= 1),"invalid clr_incr!"
    assert (chan >= 0),"invalid channel!"
    assert (chan <= 15),"invalid channel!"
    assert (start >= 0),"invalid start!"
    assert (start <= 65535),"invalid start!"
    assert (incr >= 0),"invalid increment!"
    assert (incr <= 65536*65536-1),"invalid increment!"
    self.address = address
    self.time = time
    self.start = start
    self.clr_incr = clr_incr
    self.incr = incr
    self.chan = chan

class DAC_ramp_tester:
  def __init__(self, dac_a, dac_b, device_seq0, device_seq1, main_seq):
    self.dac_a = dac_a
    DAC81416(dac_a) # initialize DAC
    self.dac_b = dac_b
    DAC81416(dac_b) # initialize DAC
    self.gpio2 = AXI_GPIO(gpio_devices['axi_gpio_2'])
    self.fifo_dac_seq0 = AXIS_FIFO(device_seq0)
    self.fifo_dac_seq1 = AXIS_FIFO(device_seq1)
    self.fifo_main_seq = AXIS_FIFO(main_seq)

  def write_point(self, fifo, point):
    #01XXAAAA TTTTTTTT DDDDDDDD DDDDDDDD
    #phase acc shifts by 12 bit => 4096
    #clr_incr    <= gpio_in(52 downto 52);
    #acc_chan    <= gpio_in(51 downto 48);
    #acc_start   <= gpio_in(47 downto 32);
    #acc_incr    <= gpio_in(31 downto  0);

    fifo.write_axis_fifo("\x01\x00" + struct.pack('>H', point.address))
    fifo.write_axis_fifo(struct.pack('>I', point.time))
    fifo.write_axis_fifo(struct.pack('>I', point.clr_incr*16*256*256 + point.chan*256*256 + point.start))
    fifo.write_axis_fifo(struct.pack('>I', point.incr))

  def reset_DAC(self):
    DAC81416(self.dac_a) # initialize DAC
    DAC81416(self.dac_b) # initialize DAC

  def mod_enable(self):
    self.gpio2.set_bit(0, channel=1)

  def mod_disable(self):
    self.gpio2.clear_bit(0, channel=1)

  def mod_report(self):
    print(self.gpio2.read_axi_gpio(channel=1))

  def dac_seq_write_points(self):
    points0=[]
    points1=[]
    #step 100 LSB per 10 us
    points0.append(DAC_seq_point(address=0,  time=0,  start=10000,incr=6553600,chan=0, clr_incr=0))
    points0.append(DAC_seq_point(address=1,  time=1,  start=1000,incr=6553600,chan=1, clr_incr=0))
    points0.append(DAC_seq_point(address=2,  time=2,  start=2000,incr=6553600,chan=2, clr_incr=0))
    points0.append(DAC_seq_point(address=3,  time=3,  start=3000,incr=6553600,chan=3, clr_incr=0))
    points0.append(DAC_seq_point(address=4,  time=4,  start=4000,incr=6553600,chan=4, clr_incr=0))
    points0.append(DAC_seq_point(address=5,  time=5,  start=5000,incr=6553600,chan=5, clr_incr=0))
    points0.append(DAC_seq_point(address=6,  time=6,  start=6000,incr=6553600,chan=6, clr_incr=0))
    points0.append(DAC_seq_point(address=7,  time=7,  start=7000,incr=6553600,chan=7, clr_incr=0))
    points0.append(DAC_seq_point(address=8,  time=8,  start=8000,incr=6553600,chan=8, clr_incr=0))
    points0.append(DAC_seq_point(address=9,  time=9,  start=9000,incr=6553600,chan=9, clr_incr=0))
    points0.append(DAC_seq_point(address=10, time=10, start=10000,incr=6553600,chan=10,clr_incr=0))
    points0.append(DAC_seq_point(address=11, time=11, start=11000,incr=6553600,chan=11,clr_incr=0))
    points0.append(DAC_seq_point(address=12, time=12, start=12000,incr=6553600,chan=12,clr_incr=0))
    points0.append(DAC_seq_point(address=13, time=13, start=13000,incr=6553600,chan=13,clr_incr=0))
    points0.append(DAC_seq_point(address=14, time=14, start=14000,incr=6553600,chan=14,clr_incr=0))
    points0.append(DAC_seq_point(address=15, time=15, start=15000,incr=6553600,chan=15,clr_incr=0))
    points0.append(DAC_seq_point(address=16, time=0,  start=0,incr=0,chan=0,clr_incr=0))
    #step 100 LSB per 10 us
    points1.append(DAC_seq_point(address=0,  time=0,  start=10000,incr=6553600,chan=0, clr_incr=0))
    points1.append(DAC_seq_point(address=1,  time=1,  start=1000,incr=6553600,chan=1, clr_incr=0))
    points1.append(DAC_seq_point(address=2,  time=2,  start=2000,incr=6553600,chan=2, clr_incr=0))
    points1.append(DAC_seq_point(address=3,  time=3,  start=3000,incr=6553600,chan=3, clr_incr=0))
    points1.append(DAC_seq_point(address=4,  time=4,  start=4000,incr=6553600,chan=4, clr_incr=0))
    points1.append(DAC_seq_point(address=5,  time=5,  start=5000,incr=6553600,chan=5, clr_incr=0))
    points1.append(DAC_seq_point(address=6,  time=6,  start=6000,incr=6553600,chan=6, clr_incr=0))
    points1.append(DAC_seq_point(address=7,  time=7,  start=7000,incr=6553600,chan=7, clr_incr=0))
    points1.append(DAC_seq_point(address=8,  time=8,  start=8000,incr=6553600,chan=8, clr_incr=0))
    points1.append(DAC_seq_point(address=9,  time=9,  start=9000,incr=6553600,chan=9, clr_incr=0))
    points1.append(DAC_seq_point(address=10, time=10, start=10000,incr=6553600,chan=10,clr_incr=0))
    points1.append(DAC_seq_point(address=11, time=11, start=11000,incr=6553600,chan=11,clr_incr=0))
    points1.append(DAC_seq_point(address=12, time=12, start=12000,incr=6553600,chan=12,clr_incr=0))
    points1.append(DAC_seq_point(address=13, time=13, start=13000,incr=6553600,chan=13,clr_incr=0))
    points1.append(DAC_seq_point(address=14, time=14, start=14000,incr=6553600,chan=14,clr_incr=0))
    points1.append(DAC_seq_point(address=15, time=15, start=15000,incr=6553600,chan=15,clr_incr=0))
    points1.append(DAC_seq_point(address=16, time=400000,  start=0,incr=0,chan=1, clr_incr=1))
    points1.append(DAC_seq_point(address=17, time=800000,  start=40000,incr=4288413696,chan=1, clr_incr=0))
    points1.append(DAC_seq_point(address=18, time=800001,  start=40000,incr=4288413696,chan=0, clr_incr=0))
    points1.append(DAC_seq_point(address=19, time=1000000,  start=0,incr=0,chan=1, clr_incr=1))
    points1.append(DAC_seq_point(address=18, time=1000001,  start=40000,incr=0,chan=0, clr_incr=1))
    points1.append(DAC_seq_point(address=20, time=1200000,  start=20000,incr=4281860096,chan=1, clr_incr=0))
    points1.append(DAC_seq_point(address=21, time=1300000,  start=0,incr=0,chan=1, clr_incr=1))
    points1.append(DAC_seq_point(address=22, time=0,  start=0,incr=0,chan=0,clr_incr=0))

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

  tester = DAC_ramp_tester(fifo_devices['DAC81416_0'], fifo_devices['DAC81416_1'], fifo_devices['DAC81416_0_seq'], fifo_devices['DAC81416_1_seq'], fifo_devices['GPIO_seq'])
  reset_all.reset()
  sleep(1)
  program(tester)
  sleep(1)
  tester.mod_enable()
  sleep(1)
  soft_trigger.trigger()
  sleep(5)
  #reset_all.reset()
  tester.mod_disable()
