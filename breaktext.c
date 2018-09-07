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
    unsigned short first;
    unsigned short last;
};

/*
 * Return TRUE if "c" is in "table[size / sizeof(struct interval)]".
 */
    static int
intable(table, size, c)
    struct interval     *table;
    size_t              size;
    int                 c;
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

/*
 * For UTF-8 character "c" return 2 for a double-width character, 1 for others.
 * Returns 0 for an unprintable character.
 * Is only correct for characters >= 0x80.
 * When ambw is 2, return 2 for a character with East Asian Width
 * class 'A'(mbiguous).
 */
    int
utf_char2cells(c)
    int         c;
{
    /* sorted list of non-overlapping intervals of East Asian Ambiguous
     * characters, generated with:
     * "uniset +WIDTH-A -cat=Me -cat=Mn -cat=Cf c" */
    static struct interval ambiguous[] = {
        {0x00A1, 0x00A1}, {0x00A4, 0x00A4}, {0x00A7, 0x00A8},
        {0x00AA, 0x00AA}, {0x00AE, 0x00AE}, {0x00B0, 0x00B4},
        {0x00B6, 0x00BA}, {0x00BC, 0x00BF}, {0x00C6, 0x00C6},
        {0x00D0, 0x00D0}, {0x00D7, 0x00D8}, {0x00DE, 0x00E1},
        {0x00E6, 0x00E6}, {0x00E8, 0x00EA}, {0x00EC, 0x00ED},
        {0x00F0, 0x00F0}, {0x00F2, 0x00F3}, {0x00F7, 0x00FA},
        {0x00FC, 0x00FC}, {0x00FE, 0x00FE}, {0x0101, 0x0101},
        {0x0111, 0x0111}, {0x0113, 0x0113}, {0x011B, 0x011B},
        {0x0126, 0x0127}, {0x012B, 0x012B}, {0x0131, 0x0133},
        {0x0138, 0x0138}, {0x013F, 0x0142}, {0x0144, 0x0144},
        {0x0148, 0x014B}, {0x014D, 0x014D}, {0x0152, 0x0153},
        {0x0166, 0x0167}, {0x016B, 0x016B}, {0x01CE, 0x01CE},
        {0x01D0, 0x01D0}, {0x01D2, 0x01D2}, {0x01D4, 0x01D4},
        {0x01D6, 0x01D6}, {0x01D8, 0x01D8}, {0x01DA, 0x01DA},
        {0x01DC, 0x01DC}, {0x0251, 0x0251}, {0x0261, 0x0261},
        {0x02C4, 0x02C4}, {0x02C7, 0x02C7}, {0x02C9, 0x02CB},
        {0x02CD, 0x02CD}, {0x02D0, 0x02D0}, {0x02D8, 0x02DB},
        {0x02DD, 0x02DD}, {0x02DF, 0x02DF}, {0x0391, 0x03A1},
        {0x03A3, 0x03A9}, {0x03B1, 0x03C1}, {0x03C3, 0x03C9},
        {0x0401, 0x0401}, {0x0410, 0x044F}, {0x0451, 0x0451},
        {0x2010, 0x2010}, {0x2013, 0x2016}, {0x2018, 0x2019},
        {0x201C, 0x201D}, {0x2020, 0x2022}, {0x2024, 0x2027},
        {0x2030, 0x2030}, {0x2032, 0x2033}, {0x2035, 0x2035},
        {0x203B, 0x203B}, {0x203E, 0x203E}, {0x2074, 0x2074},
        {0x207F, 0x207F}, {0x2081, 0x2084}, {0x20AC, 0x20AC},
        {0x2103, 0x2103}, {0x2105, 0x2105}, {0x2109, 0x2109},
        {0x2113, 0x2113}, {0x2116, 0x2116}, {0x2121, 0x2122},
        {0x2126, 0x2126}, {0x212B, 0x212B}, {0x2153, 0x2154},
        {0x215B, 0x215E}, {0x2160, 0x216B}, {0x2170, 0x2179},
        {0x2190, 0x2199}, {0x21B8, 0x21B9}, {0x21D2, 0x21D2},
        {0x21D4, 0x21D4}, {0x21E7, 0x21E7}, {0x2200, 0x2200},
        {0x2202, 0x2203}, {0x2207, 0x2208}, {0x220B, 0x220B},
        {0x220F, 0x220F}, {0x2211, 0x2211}, {0x2215, 0x2215},
        {0x221A, 0x221A}, {0x221D, 0x2220}, {0x2223, 0x2223},
        {0x2225, 0x2225}, {0x2227, 0x222C}, {0x222E, 0x222E},
        {0x2234, 0x2237}, {0x223C, 0x223D}, {0x2248, 0x2248},
        {0x224C, 0x224C}, {0x2252, 0x2252}, {0x2260, 0x2261},
        {0x2264, 0x2267}, {0x226A, 0x226B}, {0x226E, 0x226F},
        {0x2282, 0x2283}, {0x2286, 0x2287}, {0x2295, 0x2295},
        {0x2299, 0x2299}, {0x22A5, 0x22A5}, {0x22BF, 0x22BF},
        {0x2312, 0x2312}, {0x2460, 0x24E9}, {0x24EB, 0x254B},
        {0x2550, 0x2573}, {0x2580, 0x258F}, {0x2592, 0x2595},
        {0x25A0, 0x25A1}, {0x25A3, 0x25A9}, {0x25B2, 0x25B3},
        {0x25B6, 0x25B7}, {0x25BC, 0x25BD}, {0x25C0, 0x25C1},
        {0x25C6, 0x25C8}, {0x25CB, 0x25CB}, {0x25CE, 0x25D1},
        {0x25E2, 0x25E5}, {0x25EF, 0x25EF}, {0x2605, 0x2606},
        {0x2609, 0x2609}, {0x260E, 0x260F}, {0x2614, 0x2615},
        {0x261C, 0x261C}, {0x261E, 0x261E}, {0x2640, 0x2640},
        {0x2642, 0x2642}, {0x2660, 0x2661}, {0x2663, 0x2665},
        {0x2667, 0x266A}, {0x266C, 0x266D}, {0x266F, 0x266F},
        {0x273D, 0x273D}, {0x2776, 0x277F}, {0xE000, 0xF8FF},
        {0xFFFD, 0xFFFD}, /* {0xF0000, 0xFFFFD}, {0x100000, 0x10FFFD} */
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
        if (c >= 0x1100
            && (c <= 0x115f                     /* Hangul Jamo */
                || c == 0x2329
                || c == 0x232a
                || (c >= 0x2e80 && c <= 0xa4cf
                    && c != 0x303f)             /* CJK ... Yi */
                || (c >= 0xac00 && c <= 0xd7a3) /* Hangul Syllables */
                || (c >= 0xf900 && c <= 0xfaff) /* CJK Compatibility
                                                   Ideographs */
                || (c >= 0xfe30 && c <= 0xfe6f) /* CJK Compatibility Forms */
                || (c >= 0xff00 && c <= 0xff60) /* Fullwidth Forms */
                || (c >= 0xffe0 && c <= 0xffe6)
                || (c >= 0x20000 && c <= 0x2fffd)
                || (c >= 0x30000 && c <= 0x3fffd)))
            return 2;
#endif
    }

    /* Characters below 0x100 are influenced by 'isprint' option */
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
        "Last Change: 2018-09-07 22:12:10 +0800\n"
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
        "will always be in just wchar_t (UTF-16).\n"
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
            /* Displayed undisplayed characters in the buffer */
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

            /* Displayed undisplayed characters in the buffer */
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
