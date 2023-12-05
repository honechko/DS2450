#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

int main(int argc, char **argv) {
    int fd = 0;
    struct spi_ioc_transfer msg[1] = {0};
    unsigned val;
    unsigned char buf[2];

    if (argc < 3) {
	fprintf(stderr, "Usage: %s /dev/spidevX.Y value_in_millivolts\n", argv[0]);
	return 1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
	fprintf(stderr, "open(%s): %s\n", argv[1], strerror(errno));
	return 1;
    }

    sscanf(argv[2], "%u", &val);
    val &= 0x0fff;
    buf[0] = val >> 8;
    buf[1] = val;

    msg[0].tx_buf = (unsigned long)buf;
    msg[0].rx_buf = (unsigned long)buf;
    msg[0].len = 2;
    if (ioctl(fd, SPI_IOC_MESSAGE(1), msg) < 0) {
	perror("ioctl(SPI_IOC_MESSAGE)");
	return 1;
    }

    printf("shifted value: %u\n", (buf[0] << 8) | buf[1]);
}

