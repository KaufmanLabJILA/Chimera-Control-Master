from axis_fifo import AXIS_FIFO
from devices import fifo_devices
from devices import gpio_devices
from axi_gpio import AXI_GPIO
from dac81416 import DAC81416
from ad9959 import AD9959
from test_sequencer import write_point as TS_write_point
from reset_all import reset
from writeToSeqGPIO import writeToSeqGPIO
from getSeqGPIOWords import getSeqGPIOWords
import dds_lock_pll

import struct
import math

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
		self.dacRes = 65536
		self.dacRange = [-10, 10]
		self.dacRangeConv = 167772/self.dacRes
		self.dacIncrMax = 268435456
		self.dacTimeRes = 1.6e3 # in us
		self.ddsAmpRange = [0, 5]
		self.ddsFreqRange = [0, 500]
		self.ddsFreqRangeConv = 8589930 # (2^32 - 1)/500 MHz
		self.ddsAmpRangeConv = 818.4 # (2^10 - 1)/1.25 mW
		self.ddsTimeRes = 1.6e3 # in us
		# initialize DACs
		self.dac0 = DAC81416(fifo_devices['DAC81416_0'])
		self.dac1 = DAC81416(fifo_devices['DAC81416_1'])
		self.fifo_dac0_seq = AXIS_FIFO(fifo_devices['DAC81416_0_seq'])
		self.fifo_dac1_seq = AXIS_FIFO(fifo_devices['DAC81416_1_seq'])
		# initialize DDSs
		self.fifo_dds_atw_seq = AXIS_FIFO(fifo_devices['AD9959_0_seq_atw'])
		self.fifo_dds_ftw_seq = AXIS_FIFO(fifo_devices['AD9959_0_seq_ftw'])
		self.fifo_dds0_atw_seq = AXIS_FIFO(fifo_devices['AD9959_0_seq_atw'])
		self.fifo_dds0_ftw_seq = AXIS_FIFO(fifo_devices['AD9959_0_seq_ftw'])
		self.fifo_dds1_atw_seq = AXIS_FIFO(fifo_devices['AD9959_1_seq_atw'])
		self.fifo_dds1_ftw_seq = AXIS_FIFO(fifo_devices['AD9959_1_seq_ftw'])
		self.fifo_dds2_atw_seq = AXIS_FIFO(fifo_devices['AD9959_2_seq_atw'])
		self.fifo_dds2_ftw_seq = AXIS_FIFO(fifo_devices['AD9959_2_seq_ftw'])
		# self.dds = AD9959(dds_device) # initialize DDS
		self.gpio2 = AXI_GPIO(gpio_devices['axi_gpio_2'])
		self.fifo_dio_seq = AXIS_FIFO(fifo_devices['GPIO_seq'])
		self.fifo_main_seq = AXIS_FIFO(fifo_devices['GPIO_seq'])

	def initExp(self):
		print 'initializing experiment'
		self.mod_disable()
		reset()
		dds_lock_pll.dds_lock_pll() 

	def write_dio_point(self, point):
	  #01XXAAAA TTTTTTTT DDDDDDDD
	  self.fifo_dio_seq.write_axis_fifo("\x01\x00" + struct.pack('>H', point.address))
	  self.fifo_dio_seq.write_axis_fifo(struct.pack('>I', point.time))
	  self.fifo_dio_seq.write_axis_fifo(struct.pack('>I', point.outputA))
	  self.fifo_dio_seq.write_axis_fifo(struct.pack('>I', point.outputB))

	def write_dio(self, byte_buf):
	  #01XXAAAA TTTTTTTT DDDDDDDD
	  self.fifo_dio_seq.write_axis_fifo(byte_buf, MSB_first=False)

	def write_dac_point(self, fifo, point):
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

	def write_atw_point(self, fifo, point):
		#01XXAAAA TTTTTTTT DDDDDDDD DDDDDDDD
		#phase acc shifts by 12 bit => 4096
		#unused      <= gpio_in(63 downto 58);
		#acc_start   <= gpio_in(57 downto 48);
		#acc_steps   <= gpio_in(47 downto 32);
		#unused      <= gpio_in(31 downto 26);
		#acc_incr    <= gpio_in(25 downto  4);
		#unused      <= gpio_in( 3 downto  2);
		#acc_chan    <= gpio_in( 1 downto  0);

		self.fifo_dds_atw_seq.write_axis_fifo("\x01\x00" + struct.pack('>H', point.address))
		self.fifo_dds_atw_seq.write_axis_fifo(struct.pack('>I', point.time))
		self.fifo_dds_atw_seq.write_axis_fifo(struct.pack('>I', point.start*256*256 + point.steps))
		self.fifo_dds_atw_seq.write_axis_fifo(struct.pack('>I', point.incr*16+point.chan))

	def write_ftw_point(self, fifo, point):
		#01XXAAAA TTTTTTTT DDDDDDDD DDDDDDDD DDDDDDDD
		#phase acc shifts by 12 bit => 4096
		#acc_start   <= gpio_in(95 downto 64);
		#acc_steps   <= gpio_in(63 downto 48);
		#acc_incr    <= gpio_in(47 downto  4);
		#acc_chan    <= to_integer(unsigned(gpio_in( 3 downto  0)));

		incr_hi = int(math.floor(point.incr*1.0/2**28))
		incr_lo = point.incr - incr_hi * 2**28

		self.fifo_dds_ftw_seq.write_axis_fifo("\x01\x00" + struct.pack('>H', point.address))
		self.fifo_dds_ftw_seq.write_axis_fifo(struct.pack('>I', point.time))
		self.fifo_dds_ftw_seq.write_axis_fifo(struct.pack('>I', point.start))
		self.fifo_dds_ftw_seq.write_axis_fifo(struct.pack('>I', point.steps*256*256 + incr_hi))
		self.fifo_dds_ftw_seq.write_axis_fifo(struct.pack('>I', incr_lo*16+point.chan))

	def reset_DAC(self):
		self.dac = DAC81416(device) # initialize DAC

	def set_DAC(self, channel, value):
		valueInt = int((value-self.dacRange[0])*self.dacRes/(self.dacRange[1]-self.dacRange[0]))
		assert channel>=0 and channel<=31, 'Invalid channel for DAC81416 in set_DAC'
		if (channel > 15):
			channel = channel-16
			self.dac1.set_DAC(channel, valueInt)
		else:
			self.dac0.set_DAC(channel, valueInt)

	def mod_enable(self):
		self.gpio2.set_bit(0, channel=1)

	def mod_disable(self):
		self.gpio2.clear_bit(0, channel=1)

	def mod_report(self):
		print(self.gpio2.read_axi_gpio(channel=1))

	def dac_seq_write_points(self, byte_len, byte_buf, num_snapshots):
		points=[]
		for ii in range(num_snapshots):
			[t, chan, s, end, duration] = self.dac_read_point(byte_buf[ii*byte_len: ii*byte_len + byte_len])
			num_steps = (duration/self.dacTimeRes) - 1 
			if (num_steps == 0):
				ramp_inc = 0
			else:
				ramp_inc = int((end-s)*self.dacRangeConv/num_steps)
			if (ramp_inc < 0):
				ramp_inc = int(self.dacIncrMax + ramp_inc)
			print ramp_inc
			points.append(DAC_seq_point(address=ii,time=t,start=s,steps=duration,incr=ramp_inc,chan=chan))
		points.append(DAC_seq_point(address=num_snapshots, time=0,start=0,steps=0,incr=0,chan=0))
		points.append(DAC_seq_point(address=num_snapshots, time=0,start=0,steps=0,incr=0,chan=16))

		for point in points:
			print "add: ", point.address
			print "time: ", point.time
			print "start: ", point.start
			print "steps: ", point.steps
			print "incr: ", point.incr
			print "chan: ", point.chan 
			if (point.chan < 16):
				fifo = self.fifo_dac0_seq
			else:
				point.chan = point.chan - 16
				fifo = self.fifo_dac1_seq

		  	self.write_dac_point(fifo, point)

	def dds_seq_write_points(self, byte_len, byte_buf, num_snapshots):
		ftw_points=[]
		atw_points=[]
		for ii in range(num_snapshots):
			[t, channel, aorf, s, end, duration] = self.dds_read_point(byte_buf[ii*byte_len: ii*byte_len + byte_len])
			num_steps = (duration/self.ddsTimeRes)-1
			if (num_steps == 0):
				ramp_inc = 0
			else:
				ramp_inc = int((end-s)/num_steps)
			if (aorf == 'f'):
				if (ramp_inc < 0):
					ramp_inc = int(self.ddsFreqRangeConv + ramp_inc)
				ftw_points.append(DDS_ftw_seq_point(address=ii, time=t, start=s, steps=duration, incr=ramp_inc, chan=channel))
			elif (aorf == 'a'):
				if (ramp_inc < 0):
					ramp_inc = int(self.ddsAmpRangeConv + ramp_inc)
				atw_points.append(DDS_atw_seq_point(address=ii, time=t, start=s, steps=duration, incr=ramp_inc, chan=channel))
			else:
				print "invalid dds type. set to 'f' for freq or 'a' for amp"
		for channel in range(0,4,4):
			ftw_points.append(DDS_ftw_seq_point(address=num_snapshots, time=0, start=0, steps=0, incr=0, chan=channel))
			atw_points.append(DDS_atw_seq_point(address=num_snapshots, time=0, start=0, steps=0, incr=0, chan=channel))

		for point in ftw_points:
			print "freq time = ", point.time
			print "freq chan = ", point.chan
			print "freq start = ", point.start
			if (point.chan < 4):
				fifo = self.fifo_dds0_ftw_seq
			elif (4 <= point.chan < 8):
				point.chan = point.chan - 4
				fifo = self.fifo_dds1_ftw_seq
			else:
				point.chan = point.chan - 8
				fifo = self.fifo_dds2_ftw_seq
			self.write_ftw_point(fifo, point)

		for point in atw_points:
			print "amp time = ", point.time
			print "amp chan = ", point.chan
			print "amp start = ", point.start
			if (point.chan < 4):
				fifo = self.fifo_dds0_atw_seq
			elif (4 <= point.chan < 8):
				point.chan = point.chan - 4
				fifo = self.fifo_dds1_atw_seq
			else:
				point.chan = point.chan - 8
				fifo = self.fifo_dds2_atw_seq
		  	self.write_atw_point(fifo, point)

	def dds_seq_write_atw_points(self):
	    points=[]
	    #these ramps should complete in just under 64 ms
	    points.append(DDS_atw_seq_point(address=0,time=   0,start=1023,steps=0,incr=0,chan=0)) #25% to 75%
	#    points.append(DDS_atw_seq_point(address=1,time=1000,start=256,steps=1,incr=0,chan=3)) #25% to 75%
	    points.append(DDS_atw_seq_point(address=1,time=   0,start=0,steps=    0,incr=   0,chan=0))

	    for point in points:
	      self.write_atw_point(point)

	def dds_seq_write_ftw_points(self):
		points=[]
		#these ramps should complete in just under 64 ms
		points.append(DDS_ftw_seq_point(address=0,time=0,start=200000000,steps=0,incr=0,chan=0))
		# ~ points.append(DDS_ftw_seq_point(address=0,time=   0,start=800000,steps=10000,incr=30000,chan=0)) 
		points.append(DDS_ftw_seq_point(address=1,time=1,start=10000000,steps=1,incr=0,chan=3)) 
		points.append(DDS_ftw_seq_point(address=2,time=   0,start=     0,steps=    0,incr=    0,chan=0))

		for point in points:
		  self.write_ftw_point(point)

	def dio_seq_write_points(self, byte_len, byte_buf, num_snapshots):
		points=[]
		for ii in range(num_snapshots):
			[t, outA, outB] = self.dio_read_point(byte_buf[ii*byte_len: ii*byte_len + byte_len])
			points.append(GPIO_seq_point(address=ii,time=t,outputA=outA,outputB=outB))
		points.append(GPIO_seq_point(address=num_snapshots,time=6400000,outputA=0x00000000,outputB=0x00000000))
		points.append(GPIO_seq_point(address=num_snapshots+1,time=0,outputA=0x00000000,outputB=0x00000000))

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
				self.fifo_dio_seq.write_axis_fifo(word[0], MSB_first=False)

	def dio_read_point(self, snapshot):
		print snapshot
		snapshot_split = snapshot.split('_')
		t = int(snapshot_split[0].strip('t'), 16)
		out = snapshot_split[1].strip('b').strip('\0')
		outB = int(out[:8], 16)
		outA = int(out[8:], 16)
		return [t, outA, outB]

	def dac_read_point(self, snapshot):
		snapshot_split = snapshot.split('_')
		t = int(snapshot_split[0].strip('t'), 16)
		chan = int(snapshot_split[1].strip('c'), 16)
		start = int(self.dacRes*(float(snapshot_split[2].strip('s')) - self.dacRange[0])
			/(self.dacRange[1]-self.dacRange[0]))
		end = int(self.dacRes*(float(snapshot_split[3].strip('e')) - self.dacRange[0])
			/(self.dacRange[1]-self.dacRange[0]))
		duration = int(snapshot_split[4].strip('d').strip('\0'), 16)
		return [t, chan, start, end, duration]

	def dds_read_point(self, snapshot):
		snapshot_split = snapshot.split('_')
		t = int(snapshot_split[0].strip('t'), 16)
		chan = int(snapshot_split[1].strip('c'), 16)
		aorf = snapshot_split[2]
		if (aorf == 'f'):
			ddsConv = self.ddsFreqRangeConv
		else:
			ddsConv = self.ddsAmpRangeConv
		start = int(float(snapshot_split[3].strip('s'))*ddsConv)
		end = int(float(snapshot_split[4].strip('e'))*ddsConv)
		duration = int(snapshot_split[5].strip('d').strip('\0'), 16)
		return [t, chan, aorf,  start, end, duration]

if __name__ == "__main__":
	from soft_trigger import trigger
	from reset_all import reset
	import dds_lock_pll
	
	byte_buf_dio = 't00000000_b8000000100000001\0t000003E8_b0000000000000000\0t000007D0_b0000000000000000\0'
	byte_buf0 = 't00000064_c0000_a_s000.500_e000.000_d00000000\0'
	byte_buf1 = 't00000064_c0000_f_s080.000_e000.000_d00000000\0'

	seq = sequencer()
	# seq.mod_disable()
	reset()
	# dds_lock_pll.dds_lock_pll()
	# seq.dds_seq_write_points(47, byte_buf0, 1)
	# seq.dds_seq_write_points(47, byte_buf1, 1)
	# seq.dds_seq_write_atw_points()
	# seq.dds_seq_write_ftw_points()
	seq.set_DAC(0, 0)
	# seq.mod_enable()
	# trigger()