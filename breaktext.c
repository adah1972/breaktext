/* vim: set et sts=4 sw=4: */

/*
 * Copyright (C) 2008-2018 Wu Yongwei
 *
 *   Except the code copied from Vim (marked below), which is
 *   copyrighted by Bram Moolenaar
 *
 * This file, or any derivative source or binary, must be distributed
 * under GNU GPL version 2 or any later version.  However, as a special
 * permission, you may use my code (without the code copied from Vim)
 * for any purpose.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <getopt.h>
#include "linebreak.h"
#include "pctimer.h"

#define FALSE       0
#define TRUE        1

#define MAXCHARS    (8*1024*1024)
#define BOM         ((wchar_t)0xFEFF)

#define SWAPBYTE(x) ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))

int ambw = 1;
char* locale = "";
char* lang = NULL;
int width = 72;
int keep_indent = 0;
int verbose = 0;

wchar_t buffer[MAXCHARS];
char brks[MAXCHARS];


/**********************************************************************
 * Code copied from the Vim source (with trivial changes)
 */

struct interval
{
    long first;
    long last;
};

/*
 * Return TRUE if "c" is in "table[size / sizeof(struct interval)]".
 */
    static int
intable(struct interval *table, size_t size, int c)
{
    int mid, bot, top;

    /* first quick check for Latin1 etc. characters */
    if (c < table[0].first)
        return FALSE;

    /* binary search in table */
    bot = 0;
    top = (int)(size / sizeof(struct interval) - 1);
    while (top >= bot)
    {
        mid = (bot + top) / 2;
        if (table[mid].last < c)
            bot = mid + 1;
        else if (table[mid].first > c)
            top = mid - 1;
        else
            return TRUE;
    }
    return FALSE;
}

/*
 * Return TRUE for characters that can be displayed in a normal way.
 * Only for characters of 0x100 and above!
 */
    int
utf_printable(c)
    int         c;
{
#ifdef USE_WCHAR_FUNCTIONS
    /*
     * Assume the iswprint() library function works better than our own stuff.
     */
    return iswprint(c);
#else
    /* Sorted list of non-overlapping intervals.
     * 0xd800-0xdfff is reserved for UTF-16, actually illegal. */
    static struct interval nonprint[] =
    {
        {0x070f, 0x070f}, {0x180b, 0x180e}, {0x200b, 0x200f}, {0x202a, 0x202e},
        {0x206a, 0x206f}, {0xd800, 0xdfff}, {0xfeff, 0xfeff}, {0xfff9, 0xfffb},
        {0xfffe, 0xffff}
    };

    return !intable(nonprint, sizeof(nonprint), c);
#endif
}

/* Sorted list of non-overlapping intervals of East Asian Ambiguous
 * characters, generated with ../runtime/tools/unicode.vim. */
static struct interval ambiguous[] =
{
    {0x00a1, 0x00a1},
    {0x00a4, 0x00a4},
    {0x00a7, 0x00a8},
    {0x00aa, 0x00aa},
    {0x00ad, 0x00ae},
    {0x00b0, 0x00b4},
    {0x00b6, 0x00ba},
    {0x00bc, 0x00bf},
    {0x00c6, 0x00c6},
    {0x00d0, 0x00d0},
    {0x00d7, 0x00d8},
    {0x00de, 0x00e1},
    {0x00e6, 0x00e6},
    {0x00e8, 0x00ea},
    {0x00ec, 0x00ed},
    {0x00f0, 0x00f0},
    {0x00f2, 0x00f3},
    {0x00f7, 0x00fa},
    {0x00fc, 0x00fc},
    {0x00fe, 0x00fe},
    {0x0101, 0x0101},
    {0x0111, 0x0111},
    {0x0113, 0x0113},
    {0x011b, 0x011b},
    {0x0126, 0x0127},
    {0x012b, 0x012b},
    {0x0131, 0x0133},
    {0x0138, 0x0138},
    {0x013f, 0x0142},
    {0x0144, 0x0144},
    {0x0148, 0x014b},
    {0x014d, 0x014d},
    {0x0152, 0x0153},
    {0x0166, 0x0167},
    {0x016b, 0x016b},
    {0x01ce, 0x01ce},
    {0x01d0, 0x01d0},
    {0x01d2, 0x01d2},
    {0x01d4, 0x01d4},
    {0x01d6, 0x01d6},
    {0x01d8, 0x01d8},
    {0x01da, 0x01da},
    {0x01dc, 0x01dc},
    {0x0251, 0x0251},
    {0x0261, 0x0261},
    {0x02c4, 0x02c4},
    {0x02c7, 0x02c7},
    {0x02c9, 0x02cb},
    {0x02cd, 0x02cd},
    {0x02d0, 0x02d0},
    {0x02d8, 0x02db},
    {0x02dd, 0x02dd},
    {0x02df, 0x02df},
    {0x0300, 0x036f},
    {0x0391, 0x03a1},
    {0x03a3, 0x03a9},
    {0x03b1, 0x03c1},
    {0x03c3, 0x03c9},
    {0x0401, 0x0401},
    {0x0410, 0x044f},
    {0x0451, 0x0451},
    {0x2010, 0x2010},
    {0x2013, 0x2016},
    {0x2018, 0x2019},
    {0x201c, 0x201d},
    {0x2020, 0x2022},
    {0x2024, 0x2027},
    {0x2030, 0x2030},
    {0x2032, 0x2033},
    {0x2035, 0x2035},
    {0x203b, 0x203b},
    {0x203e, 0x203e},
    {0x2074, 0x2074},
    {0x207f, 0x207f},
    {0x2081, 0x2084},
    {0x20ac, 0x20ac},
    {0x2103, 0x2103},
    {0x2105, 0x2105},
    {0x2109, 0x2109},
    {0x2113, 0x2113},
    {0x2116, 0x2116},
    {0x2121, 0x2122},
    {0x2126, 0x2126},
    {0x212b, 0x212b},
    {0x2153, 0x2154},
    {0x215b, 0x215e},
    {0x2160, 0x216b},
    {0x2170, 0x2179},
    {0x2189, 0x2189},
    {0x2190, 0x2199},
    {0x21b8, 0x21b9},
    {0x21d2, 0x21d2},
    {0x21d4, 0x21d4},
    {0x21e7, 0x21e7},
    {0x2200, 0x2200},
    {0x2202, 0x2203},
    {0x2207, 0x2208},
    {0x220b, 0x220b},
    {0x220f, 0x220f},
    {0x2211, 0x2211},
    {0x2215, 0x2215},
    {0x221a, 0x221a},
    {0x221d, 0x2220},
    {0x2223, 0x2223},
    {0x2225, 0x2225},
    {0x2227, 0x222c},
    {0x222e, 0x222e},
    {0x2234, 0x2237},
    {0x223c, 0x223d},
    {0x2248, 0x2248},
    {0x224c, 0x224c},
    {0x2252, 0x2252},
    {0x2260, 0x2261},
    {0x2264, 0x2267},
    {0x226a, 0x226b},
    {0x226e, 0x226f},
    {0x2282, 0x2283},
    {0x2286, 0x2287},
    {0x2295, 0x2295},
    {0x2299, 0x2299},
    {0x22a5, 0x22a5},
    {0x22bf, 0x22bf},
    {0x2312, 0x2312},
    {0x2460, 0x24e9},
    {0x24eb, 0x254b},
    {0x2550, 0x2573},
    {0x2580, 0x258f},
    {0x2592, 0x2595},
    {0x25a0, 0x25a1},
    {0x25a3, 0x25a9},
    {0x25b2, 0x25b3},
    {0x25b6, 0x25b7},
    {0x25bc, 0x25bd},
    {0x25c0, 0x25c1},
    {0x25c6, 0x25c8},
    {0x25cb, 0x25cb},
    {0x25ce, 0x25d1},
    {0x25e2, 0x25e5},
    {0x25ef, 0x25ef},
    {0x2605, 0x2606},
    {0x2609, 0x2609},
    {0x260e, 0x260f},
    {0x261c, 0x261c},
    {0x261e, 0x261e},
    {0x2640, 0x2640},
    {0x2642, 0x2642},
    {0x2660, 0x2661},
    {0x2663, 0x2665},
    {0x2667, 0x266a},
    {0x266c, 0x266d},
    {0x266f, 0x266f},
    {0x269e, 0x269f},
    {0x26bf, 0x26bf},
    {0x26c6, 0x26cd},
    {0x26cf, 0x26d3},
    {0x26d5, 0x26e1},
    {0x26e3, 0x26e3},
    {0x26e8, 0x26e9},
    {0x26eb, 0x26f1},
    {0x26f4, 0x26f4},
    {0x26f6, 0x26f9},
    {0x26fb, 0x26fc},
    {0x26fe, 0x26ff},
    {0x273d, 0x273d},
    {0x2776, 0x277f},
    {0x2b56, 0x2b59},
    {0x3248, 0x324f},
    {0xe000, 0xf8ff},
    {0xfe00, 0xfe0f},
    {0xfffd, 0xfffd},
    {0x1f100, 0x1f10a},
    {0x1f110, 0x1f12d},
    {0x1f130, 0x1f169},
    {0x1f170, 0x1f18d},
    {0x1f18f, 0x1f190},
    {0x1f19b, 0x1f1ac},
    {0xe0100, 0xe01ef},
    {0xf0000, 0xffffd},
    {0x100000, 0x10fffd}
};

/*
 * For UTF-8 character "c" return 2 for a double-width character, 1 for others.
 * Returns 0 for an unprintable character.
 * Is only correct for characters >= 0x80.
 * When ambw is 2, return 2 for a character with East Asian Width
 * class 'A'(mbiguous).
 */
    int
utf_char2cells(int c)
{
    /* Sorted list of non-overlapping intervals of East Asian double width
     * characters, generated with ../runtime/tools/unicode.vim. */
    static struct interval doublewidth[] =
    {
        {0x1100, 0x115f},
        {0x231a, 0x231b},
        {0x2329, 0x232a},
        {0x23e9, 0x23ec},
        {0x23f0, 0x23f0},
        {0x23f3, 0x23f3},
        {0x25fd, 0x25fe},
        {0x2614, 0x2615},
        {0x2648, 0x2653},
        {0x267f, 0x267f},
        {0x2693, 0x2693},
        {0x26a1, 0x26a1},
        {0x26aa, 0x26ab},
        {0x26bd, 0x26be},
        {0x26c4, 0x26c5},
        {0x26ce, 0x26ce},
        {0x26d4, 0x26d4},
        {0x26ea, 0x26ea},
        {0x26f2, 0x26f3},
        {0x26f5, 0x26f5},
        {0x26fa, 0x26fa},
        {0x26fd, 0x26fd},
        {0x2705, 0x2705},
        {0x270a, 0x270b},
        {0x2728, 0x2728},
        {0x274c, 0x274c},
        {0x274e, 0x274e},
        {0x2753, 0x2755},
        {0x2757, 0x2757},
        {0x2795, 0x2797},
        {0x27b0, 0x27b0},
        {0x27bf, 0x27bf},
        {0x2b1b, 0x2b1c},
        {0x2b50, 0x2b50},
        {0x2b55, 0x2b55},
        {0x2e80, 0x2e99},
        {0x2e9b, 0x2ef3},
        {0x2f00, 0x2fd5},
        {0x2ff0, 0x2ffb},
        {0x3000, 0x303e},
        {0x3041, 0x3096},
        {0x3099, 0x30ff},
        {0x3105, 0x312f},
        {0x3131, 0x318e},
        {0x3190, 0x31ba},
        {0x31c0, 0x31e3},
        {0x31f0, 0x321e},
        {0x3220, 0x3247},
        {0x3250, 0x32fe},
        {0x3300, 0x4dbf},
        {0x4e00, 0xa48c},
        {0xa490, 0xa4c6},
        {0xa960, 0xa97c},
        {0xac00, 0xd7a3},
        {0xf900, 0xfaff},
        {0xfe10, 0xfe19},
        {0xfe30, 0xfe52},
        {0xfe54, 0xfe66},
        {0xfe68, 0xfe6b},
        {0xff01, 0xff60},
        {0xffe0, 0xffe6},
        {0x16fe0, 0x16fe1},
        {0x17000, 0x187f1},
        {0x18800, 0x18af2},
        {0x1b000, 0x1b11e},
        {0x1b170, 0x1b2fb},
        {0x1f004, 0x1f004},
        {0x1f0cf, 0x1f0cf},
        {0x1f18e, 0x1f18e},
        {0x1f191, 0x1f19a},
        {0x1f200, 0x1f202},
        {0x1f210, 0x1f23b},
        {0x1f240, 0x1f248},
        {0x1f250, 0x1f251},
        {0x1f260, 0x1f265},
        {0x1f300, 0x1f320},
        {0x1f32d, 0x1f335},
        {0x1f337, 0x1f37c},
        {0x1f37e, 0x1f393},
        {0x1f3a0, 0x1f3ca},
        {0x1f3cf, 0x1f3d3},
        {0x1f3e0, 0x1f3f0},
        {0x1f3f4, 0x1f3f4},
        {0x1f3f8, 0x1f43e},
        {0x1f440, 0x1f440},
        {0x1f442, 0x1f4fc},
        {0x1f4ff, 0x1f53d},
        {0x1f54b, 0x1f54e},
        {0x1f550, 0x1f567},
        {0x1f57a, 0x1f57a},
        {0x1f595, 0x1f596},
        {0x1f5a4, 0x1f5a4},
        {0x1f5fb, 0x1f64f},
        {0x1f680, 0x1f6c5},
        {0x1f6cc, 0x1f6cc},
        {0x1f6d0, 0x1f6d2},
        {0x1f6eb, 0x1f6ec},
        {0x1f6f4, 0x1f6f9},
        {0x1f910, 0x1f93e},
        {0x1f940, 0x1f970},
        {0x1f973, 0x1f976},
        {0x1f97a, 0x1f97a},
        {0x1f97c, 0x1f9a2},
        {0x1f9b0, 0x1f9b9},
        {0x1f9c0, 0x1f9c2},
        {0x1f9d0, 0x1f9ff},
        {0x20000, 0x2fffd},
        {0x30000, 0x3fffd}
    };

    /* Sorted list of non-overlapping intervals of Emoji characters that don't
     * have ambiguous or double width,
     * based on http://unicode.org/emoji/charts/emoji-list.html */
    static struct interval emoji_width[] =
    {
        {0x1f1e6, 0x1f1ff},
        {0x1f321, 0x1f321},
        {0x1f324, 0x1f32c},
        {0x1f336, 0x1f336},
        {0x1f37d, 0x1f37d},
        {0x1f396, 0x1f397},
        {0x1f399, 0x1f39b},
        {0x1f39e, 0x1f39f},
        {0x1f3cb, 0x1f3ce},
        {0x1f3d4, 0x1f3df},
        {0x1f3f3, 0x1f3f5},
        {0x1f3f7, 0x1f3f7},
        {0x1f43f, 0x1f43f},
        {0x1f441, 0x1f441},
        {0x1f4fd, 0x1f4fd},
        {0x1f549, 0x1f54a},
        {0x1f56f, 0x1f570},
        {0x1f573, 0x1f579},
        {0x1f587, 0x1f587},
        {0x1f58a, 0x1f58d},
        {0x1f590, 0x1f590},
        {0x1f5a5, 0x1f5a5},
        {0x1f5a8, 0x1f5a8},
        {0x1f5b1, 0x1f5b2},
        {0x1f5bc, 0x1f5bc},
        {0x1f5c2, 0x1f5c4},
        {0x1f5d1, 0x1f5d3},
        {0x1f5dc, 0x1f5de},
        {0x1f5e1, 0x1f5e1},
        {0x1f5e3, 0x1f5e3},
        {0x1f5e8, 0x1f5e8},
        {0x1f5ef, 0x1f5ef},
        {0x1f5f3, 0x1f5f3},
        {0x1f5fa, 0x1f5fa},
        {0x1f6cb, 0x1f6cf},
        {0x1f6e0, 0x1f6e5},
        {0x1f6e9, 0x1f6e9},
        {0x1f6f0, 0x1f6f0},
        {0x1f6f3, 0x1f6f3}
    };

    if (c >= 0x100)
    {
#ifdef USE_WCHAR_FUNCTIONS
        /*
         * Assume the library function wcwidth() works better than our own
         * stuff.  It should return 1 for ambiguous width chars!
         */
        int     n = wcwidth(c);

        if (n < 0)
            return 0;           /* WYW: no width */
        if (n > 1)
            return n;
#else
        if (!utf_printable(c))
            return 0;           /* WYW: no width */
        if (intable(doublewidth, sizeof(doublewidth), c))
            return 2;
#endif
        if (intable(emoji_width, sizeof(emoji_width), c))
            return 2;
    }

    /* WYW: Simple treatment */
    else if (c >= 0x80 && c < 0xa0)
        return 0;               /* WYW: no width */

    if (c >= 0x80 && ambw == 2 && intable(ambiguous, sizeof(ambiguous), c))
        return 2;

    return 1;
}

/*********************************************************************/


static void usage(void)
{
    fprintf(stderr,
        "Usage: breaktext [OPTION]... <Input File> [Output File]\n"
        "Last Change: 2024-03-05 22:57:00 +0800 (libunibreak %d.%d)\n"
        "\n"
        "Available options:\n"
        "  -L<locale>   Locale of the console (system locale by default)\n"
        "  -l<lang>     Language of input (asssume no language by default)\n"
        "  -w<width>    Width of output text (72 by default)\n"
        "  -i           Keep space indentation\n"
        "  -v           Be verbose\n"
        "\n"
        "If the output file is omitted, stdout will be used.\n"
        "The input file cannot be omitted, but you may use `-' for stdin.\n"
        "\n"
        "The `native' wide character type (wchar_t) is used in I/O routines,\n"
        "and the encoding used is platform-dependent.  On POSIX-compliant\n"
        "systems, the environment variables LANG, LC_ALL, and LC_CTYPE\n"
        "control the locale/encoding (unless overridden with the -L option),\n"
        "and UTF-8 will probably be used by default on modern systems.  On\n"
        "Windows, the encoding is dependent on whether stdin/stdout is used\n"
        "for I/O: console I/O will be automatically converted to/from\n"
        "wchar_t (which is UTF-16) according to the system locale setting\n"
        "(overridable with -L), but files (excepting the stdin/stdout case)\n"
        "will always be in just wchar_t (UTF-16).\n",
        (unibreak_version >> 8), (unibreak_version & 0xFF)
    );
}

static void put_buffer(wchar_t *buffer, size_t begin, size_t end, FILE *fp_out)
{
    size_t i;
    for (i = begin; i < end; ++i)
    {
        putwc(buffer[i], fp_out);
    }
}

static void put_indent(int indent, FILE *fp_out)
{
    while (indent)
    {
        putwc(L' ', fp_out);
        --indent;
    }
}

void break_text(wchar_t *buffer, char *brks, size_t len, FILE *fp_out)
{
    wchar_t ch;
    int w;
    size_t i;
    size_t last_break_pos = 0;
    size_t last_breakable_pos = 0;
    int col = 0;
    int indent = 0;
    int is_at_beginning = 1;

    for (i = 0; i < len; ++i)
    {
        if (brks[i] == LINEBREAK_MUSTBREAK)
        {
            /* Display undisplayed characters in the buffer */
            put_buffer(buffer, last_break_pos, i, fp_out);
            /* The character causing the explicit break is replaced with \n */
            putwc(L'\n', fp_out);
            /* Update positions */
            col = 0;
            indent = 0;
            is_at_beginning = 1;
            last_break_pos = last_breakable_pos = i + 1;
            continue;
        }

        /* Special processing for space-based indentation */
        if (is_at_beginning)
        {
            if (buffer[i] == L' ')
            {
                ++indent;
                /* Reset indentation if it becomes unreasonable */
                if (indent >= width / 2)
                {
                    indent = 0;
                    is_at_beginning = 0;
                }
            }
            else
            {
                is_at_beginning = 0;
            }
        }

        /* Special processing for "C++": no break. */
        if (buffer[i] == L'C' && brks[i] == LINEBREAK_ALLOWBREAK &&
                (i < len - 2 &&
                 buffer[i + 1] == L'+' && buffer[i + 2] == L'+') &&
                ((i < len - 3 && buffer[i + 3] == L' ') ||
                 brks[i + 2] < LINEBREAK_NOBREAK) &&
                (i == 0 || brks[i - 1] < LINEBREAK_NOBREAK))
        {
            brks[i] = brks[i + 1] = LINEBREAK_NOBREAK;
            --i;
            continue;
        }

        ch = buffer[i];
        w = utf_char2cells(ch);

        /* Right-margin spaces do not count */
        if (!(ch == L' ' && col == width))
        {
            col += w;
        }

        /* An breakable position encountered before the right margin */
        if (col <= width)
        {
            if (brks[i] == LINEBREAK_ALLOWBREAK)
            {
                if (buffer[i] == L'/' && col > 8)
                {   /* Ignore the breaking chance if there is a chance
                     * immediately before: no break inside "c/o", and no
                     * break after "http://" in a long line. */
                    if (last_breakable_pos > i - 2 ||
                            (width > 40 && last_breakable_pos > i - 7 &&
                             buffer[i - 1] == L'/'))
                    {
                        continue;
                    }
                    /* Special rule to treat Unix paths more nicely */
                    if (i < len - 1 && buffer[i + 1] != L' ' &&
                                       buffer[i - 1] == L' ')
                    {
                        last_breakable_pos = i;
                        continue;
                    }
                }
                last_breakable_pos = i + 1;
            }
        }

        /* Right margin crossed */
        else
        {
            /* No breakable character since the last break */
            if (last_breakable_pos == last_break_pos)
            {
                last_breakable_pos = i;
            }
            else
            {
                i = last_breakable_pos;
            }

            /* Display undisplayed characters in the buffer */
            put_buffer(buffer, last_break_pos, last_breakable_pos, fp_out);

            /* Output a new line and reset status */
            putwc(L'\n', fp_out);
            if (keep_indent)
            {
                put_indent(indent, fp_out);
                col = indent;
            }
            else
            {
                col = 0;
            }
            last_break_pos = last_breakable_pos;

            /* To be ++'d */
            --i;
        }
    }
}

int main(int argc, char *argv[])
{
    FILE *fp_in;
    FILE *fp_out;
    size_t c;
    const char opts[] = "L:l:w:iv";
    char opt;
    wint_t wch;
    const char *loc;
    pctimer_t t1, t2, t3, t4;

    if (argc == 1)
    {
        usage();
        exit(1);
    }

    for (;;)
    {
        opt = getopt(argc, argv, opts);
        if (opt == -1)
            break;
        switch (opt)
        {
        case 'L':
            locale = optarg;
            break;
        case 'l':
            lang = optarg;
            break;
        case 'w':
            width = atoi(optarg);
            if (width < 2)
            {
                fprintf(stderr, "Invalid width\n");
                exit(1);
            }
            break;
        case 'i':
            ++keep_indent;
            break;
        case 'v':
            ++verbose;
            break;
        default:
            usage();
            exit(1);
        }
    }

    if (!(optind < argc))
    {
        usage();
        exit(1);
    }

    loc = setlocale(LC_ALL, locale);

    t1 = pctimer();

    if (strcmp(argv[optind], "-") == 0)
    {
        fp_in = stdin;
    }
    else
    {
        if ( (fp_in = fopen(argv[optind], "rb")) == NULL)
        {
            perror("Cannot open input file");
            exit(1);
        }
    }

    for (c = 0; c < MAXCHARS; ++c)
    {
        wch = getwc(fp_in);
        if (wch == WEOF)
            break;
        buffer[c] = wch;
    }

    if (buffer[0] == SWAPBYTE(BOM))
    {
        fprintf(stderr, "Wrong endianness of input\n");
        exit(1);
    }
    if (buffer[0] == BOM && c > 1)
    {
        memmove(buffer, buffer + 1, (--c) * sizeof(wchar_t));
    }

    t2 = pctimer();

    init_linebreak();
    if (sizeof(wchar_t) == 2)
    {
        set_linebreaks_utf16((utf16_t*)buffer, c, lang, brks);
    }
    else if (sizeof(wchar_t) == 4)
    {
        set_linebreaks_utf32((utf32_t*)buffer, c, lang, brks);
    }
    else
    {
        fprintf(stderr, "Unexpected wchar_t size!\n");
        exit(1);
    }

    t3 = pctimer();

    if (lang && (strncmp(lang, "zh", 2) == 0 ||
                 strncmp(lang, "ja", 2) == 0 ||
                 strncmp(lang, "ko", 2) == 0))
    {
        ambw = 2;
    }

    if (optind + 1 < argc)
    {
        if ( (fp_out = fopen(argv[optind + 1], "wb")) == NULL)
        {
            perror("Cannot open output file");
            exit(1);
        }
        putwc(BOM, fp_out);
    }
    else
    {
        fp_out = stdout;
    }

    break_text(buffer, brks, c, fp_out);

    t4 = pctimer();

    if (verbose)
    {
        fprintf(stderr, "Locale:          %s\n", loc);
        fprintf(stderr, "Ambiguous width: %s\n", ambw == 1 ?
                                                 "Single" : "Double");
        fprintf(stderr, "Indentation:     %s\n", keep_indent ? "On" : "Off");
        fprintf(stderr, "Line width:      %d\n", width);
        fprintf(stderr, "Loading file:    %f s\n", t2 - t1);
        fprintf(stderr, "Finding breaks:  %f s\n", t3 - t2);
        fprintf(stderr, "Breaking text:   %f s\n", t4 - t3);
        fprintf(stderr, "TOTAL:           %f s\n", t4 - t1);
    }

    if (fp_in != stdin)
    {
        fclose(fp_in);
    }
    if (fp_out != stdout)
    {
        fclose(fp_out);
    }
    return 0;
}
