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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>

#include "atadefs.h"
#include "atagen.h"

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


char * ata_getversionstring( unsigned short ata_version )
{
	char * version = (char*) malloc(10);

	if(version == 0) { /* malloc failed */
		fprintf(stderr, "malloc failed\n");
		return 0;
	}

	memset(version, 0, 16);
	
	int i = 0;
	for(i = 0; i < 15; i++) {
		if( (ata_version >> i) > 0 ) {
			sprintf(version, "ATA-%d", i);
		}		
	}

	return version;
}


// standard *NIX usage instructions
void usage()
{
	printf( "ataidle version 0.7\n\n"
			"usage: \n"
			"ataidle [-h] [-l] [-i] [-s] [-I idle] [-S standby] [-A acoustic] [-P apm]\n"
			"\tchannel device\n\n"
			"arguments:\n"
		    "-h\t\tshow this help\n"
			"-l\t\tlist installed devices\n"
			"-I\t\tset the idle timeout in minutes\n"
			"-i\t\tput the drive into idle mode immediately\n"
			"-S\t\tset the standby timeout in minutes\n"
			"-s\t\tput the drive into standby mode immediately\n"
			"-A\t\tset the acoustic level, values 1-127\n"
			"-P\t\tset the power management level, values 1-254\n"
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




void byteswap(char * buf, int from, int to)
{
	int i;
	for(i = from; i <= to; i+=2) {
		char b1 = buf[i];
		buf[i] = buf[i+1];
		buf[i+1] = b1;
	}
}

void strpack(char * buf, int from, int to)
{
	int numchars = (to-from)+1;
	char *tmpbuf = (char*) malloc(numchars*sizeof(char));

	if(tmpbuf == 0) { /* malloc failed, skip the string packing */
		fprintf(stderr, "malloc failed\n");
		return;	
	}

	strncpy(tmpbuf, buf+from, numchars);
	memset(buf+from, 0, numchars);
	int i = 0;

	while(tmpbuf[i] == ' ')
		i++;

	int j, k = 0;
	for(j = i; j < numchars; j++)
	{
		buf[from+k] = tmpbuf[j];
		k++;
	}

	free(tmpbuf);
}


void ata_showdeviceinfo( struct ATA *ata, uint32_t ata_chan, uint32_t ata_dev )
{
	int rc = 0;
	struct ata_ident ident;
	memset(&ident, 0, sizeof(struct ata_ident));

	rc = ata_ident( ata, ata_chan, ata_dev,(struct ata_ident*)  &ident );
	int16_t * buf = (int16_t*) &ident;
	if(!rc) {
		char model[41];
		char serial[21];
		char firmware[9];

		memset(model, 0, 41);
		memset(serial, 0, 21);
		memset(firmware, 0, 9);

		strncpy(model, (void*) ident.model, 40);
		strncpy(serial, (void*) ident.serial, 20);
		strncpy(firmware, (void*) ident.firmware, 8);

		printf("Model:\t\t\t%s\n", model);
		printf("Serial:\t\t\t%s\n", serial);
		printf("Firmware Rev:\t\t%s\n", firmware);
		printf("ATA revision:\t\t%s\n", (ident.version_major > 1)? ata_getversionstring(buf[80]) : "unknown/pre ATA-2");
		printf("Geometry:\t\t%d cyls, %d heads, %d spt\n", buf[1], buf[3], buf[6]);
		int mbsize = ((ident.nsect[0] + (ident.nsect[1] << 16))*512)/1048576;
		printf("Capacity:\t\t%d%s\n", (mbsize < 1024)? mbsize : mbsize/1024, (mbsize < 1024)? "MB" : "GB");
		printf("SMART Supported: \t%s\n", (buf[82] & 1)? "yes" : "no" );
		if(buf[82] & 1)
			printf("SMART Enabled: \t\t%s\n", (buf[85] & 1)? "yes" : "no" );
		printf("APM Supported: \t\t%s\n", (buf[83] & 8)? "yes" : "no" );
		if(buf[83] & 8)
			printf("APM Enabled: \t\t%s\n", (buf[86] & 8)? "yes" : "no" );
		printf("AAC Supported: \t\t%s\n", (buf[83] & 0x200)? "yes" : "no" );
		if(buf[83] & 0x200)
			printf("AAC Enabled: \t\t%s\n", (buf[86] & 0x200)? "yes" : "no");
		
		if((buf[86] & 0x200)) {
			printf("Current AAC: \t\t%d\n", ((buf[94] & 0x00FF))-127);
			printf("Vendor Recommends AAC: \t%d\n", ((buf[94] & 0xFF00) >> 8)-127);
		}

		if((buf[86] & 8))
			printf("APM Value: \t\t%d\n", buf[91]);
		
		printf("Note:\tAAC = AutoAcoustic\n");
		printf("\tAPM = Advanced Power Management\n");
		printf("\tSMART = Self-Monitoring, Analysis and Reporting Technology\n");
	} else {
		printf("Could not get device information: is a device attached?\n");
	}
}

// set the Advanced Power Management mode for the drive.   Modern hard
// drives can have a number of power management states, ranging from
// lowest (least performance) to highest power consumption, which results
// in the highest performance.
int32_t ata_setapm(struct ATA *ata, int ata_chan, int ata_dev, uint32_t apm_val)
{
	int32_t rc = 0;

	// user inputs vale 1-126, 0x01-0xFE
	
	if( (apm_val > ATA_APM_MAXPERF) || (apm_val < 0) ) {
			printf("invalid APM value: must be %d-%d\n", 1, ATA_APM_MAXPERF-ATA_APM_MINPERF);
			rc = -1;
	}

	// allocate and initialize the ata_cmd structure
	ata_setataparams(ata, apm_val, 0);
	ata_setfeature_param(ata, ATA_APM_ENABLE);
	
	if(apm_val == 0)
			ata_setfeature_param(ata, ATA_APM_DISABLE);

	// send the APM command to the drive as a FEATURE
	if(!rc) {
		rc = ata_cmd(ata, ata_chan, ata_dev, ATA__SETFEATURES, 0);

		if(rc)
			perror("Set APM failed");
		else {
			printf("Set APM value to %d\n", apm_val);
			if(apm_val == ATA_APM_MAXPERF)
					printf("APM value set to maximum performance (most power consumption)\n");
			else if(apm_val == ATA_APM_MINPERF)
				printf("APM value set to minimum performance (least power consumption)\n");
			else if(apm_val == 0)
				printf("APM Disabled\n");
		}
	}
	return rc;
}

// sets the acoustic level on modern hard drives.   This is used to run it
// at a lower speed/performance level, which in turn reduces noise. 
int32_t ata_setacoustic(struct ATA *ata, int ata_chan, int ata_dev, uint32_t acoustic_val)
{
	int32_t rc = 0;
	static const int aac_user_offset = 127;

	acoustic_val += aac_user_offset; // scale it 0x80-0xFE, 128-254
	// range check our acoustic level parameter
	if( (acoustic_val > ATA_AUTOACOUSTIC_MAXPERF) || (acoustic_val < 0) ) {
			printf("invalid acoustic value: must be %d-%d\n", 1, ATA_AUTOACOUSTIC_MAXPERF - aac_user_offset);
			rc = -1;
	}

	ata_setataparams(ata, acoustic_val, 0);
	ata_setfeature_param(ata, ATA_AUTOACOUSTIC_ENABLE);

	// disable AAC if user specified 0, which is 127 once offset
	if(acoustic_val == 127)
		ata_setfeature_param(ata, ATA_AUTOACOUSTIC_DISABLE);

	// send the drive a SET_FEATURES command 
	// with a FEATURE of ATA_AUTOACOUSTIC_ENABLE
	if(!rc) {
		rc = ata_cmd( ata, ata_chan, ata_dev, ATA__SETFEATURES, 0 );

		if(rc)
			perror("Set AutoAcoustic failed");
		else {
			printf("Set AutoAcoustic value to %d\n", acoustic_val-aac_user_offset);
			if(acoustic_val == ATA_AUTOACOUSTIC_MAXPERF)
					printf("Acoustic value set to maximum performance (most acoustic impact)\n");
			else if(acoustic_val == ATA_AUTOACOUSTIC_MINPERF)
				printf("Acoustic value set to minimum performance (least acoustic impact)\n");
			else if(acoustic_val == 127)
				printf("Acoustic management disabled\n");
		}
	}
	return rc;
}

// command the device to spindown after idle_mins of no disk activity
int32_t 
ata_setidle(struct ATA *ata, uint32_t chan, uint32_t dev, uint32_t idle_mins)
{
	uint16_t timer_val = 0;
	int32_t rc = 0;

	rc = ata_getidleval( idle_mins, &timer_val );	
		
	if(timer_val == ATA_IDLEVAL_IMMEDIATE)
		ata_setataparams(ata, 0, 0);
	else
		ata_setataparams(ata, timer_val, 0);

	// send the IDLE command to the drive
	if(!rc) {
		if(timer_val == ATA_IDLEVAL_IMMEDIATE)
			rc = ata_cmd(ata, chan, dev, ATA_IDLE_IMMEDIATE, 0);
		else
			rc = ata_cmd(ata, chan, dev, ATA_IDLE, 0);
	}

	if(rc) { 
		if(rc == -2)
 			printf("error setting idle timeout\n");
		else
			perror("error setting idle timeout");
	} else {
		if(timer_val == ATA_IDLEVAL_IMMEDIATE)
				printf("set chan %d, dev %d to idle immediately\n",
					chan, dev);
		else if(timer_val == 0)
			printf("turned off idle timer on chan %d, dev %d\n",
					chan, dev);
		else
			printf("set chan %d, dev %d to idle after %d minutes\n", 
					chan, dev, idle_mins);
	}
	return rc;
}

// command the device to spindown after standby_mins of no disk activity
int32_t 
ata_setstandby(struct ATA *ata, uint32_t chan, uint32_t dev, uint32_t standby_mins)
{
	uint16_t timer_val = 0;
	int32_t rc = 0;

	rc = ata_getidleval( standby_mins, &timer_val );	

	if (timer_val == ATA_IDLEVAL_IMMEDIATE)
		ata_setataparams(ata, 0, 0);
	else
		ata_setataparams(ata, timer_val, 0);

	// send the STANDBY command to the drive
	if(!rc) {
		if(timer_val == ATA_IDLEVAL_IMMEDIATE)
			rc = ata_cmd(ata, chan, dev, ATA_STANDBY_IMMEDIATE, 0);
		else
			rc = ata_cmd(ata, chan, dev, ATA_STANDBY, 0);
	}	
	if(rc) {
 		perror("error setting idle timeout");
	} else {
		if( timer_val == ATA_IDLEVAL_IMMEDIATE )
				printf("set chan %d, dev %d to standby immediately\n",
					chan, dev);
		else if(timer_val == 0)
			printf("turned off standby timer on chan %d, dev %d\n", 
						chan, dev);
		else
			printf("set chan %d, dev %d to standby after %d minutes\n", 
					chan, dev, standby_mins);
	}
	return rc;
}

// list the installed devices.  This function is useful
// to find out the channel,device settings to use for
// all the other commands
void ata_listdevices( struct ATA *ata )
{
	// first, do an IDENTIFY on each channel
	uint32_t numchannels = 20;
	uint32_t numdevs;
	uint32_t i;
	
	ata_getmaxchan( ata, &numchannels );
	numdevs = numchannels*2;
	
	uint32_t identbuf_size = ( sizeof(struct ata_ident) * numdevs );
	char * identbuf = (char *) malloc(identbuf_size);
	if(identbuf == 0) { /* malloc failed, we can do nothing here but quit */
		fprintf(stderr, "malloc failed, aborting.\n");
		exit(EXIT_FAILURE);
	}
	memset(identbuf, 0, identbuf_size);
	
	for(i = 0; i < numchannels; i++) {
		ata_ident( ata, i, 0, (struct ata_ident*) (identbuf+(sizeof(struct ata_ident)*(2*i))) );
		ata_ident( ata, i, 1, (struct ata_ident*) (identbuf+((sizeof(struct ata_ident)*(2*i))+sizeof(struct ata_ident) )));
	}
		
	for(i = 0; i < numdevs; i++) {
		struct ata_ident *ident = (struct ata_ident*) (identbuf+(sizeof(struct ata_ident)*i));
		char model[41];
		memset(model, 0, 41);
		strncpy(model, (void*) ident->model, 40);

		if(ident->config != 0) {
			printf("Channel %d, Device %d\n", (i/2), (i%2 == 0)? 0 : 1);
			printf("\tModel: %s\n", model);
			printf("\n");
		}
	}
		
	free(identbuf);
}

// this function sends an IDENTIFY command to a drive
int32_t ata_ident(struct ATA *ata, uint32_t ata_chan, uint32_t ata_dev, struct ata_ident * identity)
{
	int32_t rc = 0;
	unsigned char * buf  = NULL;
	
	ata_setataparams(ata, 0, 0);
	ata_setdataout_params(ata, (char**) &buf, 512);
	rc = ata_cmd(ata, ata_chan, ata_dev, ATA__IDENTIFY, 0);
	
	if(rc) {
		// then try an ATAPI IDENTIFY, maybe it didn't like an ATA_IDENTIFY
		ata_setataparams(ata, 0, 0);
		ata_setdataout_params(ata, (char**) &buf, 512);
		rc = ata_cmd(ata, ata_chan, ata_dev, ATA__ATAPI_IDENTIFY, 0);
	}
		
	byteswap((char*)buf, 20, 39); // serial	
	byteswap((char*)buf, 46, 52); // firmware
	byteswap((char*)buf, 54, 92); // model
	strpack((char*)buf, 20, 39);

	if(!rc)
		memcpy(identity, buf, sizeof(struct ata_ident));

	return rc;
}


