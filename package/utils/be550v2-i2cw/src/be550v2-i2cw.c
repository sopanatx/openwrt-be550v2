// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * be550v2-i2cw <bus> <addr> <byte>...
 *
 * Send one I2C write message, the way the stock TP-Link LED daemon does
 * (single I2C_RDWR message, no read back).
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_LEN 32

static int parse(const char *s, unsigned long max, unsigned long *out)
{
	char *end;

	errno = 0;
	*out = strtoul(s, &end, 0);
	if (errno || end == s || *end || *out > max)
		return -1;
	return 0;
}

int main(int argc, char **argv)
{
	unsigned char buf[MAX_LEN];
	struct i2c_msg msg;
	struct i2c_rdwr_ioctl_data data;
	unsigned long bus, addr, v;
	char path[32];
	int fd, i, n;

	n = argc - 3;
	if (n < 1 || n > MAX_LEN) {
		fprintf(stderr, "usage: %s <bus> <addr> <byte>... (1..%d bytes)\n",
			argv[0], MAX_LEN);
		return 2;
	}
	if (parse(argv[1], 255, &bus) || parse(argv[2], 0x77, &addr) || addr < 0x08) {
		fprintf(stderr, "bad bus or address\n");
		return 2;
	}
	for (i = 0; i < n; i++) {
		if (parse(argv[3 + i], 255, &v)) {
			fprintf(stderr, "bad byte '%s'\n", argv[3 + i]);
			return 2;
		}
		buf[i] = v;
	}

	snprintf(path, sizeof(path), "/dev/i2c-%lu", bus);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror(path);
		return 1;
	}

	msg.addr = addr;
	msg.flags = 0;
	msg.len = n;
	msg.buf = buf;
	data.msgs = &msg;
	data.nmsgs = 1;

	if (ioctl(fd, I2C_RDWR, &data) < 0) {
		perror("I2C_RDWR");
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}
