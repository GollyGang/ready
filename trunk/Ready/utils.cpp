/*  Copyright 2011, The Ready Bunch

    This file is part of Ready.

    Ready is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ready is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ready. If not, see <http://www.gnu.org/licenses/>.         */

// local:
#include "utils.hpp"

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef _WIN32
        #include <sys/timeb.h>
        #include <sys/types.h>
        #include <winsock.h>
        // http://www.linuxjournal.com/article/5574
        void gettimeofday(struct timeval* t,void* timezone)
        {
            struct _timeb timebuffer;
            _ftime_s( &timebuffer );
            t->tv_sec = timebuffer.time;
            t->tv_usec = 1000 * timebuffer.millitm;
        }
#endif

float frand(float lower,float upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

double hypot2(double x,double y) 
{ 
    return sqrt(x*x+y*y); 
}

double hypot3(double x,double y,double z) 
{ 
    return sqrt(x*x+y*y+z*z); 
}
