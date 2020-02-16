#ifndef _UTIL_H_
#define _UTIL_H_

void 	usage();
int32_t ata_strtolong( char * src, long * dest );
int32_t ata_getidleval( uint32_t idle_mins, uint16_t *timer_val );
char *  ata_getversionstring(uint16_t ata_version);
void	byteswap(char * buf, int from, int to);
void	strpack(char * buf, int from, int to);
bool	checkargs( int argc, char ** argv, char * optstr, bool * needchandev );

#endif
