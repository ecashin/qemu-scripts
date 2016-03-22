/* based on https://www.kernel.org/doc/Documentation/networking/tuntap.txt
 */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/if.h>
#include <linux/if_tun.h>

int main(void)
{
	struct ifreq ifr;
	char dev[] = "tap1";
	int fd;

	if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
		perror("open /dev/net/tun");
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TAP;
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
		perror("ioctl");
		close(fd);
		exit(1);
	}
	sleep(100000);

	return 0;
}
