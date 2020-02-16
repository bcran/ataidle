/*-
 * Copyright 2004 Bruce Cran <bruce@cran.org.uk>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*-
 * ATAidle: a program to set the idle spindown timeout on ATA drives
 * 
 * Author: Bruce Cran <bruce@cran.org.uk>
 * Version: 0.4
 *
 */

#include <osreldate.h>

// check that we're running a version
// which has ATAng
#if __FreeBSD_version < 501110
    #error ATAidle requires ATAng to work
#endif


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

// application-specific includes
#include "ataidle.h"

// calculate the idle timer value to send to the drive.
int32_t 
ata_getidleval(uint32_t idle_mins, uint16_t *timer_val)
{
	int32_t rc = 0;
	
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
	if( (idle_mins > 21) && (idle_mins < 30) ) {
		printf("cannot set timeout for values 20-30 minutes\n" );
		rc = -2;
	}
	
	// after 30 mins, encoding is (idle_mins-29)*30, so you
	// can only encode multiples of 30 minutes
	if( idle_mins >= 30 ) {
		// if it's not a multiple of 30 minutes, or it's greater than 5 hours,
		// we can't handle it.
		if( (((idle_mins % 30) != 0) || (idle_mins > 330)) && (idle_mins != ATA_IDLEVAL_IMMEDIATE) ) {
			printf( "idle value must be a multiple of 30 minutes, "
					"up to 5 hours\n" );
			rc = -2;
		}

		// otherwise, calculate the timer value
		if(idle_mins == ATA_IDLEVAL_IMMEDIATE)
			*timer_val = ATA_IDLEVAL_IMMEDIATE;
		else
			*timer_val = 241 + (idle_mins/30);
	}

	return rc;
}
		
// open the ata control device, /dev/ata rw
int ata_open(int *fd) {
	int rc = 0;
	
	*fd = open("/dev/ata", O_RDWR);
	
	if( *fd == -1) {
		perror( "error opening ata control device" );
		rc = -1;
	}

	return rc;
}		

// send a command to the drive
int32_t
ata_cmd(int fd, uint32_t ata_chan, uint32_t ata_dev, int cmd, struct ata_cmd *atacmd)
{
	int32_t rc = 0;
	
	// check that the device actually exists, first
	if(!ata_devpresent( fd, ata_chan, ata_dev )) {
		rc = -1;
	}

	// if it does exist, send the command
	if(!rc) {
		atacmd->channel = ata_chan;
		atacmd->device = ata_dev;
		atacmd->cmd = cmd;
		rc = ioctl( fd, IOCATA, atacmd );
	}
	
	return rc;
}

// initialize the ata_cmd structure with supplied values
int32_t
ata_setataparams(int flag, int timeout, int count, int command, int seccount, struct ata_cmd * atacmd)
{
	// clear the structure to remove any random values
	memset(atacmd, 0, sizeof(struct ata_cmd));
	atacmd->u.request.flags = flag;
	atacmd->u.request.timeout = timeout;
	atacmd->u.request.count = count;
	atacmd->u.request.u.ata.command = command;
	atacmd->u.request.u.ata.count = seccount;
		
	return 0;
}

// command the device to spindown after idle_mins of no disk activity
int32_t 
ata_setidle(int fd, uint32_t ata_chan, uint32_t ata_dev, uint32_t idle_mins)
{
	uint16_t timer_val = 0;
	int32_t rc = 0;

	rc = ata_getidleval( idle_mins, &timer_val );	
		
	// allocate and then initialize the ata_cmd structure
	struct ata_cmd atacmd;
	ata_setataparams(ATA_CMD_CONTROL, ATA_CMD_TIMEOUT, 0, ATA_IDLE, timer_val, &atacmd);

	if( timer_val == ATA_IDLEVAL_IMMEDIATE ) {
			atacmd.u.request.u.ata.command = ATA_IDLE_IMMEDIATE;
	}

	// send the IDLE command to the drive
	if(!rc)
		rc = ata_cmd(fd, ata_chan, ata_dev, ATAREQUEST, &atacmd);
	
	if(rc) { 
		if(rc == -2)
 			printf("error setting idle timeout\n");
		else
			perror("error setting idle timeout");
	} else {
		if(timer_val == ATA_IDLEVAL_IMMEDIATE)
				printf("set chan %d, dev %d to idle immediately\n",
					ata_chan, ata_dev);
		else if(timer_val == 0)
			printf("turned off idle timer on chan %d, dev %d\n",
					ata_chan, ata_dev);
		else
			printf("set chan %d, dev %d to idle after %d minutes\n", 
					ata_chan, ata_dev, idle_mins);
	}
	return rc;
}

// command the device to spindown after standby_mins of no disk activity
int32_t 
ata_setstandby(int fd, uint32_t ata_chan, uint32_t ata_dev, uint32_t standby_mins)
{
	uint16_t timer_val = 0;
	int32_t rc = 0;

	rc = ata_getidleval( standby_mins, &timer_val );	

	// allocate and initialize the ata_cmd structure	
	struct ata_cmd atacmd;
	ata_setataparams(ATA_CMD_CONTROL, ATA_CMD_TIMEOUT, 0, ATA_STANDBY, timer_val, &atacmd);

	if(timer_val == ATA_IDLEVAL_IMMEDIATE)
			atacmd.u.request.u.ata.command = ATA_STANDBY_IMMEDIATE;

	// send the STANDBY command to the drive
	if(!rc)
		rc = ata_cmd(fd, ata_chan, ata_dev, ATAREQUEST, &atacmd);
	
	if(rc) {
 		perror("error setting idle timeout");
	} else {
		if( timer_val == ATA_IDLEVAL_IMMEDIATE )
				printf("set chan %d, dev %d to standby immediately\n",
					ata_chan, ata_dev);
		else if(timer_val == 0)
			printf("turned off standby timer on chan %d, dev %d\n", 
						ata_chan, ata_dev);
		else
			printf("set chan %d, dev %d to standby after %d minutes\n", 
					ata_chan, ata_dev, standby_mins);
	}
	return rc;
}

// this function sends an IDENTIFY command to a drive
int32_t ata_ident(int fd, uint32_t ata_chan, uint32_t ata_dev, struct ata_params * identity)
{
	int32_t rc = 0;
	struct ata_cmd atacmd;
	
	memset(&atacmd, 0, sizeof(struct ata_cmd));

	rc = ata_cmd(fd, ata_chan, ata_dev, ATAGPARM, &atacmd);

	if(!rc)
		memcpy(identity, &atacmd.u.param.params[ata_dev], sizeof(struct ata_params));

	return rc;
}

// sets the acoustic level on modern hard drives.   This is used to run it
// at a lower speed/performance level, which in turn reduces noise. 
int32_t ata_setacoustic(int fd, int ata_chan, int ata_dev, uint32_t acoustic_val)
{
	int32_t rc = 0;

	// range check our acoustic level parameter
	if( (acoustic_val > ATA_AUTOACOUSTIC_MAXPERF) || (acoustic_val < ATA_AUTOACOUSTIC_MINPERF) ) {
			printf("invalid acoustic value: must be 0x80-0xFE\n");
			rc = -1;
	}

	// allocate and initialize the ata_cmd structure
	struct ata_cmd atacmd;
	ata_setataparams(ATA_CMD_CONTROL, ATA_CMD_TIMEOUT, 0, ATA_SETFEATURES, acoustic_val, &atacmd);
	atacmd.u.request.u.ata.feature = ATA_AUTOACOUSTIC_ENABLE;

	// send the drive a SET_FEATURES command 
	// with a FEATURE of ATA_AUTOACOUSTIC_ENABLE
	if(!rc) {
		rc = ata_cmd( fd, ata_chan, ata_dev, ATAREQUEST, &atacmd );

		if(rc)
			perror("Set AutoAcoustic failed");
		else {
			printf("Set AutoAcoustic value to %02x\n", acoustic_val);
			if(acoustic_val == ATA_AUTOACOUSTIC_MAXPERF)
					printf("Acoustic value set to maximum performance (most acoustic impact)\n");
			else if(acoustic_val == ATA_AUTOACOUSTIC_MINPERF)
				printf("Acoustic value set to minimum performance (least acoustic impact)\n");
		}
	}
	return rc;
}


// set the Advanced Power Management mode for the drive.   Modern hard
// drives can have a number of power management states, ranging from
// lowest (least performance) to highest power consumption, which results
// in the highest performance.
int32_t ata_setapm(int fd, int ata_chan, int ata_dev, uint32_t apm_val)
{
	int32_t rc = 0;

	if( (apm_val > ATA_APM_MAXPERF) || (apm_val < ATA_APM_MINPERF) ) {
			printf("invalid acoustic value: must be %02x-%02x\n", ATA_APM_MINPERF, ATA_APM_MAXPERF);
			rc = -1;
	}

	// allocate and initialize the ata_cmd structure
	struct ata_cmd atacmd;
	ata_setataparams(ATA_CMD_CONTROL, ATA_CMD_TIMEOUT, 0, ATA_SETFEATURES, apm_val, &atacmd);
	atacmd.u.request.u.ata.feature = ATA_APM_ENABLE;

	// send the APM command to the drive as a FEATURE
	if(!rc) {
		rc = ata_cmd( fd, ata_chan, ata_dev, ATAREQUEST, &atacmd );

		if(rc)
			perror("Set APM failed");
		else {
			printf("Set APM value to %02x\n", apm_val);
			if(apm_val == ATA_APM_MAXPERF)
					printf("APM value set to maximum performance (most power consumption)\n");
			else if(apm_val == ATA_APM_MINPERF)
				printf("APM value set to minimum performance (least power consumption)\n");
		}
	}
	return rc;
}


// Dead code.  gets the power status of the drive
int32_t
ata_getpowerstatus( int fd, uint32_t ata_chan, uint32_t ata_dev )
{
	int32_t status = 0;
	struct ata_cmd atacmd;

	ata_setataparams(ATA_CMD_CONTROL, ATA_CMD_TIMEOUT, 0, ATA_POWERSTATUS_GET, 0, &atacmd);	
	
	status = ata_cmd( fd, ata_chan, ata_dev, ATAREQUEST, &atacmd );
	
	if(!status) {
			printf("debug: power mode is %d, %d\n", atacmd.u.request.u.ata.count, atacmd.u.request.count);
			status = atacmd.u.request.u.ata.count;
			printf("status register is %02x\n", atacmd.u.request.u.ata.command);
			printf("features is %02x\n", atacmd.u.request.u.ata.feature);
	} else
			perror("Error getting power status");

	return status;
}

void ata_showdeviceinfo( int fd, uint32_t ata_chan, uint32_t ata_dev )
{
	int rc = 0;
	struct ata_params ident;
	memset(&ident, 0, sizeof(struct ata_params));

	rc = ata_ident( fd, ata_chan, ata_dev, &ident );
	if(!rc) {
		char model[41];
		char serial[21];
		char firmware[9];

		memset(model, 0, 41);
		memset(serial, 0, 21);
		memset(firmware, 0, 9);

		strncpy(model, (void*) ident.model, 40);
		strncpy(serial, (void*) ident.serial, 20);
		strncpy(firmware, (void*) ident.revision, 8);

		printf("Model:\t\t%s\n", model);
		printf("Serial:\t\t%s\n", serial);
		printf("Firmware Rev:\t%s\n", firmware);
		printf("ATA revision:\t%s\n", (ident.version_major > 1)? ata_getversionstring(ident.version_major) : "unknown/pre ATA-2");
		printf("Geometry:\t%d cyls, %d heads, %d spt\n", ident.cylinders, ident.heads, ident.sectors);
		int mbsize = ((ident.lba_size_1 + (ident.lba_size_2 << 16))*512)/1048576;
		printf("Capacity:\t%d%s\n", (mbsize < 1024)? mbsize : mbsize/1024, (mbsize < 1024)? "MB" : "GB");
	} else {
		printf("error getting device information - is there a device there (check with ataidle -l)?\n");
	}
}


char * ata_getversionstring( unsigned short ata_version )
{
	char * version = (char*) malloc(10);
	memset(version, 0, 16);
	
	int i = 0;
	for(i = 0; i < 15; i++) {
		if( (ata_version >> i) > 0 ) {
			sprintf(version, "ATA-%d", i);
		}		
	}

	return version;
}
			

// list the installed devices.  This function is useful
// to find out the channel,device settings to use for
// all the other commands
void ata_listdevices( int fd )
{
	//uint32_t rc = 0;
	
	// first, do an IDENTIFY on each channel
	uint32_t numchannels;
	uint32_t numdevs;
	uint32_t i;
	
	ata_getmaxchan( fd, &numdevs );
	numchannels = numdevs/2;
	
	uint32_t identbuf_size = ( sizeof(struct ata_params) * numdevs );
	char * identbuf = (char *) malloc(identbuf_size);
	memset(identbuf, 0, identbuf_size);
	
	for(i = 0; i < numchannels; i++) {
		ata_ident( fd, i, 0, (struct ata_params*) (identbuf+(sizeof(struct ata_params)*(2*i))) );
		ata_ident( fd, i, 1, (struct ata_params*) (identbuf+((sizeof(struct ata_params)*(2*i))+sizeof(struct ata_params) )));
	}
		
	for(i = 0; i < numdevs; i++) {
		struct ata_params *ident = (struct ata_params*) (identbuf+(sizeof(struct ata_params)*i));
		char model[41];
		memset(model, 0, 41);
		strncpy(model, (void*) ident->model, 40);

		printf("Channel %d, Device %d\n", (i/2), (i%2 == 0)? 0 : 1);
		printf("\tModel: %s\n", model);
	}
		
	free(identbuf);
}

// see if a device is present by seeing what its device
// type is, when the channel is queried.  If a device is
// present is will have a non-zero type.
bool
ata_devpresent(int fd, uint32_t ata_chan, uint32_t ata_dev) 
{
	bool devpresent = false;
	struct ata_cmd atacmd;
	
	memset(&atacmd, 0, sizeof(struct ata_cmd));
	
	atacmd.channel = ata_chan;
	atacmd.device = -1;
	atacmd.cmd = ATAGPARM;

	ioctl( fd, IOCATA, &atacmd );	

	if(atacmd.u.param.type[ata_dev])
		devpresent = true;
	
	return devpresent;
}

// return the maximum valid channel id
int32_t 
ata_getmaxchan(int fd, uint32_t *maxchan)
{
	int32_t rc = 0;
    struct ata_cmd atacmd;
	
	if(!rc) {
			
		if( ata_cmd( fd, 0, 0, ATAGMAXCHANNEL, &atacmd ) == -1 ) {
			perror("error getting maximum channel");
			rc = -1;
		} else {
			*maxchan = atacmd.u.maxchan;
		}
	}

	return rc;											
}

// standard *NIX usage instructions
void usage()
{
	printf( "ataidle version 0.4\n\n"
			"usage: \n"
			"ataidle [-h] [-l] [-i] [-s] [-I idle] [-S standby] [-A acoustic] [-P apm] "
			"\t\tchannel device\n\n"
			"arguments:\n"
		    "-h\t\tshow this help\n"
			"-l\t\tlist installed devices\n"
			"-I\t\tset the idle timeout in minutes\n"
			"-i\t\tput the drive into idle mode immediately\n"
			"-S\t\tset the standby timeout in minutes\n"
			"-s\t\tput the drive into standby mode immediately\n"
			"-A\t\tset the acoustic level, usually values 128-254\n"
			"-P\t\tset the power management level\n"
		 	"channel\t\tthe channel the drive is connected to\n"
		 	"device\t\tthe device (0 or 1) the drive is connected to\n\n"
			"if no options are specified, information about the device\n"
			"connected at <channel,device> will be shown\n\n"
		 	"note:\tboth channel and device can be found\n"
		   	"\tby running \"ataidle -l\"\n" );
	exit(EXIT_FAILURE);
}

// wrapper around strtol, with additional error checking
// and simplified interface.
int32_t ata_strtolong(char * src, long * dest)
{
	long val = strtol(src, NULL, 10);
	int32_t rc = -1;
	if( ! ((errno == EINVAL) || (errno == ERANGE)) ) {
		rc = 0;
		*dest = val;
	}
	
	return rc;
}	

// check that the user has supplied us with valid arguments
bool checkargs(int argc, char ** argv, char * optstr, bool *needchandev) 
{
	int ch;
	int numargs = 1; // 1 argument, the name of the program
	bool goodargs = false;
	*needchandev = false;
	
	while ((ch = getopt(argc, argv, optstr)) != -1) {
		numargs++;
		switch(ch) {	
			case 'S':
				*needchandev = true;
				numargs++;
				break;

			case 's':
				*needchandev = true;
				break;

			case 'I':
				*needchandev = true;
				numargs++;
				break;

			case 'i':
				*needchandev = true;
				break;

			case 'P':
				*needchandev = true;
				numargs++;
				break;

			case 'A':
				*needchandev = true;
				numargs++;
				break;

			case 'l':
				// since we're just listing devices
				// found in the system, we don't need
				// any additional arguments or anything,
				// so don't do anything here.
				break;
				
			case 'h':
				printf("help:\n");
				usage();
		}
	}

	// if we've only got 3 arguments,
	// then we'll want to show the info
	// about the specified device.
	if(argc == 3)
		*needchandev = true;

	if(*needchandev)
		numargs += 2;

	if(argc == numargs) {
		long lval;
		if( (*needchandev) && ((!ata_strtolong(argv[argc-1], &lval)) &&
			(!ata_strtolong(argv[argc-2], &lval))) )
			// then valid args
			goodargs = true;
		else if(!(*needchandev))
			goodargs = true;
	}

	return goodargs;
}


// the main function
int main( int argc, char ** argv )
{
	int rc = 0;
	int ch, chan, dev;
	int fd;
	long opt_val;
	uint32_t maxchan = 0;
	bool needchandev;
	char * optstr = "hlA:S:sI:iP:";

	if( (argc == 1) || (!checkargs(argc, argv, optstr, &needchandev)) )
		usage();
	
	rc = ata_open(&fd);
	
	if(!rc && needchandev)
		rc = ata_strtolong(argv[argc-2], (long*) &chan);
	
	if(!rc)
		rc = ata_getmaxchan(fd, &maxchan);

	if( (!rc) && needchandev && ((chan < 0) || (chan > maxchan)) ) {
		printf("invalid channel\n");
		rc = -1;
	}
	
	if(!rc && needchandev) {
		rc = ata_strtolong(argv[argc-1], (long*) &dev);
		if( (rc) || (dev < 0) || (dev > 1) ) {
			printf("invalid device\n");
			rc = -1;
		}
	}

	// now we've done all the checking of parameters and everything,
	// let's see what the user wants us to do.
	if(!rc) {
		optreset = 1;
		optind = 1;
		opterr = 1;
		
		while ((ch = getopt(argc, argv, optstr)) != -1) {
			switch(ch) {	
				
				// S for Standby
				case 'S':
					rc = ata_strtolong(optarg, &opt_val);	
					if(rc)
						printf("invalid standby value\n");
					else
						rc = ata_setstandby( fd, chan, dev, opt_val );
					break;

				case 's':
					rc = ata_setstandby( fd, chan, dev, ATA_IDLEVAL_IMMEDIATE );
					break;

				// I for Idle
				case 'I':
					rc = ata_strtolong(optarg, &opt_val);	
					
					if(rc)
						printf("invalid idle value\n");
					else
						rc = ata_setidle( fd, chan, dev, opt_val );
					break;

				case 'i':
					rc = ata_setidle(fd, chan, dev, ATA_IDLEVAL_IMMEDIATE);
					break;

				// A for AutoAcoustic
				case 'A':
					rc = ata_strtolong(optarg, &opt_val);
					if(rc)
							printf("invalid acoustic value\n");
					else 
							rc = ata_setacoustic( fd, chan, dev, opt_val );
					break;
		
				// P for APM
				case 'P':
					rc = ata_strtolong(optarg, &opt_val);
					if(rc)
							printf("invalid apm value\n");
					else
							rc = ata_setapm( fd, chan, dev, opt_val );
					break;
				
				case 'l':
					printf("Listing Devices:\n\n");
					ata_listdevices(fd);
					break;
					
				// h is help
				case 'h':
					usage();
					break;
			}
		}
	}

	// fall-through: check if we've just got 2 arguments,
	// the channel and device: if so, just show information
	// about that device.
	if( argc == 3 && !rc ) {
		printf("Device Info:\n\n");
		ata_showdeviceinfo(fd, chan, dev);
	}

	// if we successfully opened the ata control
	// device, now's the time to close it.
	if(fd > 0)
		close(fd);
	
	return rc;
}
