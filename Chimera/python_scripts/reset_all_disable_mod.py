#!/usr/bin/python

from axi_gpio import AXI_GPIO
from devices import gpio_devices

def reset():
  gpio = AXI_GPIO(gpio_devices['axi_gpio_2'])
  gpio.write_axi_gpio(0xffff0000,channel=2)
  gpio.write_axi_gpio(0x0000ffff,channel=2)
  
  #disable mod
  gpio.clear_bit(0, channel=1)

if __name__ == "__main__":
  reset()
