#ifndef _ATAGEN_H_
#define _ATAGEN_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __FreeBSD__
	#include <sys/ata.h>
#else
	#include "../linux/ataidle.h"
#endif

struct ata_ident 
{
	uint16_t	config;
	uint16_t	word1;
	uint16_t	config_specific;
	uint16_t	word3;
	uint16_t	word4[2];
	uint16_t	word6;
	uint16_t	word7[2];
	uint16_t	word9;
	uint8_t		serial[20];
	uint16_t	word20[2];
	uint16_t	word22;
	uint8_t		firmware[8];
	uint8_t		model[40];
	uint16_t	max_sect_per_trans;
	uint16_t	word48;
	uint16_t	caps1;
	uint16_t	caps2;
	uint16_t	word51[2];
	uint16_t	words_valid;
	uint16_t	word54[5];
	uint16_t	sectors_per_interrupt;
	uint16_t	nsect[2];
	uint16_t	word62;
	uint16_t	mdma_supp;
	uint16_t	pio_supp;
	uint16_t	mdma_trans_time;
	uint16_t	mdma_trans_time_vendor;
	uint16_t	pio_trans_time_noflow;
	uint16_t	pio_trans_time_flow;
	uint16_t	word69[2];
	uint16_t	word71[4];
	uint16_t	queue_depth;
	uint16_t	word76[4];
	uint16_t	version_major;
	uint16_t	version_minor;
	uint16_t	cmd_supp1;
	uint16_t	cmd_supp2;
	uint16_t	cmd_supp_ext;
	uint16_t	cmd_enabled1;
	uint16_t	cmd_enabled2;
	uint16_t	cmd_ext_default;
	uint16_t	udma_modes;
	uint16_t	erase_time_security;
	uint16_t	erate_time_security_enhance;
	uint16_t	apm_value;
	uint16_t	master_passwd_rev_code;
	uint16_t	hardware_reset_result;
	uint16_t	aac_value;
	uint16_t	word95[5];
	uint16_t	max_lba48_address[4];
	uint16_t	word104[23];
	uint16_t	removable_media_features_supp;
	uint16_t	security_status;
	uint16_t	vendor_specific[31];
	uint16_t	cfa_powermode_1;
	uint16_t	word161[15];
	uint16_t	media_serial_no[30];
	uint16_t	word206[49];
	uint16_t	integrity;
};

struct ATA {
	int fd;
	uint32_t chan;
	uint32_t dev;
	uint32_t cmd;
	struct ata_cmd atacmd;
};


int     ata_open( struct ATA *ata );
void	ata_close(struct ATA *ata );
int32_t ata_setidle( struct ATA *ata, uint32_t ata_chan, 
				uint32_t ata_dev, uint32_t idle_mins );

int32_t ata_setstandby( struct ATA *ata, uint32_t ata_chan, 
				uint32_t ata_dev, uint32_t standby_mins );
int32_t ata_getmaxchan( struct ATA *ata, uint32_t *maxchan );
int32_t ata_setacoustic(struct ATA *ata, int ata_chan, 
				int ata_dev, uint32_t acoustic_val);
int32_t ata_setapm( struct ATA *ata, int ata_chan, 
				int ata_dev, uint32_t apm_val);
int32_t ata_cmd(struct ATA *ata, int chan, int dev, int atacmd, 
				int drivercmd );
void    ata_listdevices( struct ATA *ata );
bool    ata_devpresent( struct ATA *ata, uint32_t ata_chan, uint32_t ata_dev );
int32_t ata_ident( struct ATA *ata, uint32_t ata_chan, 
				uint32_t ata_dev, struct ata_ident * identity);
void    ata_showdeviceinfo( struct ATA *ata, uint32_t ata_chan, uint32_t ata_dev);
void 	ata_setfeature_param( struct ATA *ata, int feature_val);
int32_t ata_setataparams( struct ATA *ata, int seccount, int count);
void    ata_setdataout_params( struct ATA *ata, char ** databuf, int nbytes);

#endif /* _ATAIDLE_H_ */

