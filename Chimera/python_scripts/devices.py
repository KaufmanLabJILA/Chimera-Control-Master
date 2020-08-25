fifo_devices = {
    'DAC81416_0'        : '/dev/axis_fifo_0x0000000080002000',
    'DAC81416_1'        : '/dev/axis_fifo_0x0000000080003000',
    'GPIO_seq'          : '/dev/axis_fifo_0x0000000080004000',
    'DAC81416_0_seq'    : '/dev/axis_fifo_0x000000008000b000',
    'DAC81416_1_seq'    : '/dev/axis_fifo_0x000000008000c000',
    'AD9959_0'          : '/dev/axis_fifo_0x0000000080005000',
    'AD9959_1'          : '/dev/axis_fifo_0x0000000080006000',
    'AD9959_2'          : '/dev/axis_fifo_0x0000000080007000',
    'AD9959_0_seq_atw'  : '/dev/axis_fifo_0x000000008000e000',
    'AD9959_1_seq_atw'  : '/dev/axis_fifo_0x000000008000f000',
    'AD9959_2_seq_atw'  : '/dev/axis_fifo_0x0000000080010000',
    'AD9959_0_seq_ftw'  : '/dev/axis_fifo_0x0000000080008000',
    'AD9959_1_seq_ftw'  : '/dev/axis_fifo_0x0000000080009000',
    'AD9959_2_seq_ftw'  : '/dev/axis_fifo_0x000000008000a000'
  }

gpio_devices = {
    'axi_gpio_0': 0x0000000080000000,
    'axi_gpio_1': 0x0000000080001000,
    'axi_gpio_2': 0x000000008000D000
  }

if __name__ == "__main__":
  for dev in fifo_devices:
    print("{}  -  {}".format(dev,fifo_devices[dev]))
  print("")
  for dev in gpio_devices:
    print("{0:s}  -  {1:#018x}".format(dev,gpio_devices[dev]))
