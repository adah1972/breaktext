/*
 * pctimer.h    1.3 2002/10/25
 *
 * Uses Win32 performance counter functions to get a high-resolution timer
 *
 * By Wu Yongwei
 *
 */

#ifndef _PCTIMER_H

typedef double pctimer_t;

#if defined(_WIN32) || defined(__CYGWIN__)

#ifndef _WIN32
#define PCTIMER_NO_WIN32
#endif /* _WIN32 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef PCTIMER_NO_WIN32
#undef PCTIMER_NO_WIN32
#undef _WIN32
#endif /* PCTIMER_NO_WIN32 */

__inline pctimer_t pctimer(void)
{
    static LARGE_INTEGER pcount, pcfreq;
    static int initflag;

    if (!initflag)
    {
        QueryPerformanceFrequency(&pcfreq);
        initflag++;
    }

    QueryPerformanceCounter(&pcount);
    return (double)pcount.QuadPart / (double)pcfreq.QuadPart;
}

#else /* Not Win32/Cygwin */

#include <sys/time.h>

static __inline pctimer_t pctimer(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
}

#endif /* Win32/Cygwin */

#endif /* _PCTIMER_H */
