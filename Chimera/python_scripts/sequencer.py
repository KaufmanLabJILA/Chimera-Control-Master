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
	    self.dacRes = 65536
	    self.dacRange = [-10, 10]
	    self.seqTimeRes = 65536
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

	def reset_DAC(self):
		self.dac = DAC81416(device) # initialize DAC

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
			#no ramps yet
			points.append(DAC_seq_point(address=ii,time=t,start=s,steps=1,incr=1,chan=chan))		
		points.append(DAC_seq_point(address=num_snapshots, time=0,start=0,steps=0,incr=0,chan=0))
		points.append(DAC_seq_point(address=num_snapshots, time=0,start=0,steps=0,incr=0,chan=16))

		for point in points:
			if (point.chan < 16):
				fifo = self.fifo_dac0_seq
			else:
				point.chan = point.chan - 16
				fifo = self.fifo_dac1_seq

		  	self.write_dac_point(fifo, point)

	def dio_seq_write_points(self, byte_len, byte_buf, num_snapshots):
		points=[]
		for ii in range(num_snapshots):
			[t, out] = self.dio_read_point(byte_buf[ii*byte_len: ii*byte_len + byte_len])
			points.append(GPIO_seq_point(address=ii,time=t,output=out))
		points.append(GPIO_seq_point(address=num_snapshots,time=0,output=0x00000000))

		for point in points:
			print point.address
			print point.time
			print point.output

		for point in points:
		  self.write_dio_point(point)

	def dio_read_point(self, snapshot):
		snapshot_split = snapshot.split('_')
		t = int(snapshot_split[0].strip('t'), 16)
		out = int(snapshot_split[1].strip('b').strip('\0'), 16)
		return [t, out]

	def dac_read_point(self, snapshot):
		snapshot_split = snapshot.split('_')
		t = int(snapshot_split[0].strip('t'))
		chan = int(snapshot_split[1].strip('c'), 16)
		start = int((float(snapshot_split[2].strip('s')) - self.dacRange[0])
			/(self.dacRange[1]-self.dacRange[0])*self.dacRes)
		end = int((float(snapshot_split[3].strip('e')) - self.dacRange[0])
			/(self.dacRange[1]-self.dacRange[0])*self.dacRes)
		duration = int(snapshot_split[4].strip('d').strip('\0'), 16)
		return [t, chan, start, end, duration]

if __name__ == "__main__":
  seq = sequencer()
  seq.dio_seq_write_points()