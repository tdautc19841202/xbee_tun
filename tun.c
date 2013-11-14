#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int tun_init(char * dev)
{
	struct ifreq ifr;
	int fd;
	int sock;
	int err;
	const char *clonedev = "/dev/net/tun";
	int flags = IFF_TUN;
	int mtu = 200;


	if( (fd = open(clonedev, O_RDWR)) < 0 ) 
	{
		return fd;
	}

	if( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		return sock;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = flags; 
	if (*dev)
	{
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}

	if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) 
	{
     		close(fd);
		fprintf(stderr, "Error %d in ioctl TUNSETIFF: %s\n", errno, strerror(errno));
		return err;
	}
	
	strcpy(dev, ifr.ifr_name);

	ifr.ifr_mtu = 200;
	if( (err = ioctl(sock, SIOCSIFMTU, (void *)&ifr)) < 0)
	{
		close(fd);
		fprintf(stderr, "Error %d in ioctl SIOCSIFMTU: %s\n", errno, strerror(errno));
		return err;
	}

	ifr.ifr_flags = IFF_UP;
	if( (err = ioctl(sock, SIOCSIFFLAGS, (void *)&ifr)) < 0)
        {
                close(fd);
                fprintf(stderr, "Error %d in ioctl SIOCSIFFLAGS: %s\n", errno, strerror(errno));
                return err;
        }



	return fd;
}

int main(int argc, char ** argv)
{
	char device[256];
	strcpy(device, "xbee_pro868");
	int ret = tun_init(device);
	printf("Created device %d %s\n", ret, device);
	sleep(120);
}
