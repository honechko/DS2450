//#define DEBUG
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/crc16.h>
#ifdef CONFIG_W1_SLAVE_DS2450_SPI
# include <linux/spi/spi.h>
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_GPIO
# include <linux/gpio/driver.h>
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_PWM
# include <linux/pwm.h>
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_IIO
# include <linux/iio/iio.h>
# include <linux/delay.h>
#endif
#if defined(CONFIG_W1_SLAVE_DS2450_SPI) || \
    defined(CONFIG_W1_SLAVE_DS2450_GPIO) || \
    defined(CONFIG_W1_SLAVE_DS2450_PWM) || \
    defined(CONFIG_W1_SLAVE_DS2450_IIO)
# define DS2450_HAS_PRIVDATA
# include <linux/slab.h>
#endif
#include <linux/w1.h>

#define W1_FAMILY_DS2450		0x20

#define W1_DS2450_READ_MEMORY		0xAA
#define W1_DS2450_WRITE_MEMORY		0x55
#define W1_DS2450_CONVERT_V		0x3C
#ifdef CONFIG_W1_SLAVE_DS2450_SPI
#define W1_DS2450_SPI			0x5A
#endif

#define DS2450_PAGE_SIZE		8
#define DS2450_MEMORY_SIZE		32
#define DS2450_RETRIES			3

#define CRC16_INIT	0
#define CRC16_VALID	0xb001

#ifdef DS2450_HAS_PRIVDATA
struct ds2450_data {
#ifdef CONFIG_W1_SLAVE_DS2450_SPI
	struct spi_controller	*spi_master;
#ifdef HAVE_W1_RESET_COUNTER
	u64			spi_last_rcnt;
#endif
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_GPIO
	struct gpio_chip	*gpio_chip;
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_PWM
	struct pwm_chip		*pwm_chip;
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_IIO
	struct iio_dev		*iio_dev;
#endif
};
#endif

static u8 ds2450_reset_select(struct w1_slave *sl, u8 *resume)
{
	if (*resume) {
		if (w1_reset_resume_command(sl->master)) {
			*resume = 0;
			return 0;
		}
	} else {
		if (w1_reset_select_slave(sl))
			return 0;
		*resume = 1;
	}

	return 1;
}

static u8 ds2450_adc_convert(struct w1_slave *sl, u8 *resume, u8 *retries,
			     u8 mask, u8 control)
{
	u8 tbuf[3], crcbuf[2];
	u8 retrycnt = 0;
	int crc;

	tbuf[0] = W1_DS2450_CONVERT_V;
	tbuf[1] = mask;
	tbuf[2] = control;
	crc = crc16(CRC16_INIT, tbuf, 3);

	while (retrycnt <= DS2450_RETRIES && *retries) {
		if (retrycnt++)
			--*retries;

		if (!ds2450_reset_select(sl, resume))
			continue;

		w1_write_block(sl->master, tbuf, 3);

		if (w1_read_block(sl->master, crcbuf, 2) != 2 ||
			crc16(crc, crcbuf, 2) != CRC16_VALID) {
			*resume = 0;
			continue;
		}

		return 1;
	}

	return 0;
}

static u8 ds2450_mem_read(struct w1_slave *sl, u8 *resume, u8 *retries,
			  u8 *buf, u8 start, u8 len)
{
	u8 tbuf[DS2450_PAGE_SIZE + 2];
	u8 count, remain = len;
	u8 retrycnt = 0;
	int crc;

	while (remain && retrycnt <= DS2450_RETRIES && *retries) {
		if (retrycnt++)
			--*retries;

		if (!ds2450_reset_select(sl, resume))
			continue;

		tbuf[0] = W1_DS2450_READ_MEMORY;
		tbuf[1] = start;
		tbuf[2] = 0;
		crc = crc16(CRC16_INIT, tbuf, 3);
		w1_write_block(sl->master, tbuf, 3);

		while (remain) {
			count = DS2450_PAGE_SIZE - (start % DS2450_PAGE_SIZE);
			if (w1_read_block(sl->master, tbuf, count + 2) != count + 2 ||
				crc16(crc, tbuf, count + 2) != CRC16_VALID) {
				*resume = 0;
				break;
			}
			if (count > remain)
				count = remain;
			memcpy(buf, tbuf, count);
			buf += count;
			start += count;
			remain -= count;
			crc = CRC16_INIT;
			retrycnt = 0;
		}
	}

	return len - remain;
}

static u8 ds2450_mem_write(struct w1_slave *sl, u8 *resume, u8 *retries,
			   u8 *buf, u8 start, u8 len)
{
	u8 tbuf[3];
	u8 remain = len;
	u8 retrycnt = 0;
	int crc;

	while (remain && retrycnt <= DS2450_RETRIES && *retries) {
		if (retrycnt++)
			--*retries;

		if (!ds2450_reset_select(sl, resume))
			continue;

		tbuf[0] = W1_DS2450_WRITE_MEMORY;
		tbuf[1] = start;
		tbuf[2] = 0;
		crc = crc16(CRC16_INIT, tbuf, 3);
		w1_write_block(sl->master, tbuf, 3);

		while (remain) {
			crc = crc16(crc, buf, 1);
			w1_write_block(sl->master, buf, 1);
			if (w1_read_block(sl->master, tbuf, 3) != 3 ||
				crc16(crc, tbuf, 2) != CRC16_VALID ||
				tbuf[2] != *buf) {
				*resume = 0;
				break;
			}
			buf++;
			start++;
			remain--;
			crc = start;
			retrycnt = 0;
		}
	}

	return len - remain;
}

#ifdef CONFIG_W1_SLAVE_DS2450_SPI
static inline u8 _reverse(u8 v)
{
	u8 t = v;
	v = (v << 4) | (v >> 4);
	v = ((v ^ t) & 0x66) ^ t;
	t = (v << 7) | (v >> 1);
	v = (v << 1) | (v >> 7);
	return ((v ^ t) & 0x55) ^ t;
}

static u8 ds2450_spi_xfer(struct w1_slave *sl, u8 *resume, u8 *retries,
			  u8 nbits, u8 msbfirst,
			  const u8 *txp, u8 *rxp, unsigned len)
{
	struct ds2450_data *data = sl->family_data;
	u8 buf[128], retrycnt = 0;
	unsigned i, n;

	while (retrycnt <= DS2450_RETRIES && *retries) {
		if (retrycnt++)
			--*retries;

#ifdef HAVE_W1_RESET_COUNTER
		if (data->spi_last_rcnt != sl->master->reset_counter) {
			dev_dbg(&sl->dev, "spi_acnt %llu diff +%llu\n", sl->master->reset_counter, sl->master->reset_counter - data->spi_last_rcnt);
#endif

			if (!ds2450_reset_select(sl, resume))
				continue;

			w1_write_8(sl->master, W1_DS2450_SPI);

#ifdef HAVE_W1_RESET_COUNTER
			data->spi_last_rcnt = sl->master->reset_counter;
		} else {
			dev_dbg(&sl->dev, "spi_acnt %llu\n", sl->master->reset_counter);
		}
#endif

		while (len) {
			for (i = 0; len && i < sizeof(buf) / 2; len--, i++) {
				buf[i * 2] = txp ? msbfirst ?
					     _reverse(*txp++) : *txp++ : 0;
				buf[i * 2 + 1] = 0xff;
			}
			n = i;
			dev_dbg(&sl->dev, "touch_block len=%i\n", n * 2);
#ifdef HAVE_W1_TOUCH_BLOCK
			w1_touch_block(sl->master, buf, n * 2);
#else
			for (i = 0; i < n; i++) {
				w1_write_8(sl->master, buf[i * 2]);
				buf[i * 2 + 1] = w1_read_8(sl->master);
			}
#endif
			if (rxp)
				for (i = 0; i < n; i++)
					*rxp++ = msbfirst ?
						 _reverse(buf[i * 2 + 1]) :
						 buf[i * 2 + 1];
		}

		return 1;
	}

	return 0;
}
#endif /* CONFIG_W1_SLAVE_DS2450_SPI */

#ifdef CONFIG_W1_SLAVE_DS2450_IIO
static int ds2450_adc_config(struct w1_slave *sl,
			     int chan, u8 *resref_p, u8 bits_update)
{
	u8 resume = 0, retries = 10;
	u8 resref = *resref_p;
	u8 addr = 0x08 + chan * 2;
	int ret = 0;

	//dev_dbg(&sl->dev, "adc_conf c:%hhu s:%02hhx/%02hhx\n", chan, resref, bits_update);

	mutex_lock(&sl->master->bus_mutex);

	if (ds2450_mem_read(sl, &resume, &retries, resref_p, addr, 1) != 1)
		ret = -EREMOTEIO;

	bits_update &= 0x1f;
	resref = (*resref_p & ~bits_update) | (resref & bits_update);

	if ((resref != *resref_p) && ds2450_mem_write(sl, &resume, &retries,
						      &resref, addr, 1) != 1)
		ret = -EREMOTEIO;

	mutex_unlock(&sl->master->bus_mutex);

	return ret;
}
#endif /* CONFIG_W1_SLAVE_DS2450_IIO */

#if defined(CONFIG_W1_SLAVE_DS2450_SPI) || \
    defined(CONFIG_W1_SLAVE_DS2450_GPIO) || \
    defined(CONFIG_W1_SLAVE_DS2450_PWM)
static int ds2450_gpio_generic(struct w1_slave *sl,
			       u8 *oe_p, u8 oe_get, u8 oe_set,
			       u8 *oc_p, u8 oc_get, u8 oc_set,
			       u8 *in_p, u8 in_get)
{
	u8 resume = 0, retries = 10;
	u8 mode, bitm, buf[10];
	int i, idx[5] = {0, 9, 2, 4, 6};
	u8 addr1, addr2;
	u8 oe = *oe_p, oc = *oc_p, pwm = *in_p;
	u8 wmsk = oe_set | oc_set;
	u8 rmsk = oe_get | oc_get | in_get | (wmsk & 0x1f);
	int ret = 0;

	//dev_dbg(&sl->dev, "gpio e:0x%02hhx/%02hhx=%02hhx c:0x%02hhx/%02hhx=%02hhx i:0x%02hhx\n",
	//	oe_get, oe_set, oe&oe_set, oc_get, oc_set, oc&oc_set, in_get);

	*oe_p = *oc_p = *in_p = 0;
	if (!rmsk)
		return ret;

	mutex_lock(&sl->master->bus_mutex);

	addr1 = !(rmsk & 1) ? !(rmsk & 4) ? !(rmsk & 8) ? !(rmsk & 16) ?
		0 : 0x0e : 0x0c : 0x0a : 0x08;
	addr2 = (rmsk & 0x80) ? 0x1e : 0x1f;
	if ((addr1 && ds2450_mem_read(sl, &resume, &retries,
				      &buf[addr1 - 0x08],
				      addr1, 0x10 - addr1)
					  != 0x10 - addr1) ||
		((rmsk & 0x82) && ds2450_mem_read(sl, &resume, &retries,
						  &buf[addr2 - 0x1e + 8],
						  addr2, 0x20 - addr2)
						      != 0x20 - addr2))
		ret = -EREMOTEIO;

	if (!ret) {
		for (i = 0; i < 5; i++) {
			mode = buf[idx[i]];
			bitm = 1 << i;

			if ((oe_get & bitm) && (mode & 0x80))
				*oe_p |= bitm;
			if ((oc_get & bitm) && (mode & 0x40))
				*oc_p |= bitm;
			if ((in_get & bitm) && (mode & 0x20))
				*in_p |= bitm;

			if (oe_set & bitm) {
				if (oe & bitm)
					mode |=  0x80;
				else
					mode &= ~0x80;
			}
			if (oc_set & bitm) {
				if (oc & bitm)
					mode |=  0x40;
				else
					mode &= ~0x40;
			}

			buf[idx[i]] = mode;
		}

		if (rmsk & 0x80)
			*in_p = buf[8];
		buf[8] = (wmsk & 0x80) ? pwm : 0xff;

		for (i = 1; i < 8; i += 2)
			buf[i] &= ~0x40;
		buf[9] &= ~0x04;

		addr1 = !(wmsk & 1) ? !(wmsk & 4) ? !(wmsk & 8) ? !(wmsk & 16) ?
			0 : 0x0e : 0x0c : 0x0a : 0x08;
		addr2 = !(wmsk & 16) ? !(wmsk & 8) ? !(wmsk & 4) ? !(wmsk & 1) ?
			0 : 0x08 : 0x0a : 0x0c : 0x0e;
		if (addr1 && ds2450_mem_write(sl, &resume, &retries,
					      &buf[addr1 - 0x08],
					      addr1, addr2 - addr1 + 1)
						  != addr2 - addr1 + 1)
			ret = -EREMOTEIO;

		addr2 = (wmsk & 0x80) || (buf[9] & 0xc0) == 0xc0 ?
			0x1e : 0x1f;
		if ((wmsk & 2) && ds2450_mem_write(sl, &resume, &retries,
						   &buf[addr2 - 0x1e + 8],
						   addr2, 0x20 - addr2)
						       != 0x20 - addr2)
			ret = -EREMOTEIO;
	}

	mutex_unlock(&sl->master->bus_mutex);

	return ret;
}
#endif /* CONFIG_W1_SLAVE_DS2450_(SPI|GPIO|PWM) */

static ssize_t convert_write(struct file *filp, struct kobject *kobj,
			     struct bin_attribute *bin_attr, char *buf,
			     loff_t off, size_t count)
{
	struct w1_slave *sl = kobj_to_w1_slave(kobj);
	u8 resume = 0, retries = 10;
	int ret;

	if (!buf || count != 2)
		return -EINVAL;

	mutex_lock(&sl->master->bus_mutex);

	if (ds2450_adc_convert(sl, &resume, &retries, buf[0], buf[1]))
		ret = count;
	else
		ret = -EIO;

	mutex_unlock(&sl->master->bus_mutex);

	return ret;
}

static BIN_ATTR_WO(convert, 0);

static ssize_t memory_read(struct file *filp, struct kobject *kobj,
			   struct bin_attribute *bin_attr, char *buf,
			   loff_t off, size_t count)
{
	struct w1_slave *sl = kobj_to_w1_slave(kobj);
	u8 resume = 0, retries = 10;
	int ret;

	if (!buf)
		return -EINVAL;
	if (off >= DS2450_MEMORY_SIZE || count == 0)
		return 0;
	if (count > DS2450_MEMORY_SIZE - off)
		count = DS2450_MEMORY_SIZE - off;

	mutex_lock(&sl->master->bus_mutex);

	if (ds2450_mem_read(sl, &resume, &retries, buf, off, count) == count)
		ret = count;
	else
		ret = -EIO;

	mutex_unlock(&sl->master->bus_mutex);

	return ret;
}

static ssize_t memory_write(struct file *filp, struct kobject *kobj,
			    struct bin_attribute *bin_attr, char *buf,
			    loff_t off, size_t count)
{
	struct w1_slave *sl = kobj_to_w1_slave(kobj);
	u8 resume = 0, retries = 10;
	int ret;

	if (!buf)
		return -EINVAL;
	if (off >= DS2450_MEMORY_SIZE || count == 0)
		return 0;
	if (count > DS2450_MEMORY_SIZE - off)
		count = DS2450_MEMORY_SIZE - off;

	mutex_lock(&sl->master->bus_mutex);

	if (ds2450_mem_write(sl, &resume, &retries, buf, off, count) == count)
		ret = count;
	else
		ret = -EIO;

	mutex_unlock(&sl->master->bus_mutex);

	return ret;
}

static BIN_ATTR_RW(memory, DS2450_MEMORY_SIZE);

#ifdef CONFIG_W1_SLAVE_DS2450_SPI
static int ds2450_spi_setup(struct spi_device *spi)
{
	struct w1_slave *sl = spi_controller_get_devdata(spi->controller);
	u8 oe = 0x04, oc = (spi->mode & SPI_CPOL) ? 0x04 : 0x00, in;

	dev_dbg(&sl->dev, "spi_setup n=%hhu m=0x%hx/%hhu\n",
		spi->chip_select, spi->mode, spi->bits_per_word);

	return ds2450_gpio_generic(sl, &oe, 0, 0x04,
				       &oc, 0, 0x04,
				       &in, 0);
}

static void ds2450_spi_set_cs(struct spi_device *spi, bool enable)
{
	struct w1_slave *sl = spi_controller_get_devdata(spi->controller);
	u8 oe = enable ? 0x00 : 0x03, oc = 0x03 ^ oe, in;

	if (spi->mode & SPI_NO_CS)
		return;

	dev_dbg(&sl->dev, "spi_set_cs n=%hhu v=%i m=0x%hx/%hhu\n",
		spi->chip_select, (int)enable, spi->mode, spi->bits_per_word);

	ds2450_gpio_generic(sl, &oe, 0, (1 << spi->chip_select),
				&oc, 0, (1 << spi->chip_select),
				&in, 0);
}

static int ds2450_spi_xfer_one(struct spi_controller *ctlr,
			       struct spi_device *spi,
			       struct spi_transfer *t)
{
	struct w1_slave *sl = spi_controller_get_devdata(ctlr);
	u8 resume = 0, retries = 10;
	int ret;

	dev_dbg(&sl->dev, "spi_xfer_one m=0x%hx/%hhu len=%u\n",
		spi->mode, spi->bits_per_word, t->len);

	mutex_lock(&sl->master->bus_mutex);

	if (ds2450_spi_xfer(sl, &resume, &retries,
			    spi->bits_per_word, !(spi->mode & SPI_LSB_FIRST),
			    t->tx_buf, t->rx_buf, t->len))
		ret = 0;
	else
		ret = -EREMOTEIO;

	mutex_unlock(&sl->master->bus_mutex);

	return ret;
}

static int _unregister(struct device *dev, void *cs)
{
	struct spi_device *spi = to_spi_device(dev);
	if (spi->chip_select == *(u8 *)cs)
		spi_unregister_device(spi);
	return 0;
}

static ssize_t spi_bind_write(struct file *filp, struct kobject *kobj,
			      struct bin_attribute *bin_attr, char *buf,
			      loff_t off, size_t count)
{
	struct w1_slave *sl = kobj_to_w1_slave(kobj);
	struct ds2450_data *data = sl->family_data;
	struct spi_controller *ctlr;
	struct spi_device *spi;
	ssize_t ret = count;
	u8 cs;

	if (off || !buf)
		return -EINVAL;
	if (count && buf[count - 1] == '\n')
		count--;

	if (!count) {
		if (data && data->spi_master) {
			spi_unregister_controller(data->spi_master);
			data->spi_master = NULL;
		}
		return ret;
	}

	if (*buf < '0' || *buf > '1')
		return -EINVAL;

	cs = *buf++ - '0';

	if (--count && (*buf++ != ' ' || !--count))
		return -EINVAL;
	if (count && (count >= sizeof(spi->modalias) || memchr(buf, ' ', count)))
		return -EINVAL;

	if (data && data->spi_master)
		device_for_each_child(&data->spi_master->dev, &cs, _unregister);

	if (!count)
		return ret;

	if (!data) {
		data = kzalloc(sizeof(struct ds2450_data), GFP_KERNEL);
		if (!data)
			return -ENOMEM;
		sl->family_data = data;
	}

	ctlr = data->spi_master;
	if (!ctlr) {
		ctlr = spi_alloc_master(&sl->dev, 0);
		if (!ctlr)
			return -ENOMEM;

		ctlr->num_chipselect = 2;
		ctlr->mode_bits = SPI_CPHA | SPI_CPOL | SPI_LSB_FIRST |
				  SPI_NO_CS | SPI_CS_HIGH;
		ctlr->bits_per_word_mask = SPI_BPW_MASK(8);
		ctlr->setup        = &ds2450_spi_setup;
		ctlr->set_cs       = &ds2450_spi_set_cs;
		ctlr->transfer_one = &ds2450_spi_xfer_one;
		spi_controller_set_devdata(ctlr, sl);

		if (spi_register_controller(ctlr)) {
			spi_controller_put(ctlr);
			return -ENODEV;
		}

		data->spi_master = ctlr;
#ifdef HAVE_W1_RESET_COUNTER
		data->spi_last_rcnt = sl->master->reset_counter - 1;
#endif
	}

	spi = spi_alloc_device(ctlr);
	if (!spi)
		return -ENOMEM;

	spi->chip_select = cs;
	spi->irq = -1;
	memcpy(spi->modalias, buf, count);

	if (spi_add_device(spi)) {
		spi_dev_put(spi);
		return -ENODEV;
	}

	return ret;
}

static BIN_ATTR_WO(spi_bind, 0);
#endif /* CONFIG_W1_SLAVE_DS2450_SPI */

#ifdef CONFIG_W1_SLAVE_DS2450_GPIO
static int ds2450_gpio_get_dir(struct gpio_chip *chip, unsigned int offset)
{
	struct w1_slave *sl = gpiochip_get_data(chip);
	u8 oe, oc, in;
	int ret;

	dev_dbg(&sl->dev, "gpio_get_dir n=%u\n", offset);
	ret = ds2450_gpio_generic(sl, &oe, (1 << offset), 0,
				      &oc, 0, 0,
				      &in, 0);
	if (!ret)
		ret = !((oe >> offset) & 1);

	return ret;
}

static int ds2450_gpio_dir_input(struct gpio_chip *chip, unsigned offset)
{
	struct w1_slave *sl = gpiochip_get_data(chip);
	u8 oe = 0, oc = 0, in;

	dev_dbg(&sl->dev, "gpio_dir_input n=%u\n", offset);
	return ds2450_gpio_generic(sl, &oe, 0, (1 << offset),
				       &oc, 0, (1 << offset),
				       &in, 0);
}

static int ds2450_gpio_dir_output(struct gpio_chip *chip, unsigned offset,
				  int value)
{
	struct w1_slave *sl = gpiochip_get_data(chip);
	u8 oe = 0xff, oc = value ? 0xff : 0, in;

	dev_dbg(&sl->dev, "gpio_dir_output n=%u v=%d\n", offset, value);
	return ds2450_gpio_generic(sl, &oe, 0, (1 << offset),
				       &oc, 0, (1 << offset),
				       &in, 0);
}

static int ds2450_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct w1_slave *sl = gpiochip_get_data(chip);
	u8 oe, oc, in;
	int ret;

	dev_dbg(&sl->dev, "gpio_get n=%u\n", offset);
	ret = ds2450_gpio_generic(sl, &oe, 0, 0,
				      &oc, 0, 0,
				      &in, (1 << offset));
	if (!ret)
		ret = (in >> offset) & 1;

	return ret;
}

static int ds2450_gpio_get_multiple(struct gpio_chip *chip,
				    unsigned long *mask, unsigned long *bits)
{
	struct w1_slave *sl = gpiochip_get_data(chip);
	u8 oe, oc, in;
	int ret;

	dev_dbg(&sl->dev, "gpio_get_multiple m=0x%02hhx\n", (u8)*mask);
	ret = ds2450_gpio_generic(sl, &oe, 0, 0,
				      &oc, 0, 0,
				      &in, (u8)*mask & 0x1f);
	if (!ret)
		*bits = in;

	return ret;
}

static void ds2450_gpio_set(struct gpio_chip *chip, unsigned offset,
			    int value)
{
	struct w1_slave *sl = gpiochip_get_data(chip);
	u8 oe = 0xff, oc = value ? 0xff : 0, in;

	dev_dbg(&sl->dev, "gpio_set n=%u v=%d\n", offset, value);
	ds2450_gpio_generic(sl, &oe, 0, (1 << offset),
				&oc, 0, (1 << offset),
				&in, 0);
}

static void ds2450_gpio_set_multiple(struct gpio_chip *chip,
				     unsigned long *mask, unsigned long *bits)
{
	struct w1_slave *sl = gpiochip_get_data(chip);
	u8 oe = 0xff, oc = (u8)*bits & (u8)*mask, in;

	dev_dbg(&sl->dev, "gpio_set_multiple m=0x%02hhx b=0x%02hhx\n", (u8)*mask, oc);
	ds2450_gpio_generic(sl, &oe, 0, (u8)*mask & 0x1f,
				&oc, 0, (u8)*mask & 0x1f,
				&in, 0);
}

static int ds2450_gpio_set_config(struct gpio_chip *chip, unsigned int offset,
				  unsigned long config)
{
	struct w1_slave *sl = gpiochip_get_data(chip);
	u8 oe = 0, oc = 0, in;

	dev_dbg(&sl->dev, "gpio_set_config n=%u c=0x%04lx\n", offset, config);
	switch (pinconf_to_config_param(config)) {
	case PIN_CONFIG_BIAS_PULL_UP:
		oc = 0xff;
	case PIN_CONFIG_BIAS_PULL_PIN_DEFAULT:
	case PIN_CONFIG_BIAS_DISABLE:
		return ds2450_gpio_generic(sl, &oe, 0, (1 << offset),
					       &oc, 0, (1 << offset),
					       &in, 0);
	default:
		break;
	}

	return -ENOTSUPP;
}

static ssize_t gpio_enable_write(struct file *filp, struct kobject *kobj,
				 struct bin_attribute *bin_attr, char *buf,
				 loff_t off, size_t count)
{
	struct w1_slave *sl = kobj_to_w1_slave(kobj);
	struct ds2450_data *data = sl->family_data;
	struct gpio_chip *chip;
	ssize_t ret = count;

	if (off || !buf)
		return -EINVAL;
	if (count && buf[count - 1] == '\n')
		count--;
	if (count > 1 || (count && *buf != '0' && *buf != '1'))
		return -EINVAL;

	if (data && data->gpio_chip) {
		//if (!count || *buf == '0') {
		gpiochip_remove(data->gpio_chip);
		kfree(data->gpio_chip);
		data->gpio_chip = NULL;
		//}
		//return ret;
	}

	if (!count || *buf == '0')
		return ret;

	if (!data) {
		data = kzalloc(sizeof(struct ds2450_data), GFP_KERNEL);
		if (!data)
			return -ENOMEM;
		sl->family_data = data;
	}

	chip = kzalloc(sizeof(struct gpio_chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->label = dev_name(&sl->dev);
	chip->parent = &sl->dev;
	chip->owner = THIS_MODULE;
	chip->base = -1;
	chip->ngpio = 5;
	chip->can_sleep = true;
	chip->get_direction    = &ds2450_gpio_get_dir;
	chip->direction_input  = &ds2450_gpio_dir_input;
	chip->direction_output = &ds2450_gpio_dir_output;
	chip->get              = &ds2450_gpio_get;
	chip->get_multiple     = &ds2450_gpio_get_multiple;
	chip->set              = &ds2450_gpio_set;
	chip->set_multiple     = &ds2450_gpio_set_multiple;
	chip->set_config       = &ds2450_gpio_set_config;

	if (gpiochip_add_data(chip, sl)) {
		kfree(chip);
		return -ENODEV;
	}

	data->gpio_chip = chip;

	return ret;
}

static BIN_ATTR_WO(gpio_enable, 0);
#endif /* CONFIG_W1_SLAVE_DS2450_GPIO */

#ifdef CONFIG_W1_SLAVE_DS2450_PWM
struct ds2450_pwm_chip {
	struct pwm_chip		chip;
	void			*data;
};
#define pwmchip_get_data(chip) \
	(container_of((chip), struct ds2450_pwm_chip, chip)->data)

static void ds2450_pwm_get_state(struct pwm_chip *chip, struct pwm_device *pwm,
				 struct pwm_state *state)
{
	struct w1_slave *sl = pwmchip_get_data(chip);
	u8 oe, oc, in;

	dev_dbg(&sl->dev, "ds2450_pwm_get_state\n");

	state->polarity = PWM_POLARITY_NORMAL;
	state->period = 256;
	state->duty_cycle = 0;
	state->enabled = false;

	if (!ds2450_gpio_generic(sl, &oe, 0x82, 0,
				     &oc, 0x82, 0,
				     &in, 0)) {
		if (oe & 2)
			state->enabled = true;
		if ((oe & 2) && (oc & 2))
			state->duty_cycle = in + 1;
	}
}

static int ds2450_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			    const struct pwm_state *state)
{
	struct w1_slave *sl = pwmchip_get_data(chip);
	u8 oe, oc, in;

	dev_dbg(&sl->dev, "ds2450_pwm_apply e=%i d=%u/%u\n",
		(int)state->enabled, state->duty_cycle, state->period);

	if (state->polarity != PWM_POLARITY_NORMAL || state->period != 256)
		return -EINVAL;

	oe = state->enabled ? 0x02 : 0x00;
	oc = (state->enabled && state->duty_cycle) ? 0x02 : 0x00;
	in = (u8)state->duty_cycle - 1;

	return ds2450_gpio_generic(sl, &oe, 0, (oc & 0x02) ? 0x82 : 0x02,
				       &oc, 0, 0x02,
				       &in, 0);
}

static const struct pwm_ops ds2450_pwm_ops = {
	.get_state    = &ds2450_pwm_get_state,
	.apply        = &ds2450_pwm_apply,
	.owner = THIS_MODULE,
};

static ssize_t pwm_enable_write(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr, char *buf,
				loff_t off, size_t count)
{
	struct w1_slave *sl = kobj_to_w1_slave(kobj);
	struct ds2450_data *data = sl->family_data;
	struct ds2450_pwm_chip *chip;
	ssize_t ret = count;

	if (off || !buf)
		return -EINVAL;
	if (count && buf[count - 1] == '\n')
		count--;
	if (count > 1 || (count && *buf != '0' && *buf != '1'))
		return -EINVAL;

	if (data && data->pwm_chip) {
		//if (!count || *buf == '0') {
		pwmchip_remove(data->pwm_chip);
		kfree(container_of(data->pwm_chip,
				   struct ds2450_pwm_chip, chip));
		data->pwm_chip = NULL;
		//}
		//return ret;
	}

	if (!count || *buf == '0')
		return ret;

	if (!data) {
		data = kzalloc(sizeof(struct ds2450_data), GFP_KERNEL);
		if (!data)
			return -ENOMEM;
		sl->family_data = data;
	}

	chip = kzalloc(sizeof(struct ds2450_pwm_chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->chip.base = -1;
	chip->chip.npwm = 1;
	chip->chip.ops = &ds2450_pwm_ops;
	chip->chip.dev = &sl->dev;
	chip->data = sl;

	if (pwmchip_add(&chip->chip)) {
		kfree(chip);
		return -ENODEV;
	}

	data->pwm_chip = &chip->chip;

	return ret;
}

static BIN_ATTR_WO(pwm_enable, 0);
#endif /* CONFIG_W1_SLAVE_DS2450_PWM */

#ifdef CONFIG_W1_SLAVE_DS2450_IIO
#define iiodev_get_data(iiodev) (*(void **)(iio_priv(iiodev)))

static int ds2450_adc_read_raw(struct iio_dev *iio_dev,
			       struct iio_chan_spec const *chan,
			       int *val, int *val2, long mask)
{
	struct w1_slave *sl = iiodev_get_data(iio_dev);
	u8 resume = 0, retries = 10;
	u8 buf[2];
	int ret;

	dev_dbg(&sl->dev, "ds2450_adc_read_raw c=%i m=%li\n", chan->channel, mask);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (!val)
			return -EINVAL;

		mutex_lock(&sl->master->bus_mutex);

		if (ds2450_adc_convert(sl, &resume, &retries,
				       (1 << chan->channel), 0))
			ret = 0;
		else
			ret = -EIO;

		mutex_unlock(&sl->master->bus_mutex);

		if (ret)
			return ret;

		msleep(2);
		resume = 0;

		mutex_lock(&sl->master->bus_mutex);

		if (ds2450_mem_read(sl, &resume, &retries,
				    buf, chan->channel * 2, 2) == 2)
			ret = 0;
		else
			ret = -EIO;

		mutex_unlock(&sl->master->bus_mutex);

		if (ret)
			return ret;

		*val = buf[0] + (buf[1] << 8);
		return IIO_VAL_INT;

	case IIO_CHAN_INFO_SCALE:
		if (!val || !val2)
			return -EINVAL;

		*val  = 1;
		*val2 = 16;
		return IIO_VAL_FRACTIONAL_LOG2;
	}

	return -EINVAL;
}

static const struct iio_info ds2450_iio_info = {
	.read_raw = &ds2450_adc_read_raw,
};

static ssize_t ds2450_adc_get_pin(struct iio_dev *iio_dev,
				  uintptr_t private,
				  const struct iio_chan_spec *chan,
				  char *buf)
{
	if (!buf)
		return -EINVAL;

	return sprintf(buf, "%i\n", chan->channel ? chan->channel + 1 : 0);
}

static ssize_t ds2450_adc_get_res(struct iio_dev *iio_dev,
				  uintptr_t private,
				  const struct iio_chan_spec *chan,
				  char *buf)
{
	struct w1_slave *sl = iiodev_get_data(iio_dev);
	u8 resref = 0;
	int ret;

	dev_dbg(&sl->dev, "ds2450_adc_get_res c=%i\n", chan->channel);

	if (!buf)
		return -EINVAL;

	ret = ds2450_adc_config(sl, chan->channel, &resref, 0x00);
	if (ret)
		return ret;

	resref &= 0x0f;

	return sprintf(buf, "%u\n", resref ? (unsigned)resref : 16);
}

static ssize_t ds2450_adc_set_res(struct iio_dev *iio_dev,
				  uintptr_t private,
				  const struct iio_chan_spec *chan,
				  const char *buf, size_t len)
{
	struct w1_slave *sl = iiodev_get_data(iio_dev);
	unsigned val = 0;
	u8 resref;
	int ret;

	dev_dbg(&sl->dev, "ds2450_adc_set_res c=%i\n", chan->channel);

	if (!buf || kstrtouint(buf, 10, &val) || val > 16)
		return -EINVAL;

	resref = val & 0x0f;

	ret = ds2450_adc_config(sl, chan->channel, &resref, 0x0f);
	if (ret)
		return ret;

	return len;
}

static ssize_t ds2450_adc_avail_res(struct iio_dev *iio_dev,
				    uintptr_t private,
				    const struct iio_chan_spec *chan,
				    char *buf)
{
	if (!buf)
		return -EINVAL;

	return sprintf(buf, "[1 1 16]\n");
}

static int ds2450_adc_get_ref(struct iio_dev *iio_dev,
			      const struct iio_chan_spec *chan)
{
	struct w1_slave *sl = iiodev_get_data(iio_dev);
	u8 resref = 0;
	int ret;

	dev_dbg(&sl->dev, "ds2450_adc_get_ref c=%i\n", chan->channel);

	ret = ds2450_adc_config(sl, chan->channel, &resref, 0x00);
	if (ret)
		return ret;

	return (resref >> 4) & 1;
}

static int ds2450_adc_set_ref(struct iio_dev *iio_dev,
			      const struct iio_chan_spec *chan,
			      unsigned val)
{
	struct w1_slave *sl = iiodev_get_data(iio_dev);
	u8 resref;
	int ret;

	dev_dbg(&sl->dev, "ds2450_adc_set_ref c=%i v=%u\n", chan->channel, val);

	resref = (val & 1) << 4;

	ret = ds2450_adc_config(sl, chan->channel, &resref, 0x10);
	if (ret)
		return ret;

	return 0;
}

static const char * const ds2450_adc_references[] = { "vcc", "1v1" };

static const struct iio_enum ds2450_adc_reference = {
	.items = ds2450_adc_references,
	.num_items = ARRAY_SIZE(ds2450_adc_references),
	.get = &ds2450_adc_get_ref,
	.set = &ds2450_adc_set_ref,
};

static const struct iio_chan_spec_ext_info ds2450_adc_ext_info[] = {
	{
		.name = "pin",
		.read  = &ds2450_adc_get_pin,
		.shared = IIO_SEPARATE,
	},
	{
		.name = "resolution",
		.read  = &ds2450_adc_get_res,
		.write = &ds2450_adc_set_res,
		.shared = IIO_SEPARATE,
	},
	{
		.name = "resolution_available",
		.read  = &ds2450_adc_avail_res,
		.shared = IIO_SHARED_BY_TYPE,
	},
	IIO_ENUM("reference", IIO_SEPARATE, &ds2450_adc_reference),
	IIO_ENUM_AVAILABLE("reference", &ds2450_adc_reference),
	{ },
};

#define DS2450_ADC_CHANNEL(_chan, _name) {			\
	.type = IIO_VOLTAGE,					\
	.indexed = 1,						\
	.channel = (_chan),					\
	.datasheet_name = (_name),				\
	.ext_info = ds2450_adc_ext_info,			\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),	\
}

static const struct iio_chan_spec ds2450_adc_channels[] = {
	DS2450_ADC_CHANNEL(0, "P0"),
	DS2450_ADC_CHANNEL(1, "P2"),
	DS2450_ADC_CHANNEL(2, "P3"),
	DS2450_ADC_CHANNEL(3, "P4"),
};

static ssize_t iio_enable_write(struct file *filp, struct kobject *kobj,
				struct bin_attribute *bin_attr, char *buf,
				loff_t off, size_t count)
{
	struct w1_slave *sl = kobj_to_w1_slave(kobj);
	struct ds2450_data *data = sl->family_data;
	struct iio_dev *iio_dev;
	ssize_t ret = count;

	if (off || !buf)
		return -EINVAL;
	if (count && buf[count - 1] == '\n')
		count--;
	if (count > 1 || (count && *buf != '0' && *buf != '1'))
		return -EINVAL;

	if (data && data->iio_dev) {
		//if (!count || *buf == '0') {
		iio_device_unregister(data->iio_dev);
		iio_device_free(data->iio_dev);
		data->iio_dev = NULL;
		//}
		//return ret;
	}

	if (!count || *buf == '0')
		return ret;

	if (!data) {
		data = kzalloc(sizeof(struct ds2450_data), GFP_KERNEL);
		if (!data)
			return -ENOMEM;
		sl->family_data = data;
	}

	iio_dev = iio_device_alloc(sizeof(void *));
	if (!iio_dev)
		return -ENOMEM;

	iio_dev->dev.parent = &sl->dev;
	iio_dev->info = &ds2450_iio_info;
	iio_dev->name = dev_name(&sl->dev);
	iio_dev->modes = INDIO_DIRECT_MODE;
	iio_dev->channels = ds2450_adc_channels;
	iio_dev->num_channels = 4;
	iiodev_get_data(iio_dev) = sl;

	if (iio_device_register(iio_dev)) {
		iio_device_free(iio_dev);
		return -ENODEV;
	}

	data->iio_dev = iio_dev;

	return ret;
}

static BIN_ATTR_WO(iio_enable, 0);
#endif /* CONFIG_W1_SLAVE_DS2450_IIO */

static struct bin_attribute *w1_ds2450_bin_attrs[] = {
	&bin_attr_convert,
	&bin_attr_memory,
#ifdef CONFIG_W1_SLAVE_DS2450_SPI
	&bin_attr_spi_bind,
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_GPIO
	&bin_attr_gpio_enable,
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_PWM
	&bin_attr_pwm_enable,
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_IIO
	&bin_attr_iio_enable,
#endif
	NULL,
};

static const struct attribute_group w1_ds2450_group = {
	.bin_attrs = w1_ds2450_bin_attrs,
};

static const struct attribute_group *w1_ds2450_groups[] = {
	&w1_ds2450_group,
	NULL,
};

#ifdef DS2450_HAS_PRIVDATA
static int w1_ds2450_add_slave(struct w1_slave *sl)
{
	sl->family_data = NULL;
	return 0;
}

static void w1_ds2450_remove_slave(struct w1_slave *sl)
{
	struct ds2450_data *data = sl->family_data;

	if (data) {
#ifdef CONFIG_W1_SLAVE_DS2450_SPI
		if (data->spi_master)
			spi_unregister_controller(data->spi_master);
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_GPIO
		if (data->gpio_chip) {
			gpiochip_remove(data->gpio_chip);
			kfree(data->gpio_chip);
		}
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_PWM
		if (data->pwm_chip) {
			pwmchip_remove(data->pwm_chip);
			kfree(container_of(data->pwm_chip,
					   struct ds2450_pwm_chip, chip));
		}
#endif
#ifdef CONFIG_W1_SLAVE_DS2450_IIO
		if (data->iio_dev) {
			iio_device_unregister(data->iio_dev);
			iio_device_free(data->iio_dev);
		}
#endif
		kfree(data);
	}
	sl->family_data = NULL;
}
#endif

static struct w1_family_ops w1_ds2450_fops = {
#ifdef DS2450_HAS_PRIVDATA
	.add_slave      = w1_ds2450_add_slave,
	.remove_slave   = w1_ds2450_remove_slave,
#endif
	.groups		= w1_ds2450_groups,
};

static struct w1_family w1_ds2450_family = {
	.fid = W1_FAMILY_DS2450,
	.fops = &w1_ds2450_fops,
};

module_w1_family(w1_ds2450_family);

MODULE_AUTHOR("honechko <kernel@honey.com.ua>");
MODULE_DESCRIPTION("1-wire driver for DS2450 compatible Port Expander");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("w1-family-" __stringify(W1_FAMILY_DS2450));
