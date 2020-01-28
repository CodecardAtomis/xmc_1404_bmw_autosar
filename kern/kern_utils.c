#include "kern_utils.h"

#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(unsigned short int) - 1) & ~(sizeof(unsigned short int) - 1) )
typedef char * va_list;
 
#define va_start(ap,v)  ( ap = (va_list)&v + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)  ( ap = (va_list)0 )
//--------------------------------------------
//      void memset (void *ptr, int count, char byte)
//--------------------------------------------
/*void memset (void *ptr, int count, char byte)
{
    char *pmem;
    
    pmem = (char *)ptr;
    
    while (count)
    {
        *pmem = byte;
        pmem++;
        count--;
    }
}
//--------------------------------------------
//      long strlen (char *str_in)
//--------------------------------------------
long strlen (char *str_in)
{
    long len = 0;
    
    while (*str_in)
    {
        str_in++;
        len++;
    }
    
    return len;
}
//------------------------------------------------------------------------
//     void snprintf(char *buff_out, long buff_sz,const char *fmt,...)
//------------------------------------------------------------------------
void sprintf (char *buff_out, const char *fmt,...)
{
    va_list ap;
    char buf[10];
    char *s;
    unsigned u;
    int c;
    const char *hex = "0123456789abcdef";
    
    va_start(ap, fmt);
    while ((c = *fmt++)) 
    {
        if (c == '%') 
        {
            c = *fmt++;
            switch (c) 
            {
                case 'c':
                    *buff_out = va_arg(ap, char);
                    buff_out++;
                    continue;
                    
                case 's':
                    for (s = va_arg(ap, char *); *s; s++)
                    { 
                        *buff_out = *s;
                        buff_out++;
                    }
                    continue;
                    
                case 'd':       
                case 'u':
                    u = va_arg(ap, unsigned);
                    s = buf;
                    do
                      *s++ = 0x30 + u % 10U;
                    while (u /= 10U);
                    
            dumpbuf:;
                    
                    while (--s >= buf)
                    {
                        *buff_out = *s;
                        buff_out++;
                    }
                    continue;
                    
                 case 'x':
                    u = va_arg(ap, unsigned);
                    s = buf;
                    do
                        *s++ = hex[u & 0xfu];
                    while (u >>= 4);
                    goto dumpbuf;
               }
           }
           *buff_out = c;
            buff_out++;
    }
    
    *buff_out = 0;
    va_end(ap);
}
*/
