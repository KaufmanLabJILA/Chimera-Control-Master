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
from soft_trigger import trigger
from time import sleep

import struct
# import math

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
        self.dacRes = 65536 #0xffff
        self.dacRange = [-10, 10]
        self.dacRangeConv = float(167772)/self.dacRes
        self.dacIncrMax = 0xfffffff # 16 atw + 12 acc = 28bit #268435455
        self.dacRampTimeRes = 2000 #20us in the unit of system clk (10ns)

        self.ddsAmpRange = [0, 5]
        self.ddsFreqRange = [0, 500]

        self.accUpdateFreq = 1.0 # accumulator update freq in MHz
        self.ddsUpdateFreq = 50.0 # dds update freq in kHz
        self.ddsFreqRes = 0xffffffff / 500.0

        self.ddsFreqRangeConv = 0xffffffff / 500.0  #8589930 # (2^32 - 1)/500 MHz
        self.ddsAmpRangeConv = 0x3ff/100 # (2^10 - 1)/100
        self.ddsFreqIncMax = 0xfffffffffff # 32 ftw + 12 acc = 44bit
        self.ddsAmpIncMax = 0x3fffff # 10 atw + 12 acc = 22bit
        self.ddsTimeRes = 1.0e3 # in us

        # initialize DACs
        self.dac0 = DAC81416(fifo_devices['DAC81416_0'])
        self.dac1 = DAC81416(fifo_devices['DAC81416_1'])
        self.dds0 = AD9959(fifo_devices['AD9959_0'])
        self.dds1 = AD9959(fifo_devices['AD9959_1'])
        self.dds2 = AD9959(fifo_devices['AD9959_2'])
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

    def initExp(self):
        print('******************************************************************************************************************************************************************')
        print('initializing experiment')
        self.mod_disable()
        reset()
        dds_lock_pll.dds_lock_pll()

    def getWord(self, bytes):
        return bytes[3] + bytes[2] + bytes[1] + bytes[0]

    def soft_trigger(self):
        trigger()

    def write_dio_point(self, point):
        #01XXAAAA TTTTTTTT DDDDDDDD
        self.fifo_dio_seq.write_axis_fifo(b"\x01\x00" + struct.pack('>H', point.address))
        self.fifo_dio_seq.write_axis_fifo(struct.pack('>I', point.time))
        self.fifo_dio_seq.write_axis_fifo(struct.pack('>I', point.outputA))
        self.fifo_dio_seq.write_axis_fifo(struct.pack('>I', point.outputB))

    def write_dio(self, byte_buf):
        #01XXAAAA TTTTTTTT DDDDDDDD
        self.fifo_dio_seq.write_axis_fifo(byte_buf, MSB_first=False)

    def write_dac_point(self, fifo, point):
        #01XXAAAA TTTTTTTT DDDDDDDD DDDDDDDD
        #phase acc shifts by 12 bit => 4096
        #clr_incr    <= gpio_in(52 downto 52);
        #acc_chan    <= gpio_in(51 downto 48);
        #acc_start   <= gpio_in(47 downto 32);
        #acc_incr    <= gpio_in(31 downto  0);
        fifo.write_axis_fifo(b"\x01\x00" + struct.pack('>H', point.address))
        fifo.write_axis_fifo(struct.pack('>I', point.time))
        fifo.write_axis_fifo(struct.pack('>I', point.clr_incr*16*256*256 + point.chan*256*256 + point.start))
        fifo.write_axis_fifo(struct.pack('>I', point.incr))

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
        print('addr', point.address, 'time', point.time, 'start', point.start, 'steps', point.steps, 'incr', point.incr, 'channel', point.chan)
        fifo.write_axis_fifo(b"\x01\x00" + struct.pack('>H', point.address))
        fifo.write_axis_fifo(struct.pack('>I', point.time))
        fifo.write_axis_fifo(struct.pack('>I', (point.start << 16) + point.steps))
        fifo.write_axis_fifo(struct.pack('>I', (point.incr << 4) + point.chan))


    def write_ftw_point(self, fifo, point):
        #01XXAAAA TTTTTTTT DDDDDDDD DDDDDDDD DDDDDDDD
        #phase acc shifts by 12 bit => 4096
        #acc_start   <= gpio_in(95 downto 64);
        #acc_steps   <= gpio_in(63 downto 48);
        #acc_incr    <= gpio_in(47 downto  4) 32 ftw + 12 phase acc;
        #acc_chan    <= to_integer(unsigned(gpio_in( 3 downto  0)));

        incr_hi = point.incr >> 28 #(point.incr & (0xffff << 28)) # acc_incr_hi    <= gpio_in(47 downto  32)
        incr_lo = point.incr & ((1 << 28) - 1)  # acc_incr_lo    <= gpio_in(31 downto  4)
        # print point.steps * 256 * 256, incr_hi,incr_lo, point.incr, point.steps * 256 * 256 + incr_hi
        # print point.incr & (0xffff>>28), point.incr & ((1<<28)-1), (point.steps << 16) + incr_hi, (incr_lo << 4) + point.chan
        # print incr_hi, incr_lo, point.steps * 256 * 256 + incr_hi, incr_lo * 16 + point.chan
        print('addr', point.address, 'time', point.time, 'start', point.start, 'steps', point.steps, 'incr', point.incr, 'channel', point.chan)
        fifo.write_axis_fifo(b"\x01\x00" + struct.pack('>H', point.address))
        fifo.write_axis_fifo(struct.pack('>I', point.time))
        fifo.write_axis_fifo(struct.pack('>I', point.start))
        fifo.write_axis_fifo(struct.pack('>I', (point.steps << 16) + incr_hi))
        fifo.write_axis_fifo(struct.pack('>I', (incr_lo << 4) + point.chan))


    def reset_DAC(self):
    	self.dac = DAC81416(device) # initialize DAC

    def set_DAC(self, channel, value):
    	valueInt = int((value-self.dacRange[0])*self.dacRes/(self.dacRange[1]-self.dacRange[0]) + 0.5)
    	assert channel>=0 and channel<=31, 'Invalid channel for DAC81416 in set_DAC'
    	assert valueInt>=0 and valueInt<=65536, 'Invalid value for DAC81416 in set_DAC'
    	if valueInt == 65536:
    		valueInt -= 1
    	if (channel > 15):
    		channel = channel-16
    		self.dac1.set_DAC(channel, valueInt)
    	else:
    		self.dac0.set_DAC(channel, valueInt)

    def set_DDS(self, channel, freq, amp=None):
    	assert channel>=0 and channel<=11, 'Invalid channel for AD9959 in set_DDS'
    	dds_lock_pll.dds_lock_pll()
    	if (channel > 7):
    		channel = channel-8
    		self.dds2.set_DDS(channel, freq, amp)
    	elif (3 < channel < 8):
    		channel = channel-4
    		self.dds1.set_DDS(channel, freq, amp)
    	else:
    		self.dds0.set_DDS(channel, freq, amp)

    def lock_PLL(self):
        dds_lock_pll.dds_lock_pll()

    def mod_enable(self):
    	self.gpio2.set_bit(0, channel=1)

    def mod_disable(self):
    	self.gpio2.clear_bit(0, channel=1)

    def reset_disable_mod(self):
    	print('disabling mod and resetting sequencers')
    	self.gpio2.write_axi_gpio(0xffff0000,channel=2)
    	self.gpio2.write_axi_gpio(0x0000ffff,channel=2)
    	self.mod_disable()

    def mod_report(self):
    	print(self.gpio2.read_axi_gpio(channel=1))

    def initialize_DAC(self, dac):
        points = []
        for i in range(16):
            points.append(DAC_seq_point(address=i,time=i,start=32768,incr=0,chan=i,clr_incr=1))
        return points

    def dac_seq_write_points(self, byte_len, byte_buf, num_snapshots):
        # print('DAC points')
        # print(byte_buf)
        points0 = []
        points1 = []
        points0Sorted = []
        points1Sorted = []
        # print('initialize DACs')
        points0 = self.initialize_DAC(0)
        points1 = self.initialize_DAC(1)

        for ii in range(num_snapshots):
            # t is in 10ns, s is in V, end is in V, duration is in 10ns
            [t, chan, s, end, duration] = self.dac_read_point(byte_buf[ii*byte_len: ii*byte_len + byte_len])
            # print('time',t,'channel', chan,'start', s,'end', end,'duration', duration)
            num_steps = int(duration/self.dacRampTimeRes + 0.5)
            # num_steps = int(duration * self.accUpdateFreq) # duration is in us and accUpdateFreq is in MHz
            if (num_steps < 1):
            	ramp_inc = 0
            	clr_incr = 1
            else:
            	ramp_inc = int(abs((end<<16)-(s<<16))/num_steps+0.5)
            	clr_incr = 0
            	# print(s, end, num_steps, ramp_inc)
            if (end<s and ramp_inc != 0):
            	ramp_inc = int(self.dacIncrMax  + 1 - ramp_inc)
            # print('time',t,'channel', chan,'start', s,'end', end,'duration', duration, 'num_step',num_steps, 'ramp_inc: ', ramp_inc)
            t = int(int(t/self.dacRampTimeRes + 0.5) * self.dacRampTimeRes + (chan%16))
            if (chan < 16):
                points0.append(DAC_seq_point(address=len(points0),time=t,start=s,incr=ramp_inc,chan=chan,clr_incr=clr_incr))
                if (num_steps >= 1):
                    points0.append(DAC_seq_point(address=len(points0),time=t+duration,start=end,incr=0,chan=chan,clr_incr=1))
            else:
                points1.append(DAC_seq_point(address=len(points1),time=t,start=s,incr=ramp_inc,chan=chan-16,clr_incr=clr_incr))
                if (num_steps >= 1):
                    points1.append(DAC_seq_point(address=len(points0),time=t+duration,start=end,incr=0,chan=chan-16,clr_incr=1))

        if (len(points0) != 0):
            points0Sorted = sorted(points0, key=lambda p: p.time)
            points0Sorted.append(DAC_seq_point(address=len(points0Sorted), time=0,  start=0,incr=0,chan=0,clr_incr=0))
        if (len(points0) != 0):
            points1Sorted = sorted(points1, key=lambda p: p.time)
            points1Sorted.append(DAC_seq_point(address=len(points1Sorted), time=0,  start=0,incr=0,chan=0,clr_incr=0))

        # print(points0Sorted)
        # for point in points0:
        #     for key, value in point.__dict__.iteritems():
        #         if (not isinstance(value,int)):
        #             print(point)

        for i, point in enumerate(points0Sorted):
            point.address = i
            # print('DAC_seq_point(',
            # ' address=', point.address,
            # ', time = ', point.time,
            # ', start =', point.start,
            # ', incr = ', point.incr,
            # ', chan=', point.chan,
            # ', clr_incr=', point.clr_incr, ')')
            self.write_dac_point(self.fifo_dac0_seq, point)

        for i, point in enumerate(points1Sorted):
            point.address = i
            # print('DAC_seq_point(',
            # ' address=', point.address,
            # ', time = ', point.time,
            # ', start =', point.start,
            # ', incr = ', point.incr,
            # ', chan=', point.chan,
            # ', clr_incr=', point.clr_incr, ')')
            self.write_dac_point(self.fifo_dac1_seq, point)


    def dds_seq_write_points(self, byte_len, byte_buf, num_snapshots):
    	ftw_points0=[]
    	ftw_points1=[]
    	ftw_points2=[]
    	atw_points0=[]
    	atw_points1=[]
    	atw_points2=[]
    	for ii in range(num_snapshots):
    		[t, channel, aorf, s, end, duration] = self.dds_read_point(byte_buf[ii*byte_len: ii*byte_len + byte_len])

    		# num_steps = (duration/self.ddsTimeRes)
    		num_steps = int(duration * self.accUpdateFreq) # duration is in us and accUpdateFreq is in MHz
    		if (num_steps == 0):
    			ramp_inc = 0
    		else:
    			# ramp_inc = int((end-s)/num_steps) * 4 #somehow setting ddsTimeRes to 1.0e3 and add *4 works for freq ramp final value
    			ramp_inc = int((end - s)*1.0 / num_steps * 4096)
    			# * 1.0 just to avoid zero if end-s < num_steps(this is not true in python3, but in python2 int(2)/int(3)=0),
    			# 4096=0xfff+1 comes from the 12bit accumulator
    			print('time',t,'channel', channel, aorf,'start', s,'end', end,'duration', duration, 'num_step',num_steps, 'ramp_inc: ', ramp_inc)
    		if (aorf == b'f'):
    			if (ramp_inc < 0):
    				# ramp_inc = int(self.ddsFreqRangeConv + ramp_inc)
    				ramp_inc = int(self.ddsFreqIncMax + ramp_inc)
    			if (channel < 4):
    				ftw_points0.append(DDS_ftw_seq_point(address=len(ftw_points0), time=t, start=s, steps=num_steps, incr=ramp_inc, chan=channel))
    			elif (4 <= channel < 8):
    				channel = channel-4
    				ftw_points1.append(DDS_ftw_seq_point(address=len(ftw_points1), time=t, start=s, steps=num_steps, incr=ramp_inc, chan=channel))
    			else:
    				channel = channel-8
    				ftw_points2.append(DDS_ftw_seq_point(address=len(ftw_points2), time=t, start=s, steps=num_steps, incr=ramp_inc, chan=channel))
    		elif (aorf == b'a'):
    			if (ramp_inc < 0):
    				ramp_inc = int(self.ddsAmpIncMax + ramp_inc)
    			if (channel < 4):
    				atw_points0.append(DDS_atw_seq_point(address=len(atw_points0), time=t, start=s, steps=num_steps, incr=ramp_inc, chan=channel))
    			elif (4 <= channel < 8):
    				channel = channel-4
    				atw_points1.append(DDS_atw_seq_point(address=len(atw_points1), time=t, start=s, steps=num_steps, incr=ramp_inc, chan=channel))
    			else:
    				channel = channel-8
    				atw_points2.append(DDS_atw_seq_point(address=len(atw_points2), time=t, start=s, steps=num_steps, incr=ramp_inc, chan=channel))
    		else:
    			print("invalid dds type. set to 'f' for freq or 'a' for amp")
    	if (len(atw_points0) != 0):
    		atw_points0.append(DDS_atw_seq_point(address=len(atw_points0), time=0, start=0, steps=0, incr=0, chan=0))
    	if (len(atw_points1) != 0):
    		atw_points1.append(DDS_atw_seq_point(address=len(atw_points1), time=0, start=0, steps=0, incr=0, chan=0))
    	if (len(atw_points2) != 0):
    		atw_points2.append(DDS_atw_seq_point(address=len(atw_points2), time=0, start=0, steps=0, incr=0, chan=0))
    	if (len(ftw_points0) != 0):
    		ftw_points0.append(DDS_ftw_seq_point(address=len(ftw_points0), time=0, start=0, steps=0, incr=0, chan=0))
    	if (len(ftw_points1) != 0):
    		ftw_points1.append(DDS_ftw_seq_point(address=len(ftw_points1), time=0, start=0, steps=0, incr=0, chan=0))
    	if (len(ftw_points2) != 0):
    		ftw_points2.append(DDS_ftw_seq_point(address=len(ftw_points2), time=0, start=0, steps=0, incr=0, chan=0))

    	for point in ftw_points0:
    		print("ftw dds0")
    		self.write_ftw_point(self.fifo_dds0_ftw_seq, point)
    	for point in ftw_points1:
    		print("ftw dds1")
    		self.write_ftw_point(self.fifo_dds1_ftw_seq, point)
    	for point in ftw_points2:
    		print("ftw dds2")
    		print(point.address, point.time, point.chan)
    		self.write_ftw_point(self.fifo_dds2_ftw_seq, point)

    	for point in atw_points0:
    		print("atw dds0")
    		self.write_atw_point(self.fifo_dds0_atw_seq, point)
    	for point in atw_points1:
    		print("atw dds1")
    		self.write_atw_point(self.fifo_dds1_atw_seq, point)
    	for point in atw_points2:
    		print("atw dds2")
    		self.write_atw_point(self.fifo_dds2_atw_seq, point)

    def dds_seq_write_atw_points(self):
        points=[]
        #these ramps should complete in just under 64 ms
        points.append(DDS_atw_seq_point(address=0,time=   0,start=1023,steps=0,incr=0,chan=0)) #25% to 75%
    #    points.append(DDS_atw_seq_point(address=1,time=1000,start=256,steps=1,incr=0,chan=3)) #25% to 75%
        points.append(DDS_atw_seq_point(address=1,time=   0,start=0,steps=    0,incr=   0,chan=0))

        for point in points:
          self.write_atw_point(self.fifo_dds_atw_seq, point)

    def dds_seq_write_ftw_points(self):
    	points=[]
    	# points.append(DDS_ftw_seq_point(address=0,time=0,start=200000000,steps=0,incr=0,chan=0))
    	# ~ points.append(DDS_ftw_seq_point(address=0,time=   0,start=800000,steps=10000,incr=30000,chan=0))
    	points.append(DDS_ftw_seq_point(address=0,time=0,start=800000,steps=0,incr=0,chan=1))
    	points.append(DDS_ftw_seq_point(address=1,time=20000,start=500000,steps=0,incr=0,chan=2))
    	points.append(DDS_ftw_seq_point(address=2,time=0,start=0,steps=0,incr=0,chan=0))

    	for point in points:
    	  self.write_ftw_point(self.fifo_dds_ftw_seq, point)

    def dio_seq_write_points(self, byte_len, byte_buf, num_snapshots):
    	# print('DIO points')
    	points=[]
    	for ii in range(num_snapshots):
    		[t, outA, outB] = self.dio_read_point(byte_buf[ii*byte_len: ii*byte_len + byte_len])
    		points.append(GPIO_seq_point(address=ii,time=t,outputA=outA,outputB=outB))
    	# points.append(GPIO_seq_point(address=num_snapshots,time=6400000,outputA=0x00000000,outputB=0x00000000))
    	points.append(GPIO_seq_point(address=num_snapshots,time=0,outputA=0x00000000,outputB=0x00000000))

    	# with open("/dev/axis_fifo_0x0000000080004000", "wb") as character:
    	for point in points:
    			# writeToSeqGPIO(character, point)
    		seqWords = getSeqGPIOWords(point)
    		for word in  seqWords:
    			# print(word)
    			self.fifo_dio_seq.write_axis_fifo(word[0], MSB_first=False)

    def dio_read_point(self, snapshot):
    	# print(snapshot)
    	snapshot_split = snapshot.split(b'_')
    	t = int(snapshot_split[0].strip(b't'), 16)
    	out = snapshot_split[1].strip(b'b').strip(b'\0')
    	outB = int(out[:8], 16)
    	outA = int(out[8:], 16)
    	return [t, outA, outB]

    def dac_read_point(self, snapshot):
    	# print(snapshot)
    	snapshot_split = snapshot.split(b'_')
    	t = int(snapshot_split[0].strip(b't'), 16)
    	chan = int(snapshot_split[1].strip(b'c'), 16)
    	start = int(self.dacRes*(float(snapshot_split[2].strip(b's')) - self.dacRange[0])
    		/(self.dacRange[1]-self.dacRange[0]))
    	end = int(self.dacRes*(float(snapshot_split[3].strip(b'e')) - self.dacRange[0])
    		/(self.dacRange[1]-self.dacRange[0]))
    	duration = int(snapshot_split[4].strip(b'd').strip(b'\0'), 16)
    	return [t, chan, start, end, duration]

    def dds_read_point(self, snapshot):
    	print(snapshot)
    	snapshot_split = snapshot.split(b'_')
    	t = int(snapshot_split[0].strip(b't'), 16)
    	chan = int(snapshot_split[1].strip(b'c'), 16)
    	aorf = snapshot_split[2]
    	if (aorf == 'f'):
    		ddsConv = self.ddsFreqRangeConv
    	else:
    		ddsConv = self.ddsAmpRangeConv
    	start = int(float(snapshot_split[3].strip(b's'))*ddsConv)
    	end = int(float(snapshot_split[4].strip(b'e'))*ddsConv)
    	duration = int(snapshot_split[5].strip(b'd').strip(b'\0'), 16)
    	return [t, chan, aorf,  start, end, duration]

if __name__ == "__main__":
    from soft_trigger import trigger
    from reset_all import reset
    import dds_lock_pll

    import time

    byte_buf_dio = b't00000010_b0000000000000001\0' \
    			   b't00000A00_b0000000000000000\0' \
    			   b't00030d40_b0000000000000001\0' \
    			   b't00F61a80_b0000000000000000\0'
    #byte_buf_dds = 't00000064_c0001_f_s100.000_e050.000_000009ff0\0' \
    #			   't00010064_c0002_f_s080.000_e050.000_000009ff0\0'
    byte_buf_dds = b't00000064_c0000_f_s080.000_e000.000_d00000000\0'
                    # b't00010064_c0000_a_s100.000_e000.000_d00000000\0'

    # byte_buf_dac = 't00000000_c0000_s05.000_e00.000_d0000c350\0' \
    # 			   't005b8d80_c0000_s05.000_e00.000_d0000c350\0'
    byte_buf_dac = b't00008100_c0000_s00.000_e00.000_d00000000\0' \
    			   b't000b8d80_c0000_s00.000_e05.000_d0011c350\0' \
                   b't001c8d80_c0001_s00.000_e02.000_d0011c350\0'

    # print(byte_buf_dac)
    seq = sequencer()

    reset()
    dds_lock_pll.dds_lock_pll()
    #seq.set_DDS(1, 100, 10)
    sleep(0.5)
    seq.dds_seq_write_points(46, byte_buf_dds, 1)
    #time.sleep(0.005)
    #seq.set_DDS(1, 100, 0)
    # seq.dds_seq_write_atw_points()
    #seq.dds_seq_write_ftw_points()
    # seq.set_DAC(0, 1)
    sleep(0.5)
    seq.dac_seq_write_points(42, byte_buf_dac, 3)
    sleep(0.5)
    seq.dio_seq_write_points(28, byte_buf_dio, 4)
    sleep(0.5)
    seq.mod_enable()
    sleep(0.5)
    trigger()
    sleep(0.5)
    seq.mod_disable()
