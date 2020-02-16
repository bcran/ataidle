/* ataidle: a program to set the idle spindown timeout on ATA drives
 * 
 * Author: Bruce Cran
 * Email: bruce@cran.org.uk
 *
 * Version: 0.01
 */
 
// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

// sys includes
#include <sys/types.h>
#include <sys/ata.h>
#include <sys/ioctl.h>

#include "ataidle.h"

// calculate the idle timer value to send to the drive.
int32_t 
ata_getidleval(uint32_t idle_mins, uint8_t *timer_val)
{
	uint32_t rc = 0;
	
	// idle value of 0 disabled spindown
	if( idle_mins == 0 )
		*timer_val = 0;
	
	// value is (value*5)s between 1 and 20 minutes
	if( idle_mins <= 20 )
		*timer_val = (idle_mins*60)/5;
	
	// special case for 21 minutes
	if( idle_mins == 21 )
		*timer_val = 252;

	// there is no encoding for values between 21 and 30 minutes
	if( idle_mins > 21 && idle_mins < 30 ) {
		printf("cannot set timeout for values 20-30 minutes\n" );
		rc = -1;
	}
	
	// after 30 mins, encoding is (idle_mins-29)*30, so you
	// can only encode multiples of 30 minutes
	if( idle_mins >= 30) {
		if( ((idle_mins % 30) != 0) || (idle_mins > 330) ) {
			printf( "idle value must be a multiple of 30 minutes, "
					"up to 5 hours\n" );
			rc = -1;
		}

		*timer_val = 241 + (idle_mins/30);
	}

	return rc;
}
		
// open the ata control device, /dev/ata rw
int ata_open(int *fd) {
	int rc = 0;
	*fd = open("/dev/ata", O_RDWR);
	if( *fd == -1) {
		printf( "error opening ata control device\n" );
		rc = -1;
	}

	return rc;
}

// command the device to spindown after idle_mins of no disk activity
int32_t 
ata_setidle(int fd, uint32_t ata_chan, uint32_t ata_dev, uint32_t idle_mins)
{
	uint8_t timer_val = 0;
	int32_t rc = 0;

	if(!ata_devpresent( fd, ata_chan, ata_dev )) {
		printf("error: no device found at chan %d, dev %d\n", ata_chan, ata_dev);
		rc = -1;
	}
	
	if(!rc)
		rc = ata_getidleval( idle_mins, &timer_val );	
		
	struct ata_cmd * atacmd = ( struct ata_cmd* ) malloc( 512 );
	memset( atacmd, 0, 512 );
	
	atacmd->channel = ata_chan;
 	atacmd->device = ata_dev;
 	atacmd->cmd = ATAREQUEST;
	atacmd->u.request.flags = ATA_CMD_CONTROL;
	// wait 10 seconds for command to execute
	atacmd->u.request.timeout = 10; 
	atacmd->u.request.count = 0;
	atacmd->u.request.u.ata.command = ATA_IDLE;
 	atacmd->u.request.u.ata.count = timer_val;
 	
	if(!rc) {
		if( ioctl( fd, IOCATA, atacmd ) == -1 ) {
 			perror("error setting idle timeout");
			rc = -1;
		} else {
			printf("set chan %d, dev %d to spindown after %d minutes\n", 
					ata_chan, ata_dev, idle_mins);
		}
	}
	
	return rc;
}

bool
ata_devpresent(int fd, int32_t chan, int32_t dev) 
{
	bool devpresent = false;
	struct ata_cmd atacmd;
	
	memset(&atacmd, 0, sizeof(struct ata_cmd));
	
	atacmd.channel = chan;
	atacmd.device = -1;
	atacmd.cmd = ATAGPARM;
	
	ioctl( fd, IOCATA, &atacmd );

	if(atacmd.u.param.type[dev])
		devpresent = true;
	
	return devpresent;
}

// return the maximum valid channel id
int32_t 
ata_getmaxchan(int fd, int32_t *maxchan)
{
	int32_t rc = 0;
    struct ata_cmd * atacmd = ( struct ata_cmd* ) malloc( 512 );
	memset( atacmd, 0, 512 );
	char cbuf[512];
	memset(cbuf,0,512);
	
	atacmd->channel = 0;
	atacmd->device = 0;
	atacmd->cmd = ATAGMAXCHANNEL;
	
	if(!rc) {
		if( ioctl( fd, IOCATA, atacmd ) == -1 ) {
			perror("error getting maximum channel");
			rc = -1;
		} else {
			*maxchan = atacmd->u.maxchan;
		}
	}

	return rc;											
}

void usage()
{
	printf( "usage: ataidle -S <idle_time> <channel> <device>\n" );
	exit(EXIT_FAILURE);
}

int main( int argc, char ** argv )
{
	int ch, rc = 0, chan, dev;
	long opt_val;
	int fd;
	int maxchan = 0;
	
	if( argc == 1 )
		usage();
	// parse the command-line options
	if( argc < 4 )
		usage();

	chan = strtol(argv[argc-2], NULL, 0);
	rc = ata_open(&fd);
	if(!rc)
		rc = ata_getmaxchan(fd, &maxchan);

	
	if( (errno == EINVAL) || (errno == ERANGE) || 
			(chan < 0) || (chan > maxchan) ) {
		printf("invalid channel\n");
		rc = -1;
	}
	
	dev  = strtol(argv[argc-1], NULL, 0);

	if( (errno == EINVAL) || (errno == ERANGE) || (dev < 0) || (dev > 1) ) {
		printf("invalid device\n");
		rc = -1;
	}

	if(!rc)
		rc = ata_open(&fd);
	
	if(!rc) {
	
		while ((ch = getopt(argc, argv, "S:I")) != -1) {
			switch(ch) {
				case 'S':
					opt_val = strtol( optarg, NULL, 0);
					if( (errno == EINVAL) || (errno == ERANGE) ) {
						printf("invalid idle value\n");
						rc = -1;
					}

					if(!rc)
						rc = ata_setidle( fd, chan, dev, opt_val );
					break;
			}
		}
	}
	
	return rc;
}
