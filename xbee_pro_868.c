#include "xbee_pro_868.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define BAUDRATE B115200


static struct termios oldtio,newtio;

static int tun_fd;

int tun_init(char * dev)
{
	struct ifreq ifr;
	int sock;
	int err;
	const char *clonedev = "/dev/net/tun";
	int flags = IFF_TUN;
	int mtu = 200;


	if( (tun_fd = open(clonedev, O_RDWR)) < 0 ) 
	{
		return tun_fd;
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

	if( (err = ioctl(tun_fd, TUNSETIFF, (void *) &ifr)) < 0 ) 
	{
     		close(tun_fd);
		fprintf(stderr, "Error %d in ioctl TUNSETIFF: %s\n", errno, strerror(errno));
		return err;
	}
	
	strcpy(dev, ifr.ifr_name);

	ifr.ifr_mtu = mtu;
	if( (err = ioctl(sock, SIOCSIFMTU, (void *)&ifr)) < 0)
	{
		close(tun_fd);
		fprintf(stderr, "Error %d in ioctl SIOCSIFMTU: %s\n", errno, strerror(errno));
		return err;
	}

	ifr.ifr_flags = IFF_UP;
	if( (err = ioctl(sock, SIOCSIFFLAGS, (void *)&ifr)) < 0)
        {
                close(tun_fd);
                fprintf(stderr, "Error %d in ioctl SIOCSIFFLAGS: %s\n", errno, strerror(errno));
                return err;
        }



	return tun_fd;
}


int xbee_api_switch_api_mode(int fd)
{
	const char * init0 = "+++";
	const char * init1 = "ATAP 1\r";
	const char * init2 = "ATCN\r";

	int at_ret = 0;
	char at_resp[128];
	write(fd, (void *)init0, strlen(init0));
	printf("Sent %s to XBee\n", init0);
	at_ret = read(fd, (char *)at_resp, 7);
	at_resp[at_ret-1] = '\0';
	printf("XBee returned: '%s' length %d\n", at_resp, at_ret);

	write(fd, (void *)init1, strlen(init1) + 1);
	printf("Sent %s to XBee\n", init1);
	at_ret = read(fd, (char *)at_resp, 7);
	at_resp[at_ret-1] = '\0';
	printf("XBee returned: '%s' length %d\n", at_resp, at_ret);

	write(fd, (void *)init2, strlen(init2) + 1);
	printf("Sent %s to XBee\n", init2);
	at_ret = read(fd, (char *)at_resp, 7);
	at_resp[at_ret-1] = '\0';
	printf("XBee returned: '%s' length %d\n", at_resp, at_ret);

	return 0;
}

int xbee_api_open(const char * filename)
{

	int fd = open(filename, O_RDWR|O_NOCTTY);
	if(fd == -1)
	{
		fprintf(stderr, "Error opening device '%s': Error %d %s\n", filename, errno, strerror(errno));
		return -1;
	}
	fprintf(stderr, "File '%s' open\n", filename);

	int ret = tcgetattr(fd, &oldtio);
	if(ret == -1)
	{
		fprintf(stderr, "Error in tcgetattr on '%s': Error %d %s\n", filename, errno, strerror(errno));
		return -1;
	}
	fprintf(stderr, "tcgetattr\n");

	bzero(&newtio, sizeof(newtio));
        newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;
        newtio.c_lflag = 0;
        newtio.c_cc[VTIME] = 0;
        newtio.c_cc[VMIN] = 1;

	ret = tcflush(fd, TCIFLUSH);	
	if(ret == -1)
	{
		fprintf(stderr, "Error in tcflush on '%s': Error %d %s\n", filename, errno, strerror(errno));
		return -1;
	}
	fprintf(stderr, "tcflush\n");

	ret = tcsetattr(fd, TCSANOW, &newtio);
	if(ret == -1)
	{
		fprintf(stderr, "Error in tcsetattr on '%s': Error %d %s\n", filename, errno, strerror(errno));
		return -1;
	}
	fprintf(stderr, "tcsetattr\n");

	ret = xbee_api_switch_api_mode(fd);
	if(ret == -1)
	{
		fprintf(stderr, "Error switching to API mode\n");
		return -1;
	}


	return fd;
}

int xbee_api_close(int fd)
{
	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
	return 0;
}

int xbee_read(int fd, uint8_t * data, uint16_t len)
{
	fprintf(stderr, "xbee_read(%d, data, %d)\n", fd, len);
	uint8_t * it = data;
	int ret = 0;
	int red = 0;
	while(ret < len)        //Keep reading until the complete package is seen
	{
		red = read(fd, (void *)it, len-ret);
		if(red > 0)
			ret+=red;
		else
			return red;
		it+=red;
	}
	return ret;

}

uint8_t xbee_api_calc_checksum(const uint8_t * data, uint16_t len)
{
	uint8_t checksum = 0x00;
	for(uint16_t i=0; i<len; i++)
	{
		checksum += data[i];
	}
	checksum = 0xff - checksum;
	return checksum;
}

int xbee_api_print_frame(int fd, const uint8_t * data, uint16_t len)
{
	for(uint16_t i=0; i<len; i++)
		fprintf(stderr, "%02x ", data[i]);
	fprintf(stderr, "\n");
	return len;
}

int xbee_api_uart_send_frame(int fd, const uint8_t * data, uint16_t len)
{
	fprintf(stderr, "xbee_api_uart_send_frame\n");
	int ret = write(fd, (void *)data, len);
	fprintf(stderr, "xbee_api_uart_send_frame done\n");
	
	return ret;
}

int xbee_api_send_frame(int fd, uint8_t type, const uint8_t * data, uint16_t len)
{
	fprintf(stderr, "xbee_api_send_frame\n");
	uint16_t totallen = sizeof(struct xbee_api_head) + len + sizeof(struct xbee_api_checksum);
	uint8_t * buf = (uint8_t *)malloc(totallen);
	uint8_t * end = buf + totallen;

	struct xbee_api_head * api_head = (struct xbee_api_head *)buf;
	uint8_t * api_payload = (uint8_t *)(buf + sizeof(struct xbee_api_head));	
	struct xbee_api_checksum * api_checksum = (struct xbee_api_checksum *)(buf + sizeof(struct xbee_api_head) + len);

	assert(api_payload <= end);
	assert((uint8_t *)api_checksum <= end);

	api_head->start_delim = 0x7e;
	api_head->length = htobe16(len+1);
	api_head->type = type;
	
	memcpy(api_payload, data, len);

	api_checksum->checksum = xbee_api_calc_checksum(&(api_head->type), len + 1);

	xbee_api_print_frame(fd, buf, totallen);
	xbee_api_uart_send_frame(fd, buf, totallen);

	return 0;
}


int xbee_api_send_tx_req(int fd, xbee_address dest[8], const uint8_t * data, uint16_t len)
{
	fprintf(stderr, "xbee_api_send_tx_frame\n");

	uint16_t totallen = sizeof(struct xbee_api_frame_tx_req) + len;
	uint8_t * buf = (uint8_t *)malloc(totallen);

	struct xbee_api_frame_tx_req * api_tx_req = (struct xbee_api_frame_tx_req *)buf;
	api_tx_req->frame_id = 0x01;
	api_tx_req->dest_net[0] = 0xff;
	api_tx_req->dest_net[1] = 0xfe;
	api_tx_req->bcast_rad = 0;
	api_tx_req->tx_options = 0;
	
	for(int i=0; i<8; i++)
		api_tx_req->dest_host[i] = dest[i];

	uint8_t * payload = buf + sizeof(struct xbee_api_frame_tx_req);
	memcpy(payload, data, len);

	xbee_api_send_frame(fd, xbee_frametype_tx_req, (uint8_t *)api_tx_req, totallen);

	free(buf);

	return 0;		
}

int xbee_api_send_at_cmd(int fd, unsigned char cmd[2], uint8_t data[2])
{
	 fprintf(stderr, "xbee_api_send_at_cmd\n");
	struct xbee_api_frame_at_cmd api_frame_at_cmd;
	api_frame_at_cmd.frame_id = 0x01;
	api_frame_at_cmd.at_cmd[0] = cmd[0];
	api_frame_at_cmd.at_cmd[1] = cmd[1];
//	api_frame_at_cmd.at_data[0] = data[0];
//	api_frame_at_cmd.at_data[1] = data[1];

	xbee_api_send_frame(fd, xbee_frametype_at_cmd, (uint8_t *)&api_frame_at_cmd, sizeof(struct xbee_api_frame_at_cmd));
	return 0;
}

int xbee_api_read_frame(int fd)
{
	fprintf(stderr, "xbee_api_read_frame\n");
	uint8_t * buf = (uint8_t *)malloc(65535);
	
	struct xbee_api_head * api_head = (struct xbee_api_head *)buf;
	api_head->start_delim = 0x00;	
	int ret = 0;
	bool read_correct_frame = false;
	
	while(!read_correct_frame)
	{

		fprintf(stderr, "Trying to read correct frame\n");
		while(api_head->start_delim != 0x7e)
		{
			ret = xbee_read(fd, (uint8_t *)&(api_head->start_delim), 1);
			fprintf(stderr, "Read start delimiter %02x\n", api_head->start_delim);
		}
		fprintf(stderr, "Read start delimiter\n");

	
		ret = xbee_read(fd, (uint8_t *)&(api_head->length), sizeof(api_head->length));
		uint16_t payload_length = be16toh(api_head->length);
		fprintf(stderr, "Read %d length\n", payload_length);
		
		ret = xbee_read(fd, (uint8_t  *)&(api_head->type), payload_length);
		fprintf(stderr, "Read payload\n");
	
		uint8_t * payload = (uint8_t *)(buf + sizeof(struct xbee_api_head));
		struct xbee_api_checksum * api_checksum = (struct xbee_api_checksum *)(buf + sizeof(struct xbee_api_head) + payload_length);
		ret = xbee_read(fd, (uint8_t *)&(api_checksum), sizeof(struct xbee_api_checksum));
		fprintf(stderr, "Read checksum\n");

		uint8_t my_checksum = xbee_api_calc_checksum(&(api_head->type), payload_length + 1);

		if(my_checksum == api_checksum->checksum)
		{
			fprintf(stderr, "Checksum incorrect!\n");
			continue;
		}
		read_correct_frame = true;

		if(api_head->type == xbee_frametype_tx_status)
		{
			xbee_api_tx_status(fd, payload, payload_length - 1);
		}	
		else if(api_head->type == xbee_frametype_rx_packet)
		{
			xbee_api_rx_packet(fd, payload, payload_length - 1);
		}
		else if(api_head->type == xbee_frametype_at_cmd_response)
		{
			xbee_api_at_cmd_response(fd, payload, payload_length -1);
		}
		else
		{
			fprintf(stderr, "Read unknown frame of type %02x\n", api_head->type);
			read_correct_frame = false;
		
		}
	}
	return 0;		
}

void xbee_api_tx_status(int fd, const uint8_t * buf, const uint16_t len)
{
	struct xbee_api_frame_tx_response * api_tx_response = (struct xbee_api_frame_tx_response *)buf;
	fprintf(stderr, "xbee_api_tx_status: frame_id=%08x retries=%08x status=%08x\n", api_tx_response->frame_id, api_tx_response->retries, api_tx_response->status);

}


void xbee_api_rx_packet(int fd, const uint8_t * buf, const uint16_t len)
{
	struct xbee_api_frame_rx * api_frame_rx = (struct xbee_api_frame_rx *)buf;
	const uint8_t * payload = buf + sizeof(struct xbee_api_frame_rx);
	uint16_t paylen = len - sizeof(struct xbee_api_frame_rx);
	fprintf(stderr, "xbee_api_rx_packet: source=%02x%02x %02x%02x %02x%02x %02x%02x payload='%s'\n",
			api_frame_rx->src_host[0], api_frame_rx->src_host[1],
			api_frame_rx->src_host[2], api_frame_rx->src_host[3],
			api_frame_rx->src_host[4], api_frame_rx->src_host[5],
			api_frame_rx->src_host[6], api_frame_rx->src_host[7],
			payload);

	write(tun_fd, (void *)payload, paylen);

//	unsigned char at[2] = { 'D', 'B' };
//	xbee_api_send_at_cmd(fd, at, NULL);

}

void xbee_api_at_cmd_response(int fd, const uint8_t * buf, const uint16_t len)
{
	struct xbee_api_frame_at_cmd_response * api_frame_at_cmd_response = (struct xbee_api_frame_at_cmd_response *)buf;
	uint16_t paylen = len - sizeof(struct xbee_api_frame_at_cmd_response);
	const uint8_t * payload = buf + sizeof(struct xbee_api_frame_at_cmd_response);
	
	fprintf(stderr, "xbee_api_at_cmd_response: got response to command '%c%c' of type %02x length %d ", api_frame_at_cmd_response->at_cmd[0], api_frame_at_cmd_response->at_cmd[1], api_frame_at_cmd_response->at_status, paylen);
	if(paylen > 0)
	{
		const uint16_t * pay16 = (const uint16_t *)payload;
		uint16_t value = be16toh(*pay16);
		fprintf(stderr, "%d dB", -value);
	}
	fprintf(stderr, "\n");

	
}

int main(int argc, char ** argv)
{
	if(argc < 3)
	{
		fprintf(stderr, "Usage: %s xbee_device interface_name\n", argv[0]);
		return -1;
	}

        xbee_address dest[8];
        dest[0] = 0x00;
        dest[1] = 0x00;
        dest[2] = 0x00;
        dest[3] = 0x00;
        dest[4] = 0x00;
        dest[5] = 0x00;
        dest[6] = 0xff;
        dest[7] = 0xff;
		
	int xbee = xbee_api_open(argv[1]);
	int tun = tun_init(argv[2]);
	if(tun < 0)
	{
		fprintf(stderr, "tun_init returned %d! Error %d: %s\n", tun, errno, strerror(errno));
		return -1;
	}

	int pid = fork();
	if(pid == 0)
	{
		while(1)
		{
			xbee_api_read_frame(xbee);
		}
	}
	else
	{
		uint16_t buflen = 65535;
		uint8_t * buf = (uint8_t *)malloc(buflen);
		uint16_t readlen = 0;
		while(1)
		{
			readlen = read(tun_fd, (void *)buf, buflen);
			xbee_api_send_tx_req(xbee, dest, buf, readlen);
		}
	}
	
}
