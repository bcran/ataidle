/*-
 *  
 * Copyright 2004 Rebecca Cran <rebecca@bsdio.com>.  All rights reserved.
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
 * Author: Rebecca Cran <rebecca@bsdio.com>
 * Version: 0.7
 *
 */

// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/hdreg.h>

// application-specific includes
#include "ataidle.h"
#include "../mi/atagen.h"
#include "../mi/atadefs.h"
#include "../mi/util.h"
		
// open the ata control device, /dev/ata rw
int ata_open(struct ATA *ata) {
	// This is a NOP, since Linux doesn't have
	// an ATA control device.  We will open the real
	// device later on, in the specific functions

	return 0;
}		

// send a command to the drive
int32_t
ata_cmd(struct ATA *ata, int ata_chan, int ata_dev, int cmd, int drivercmd)
{
	int32_t rc = 0;
	int fd = 0;
	
	int chan = ata_chan * 2;
	int devnum = chan + ata_dev;
	char devchar = 'a' + devnum;
	char device[20];
	sprintf(device, "/dev/hd%c", devchar);
	fd = open(device, O_RDONLY);
	if( fd < 0 )
		rc = -1;
	
	if(!rc) {
		ata->atacmd.cmd = cmd;
		rc = ioctl( fd, HDIO_DRIVE_CMD, &ata->atacmd );
		close(fd);
	}
	
	return rc;
}

// initialize the ata_cmd structure with supplied values
int32_t
ata_setataparams(struct ATA *ata, int seccount, int count)
{
	// clear the structure to remove any random values
	memset(&ata->atacmd, 0, sizeof(struct ata_cmd));
	
	if(seccount != 0)
		ata->atacmd.sector_number = seccount;
	else
		ata->atacmd.sector_count = 1;
	
	return 0;
}

void ata_setdataout_params(struct ATA *ata, char ** databuf, int nbytes)
{
	*databuf = (char*) ata->atacmd.buf;
}


// see if a device is present by seeing what its device
// type is, when the channel is queried.  If a device is
// present is will have a non-zero type.
bool
ata_devpresent(struct ATA *ata, uint32_t ata_chan, uint32_t ata_dev) 
{
	// is a NOP on Linux - if you can open the device file, it's there!
	return 0;
}

// return the maximum valid channel id
int32_t 
ata_getmaxchan(struct ATA *ata, uint32_t *maxchan)
{
	int rc = 0, fd = 0, ndevs = 0;
	char devname[15];
	bool done = false;

	// keep trying to open hdX (a-z), stop when an open fails.
	while(!done && (ndevs < 26)) {
		memset(devname, 0, 15);
		snprintf(devname, 15, "/dev/hd%c", 'a'+ndevs);
		fd = open(devname, O_RDONLY);
		if(fd < 0) {
			done = true;
		} else {
			close(fd);
			ndevs++;
		}
	}

	double devs = (double) ndevs;
	*maxchan = ceil(devs/2);
	*maxchan = 8;
	return rc;
}

void ata_close(struct ATA *ata)
{
	// don't do anything - device is already closed
}

void ata_setfeature_param(struct ATA *ata, int feature)
{
	ata->atacmd.feature = feature;
}

