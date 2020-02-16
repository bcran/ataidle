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


#ifndef _ATAIDLE_H_
#define _ATAIDLE_H_

#include <stdint.h>
#include <stdbool.h>

static const uint32_t ATA_IDLE = 0xE3;
static const uint32_t ATA_IDLE_IMMEDIATE = 0xE1;
static const uint32_t ATA_STANDBY = 0xE2;
static const uint32_t ATA_STANDBY_IMMEDIATE = 0xE0;
static const uint32_t ATA_AUTOACOUSTIC_ENABLE = 0x42;
static const uint32_t ATA_APM_ENABLE = 0x05;
static const uint32_t ATA_AUTOACOUSTIC_MAXPERF = 0xFE;
static const uint32_t ATA_AUTOACOUSTIC_MINPERF = 0x80;
static const uint32_t ATA_APM_MINPOWER_NO_STANDBY = 0x80;
static const uint32_t ATA_APM_MINPERF = 0x01;
static const uint32_t ATA_APM_MAXPERF = 0xFE;
static const uint32_t ATA_POWERSTATUS_GET = 0xE5;
static const uint32_t ATA_CMD_TIMEOUT = 10;
static const uint32_t ATA_IDLEVAL_IMMEDIATE = 300;

int32_t ata_strtolong( char * src, long * dest );
bool checkargs( int argc, char ** argv, char * optstr, bool * needchandev );

int		ata_open( int *fd );
int32_t ata_getidleval( uint32_t idle_mins, uint16_t *timer_val );
int32_t ata_setidle( int fd, uint32_t ata_chan, uint32_t ata_dev, 
						uint32_t idle_mins );
int32_t	ata_setstandby( int fd, uint32_t ata_chan, uint32_t ata_dev,
						uint32_t standby_mins );
int32_t ata_getmaxchan( int fd, uint32_t *maxchan );
int32_t ata_cmd(int fd, uint32_t chan, uint32_t dev, int cmd, struct ata_cmd * );
void	ata_listdevices( int fd );
bool    ata_devpresent( int fd, uint32_t ata_chan, uint32_t ata_dev );
int32_t ata_ident(int fd, uint32_t ata_chan, uint32_t ata_dev, struct ata_params * identity);
void    ata_showdeviceinfo(int fd, uint32_t ata_chan, uint32_t ata_dev);
char *  ata_getversionstring(uint16_t ata_version);

#endif /* _ATAIDLE_H_ */

