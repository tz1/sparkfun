#include <avr/io.h>

#define ENABTW ((1<<TWINT)|(1<<TWEN)|(0<<TWIE)) // 0x80 4 1
#define START TWCR = (ENABTW|(1<<TWSTA))        // 0x20
#define STOP TWCR = (ENABTW|(1<<TWSTO)) // 0x10
#define SEND(x)  TWDR = x;  TWCR = ENABTW;
#define RECV(ack) TWCR = ENABTW | (ack? (1<<TWEA) : 0 );
unsigned char twista;
unsigned twitmo;
#define WAIT twitmo=0; while (!((twista = TWCR) & (1 << TWINT)) && ++twitmo);

/////===================================////////////////////
void TWIinit(void)
{
    DDRC &= ~0x30;              // pullup
    PORTC |= 0x30;              // pullup
    TWBR = (((F_CPU/400000)-16)/2); // 400 khz
    TWCR |= (1 << TWEN);
}

void TWIdocmd(unsigned char *msg)
{
    unsigned int mlen, rdwrf;

    while ((mlen = *msg++)) {
        rdwrf = *msg & 1;
        START;
        WAIT;
        do {
            SEND(*msg++);
            WAIT;
            // should check for ACK - twista == SAWA or SDWA
        } while (--mlen && !rdwrf);
        // read
        while (mlen--) {
            RECV(mlen);
            WAIT;
            *msg++ = TWDR;
        }
    }
    STOP;
}
