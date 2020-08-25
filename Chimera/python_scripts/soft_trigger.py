from axi_gpio import AXI_GPIO
from devices import gpio_devices
import struct

def trigger():
  gpio = AXI_GPIO(gpio_devices['axi_gpio_2'])
  reg=gpio.read_axi_gpio(channel=1)

  data_unset=struct.pack('4B', ord(reg[0]), ord(reg[1]), ord(reg[2]), ord(reg[3])&253) #mask bit1 on LSB
  data_set=struct.pack('4B', ord(reg[0]), ord(reg[1]), ord(reg[2]), ord(reg[3])|2)     #set bit1 on LSB and write

  gpio.write_axi_gpio(data_unset,channel=1)
  gpio.write_axi_gpio(data_set,channel=1)
  gpio.write_axi_gpio(data_unset,channel=1)

if __name__ == "__main__":
  trigger()
