void txenqueue( unsigned char *buf, unsigned len );
void uartputch(unsigned char c);
void inituart(unsigned baud);
int uartgetch(void);
unsigned kbhit(void);
#include <stdio.h>
