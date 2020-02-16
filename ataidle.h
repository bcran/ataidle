#ifndef _ATAIDLE_H_
#define _ATAIDLE_H_

#include <stdint.h>
#include <stdbool.h>

static const uint32_t ATA_IDLE = 0xE3;

int ata_open(int *fd);
int32_t ata_getidleval(uint32_t idle_mins, uint8_t *timer_val);
int32_t ata_setidle(int fd, uint32_t ata_chan, uint32_t ata_dev, 
					uint32_t idle_mins);
int32_t ata_getmaxchan(int fd, int32_t *maxchan);
bool    ata_devpresent(int fd, int32_t ata_chan, int32_t ata_dev);

#endif

