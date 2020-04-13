#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LEN_BYTE_BUF 4

int main (int argc, char* argv[])
{
  //DAC0
  int seq_fd;

  seq_fd = open("/dev/axis_fifo_0x0000000080004000", O_RDWR);
        
  if(seq_fd == -1)
  {
    printf("Cannot open sequencer device");
    exit(1);
  }

  char byte_buf[LEN_BYTE_BUF];
  byte_buf[3]=0x01; //enable for DIO
  byte_buf[2]=0x00;
  byte_buf[1]=0x00; //address 0x00
  byte_buf[0]=0x00; //address 0x00
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  byte_buf[3]=0x00; //timestamp (32-bit)
  byte_buf[2]=0x00; //timestamp (32-bit)
  byte_buf[1]=0x00; //timestamp (32-bit)
  byte_buf[0]=0x01; //timestamp (32-bit)
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  byte_buf[3]=0x00; //output (32-bit)
  byte_buf[2]=0x00; //output (32-bit)
  byte_buf[1]=0x00; //output (32-bit)
  byte_buf[0]=0x02; //output (32-bit)
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);



  byte_buf[3]=0x01; //enable for DIO
  byte_buf[2]=0x00;
  byte_buf[1]=0x00; //address 0x01
  byte_buf[0]=0x01; //address 0x01
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  byte_buf[3]=0x00; //timestamp (32-bit)
  byte_buf[2]=0x00; //timestamp (32-bit)
  byte_buf[1]=0x00; //timestamp (32-bit)
  byte_buf[0]=0xC8; //timestamp (32-bit)
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  byte_buf[3]=0x00; //output (32-bit)
  byte_buf[2]=0x00; //output (32-bit)
  byte_buf[1]=0x00; //output (32-bit)
  byte_buf[0]=0x01; //output (32-bit)
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);



  byte_buf[3]=0x01; //enable for DIO
  byte_buf[2]=0x00;
  byte_buf[1]=0x00; //address 0x02
  byte_buf[0]=0x02; //address 0x02
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  byte_buf[3]=0x00; //timestamp (32-bit)
  byte_buf[2]=0x00; //timestamp (32-bit)
  byte_buf[1]=0x01; //timestamp (32-bit)
  byte_buf[0]=0x2C; //timestamp (32-bit)
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  byte_buf[3]=0x00; //output (32-bit)
  byte_buf[2]=0x00; //output (32-bit)
  byte_buf[1]=0x00; //output (32-bit)
  byte_buf[0]=0x02; //output (32-bit)
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);



  byte_buf[3]=0x01; //enable for DIO
  byte_buf[2]=0x00;
  byte_buf[1]=0x00; //address 0x02
  byte_buf[0]=0x03; //address 0x02
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  byte_buf[3]=0x00; //timestamp (32-bit)
  byte_buf[2]=0x00; //timestamp (32-bit)
  byte_buf[1]=0x01; //timestamp (32-bit)
  byte_buf[0]=0x90; //timestamp (32-bit)
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  byte_buf[3]=0x00; //output (32-bit)
  byte_buf[2]=0x00; //output (32-bit)
  byte_buf[1]=0x00; //output (32-bit)
  byte_buf[0]=0x01; //output (32-bit)
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);



  // TERMINATION
  byte_buf[3]=0x01; //enable for DIO
  byte_buf[2]=0x00;
  byte_buf[1]=0x00; //address 0x03
  byte_buf[0]=0x04; //address 0x03
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  byte_buf[3]=0x00; //timestamp (32-bit)
  byte_buf[2]=0x00; //timestamp (32-bit)
  byte_buf[1]=0x00; //timestamp (32-bit)
  byte_buf[0]=0x00; //timestamp (32-bit)
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  byte_buf[3]=0x00; //output (32-bit)
  byte_buf[2]=0x00; //output (32-bit)
  byte_buf[1]=0x00; //output (32-bit)
  byte_buf[0]=0x00; //output (32-bit)
  write(seq_fd, &byte_buf, LEN_BYTE_BUF);

  close(seq_fd);
}
