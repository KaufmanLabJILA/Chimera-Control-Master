from axis_fifo import AXIS_FIFO
from devices import fifo_devices

from axi_gpio import AXI_GPIO
from devices import gpio_devices

from dac81416 import DAC81416
from ad9959 import AD9959
import struct

from dds_lock_pll import writeToDDS
from getSeqGPIOWords import getSeqGPIOWords

class GPIO_seq_point:
	def __init__(self, address, time, outputA, outputB):
		self.address = address
		self.time = time
		self.outputA = outputA
		self.outputB = outputB

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

class DDS_ramp_tester:
	def __init__(self, device, device_atw_seq, device_ftw_seq, main_seq):
		#self.dds	= AD9959(device) # initialize DDS	-- not needed for now

		self.gpio2 = AXI_GPIO(gpio_devices['axi_gpio_2'])

		self.fifo_dds_atw_seq = AXIS_FIFO(device_atw_seq)
		self.fifo_dds_ftw_seq = AXIS_FIFO(device_ftw_seq)
		self.fifo_main_seq	 = AXIS_FIFO(main_seq)

	def write_atw_point(self, point):
	 #01XXAAAA TTTTTTTT DDDDDDDD DDDDDDDD
	 #phase acc shifts by 12 bit => 4096
	 #unused		<= gpio_in(63 downto 58);
	 #acc_start	<= gpio_in(57 downto 48);
	 #acc_steps	<= gpio_in(47 downto 32);
	 #unused		<= gpio_in(31 downto 26);
	 #acc_incr	 <= gpio_in(25 downto	4);
	 #unused		<= gpio_in( 3 downto	2);
	 #acc_chan	 <= gpio_in( 1 downto	0);

	 self.fifo_dds_atw_seq.write_axis_fifo(b"\x01\x00" + struct.pack('>H', point.address))
	 self.fifo_dds_atw_seq.write_axis_fifo(struct.pack('>I', point.time))
	 self.fifo_dds_atw_seq.write_axis_fifo(struct.pack('>I', point.start*256*256 + point.steps))
	 self.fifo_dds_atw_seq.write_axis_fifo(struct.pack('>I', point.incr*16+point.chan))

	def write_ftw_point(self, point):
	 #01XXAAAA TTTTTTTT DDDDDDDD DDDDDDDD DDDDDDDD
	 #phase acc shifts by 12 bit => 4096
	 #acc_start	<= gpio_in(95 downto 64);
	 #acc_steps	<= gpio_in(63 downto 48);
	 #acc_incr	 <= gpio_in(47 downto	4);
	 #acc_chan	 <= to_integer(unsigned(gpio_in( 3 downto	0)));

	 incr_hi = point.incr >> 28 #(point.incr & (0xffff << 28)) # acc_incr_hi	 <= gpio_in(47 downto	32)
	 incr_lo = point.incr & ((1 << 28) - 1)	# acc_incr_lo	 <= gpio_in(31 downto	4)

	 self.fifo_dds_ftw_seq.write_axis_fifo(b"\x01\x00" + struct.pack('>H', point.address))
	 self.fifo_dds_ftw_seq.write_axis_fifo(struct.pack('>I', point.time))
	 self.fifo_dds_ftw_seq.write_axis_fifo(struct.pack('>I', point.start))
	 self.fifo_dds_ftw_seq.write_axis_fifo(struct.pack('>I', point.steps*256*256 + incr_hi))
	 self.fifo_dds_ftw_seq.write_axis_fifo(struct.pack('>I', incr_lo*16+point.chan))

	#def reset_DDS(self): #--not needed
	#	self.dds = AD9959(device) # initialize DDS

	def mod_enable(self):
		self.gpio2.set_bit(0, channel=1)

	def mod_disable(self):
		self.gpio2.clear_bit(0, channel=1)

	def mod_report(self):
		print(self.gpio2.read_axi_gpio(channel=1))

	def dds_seq_write_atw_points(self):
		points=[]
		#these ramps should complete in just under 64 ms
		points.append(DDS_atw_seq_point(address=0,time=10000,start=1023,steps=0,incr=0,chan=1)) #25% to 75%
		#	 points.append(DDS_atw_seq_point(address=1,time=1000,start=256,steps=1,incr=0,chan=3)) #25% to 75%
		points.append(DDS_atw_seq_point(address=1,time=0,start=0,steps=0,incr=0,chan=0))

		for point in points:
			self.write_atw_point(point)

	def dds_seq_write_ftw_points(self):
		points=[]
		#these ramps should complete in just under 64 ms
		points.append(DDS_ftw_seq_point(address=0,time=0,start=472446000,steps=0,incr=0,chan=0))
		#points.append(DDS_ftw_seq_point(address=1,time=	20000, start=4000000,steps=0,incr=0,chan=0))
		# points.append(DDS_ftw_seq_point(address=1,time=1,start=1000000,steps=1,incr=0,chan=3))
		points.append(DDS_ftw_seq_point(address=1,time=0,start=0,steps=0,incr=0,chan=0))

		for point in points:
			print('DDS_ftw_seq_point(address=',point.address,',time=',point.time,',start=',point.start,',steps=',point.steps,',incr=',point.incr,',chan=',point.chan,'))')
			self.write_ftw_point(point)


	def dio_seq_write_points(self):
		points=[]
		points.append(GPIO_seq_point(address=0,time=1,outputA=0x00000001,outputB=0x00000001))
		points.append(GPIO_seq_point(address=1,time=20000,outputA=0x00000000,outputB=0x00000000))
		points.append(GPIO_seq_point(address=2,time=40000,outputA=0x00000001,outputB=0x00000001))
		points.append(GPIO_seq_point(address=3,time=6400000,outputA=0x00000000,outputB=0x00000000))
		points.append(GPIO_seq_point(address=4,time=0,outputA=0x00000000,outputB=0x00000000))

		for point in points:
			print("add: ", point.address)
			print("time: ", point.time)
			print("outputA: ", point.outputA)
			print("outputB: ", point.outputB)

		# with open("/dev/axis_fifo_0x0000000080004000", "r+b") as character:
		for point in points:
			 # writeToSeqGPIO(character, point)
			seqWords = getSeqGPIOWords(point)
			for word in	seqWords:
			 # print(word)
			 self.fifo_main_seq.write_axis_fifo(word[0], MSB_first=False)

def program(tester):
	#tester.dds_seq_write_atw_points()
	tester.dds_seq_write_ftw_points()
	tester.dio_seq_write_points()

	# ~ print('Next, we need to enable modulation')
	# ~ print('	tester.mod_enable()')
	# ~ print('Now, we can use the software trigger')
	# ~ print('	trigger()')
	# ~ print('All AXI peripherals can be reset, note this does not disable modulation')
	# ~ print('	reset()')
	# ~ print('Finally, don\'t forget to disable modulation again')
	# ~ print('	tester.mod_disable()')

if __name__ == "__main__":
	from soft_trigger import trigger
	from reset_all import reset
	import sys
	import dds_lock_pll

	tester = DDS_ramp_tester(fifo_devices['AD9959_0'], fifo_devices['AD9959_0_seq_atw'], fifo_devices['AD9959_0_seq_ftw'], fifo_devices['GPIO_seq'])
	tester.mod_disable()
	reset()
	dds_lock_pll.dds_lock_pll()
	program(tester)
	tester.mod_enable()
	trigger()
