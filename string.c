#include "XMC1400.h"
#include "uart.h"
#include "string.h"
//#include "stdio.h"

#include "kern\kern_types.h"
#include "kern\kern_task.h"
#include "kern\kern_event.h"
#include "kern\kern_queue.h"
#include "kern\kern_sem.h"
#include "kern\kern_utils.h"
#include "kern\kern.h"

#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(unsigned short int) - 1) & ~(sizeof(unsigned short int) - 1) )
typedef char * va_list;

#define va_start(ap,v)  ( ap = (va_list)&v + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)  ( ap = (va_list)0 )

extern _US0     *pus0;
extern _SEM  printf_semaphore;

const char low_case_array[] = {
    'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
};

const char up_case_array[] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'
};

const char ascii_array[] = {
    'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    ':','\\','_','.','0','1','2','3','4','5','6','7','8','9'
};

extern int debug_ready;
//--------------------------------------------
//      int check_string_valid (char *str_in)
//--------------------------------------------
int check_string_valid (char *str_in, int count)
{
    int i,valid_status,k;

    for (k = 0; k < count; k++)
    {
        valid_status = 0;
        for (i = 0; i < sizeof(ascii_array); i++)
        {
            if (*str_in == ascii_array[i])
            {
                valid_status = 1;
                break;
            }
        }

        if (!valid_status)
            return 0;

        str_in++;
    }

    return 1;
}
//--------------------------------------------
//      int base_low_case (char *str_in)
//--------------------------------------------
int base_low_case (char *str_in)
{
    int len = strlen(str_in);
    int i,k;
    int ret = 1;

    for (i = 0; i < len; i++)
    {
        if (*(str_in + i) == '.')
            return ret;

        for (k = 0; k < sizeof(up_case_array); k++)
        {
            if (*(str_in + i) == up_case_array[k])
                ret = 0;
        }
    }

    return ret;
}
//--------------------------------------------
//      int ext_low_case (char *str_in)
//--------------------------------------------
int ext_low_case (char *str_in)
{
    int len = strlen(str_in);
    int i,k;
    int ret = 1;

    while ((len > 0) && (*str_in != '.'))
    {
        len--;
        str_in++;
    }

    if (len == 0)
        return 0;

    for (i = 0; i < len; i++)
    {
        if (*(str_in + i) == '.')
            return ret;

        for (k = 0; k < sizeof(up_case_array); k++)
        {
            if (*(str_in + i) == up_case_array[k])
                ret = 0;
        }
    }

    return ret;
}
//--------------------------------------------
//      int str_all_up_case (char *str_in)
//--------------------------------------------
int str_all_up_case (char *str_in)
{
    int len = strlen(str_in);
    int i,k;

    for (i = 0; i < len; i++)
    {
        for (k = 0; k < sizeof(low_case_array); k++)
          if (*(str_in + i) == low_case_array[k])
              return 0;
    }

    return 1;
}
//--------------------------------------------
//      int str_all_low_case (char *str_in)
//--------------------------------------------
int str_all_low_case (char *str_in)
{
    int len = strlen(str_in);
    int i,k;

    for (i = 0; i < len; i++)
    {
        for (k = 0; k < sizeof(up_case_array); k++)
          if (*(str_in + i) == up_case_array[k])
              return 0;
    }

    return 1;
}
//--------------------------------------------
//      void str_low_case_ext (char *str_in)
//--------------------------------------------
void str_low_case_ext (char *str_in)
{
    int len = strlen(str_in);
    int i,k;
    int start_convert = 0;

    for (i = 0; i < len; i++)
    {
        if ((i > 7) || (*(str_in + i) == '.'))
            start_convert = 1;

        if (start_convert)
        {
            for (k = 0; k < sizeof(up_case_array); k++)
              if (*(str_in + i) == up_case_array[k])
                  *(str_in + i) += 0x20;
        }
    }
}
//--------------------------------------------
//      void str_low_case_base (char *str_in)
//--------------------------------------------
void str_low_case_base (char *str_in)
{
    int len = strlen(str_in);
    int i,k;

    for (i = 0; i < len; i++)
    {
        if ((i > 8) || (*(str_in + i) == '.'))
            break;

        for (k = 0; k < sizeof(up_case_array); k++)
          if (*(str_in + i) == up_case_array[k])
              *(str_in + i) += 0x20;
    }
}
//--------------------------------------------
//      void str_low_case (char *str_in)
//--------------------------------------------
void str_low_case (char *str_in)
{
    int len = strlen(str_in);
    int i,k;

    for (i = 0; i < len; i++)
    {
        for (k = 0; k < sizeof(up_case_array); k++)
          if (*(str_in + i) == up_case_array[k])
              *(str_in + i) += 0x20;
    }
}
//--------------------------------------------
//      void str_up_case (char *str_in)
//--------------------------------------------
void str_up_case (char *str_in)
{
    int len = strlen(str_in);
    int i,k;

    for (i = 0; i < len; i++)
    {
        for (k = 0; k < sizeof(low_case_array); k++)
          if (*(str_in + i) == low_case_array[k])
              *(str_in + i) -= 0x20;
    }
}
//--------------------------------------------
//      void xputchar (short int ch)
//--------------------------------------------
//_FILE *global_log_fh;

void xputchar (short int ch)
{
// ---- bluetooth ------ //
/*    if (ch == '\n')
        us2_send_byte('\r');

    us2_send_byte(ch);
*/
// ---- USART0 ------ //

    if (ch == '\n')
        pus0->send_byte('\r');

    pus0->send_byte(ch);
	
}
//--------------------------------------------
//      int byte_count_in_block (char *src, long len, char bt)
//--------------------------------------------
int byte_count_in_block (char *src, long len, char bt)
{
    int i;
    int z = 0;

    for (i = 0; i < len; i++)
    {
        if (*src == bt)
            z++;
    }

    return z;
}
//--------------------------------------------
//      void memcpy (char *dest, char *src, char len)
//--------------------------------------------
void memcpy (char *dest, char *src, long len)
{
  int i;

  for (i = 0; i < len; i++)
  {
    *dest = *src;
    dest++; src++;
  }
}
//--------------------------------------------
//      void memset (void *ptr, int count, char byte)
//--------------------------------------------
void memset (void *ptr, int count, char byte)
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
//     char strcmp (char *str1, char *str2, char len)
//--------------------------------------------
char strcmp (char *str1, char *str2, int len)
{
    while (len--)
    {
        if (*str1 != *str2)
            return 0;

        str1++;
        str2++;
    }
    return 1;
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
//--------------------------------------------
//     void printf(const char *fmt,...)
//--------------------------------------------
void printf(const char *fmt,...)
{
    va_list ap;
    char buf[10];
    char *s;
    unsigned u;
    int c;
    const char *hex = "0123456789abcdef";

    if (debug_ready == 0)
        return;

    if (debug_ready)
    {
        if (kern_sem_acquire(&printf_semaphore,1000) != ERR_NO_ERR)
            return;
    }

    va_start(ap, fmt);
    while ((c = *fmt++))
    {
        if (c == '%')
        {
            c = *fmt++;
            switch (c)
            {
                case 'c':
                    xputchar(va_arg(ap, int));
                    continue;

                case 's':
                    for (s = va_arg(ap, char *); *s; s++)
                        xputchar(*s);
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
                        xputchar(*s);
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
           xputchar(c);
    }
    va_end(ap);

    if (debug_ready)
        kern_sem_signal(&printf_semaphore);
}
