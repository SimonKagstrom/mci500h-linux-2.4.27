
#ifndef __INTERVAL_H__
#define __INTERVAL_H__

#include <asm/arch/tsc.h>

extern unsigned long llsqrt(unsigned long long);

struct interval
{
   unsigned long long starttsc;
   unsigned long long total;
   unsigned long long total2;
   unsigned long long avg;
   unsigned long std;
   unsigned long long min;
   unsigned long long max;
   unsigned long count;
};

typedef struct interval interval_t;

#define INTERVAL_INIT(interval)  \
    		do \
		{ \
		    (interval).count = 0; \
		    (interval).total = 0; \
		    (interval).total2 = 0; \
		    (interval).min = -1ULL; \
		    (interval).max = 0; \
		    (interval).avg = 0; \
		} while (0)

#define INTERVAL_START(interval) \
		((interval).starttsc = ReadTSC64())

#define INTERVAL_STOP(interval)  \
    		do \
		{ \
		    unsigned long long delta = ReadTSC64() - (interval).starttsc; \
		    (interval).total += delta; \
		    (interval).total2 += delta * delta; \
		    (interval).count++; \
		    if ((interval).min > delta) \
			(interval).min = delta; \
		    if ((interval).max < delta) \
			(interval).max = delta; \
		} while (0)

#define INTERVAL_CALC(interval)  \
    		do \
		{ \
		    (interval).avg = 0; \
		    (interval).std = 0; \
		    if ((interval).count > 0) \
			(interval).avg = (interval).total / (interval).count; \
		    else \
			(interval).min = 0ULL; \
		    if ((interval).count > 1) \
		    { \
			unsigned long n = (interval).count; \
			unsigned long long t = (interval).total; \
			(interval).std = llsqrt(((n * (interval).total2) - (t * t)) / (n * (n - 1))); \
		    } \
		} while (0)

#endif

