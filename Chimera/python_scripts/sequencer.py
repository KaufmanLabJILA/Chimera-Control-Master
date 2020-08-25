from axis_fifo import AXIS_FIFO
from devices import fifo_devices
from devices import gpio_devices
from axi_gpio import AXI_GPIO
from dac81416 import DAC81416
from ad9959 import AD9959

import struct

class GPIO_seq_point:
  def __init__(self, address, time, output):
    self.address = address
    self.time = time
    self.output = output

class DAC_seq_point:
  def __init__(self, address, time, start, steps, incr, chan):
    self.address = address
    self.time = time
    self.start = start
    self.steps = steps
    self.incr = incr
    self.chan = chan

class DDS_atw_seq_point:
  def __init__(self, address, time, start, steps, incr, chan):
    self.address = address
    self.time = time
    self.start = start
    self.steps = steps
    self.incr = incr
    self.chan = chan

class DDS_ftw_seq_point:
  def __init__(self, address, time, start, steps, incr, chan):
    self.address = address
    self.time = time
    self.start = start
    self.steps = steps
    self.incr = incr
    self.chan = chan

class sequencer:
	def __init__(self):
		# initialize DACs
	    self.dac0 = DAC81416(fifo_devices['DAC81416_0'])
	    self.dac1 = DAC81416(fifo_devices['DAC81416_1'])
	    self.fifo_dac0_seq = AXIS_FIFO(fifo_devices['DAC81416_0_seq'])
	    self.fifo_dac1_seq = AXIS_FIFO(fifo_devices['DAC81416_1_seq'])
	    # self.dds = AD9959(dds_device) # initialize DDS
	    self.gpio2 = AXI_GPIO(gpio_devices['axi_gpio_2'])

	    self.fifo_dio_seq = AXIS_FIFO(fifo_devices['GPIO_seq'])
	    # self.fifo_dds_atw_seq = AXIS_FIFO(device_atw_seq)
    	# self.fifo_dds_ftw_seq = AXIS_FIFO(device_ftw_seq)

	def write_dio_point(self, point):
	  #01XXAAAA TTTTTTTT DDDDDDDD
	  self.fifo_dio_seq.write_axis_fifo("\x01\x00" + struct.pack('>H', point.address))
	  self.fifo_dio_seq.write_axis_fifo(struct.pack('>I', point.time))
	  self.fifo_dio_seq.write_axis_fifo(struct.pack('>I', point.output))

	def write_dio(self, byte_buf):
	  #01XXAAAA TTTTTTTT DDDDDDDD
	  self.fifo_dio_seq.write_axis_fifo(byte_buf, MSB_first=False)

	def write_dac_point(self, point):
		#01XXAAAA TTTTTTTT DDDDDDDD DDDDDDDD
		#phase acc shifts by 12 bit => 4096
		#acc_start   <= gpio_in(63 downto 48);
		#acc_steps   <= gpio_in(47 downto 32);
		#acc_incr    <= gpio_in(31 downto  4);
		#acc_chan    <= to_integer(unsigned(gpio_in( 3 downto  0)));

		self.fifo_dac_seq.write_axis_fifo("\x01\x00" + struct.pack('>H', point.address))
		self.fifo_dac_seq.write_axis_fifo(struct.pack('>I', point.time))
		self.fifo_dac_seq.write_axis_fifo(struct.pack('>I', point.start*256*256 + point.steps))
		self.fifo_dac_seq.write_axis_fifo(struct.pack('>I', point.incr*16+point.chan))

	def reset_DAC(self):
		self.dac = DAC81416(device) # initialize DAC

	def mod_enable(self):
		self.gpio2.set_bit(0, channel=1)

	def mod_disable(self):
		self.gpio2.clear_bit(0, channel=1)

	def mod_report(self):
		print(self.gpio2.read_axi_gpio(channel=1))

	def dac_seq_write_points(self):
		points=[]
		#these ramps should complete in just under 64 ms
		points.append(DAC_seq_point(address=0,time=0,start=256*128,steps=60000,incr=128,chan=0))
		# ~ points.append(DAC_seq_point(address=1,time=10,start=256*128,steps=65535,incr=512,chan=3))
		# ~ points.append(DAC_seq_point(address=2,time=10000,start=256*128,steps=65535,incr=128,chan=0))
		points.append(DAC_seq_point(address=1, time=0,start=0,steps=0,incr=0,chan=0))

		for point in points:
		  self.write_dac_point(point)

	def dio_seq_write_points(self):
		points=[]
		points.append(GPIO_seq_point(address=0,time=0,output=0xFFFFFFFF))
		points.append(GPIO_seq_point(address=1,time=1000,output=0x00000000))
		points.append(GPIO_seq_point(address=2,time=2000,output=0xFFFFFFFF))
		points.append(GPIO_seq_point(address=3,time=40000,output=0x00000000))
		points.append(GPIO_seq_point(address=4,time=0,output=0x00000000))

		for point in points:
		  self.write_dio_point(point)

if __name__ == "__main__":
  seq = sequencer()
  seq.dio_seq_write_points()