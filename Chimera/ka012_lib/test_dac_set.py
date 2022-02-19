from devices import fifo_devices
from dac81416 import DAC81416

d = DAC81416(fifo_devices['DAC81416_0'])
# d.set_DAC(3, 256*1)
d.set_DAC(0, int(1.0/4*65535))

d2 = DAC81416(fifo_devices['DAC81416_1'])
d2.set_DAC(0, int(1.0/4*65535))
