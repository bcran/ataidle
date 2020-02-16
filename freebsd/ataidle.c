/*-
 *  
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
 * Version: 0.5
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
#include "../mi/atagen.h"
#include "../mi/atadefs.h"
#include "../mi/util.h"
		
// open the ata control device, /dev/ata rw
int ata_open(struct ATA *ata) {
	int rc = 0;
	
	ata->fd = open("/dev/ata", O_RDWR);
	
	if( ata->fd == -1) {
		perror( "error opening ata control device" );
		rc = -1;
	}

	return rc;
}		

// send a command to the drive
int32_t
ata_cmd(struct ATA *ata, int chan, int dev, int atacmd, int drivercmd)
{
	int32_t rc = 0;

	// check that the device actually exists, first
	if(!ata_devpresent( ata, chan, dev )) {
		rc = -1;
	}

	// if it does exist, send the command
	if(!rc) {
		ata->atacmd.channel = chan;
		ata->atacmd.device = dev;
		if(drivercmd != 0)
			ata->atacmd.cmd = drivercmd;
		else {
			ata->atacmd.cmd = ATAREQUEST;
		}
		
		ata->atacmd.u.request.u.ata.command = atacmd;
		rc = ioctl( ata->fd, IOCATA, &(ata->atacmd) );
	}
	
	return rc;
}

// initialize the ata_cmd structure with supplied values
int32_t
ata_setataparams(struct ATA *ata, int seccount, int count)
{
	// clear the structure to remove any random values
	memset(& ata->atacmd, 0, sizeof(struct ata_cmd));
	ata->atacmd.cmd = ATAREQUEST;
	ata->atacmd.u.request.flags = ATA_CMD_CONTROL;
	ata->atacmd.u.request.timeout = ATA_CMD_TIMEOUT;
	ata->atacmd.u.request.count = count;
	ata->atacmd.u.request.u.ata.count = seccount;
		
	return 0;
}

void ata_setfeature_param(struct ATA *ata, int feature_val)
{
	ata->atacmd.u.request.u.ata.feature = feature_val;
}

void ata_setdataout_params(struct ATA *ata, char ** databuf, int nbytes)
{
	*databuf = (char*) malloc(nbytes);
	memset(*databuf, 0, nbytes);
	ata->atacmd.u.request.data = *databuf;
	ata->atacmd.u.request.count = nbytes;
	ata->atacmd.u.request.flags = ATA_CMD_READ;
}	

// see if a device is present by seeing what its device
// type is, when the channel is queried.  If a device is
// present it will have a non-zero type.
bool
ata_devpresent(struct ATA *ata, uint32_t ata_chan, uint32_t ata_dev) 
{
	bool devpresent = false;
	struct ATA myata;
	memcpy(&myata, ata, sizeof(struct ATA));
	
	myata.atacmd.channel = ata_chan;
	myata.atacmd.device = -1;
	myata.atacmd.cmd = ATAGPARM;

	ioctl( myata.fd, IOCATA, &myata.atacmd );	

	if(myata.atacmd.u.param.type[ata_dev])
		devpresent = true;
	
	return devpresent;
}

// return the maximum valid channel id
int32_t 
ata_getmaxchan(struct ATA *ata, uint32_t *maxchan)
{
	int32_t rc = 0;
	
	if(!rc) {
			
		if( ata_cmd(ata, 0, 0, 0, ATAGMAXCHANNEL ) == -1 ) {
			perror("error getting maximum channel");
			rc = -1;
		} else {
			*maxchan = ata->atacmd.u.maxchan;
		}
	}

	return rc;											
}

void ata_close(struct ATA *ata)
{
	if(ata->fd > 0)
		close(ata->fd);
}

