#include "debug.h"

#include <stdio.h>
#include <time.h>

static clock_t g_chrono_last_call = 0;
static const unsigned int threshold = 0;

void chrono_zero()
{
	g_chrono_last_call = clock();
}

void chrono_toggle()
{
	unsigned int count = (clock() - g_chrono_last_call) * 1000 / CLOCKS_PER_SEC;
	if ( count >= threshold ) 
		printf( "Chrono: %dms\n", count );
	g_chrono_last_call = clock();
}
