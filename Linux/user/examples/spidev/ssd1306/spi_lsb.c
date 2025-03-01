#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
int main(int argc, char **argv) {
  char s = 1;
  int fd = open(argv[1], O_RDWR);
  ioctl(fd, SPI_IOC_WR_LSB_FIRST, &s);
}

