#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define DS2450_C
#include "ds2450.h"

static unsigned char ds2450_addr0[5] = {0x08, 0x1f, 0x0a, 0x0c, 0x0e};
static unsigned char ds2450_addr1[5] = {0x09, 0x1f, 0x0b, 0x0d, 0x0f};

static int ds2450_memfd(ds2450_t d) {
    char path[45];
    int ret = read(d->mem_fd, NULL, 0);
    if (!ret)
	return 0;
    close(d->mem_fd);
    sprintf(path, "/sys/bus/w1/devices/%.15s/memory", d->id);
    d->mem_fd = open(path, O_RDWR);
    return d->mem_fd < 0 ? -1 : 0;
}
static int ds2450_adcfd(ds2450_t d) {
    char path[45];
    int ret = write(d->adc_fd, NULL, 0);
    if (!ret)
	return 0;
    close(d->adc_fd);
    sprintf(path, "/sys/bus/w1/devices/%.15s/convert", d->id);
    d->adc_fd = open(path, O_WRONLY);
    return d->adc_fd < 0 ? -1 : 0;
}
static int ds2450_memread(ds2450_t d, unsigned char *buf,
			  unsigned base, unsigned len) {
    int ret;
    if (lseek(d->mem_fd, base, 0) < 0 ||
	(ret = read(d->mem_fd, buf, len)) < 0)
	return -1;
    if (ret == len)
	return 0;
    errno = EFBIG;
    return -1;
}
static int ds2450_memwrite(ds2450_t d, unsigned char *buf,
			   unsigned base, unsigned len) {
    int ret;
    if (lseek(d->mem_fd, base, 0) < 0 ||
	(ret = write(d->mem_fd, buf, len)) < 0)
	return -1;
    if (ret == len)
	return 0;
    errno = EFBIG;
    return -1;
}
static int ds2450_adcstart(ds2450_t d, unsigned char mode, unsigned char ctl) {
    unsigned char buf[2];
    int ret;
    buf[0] = mode;
    buf[1] = ctl;
    if ((ret = write(d->adc_fd, buf, 2)) < 0)
	return -1;
    if (ret == 2)
	return 0;
    errno = EFBIG;
    return -1;
}

ds2450_t ds2450_new(const char *id) {
    ds2450_t d = malloc(sizeof(*d));
    if (d == NULL)
	return NULL;
    strncpy(d->id, id, 15);
    d->id[15] = '\0';
    d->mem_fd = d->adc_fd = -1;
    return d;
}
void ds2450_free(ds2450_t d) {
    if (d->mem_fd >= 0)
	close(d->mem_fd);
    if (d->adc_fd >= 0)
	close(d->adc_fd);
    free(d);
}
int ds2450_pinMode(ds2450_t d, unsigned pin, unsigned mode) {
    unsigned char buf[2];
    unsigned base, len = 1;
    if (pin > sizeof(ds2450_addr0)) {
	errno = EINVAL;
	return -1;
    }
    if (ds2450_memfd(d))
	return -1;
    base = ds2450_addr0[pin];
    if (ds2450_memread(d, buf, base, len))
	return -1;
    buf[0] = (buf[0] & ~0xc0) | (mode << 6);
    if (pin == 1) {
	buf[1] = buf[0] & ~0x04;
	buf[0] = 0xff;
	base--;
	len++;
    }
    if (ds2450_memwrite(d, buf, base, len))
	return -1;
    return 0;
}
int ds2450_digitalRead(ds2450_t d, unsigned pin) {
    unsigned char buf[1];
    unsigned base;
    if (pin > sizeof(ds2450_addr0)) {
	errno = EINVAL;
	return -1;
    }
    if (ds2450_memfd(d))
	return -1;
    base = ds2450_addr0[pin];
    if (ds2450_memread(d, buf, base, 1))
	return -1;
    return (buf[0] >> 5) & 1;
}
int ds2450_analogWrite(ds2450_t d, unsigned pin, unsigned val) {
    unsigned char buf[2];
    if (pin != 1) {
	errno = EINVAL;
	return -1;
    }
    if (ds2450_memfd(d))
	return -1;
    if (ds2450_memread(d, buf, 0x1f, 1))
	return -1;
    buf[1] = (buf[0] & ~0x04) | 0xc0;
    buf[0] = val;
    if (ds2450_memwrite(d, buf, 0x1e, 2))
	return -1;
    return 0;
}
#define pinmask(p) ((p) != 4 ? (p) != 3 ? (p) != 2 ? (p) ? 0 : 1 : 2 : 4 : 8)
int ds2450_analogConvert(ds2450_t d, unsigned p1, unsigned p2,
				     unsigned p3, unsigned p4) {
    unsigned char mode = pinmask(p1) | pinmask(p2) | pinmask(p3) | pinmask(p4);
    if (!mode) {
	errno = EINVAL;
	return -1;
    }
    if (ds2450_adcfd(d))
	return -1;
    if (ds2450_adcstart(d, mode, 0x00))
	return -1;
    return 0;
}
int ds2450_analogReadLast(ds2450_t d, unsigned pin) {
    unsigned char buf[2];
    unsigned base;
    if (pin > sizeof(ds2450_addr0) || pin == 1) {
	errno = EINVAL;
	return -1;
    }
    if (ds2450_memfd(d))
	return -1;
    base = ds2450_addr0[pin] - 0x08;
    if (ds2450_memread(d, buf, base, 2))
	return -1;
    return buf[0] | (buf[1] << 8);
}
int ds2450_analogReadLastAll(ds2450_t d, unsigned short res[4]) {
    unsigned char buf[8];
    if (ds2450_memfd(d))
	return -1;
    if (ds2450_memread(d, buf, 0x00, 8))
	return -1;
    res[0] = buf[0] | (buf[1] << 8);
    res[1] = buf[2] | (buf[3] << 8);
    res[2] = buf[4] | (buf[5] << 8);
    res[3] = buf[6] | (buf[7] << 8);
    return 0;
}
int ds2450_analogRead(ds2450_t d, unsigned pin) {
    if (ds2450_analogConvert(d, pin, -1,-1,-1))
	return -1;
    usleep(8*1000);
    return ds2450_analogReadLast(d, pin);
}
int ds2450_analogResolution(ds2450_t d, unsigned pin, unsigned val) {
    unsigned char buf[1];
    unsigned base;
    if (pin > sizeof(ds2450_addr0) || pin == 1) {
	errno = EINVAL;
	return -1;
    }
    if (ds2450_memfd(d))
	return -1;
    base = ds2450_addr0[pin];
    if (ds2450_memread(d, buf, base, 1))
	return -1;
    buf[0] = (buf[0] & ~0x0f) | (val & 0x0f);
    if (ds2450_memwrite(d, buf, base, 1))
	return -1;
    return 0;
}
int ds2450_analogReference(ds2450_t d, unsigned pin, unsigned val) {
    unsigned char buf[1];
    unsigned base;
    if (pin > sizeof(ds2450_addr0) || pin == 1) {
	errno = EINVAL;
	return -1;
    }
    if (ds2450_memfd(d))
	return -1;
    base = ds2450_addr0[pin];
    if (ds2450_memread(d, buf, base, 1))
	return -1;
    buf[0] = (buf[0] & ~0x10) | (val ? 0x10 : 0x00);
    if (ds2450_memwrite(d, buf, base, 1))
	return -1;
    return 0;
}
