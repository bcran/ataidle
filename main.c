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
 * Version: 0.6
 *
 */

// standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

// application-specific includes
#include "mi/atadefs.h"
#include "mi/util.h"
#include "mi/atagen.h"		

#ifdef __FreeBSD__
	#include <osreldate.h>

	#if __FreeBSD_version < 501110
		#error ATAidle requires ATAng to function
	#endif
#endif

extern char * optarg;
extern int optind, optopt, opterr, optreset;

// the main function
int main( int argc, char ** argv )
{
	int rc = 0;
	int ch, chan, dev;
	struct ATA *ata = (struct ATA*) malloc(sizeof(struct ATA));
	long opt_val;
	uint32_t maxchan = 0;
	bool needchandev;
	char * optstr = "hlA:S:sI:iP:";

	if (ata == 0) { /* malloc failed, abort */
		fprintf(stderr, "malloc failed, aborting.\n");
		exit(EXIT_FAILURE);
	}

	if( (argc == 1) || (!checkargs(argc, argv, optstr, &needchandev)) )
		usage();
	
	rc = ata_open(ata);
	
	if(!rc && needchandev)
		rc = ata_strtolong(argv[argc-2], (long*) &chan);
	
	if(!rc)
		rc = ata_getmaxchan(ata, &maxchan);

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
		//optreset = 1;
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
						rc = ata_setstandby( ata, chan, dev, opt_val );
					break;

				case 's':
					rc = ata_setstandby( ata, chan, dev, ATA_IDLEVAL_IMMEDIATE );
					break;

				// I for Idle
				case 'I':
					rc = ata_strtolong(optarg, &opt_val);	
					
					if(rc)
						printf("invalid idle value\n");
					else
						rc = ata_setidle( ata, chan, dev, opt_val );
					break;

				case 'i':
					rc = ata_setidle(ata, chan, dev, ATA_IDLEVAL_IMMEDIATE);
					break;

				// A for AutoAcoustic
				case 'A':
					rc = ata_strtolong(optarg, &opt_val);
					if(rc)
							printf("invalid acoustic value\n");
					else 
							rc = ata_setacoustic( ata, chan, dev, opt_val );
					break;
		
				// P for APM
				case 'P':
					rc = ata_strtolong(optarg, &opt_val);
					if(rc)
							printf("invalid apm value\n");
					else
							rc = ata_setapm( ata, chan, dev, opt_val );
					break;
				
				case 'l':
					printf("Listing Devices:\n\n");
					ata_listdevices(ata);
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
		ata_showdeviceinfo(ata, chan, dev);
	}

	// if we successfully opened the ata control
	// device, now's the time to close it.
	ata_close(ata);
	free(ata);
	
	return rc;
}

