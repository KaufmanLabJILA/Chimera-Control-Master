from devices import fifo_devices
from dac81416 import DAC81416

d = DAC81416(fifo_devices['DAC81416_0'])
# d.set_DAC(3, 256*1)
d.set_DAC(0, 32768)
