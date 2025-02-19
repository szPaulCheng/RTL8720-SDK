/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: lightduer_snprintf.c
 * Auth: Leliang Zhang(zhangleliang@baidu.com)
 * Desc: Provide snprintf/vsnprintf implementation,
 *       because esp32 platform, sprintf need too much stack,
 *       so these codes here, try to avoid the huge stack-implementation.
 * reference:
 */
#ifdef ENABLE_LIGHTDUER_SNPRINTF
#include "lightduer_snprintf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef isdigit
#undef isdigit
#endif
#define isdigit(c) ((c) >= '0' && (c) <= '9')

#define ZEROPAD    1         /* pad with zero */
#define SIGN       2         /* unsigned/signed long */
#define PLUS       4         /* show plus */
#define SPACE      8         /* space if plus */
#define LEFT       16        /* left justified */
#define SMALL      32        /* Must be 32 == 0x20 */
#define SPECIAL    64        /* 0x */

/* For copying strings longer or equal to 'breakeven_point'
 * it is more efficient to call memcpy() than to do it inline.
 * The value depends mostly on the processor architecture,
 * but also on the compiler and its optimization capabilities.
 * The value is not critical, some small value greater than zero
 * will be just fine if you don't care to squeeze every drop
 * of performance out of the code.
 *
 * Small values favor memcpy, large values favor inline code.
 */
#if defined(__i386__)  || defined(__i386)
#define breakeven_point  12    /* Intel Pentium/Linux - gcc 2.96 */
#endif

#ifndef breakeven_point
#define breakeven_point   6    /* some reasonable one-size-fits-all value */
#endif

#define fast_memcpy(d,s,n) \
    {\
        register size_t nn = (size_t)(n);\
        if (nn >= breakeven_point) {\
            memcpy((d), (s), nn);\
        } else if (nn > 0) { /* proc call overhead is worth only for large strings*/\
            register char *dd = NULL;\
            register const char *ss = NULL;\
            for (ss = (s), dd = (d); nn > 0; nn--) {\
                *dd++ = *ss++;\
            }\
        }\
    }

#define fast_memset(d,c,n) \
    {\
        register size_t nn = (size_t)(n);\
        if (nn >= breakeven_point) {\
            memset((d), (int)(c), nn);\
        } else if (nn > 0) { /* proc call overhead is worth only for large strings*/\
            register char *dd = NULL;\
            register const int cc = (int)(c);\
            for (dd = (d); nn > 0; nn--) {\
              *dd++ = cc;\
            }\
        }\
    }

#define __do_div(n, base) ({ \
int __res; \
__res = ((unsigned long) n) % (unsigned) base; \
n = ((unsigned long) n) / (unsigned) base; \
__res; })

static int skip_atoi(const char **s)
{
    int i = 0;

    while (isdigit(**s)) {
        i = i * 10 + *((*s)++) - '0';
    }

    return i;
}

static char *number(char *str, long num, int base, int size, int precision,
                    int type)
{
    /* we are called with base 8, 10 or 16, only, thus don't need "G..."  */
    static const char digits[16] = "0123456789ABCDEF"; /* "GHIJKLMNOPQRSTUVWXYZ"; */
    char tmp[66];
    char c = 0;
    char sign = 0;
    char locase = 0;
    int i = 0;
    /* locase = 0 or 0x20. ORing digits or letters with 'locase'
     * produces same digits or (maybe lowercased) letters */
    locase = (type & SMALL);

    if (type & LEFT) {
        type &= ~ZEROPAD;
    }

    if (base < 2 || base > 16) {
        return NULL;
    }

    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;

    if (type & SIGN) {
        if (num < 0) {
            sign = '-';
            num = -num;
            size--;
        } else if (type & PLUS) {
            sign = '+';
            size--;
        } else if (type & SPACE) {
            sign = ' ';
            size--;
        }
    }

    if (type & SPECIAL) {
        if (base == 16) {
            size -= 2;
        } else if (base == 8) {
            size--;
        }
    }

    i = 0;

    if (num == 0) {
        tmp[i++] = '0';
    } else
        while (num != 0) {
            tmp[i++] = (digits[__do_div(num, base)] | locase);
        }

    if (i > precision) {
        precision = i;
    }

    size -= precision;

    if (!(type & (ZEROPAD + LEFT)))
        while (size-- > 0) {
            *str++ = ' ';
        }

    if (sign) {
        *str++ = sign;
    }

    if (type & SPECIAL) {
        if (base == 8) {
            *str++ = '0';
        } else if (base == 16) {
            *str++ = '0';
            *str++ = ('X' | locase);
        }
    }

    if (!(type & LEFT))
        while (size-- > 0) {
            *str++ = c;
        }

    while (i < precision--) {
        *str++ = '0';
    }

    while (i-- > 0) {
        *str++ = tmp[i];
    }

    while (size-- > 0) {
        *str++ = ' ';
    }

    return str;
}

static int duer_vsprintf(char *buf, const char *fmt, va_list args)
{
    int len = 0;
    unsigned long num = 0;
    int i = 0;
    int base = 0;
    char *str = NULL;
    const char *s = NULL;
    int flags = 0;        /* flags to number() */
    int field_width = 0;    /* width of output field */
    int precision = 0;        /* min. # of digits for integers; max
                   number of chars for from string */
    int qualifier = 0;        /* 'h', 'l', or 'L' for integer fields */

    for (str = buf; *fmt; ++fmt) {
        if (*fmt != '%') {
            *str++ = *fmt;
            continue;
        }

        /* process flags */
        flags = 0;
repeat:
        ++fmt;        /* this also skips first '%' */

        switch (*fmt) {
        case '-':
            flags |= LEFT;
            goto repeat;

        case '+':
            flags |= PLUS;
            goto repeat;

        case ' ':
            flags |= SPACE;
            goto repeat;

        case '#':
            flags |= SPECIAL;
            goto repeat;

        case '0':
            flags |= ZEROPAD;
            goto repeat;
        }

        /* get field width */
        field_width = -1;

        if (isdigit(*fmt)) {
            field_width = skip_atoi(&fmt);
        } else if (*fmt == '*') {
            ++fmt;
            /* it's the next argument */
            field_width = va_arg(args, int);

            if (field_width < 0) {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        /* get the precision */
        precision = -1;

        if (*fmt == '.') {
            ++fmt;

            if (isdigit(*fmt)) {
                precision = skip_atoi(&fmt);
            } else if (*fmt == '*') {
                ++fmt;
                /* it's the next argument */
                precision = va_arg(args, int);
            }

            if (precision < 0) {
                precision = 0;
            }
        }

        /* get the conversion qualifier */
        qualifier = -1;

        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
            qualifier = *fmt;
            ++fmt;
        }

        /* default base */
        base = 10;

        switch (*fmt) {
        case 'c':
            if (!(flags & LEFT))
                while (--field_width > 0) {
                    *str++ = ' ';
                }

            *str++ = (unsigned char)va_arg(args, int);

            while (--field_width > 0) {
                *str++ = ' ';
            }

            continue;

        case 's':
            s = va_arg(args, char *);
            len = strnlen(s, precision);

            if (!(flags & LEFT))
                while (len < field_width--) {
                    *str++ = ' ';
                }

            for (i = 0; i < len; ++i) {
                *str++ = *s++;
            }

            while (len < field_width--) {
                *str++ = ' ';
            }

            continue;

        case 'p':
            if (field_width == -1) {
                field_width = 2 * sizeof(void *);
                flags |= ZEROPAD;
            }

            str = number(str,
                         (unsigned long)va_arg(args, void *), 16,
                         field_width, precision, flags);
            continue;

        case 'n':
            if (qualifier == 'l') {
                long *ip = va_arg(args, long *);
                *ip = (str - buf);
            } else {
                int *ip = va_arg(args, int *);
                *ip = (str - buf);
            }

            continue;

        case '%':
            *str++ = '%';
            continue;

        /* integer number formats - set up the flags and "break" */
        case 'o':
            base = 8;
            break;

        case 'x':
            flags |= SMALL;

        case 'X':
            base = 16;
            break;

        case 'd':
        case 'i':
            flags |= SIGN;

        case 'u':
            break;

        default:
            *str++ = '%';

            if (*fmt) {
                *str++ = *fmt;
            } else {
                --fmt;
            }

            continue;
        }

        if (qualifier == 'l') {
            num = va_arg(args, unsigned long);
        } else if (qualifier == 'h') {
            num = (unsigned short)va_arg(args, int);

            if (flags & SIGN) {
                num = (short)num;
            }
        } else if (flags & SIGN) {
            num = va_arg(args, int);
        } else {
            num = va_arg(args, unsigned int);
        }

        str = number(str, num, base, field_width, precision, flags);
    }

    *str = '\0';
    return str - buf;
}

int duer_sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    int i = 0;
    va_start(args, fmt);
    i = duer_vsprintf(buf, fmt, args);
    va_end(args);
    return i;
}

int lightduer_vsnprintf(char *str, size_t str_m, const char *fmt, va_list ap)
{
    size_t str_l = 0;
    const char *p = fmt;

    /* In contrast with POSIX, the ISO C99 now says
     * that str can be NULL and str_m can be 0.
     * This is more useful than the old:  if (str_m < 1) return -1; */

    if (!p) {
        p = "";
    }

    while (*p) {
        if (*p != '%') {
            // if (str_l < str_m) str[str_l++] = *p++;    -- this would be sufficient
            // but the following code achieves better performance for cases
            // where format string is long and contains few conversions
            const char *q = strchr(p + 1, '%');
            size_t n = !q ? strlen(p) : (q - p);

            if (str_l < str_m) {
                size_t avail = str_m - str_l;
                fast_memcpy(str + str_l, p, (n > avail ? avail : n));
            }

            p += n;
            str_l += n;
        } else {
            const char *starting_p = NULL;
            size_t min_field_width = 0;
            size_t precision = 0;
            int zero_padding = 0;
            int precision_specified = 0;
            int justify_left = 0;
            int alternate_form = 0;
            int force_sign = 0;
            int space_for_positive = 1; /* If both the ' ' and '+' flags appear,
                                     the ' ' flag should be ignored. */
            char length_modifier = '\0';            /* allowed values: \0, h, l, L */
            char tmp[32];/* temporary buffer for simple numeric->string conversion */
            const char *str_arg = NULL;      /* string address in case of string argument */
            size_t str_arg_l = 0;         /* natural field width of arg without padding
                                   and sign */
            unsigned char uchar_arg = 0;
            /* unsigned char argument value - only defined for c conversion.
               N.B. standard explicitly states the char argument for
               the c conversion is unsigned */
            size_t number_of_zeros_to_pad = 0;
            /* number of zeros to be inserted for numeric conversions
               as required by the precision or minimal field width */
            size_t zero_padding_insertion_ind = 0;
            /* index into tmp where zero padding is to be inserted */
            char fmt_spec = '\0';
            /* current conversion specifier character */
            str_arg = NULL;
            starting_p = p;
            p++;  /* skip '%' */

            /* parse flags */
            while (*p == '0' || *p == '-' || *p == '+' ||
                    *p == ' ' || *p == '#' || *p == '\'') {
                switch (*p) {
                case '0':
                    zero_padding = 1;
                    break;

                case '-':
                    justify_left = 1;
                    break;

                case '+':
                    force_sign = 1;
                    space_for_positive = 0;
                    break;

                case ' ':
                    force_sign = 1;
                    /* If both the ' ' and '+' flags appear, the ' ' flag should be ignored */
#ifdef PERL_COMPATIBLE
                    /* ... but in Perl the last of ' ' and '+' applies */
                    space_for_positive = 1;
#endif
                    break;

                case '#':
                    alternate_form = 1;
                    break;

                case '\'':
                    break;
                }

                p++;
            }

            /* If the '0' and '-' flags both appear, the '0' flag should be ignored. */

            /* parse field width */
            if (*p == '*') {
                int j = va_arg(ap, int);
                p++;

                if (j >= 0) {
                    min_field_width = j;
                } else {
                    min_field_width = -j;
                    justify_left = 1;
                }
            } else if (isdigit((int)(*p))) {
                /* size_t could be wider than unsigned int;
                   make sure we treat argument like common implementations do */
                unsigned int uj = *p++ - '0';

                while (isdigit((int)(*p))) {
                    uj = 10 * uj + (unsigned int)(*p++ - '0');
                }

                min_field_width = uj;
            }

            /* parse precision */
            if (*p == '.') {
                p++;
                precision_specified = 1;

                if (*p == '*') {
                    int j = va_arg(ap, int);
                    p++;

                    if (j >= 0) {
                        precision = j;
                    } else {
                        precision_specified = 0;
                        precision = 0;
                        /* NOTE:
                         *   Solaris 2.6 man page claims that in this case the precision
                         *   should be set to 0.  Digital Unix 4.0, HPUX 10 and BSD man page
                         *   claim that this case should be treated as unspecified precision,
                         *   which is what we do here.
                         */
                    }
                } else if (isdigit((int)(*p))) {
                    /* size_t could be wider than unsigned int;
                       make sure we treat argument like common implementations do */
                    unsigned int uj = *p++ - '0';

                    while (isdigit((int)(*p))) {
                        uj = 10 * uj + (unsigned int)(*p++ - '0');
                    }

                    precision = uj;
                }
            }

            /* parse 'h', 'l' and 'll' length modifiers */
            if (*p == 'h' || *p == 'l') {
                length_modifier = *p;
                p++;

                if (length_modifier == 'l' && *p == 'l') {   /* double l = long long */
#ifdef SNPRINTF_LONGLONG_SUPPORT
                    length_modifier = '2';                  /* double l encoded as '2' */
#else
                    length_modifier = 'l';                 /* treat it as a single 'l' */
#endif
                    p++;
                }
            }

            fmt_spec = *p;

            /* common synonyms: */
            switch (fmt_spec) {
            case 'i':
                fmt_spec = 'd';
                break;

            case 'D':
                fmt_spec = 'd';
                length_modifier = 'l';
                break;

            case 'U':
                fmt_spec = 'u';
                length_modifier = 'l';
                break;

            case 'O':
                fmt_spec = 'o';
                length_modifier = 'l';
                break;

            default:
                break;
            }

            /* get parameter value, do initial processing */
            switch (fmt_spec) {
            case '%': /* % behaves similar to 's' regarding flags and field widths */
            case 'c': /* c behaves similar to 's' regarding flags and field widths */
            case 's':
                length_modifier = '\0';          /* wint_t and wchar_t not supported */
                /* the result of zero padding flag with non-numeric conversion specifier*/
                /* is undefined. Solaris and HPUX 10 does zero padding in this case,    */
                /* Digital Unix and Linux does not. */
#if !defined(SOLARIS_COMPATIBLE) && !defined(HPUX_COMPATIBLE)
                zero_padding = 0;    /* turn zero padding off for string conversions */
#endif
                str_arg_l = 1;

                switch (fmt_spec) {
                case '%':
                    str_arg = p;
                    break;

                case 'c': {
                    int j = va_arg(ap, int);
                    uchar_arg = (unsigned char) j;   /* standard demands unsigned char */
                    str_arg = (const char *) &uchar_arg;
                    break;
                }

                case 's':
                    str_arg = va_arg(ap, const char *);

                    if (!str_arg) {
                        str_arg_l = 0;
                    }
                    /* make sure not to address string beyond the specified precision !!! */
                    else if (!precision_specified) {
                        str_arg_l = strlen(str_arg);
                    }
                    /* truncate string if necessary as requested by precision */
                    else if (precision == 0) {
                        str_arg_l = 0;
                    } else {
                        /* memchr on HP does not like n > 2^31  !!! */
                        const char *q = memchr(str_arg, '\0',
                                               precision <= 0x7fffffff ? precision : 0x7fffffff);
                        str_arg_l = !q ? precision : (q - str_arg);
                    }

                    break;

                default:
                    break;
                }

                break;

            case 'd':
            case 'u':
            case 'o':
            case 'x':
            case 'X':
            case 'p': {
                /* NOTE: the u, o, x, X and p conversion specifiers imply
                         the value is unsigned;  d implies a signed value */
                int arg_sign = 0;
                /* 0 if numeric argument is zero (or if pointer is NULL for 'p'),
                  +1 if greater than zero (or nonzero for unsigned arguments),
                  -1 if negative (unsigned argument is never negative) */
                int int_arg = 0;
                unsigned int uint_arg = 0;
                /* only defined for length modifier h, or for no length modifiers */
                long int long_arg = 0;
                unsigned long int ulong_arg = 0;
                /* only defined for length modifier l */
                void *ptr_arg = NULL;
                /* pointer argument value -only defined for p conversion */
#ifdef SNPRINTF_LONGLONG_SUPPORT
                long long int long_long_arg = 0;
                unsigned long long int ulong_long_arg = 0;
                /* only defined for length modifier ll */
#endif

                if (fmt_spec == 'p') {
                    /* HPUX 10: An l, h, ll or L before any other conversion character
                     *   (other than d, i, u, o, x, or X) is ignored.
                     * Digital Unix:
                     *   not specified, but seems to behave as HPUX does.
                     * Solaris: If an h, l, or L appears before any other conversion
                     *   specifier (other than d, i, u, o, x, or X), the behavior
                     *   is undefined. (Actually %hp converts only 16-bits of address
                     *   and %llp treats address as 64-bit data which is incompatible
                     *   with (void *) argument on a 32-bit system).
                     */
#ifdef SOLARIS_COMPATIBLE
#  ifdef SOLARIS_BUG_COMPATIBLE
                    /* keep length modifiers even if it represents 'll' */
#  else

                    if (length_modifier == '2') {
                        length_modifier = '\0';
                    }

#  endif
#else
                    length_modifier = '\0';
#endif
                    ptr_arg = va_arg(ap, void *);

                    if (ptr_arg != NULL) {
                        arg_sign = 1;
                    }
                } else if (fmt_spec == 'd') {  /* signed */
                    switch (length_modifier) {
                    case '\0':
                    case 'h':
                        /* It is non-portable to specify a second argument of char or short
                         * to va_arg, because arguments seen by the called function
                         * are not char or short.  C converts char and short arguments
                         * to int before passing them to a function.
                         */
                        int_arg = va_arg(ap, int);

                        if (int_arg > 0) {
                            arg_sign =  1;
                        } else if (int_arg < 0) {
                            arg_sign = -1;
                        }

                        break;

                    case 'l':
                        long_arg = va_arg(ap, long int);

                        if (long_arg > 0) {
                            arg_sign =  1;
                        } else if (long_arg < 0) {
                            arg_sign = -1;
                        }

                        break;
#ifdef SNPRINTF_LONGLONG_SUPPORT

                    case '2':
                        long_long_arg = va_arg(ap, long long int);

                        if (long_long_arg > 0) {
                            arg_sign =  1;
                        } else if (long_long_arg < 0) {
                            arg_sign = -1;
                        }

                        break;
#endif
                    }
                } else {  /* unsigned */
                    switch (length_modifier) {
                    case '\0':
                    case 'h':
                        uint_arg = va_arg(ap, unsigned int);

                        if (uint_arg) {
                            arg_sign = 1;
                        }

                        break;

                    case 'l':
                        ulong_arg = va_arg(ap, unsigned long int);

                        if (ulong_arg) {
                            arg_sign = 1;
                        }

                        break;
#ifdef SNPRINTF_LONGLONG_SUPPORT

                    case '2':
                        ulong_long_arg = va_arg(ap, unsigned long long int);

                        if (ulong_long_arg) {
                            arg_sign = 1;
                        }

                        break;
#endif
                    }
                }

                str_arg = tmp;
                str_arg_l = 0;
                /* NOTE:
                 *   For d, i, u, o, x, and X conversions, if precision is specified,
                 *   the '0' flag should be ignored. This is so with Solaris 2.6,
                 *   Digital UNIX 4.0, HPUX 10, Linux, FreeBSD, NetBSD; but not with Perl.
                 */
#ifndef PERL_COMPATIBLE

                if (precision_specified) {
                    zero_padding = 0;
                }

#endif

                if (fmt_spec == 'd') {
                    if (force_sign && arg_sign >= 0) {
                        tmp[str_arg_l++] = space_for_positive ? ' ' : '+';
                    }

                    /* leave negative numbers for sprintf to handle,
                       to avoid handling tricky cases like (short int)(-32768) */
#ifdef LINUX_COMPATIBLE
                } else if (fmt_spec == 'p' && force_sign && arg_sign > 0) {
                    tmp[str_arg_l++] = space_for_positive ? ' ' : '+';
#endif
                } else if (alternate_form) {
                    if (arg_sign != 0 && (fmt_spec == 'x' || fmt_spec == 'X')) {
                        tmp[str_arg_l++] = '0';
                        tmp[str_arg_l++] = fmt_spec;
                    }

                    /* alternate form should have no effect for p conversion, but ... */
#ifdef HPUX_COMPATIBLE
                    else if (fmt_spec == 'p'
                             /* HPUX 10: for an alternate form of p conversion,
                              *          a nonzero result is prefixed by 0x. */
#ifndef HPUX_BUG_COMPATIBLE
                             /* Actually it uses 0x prefix even for a zero value. */
                             && arg_sign != 0
#endif
                            ) {
                        tmp[str_arg_l++] = '0';
                        tmp[str_arg_l++] = 'x';
                    }

#endif
                }

                zero_padding_insertion_ind = str_arg_l;

                if (!precision_specified) {
                    precision = 1;    /* default precision is 1 */
                }

                if (precision == 0 && arg_sign == 0
#if defined(HPUX_BUG_COMPATIBLE) || defined(LINUX_COMPATIBLE)
                        && fmt_spec != 'p'
                        /* HPUX 10 man page claims: With conversion character p the result of
                         * converting a zero value with a precision of zero is a null string.
                         * Actually HP returns all zeroes, and Linux returns "(nil)". */
#endif
                   ) {
                    /* converted to null string */
                    /* When zero value is formatted with an explicit precision 0,
                       the resulting formatted string is empty (d, i, u, o, x, X, p).   */
                } else {
                    char f[5];
                    int f_l = 0;
                    f[f_l++] = '%';    /* construct a simple format string for sprintf */

                    if (!length_modifier) { }
                    else if (length_modifier == '2') {
                        f[f_l++] = 'l';
                        f[f_l++] = 'l';
                    } else {
                        f[f_l++] = length_modifier;
                    }

                    f[f_l++] = fmt_spec;
                    f[f_l++] = '\0';

                    if (fmt_spec == 'p') {
                        str_arg_l += duer_sprintf(tmp + str_arg_l, f, ptr_arg);
                    } else if (fmt_spec == 'd') { /* signed */
                        switch (length_modifier) {
                        case '\0':
                        case 'h':
                            str_arg_l += duer_sprintf(tmp + str_arg_l, f, int_arg);
                            break;

                        case 'l':
                            str_arg_l += duer_sprintf(tmp + str_arg_l, f, long_arg);
                            break;
#ifdef SNPRINTF_LONGLONG_SUPPORT

                        case '2':
                            str_arg_l += duer_sprintf(tmp + str_arg_l, f, long_long_arg);
                            break;
#endif
                        }
                    } else {  /* unsigned */
                        switch (length_modifier) {
                        case '\0':
                        case 'h':
                            str_arg_l += duer_sprintf(tmp + str_arg_l, f, uint_arg);
                            break;

                        case 'l':
                            str_arg_l += duer_sprintf(tmp + str_arg_l, f, ulong_arg);
                            break;
#ifdef SNPRINTF_LONGLONG_SUPPORT

                        case '2':
                            str_arg_l += duer_sprintf(tmp + str_arg_l, f, ulong_long_arg);
                            break;
#endif
                        }
                    }

                    /* include the optional minus sign and possible "0x"
                       in the region before the zero padding insertion point */
                    if (zero_padding_insertion_ind < str_arg_l &&
                            tmp[zero_padding_insertion_ind] == '-') {
                        zero_padding_insertion_ind++;
                    }

                    if (zero_padding_insertion_ind + 1 < str_arg_l &&
                            tmp[zero_padding_insertion_ind]   == '0' &&
                            (tmp[zero_padding_insertion_ind + 1] == 'x' ||
                             tmp[zero_padding_insertion_ind + 1] == 'X')) {
                        zero_padding_insertion_ind += 2;
                    }
                }

                {
                    size_t num_of_digits = str_arg_l - zero_padding_insertion_ind;

                    if (alternate_form && fmt_spec == 'o'
#ifdef HPUX_COMPATIBLE                                  /* ("%#.o",0) -> ""  */
                            && (str_arg_l > 0)
#endif
#ifdef DIGITAL_UNIX_BUG_COMPATIBLE                      /* ("%#o",0) -> "00" */
#else
                            /* unless zero is already the first character */
                            && !(zero_padding_insertion_ind < str_arg_l
                                 && tmp[zero_padding_insertion_ind] == '0')
#endif
                       ) {        /* assure leading zero for alternate-form octal numbers */
                        if (!precision_specified || precision < num_of_digits + 1) {
                            /* precision is increased to force the first character to be zero,
                               except if a zero value is formatted with an explicit precision
                               of zero */
                            precision = num_of_digits + 1;
                            precision_specified = 1;
                        }
                    }

                    /* zero padding to specified precision? */
                    if (num_of_digits < precision) {
                        number_of_zeros_to_pad = precision - num_of_digits;
                    }
                }

                /* zero padding to specified minimal field width? */
                if (!justify_left && zero_padding) {
                    int n = min_field_width - (str_arg_l + number_of_zeros_to_pad);

                    if (n > 0) {
                        number_of_zeros_to_pad += n;
                    }
                }

                break;
            }

            default: /* unrecognized conversion specifier, keep format string as-is*/
                zero_padding = 0;  /* turn zero padding off for non-numeric convers. */
#ifndef DIGITAL_UNIX_COMPATIBLE
                justify_left = 1;
                min_field_width = 0;                /* reset flags */
#endif
#if defined(PERL_COMPATIBLE) || defined(LINUX_COMPATIBLE)
                /* keep the entire format string unchanged */
                str_arg = starting_p;
                str_arg_l = p - starting_p;
                /* well, not exactly so for Linux, which does something inbetween,
                 * and I don't feel an urge to imitate it: "%+++++hy" -> "%+y"  */
#else
                /* discard the unrecognized conversion, just keep *
                 * the unrecognized conversion character          */
                str_arg = p;
                str_arg_l = 0;
#endif

                if (*p) {
                    str_arg_l++;
                }  /* include invalid conversion specifier unchanged

                                 if not at end-of-string */
                break;
            }

            if (*p) {
                p++;    /* step over the just processed conversion specifier */
            }

            // insert padding to the left as requested by min_field_width
            // this does not include the zero padding in case of numerical conversions
            if (!justify_left) {                /* left padding with blank or zero */
                int n = min_field_width - (str_arg_l + number_of_zeros_to_pad);

                if (n > 0) {
                    if (str_l < str_m) {
                        size_t avail = str_m - str_l;
                        fast_memset(str + str_l, (zero_padding ? '0' : ' '), (n > avail ? avail : n));
                    }

                    str_l += n;
                }
            }

            // zero padding as requested by the precision or by the minimal field width
            // for numeric conversions required?
            if (number_of_zeros_to_pad <= 0) {
                // will not copy first part of numeric right now,
                // force it to be copied later in its entirety
                zero_padding_insertion_ind = 0;
            } else {
                // insert first part of numerics (sign or '0x') before zero padding
                int n = zero_padding_insertion_ind;

                if (n > 0) {
                    if (str_l < str_m) {
                        size_t avail = str_m - str_l;
                        fast_memcpy(str + str_l, str_arg, (n > avail ? avail : n));
                    }

                    str_l += n;
                }

                // insert zero padding as requested by the precision or min field width
                n = number_of_zeros_to_pad;

                if (n > 0) {
                    if (str_l < str_m) {
                        size_t avail = str_m - str_l;
                        fast_memset(str + str_l, '0', (n > avail ? avail : n));
                    }

                    str_l += n;
                }
            }

            // insert formatted string
            // (or as-is conversion specifier for unknown conversions)
            {
                int n = str_arg_l - zero_padding_insertion_ind;

                if (n > 0) {
                    if (str_l < str_m) {
                        size_t avail = str_m - str_l;
                        fast_memcpy(str + str_l, str_arg + zero_padding_insertion_ind,
                                    (n > avail ? avail : n));
                    }

                    str_l += n;
                }
            }

            /* insert right padding */
            if (justify_left) {          /* right blank padding to the field width */
                int n = min_field_width - (str_arg_l + number_of_zeros_to_pad);

                if (n > 0) {
                    if (str_l < str_m) {
                        size_t avail = str_m - str_l;
                        fast_memset(str + str_l, ' ', (n > avail ? avail : n));
                    }

                    str_l += n;
                }
            }
        }
    }

    if (str_m > 0) {
        // make sure the string is null-terminated
        //                    even at the expense of overwriting the last character
        //                    (shouldn't happen, but just in case)
        str[str_l <= str_m - 1 ? str_l : str_m - 1] = '\0';
    }

    // Return the number of characters formatted (excluding trailing null
    // character), that is, the number of characters that would have been
    // written to the buffer if it were large enough.
    //
    // The value of str_l should be returned, but str_l is of unsigned type
    // size_t, and snprintf is int, possibly leading to an undetected
    // integer overflow, resulting in a negative return value, which is illegal.
    // Both XSH5 and ISO C99 (at least the draft) are silent on this issue.
    // Should errno be set to EOVERFLOW and EOF returned in this case???

    return (int) str_l;
}

int lightduer_snprintf(char *str, size_t str_m, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int ret = lightduer_vsnprintf(str, str_m, fmt, ap);
    va_end(ap);
    return ret;
}

#endif // ENABLE_LIGHTDUER_SNPRINTF
