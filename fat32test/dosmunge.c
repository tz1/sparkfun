// A-Z, 0-9, 
#include "typefix.h"

static u8 validos[] = "$%'`-@{}~!#()&_^";

inline u8 *tzstrchr(u8 * here, u8 in)
{
    while (*here)
        if (*here++ == in)
            return here;
    return 0;
}

int dosmunge(u8 * namein, u8 * nameout)
{
    u8 cx, dot, len;
    len = 0;
    dot = 0;
    for (;;) {
        cx = *namein++;
        if (len == 11)
            while (cx && cx != '.')     // short circuit to tail
                cx = *namein++;
        if (!cx)
            break;
        if (cx == '.') {
            if (!len)
                return -1;      // no filename
            while (len < 8)
                nameout[len++] = ' ';
            len = 8;
            dot = 1;
            continue;
        }
        //        if( len == 11 )
        //            continue;
        if (cx == ' ')          // compress-remove spaces
            continue;           // or mabye change to '_'?
        if (cx >= 'a' && cx <= 'z')
            cx -= 32;
        // technically space is valid, but DOSent work everywhere.
        if (!(cx >= 'A' && cx <= 'Z')
          && !(cx >= '0' && cx <= '9')
          && !tzstrchr(validos, cx))
            return -2;          // invalid char - or maybe change to ~^_?
        nameout[len++] = cx;
    }
    if (!len)
        return -1;              // no filename
    while (len < 11)
        nameout[len++] = ' ';
    nameout[12] = 0;
    return 0;
}
