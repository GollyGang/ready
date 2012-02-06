/*  Copyright 2011, 2012 The Ready Bunch

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
#include <math.h>

#ifdef _WIN32
    #include <sys/timeb.h>
    #include <sys/types.h>
    #include <winsock.h>
    // http://www.linuxjournal.com/article/5574
    void gettimeofday(struct timeval* t,void* timezone)
    {
        struct _timeb timebuffer;
        #if _MSC_VER < 1400
            _ftime( &timebuffer );
        #else
            // MSVC 2005+
            _ftime_s( &timebuffer );
        #endif
        t->tv_sec = (long)timebuffer.time;
        t->tv_usec = 1000 * timebuffer.millitm;
    }
#else
    #include <sys/time.h>
#endif

double get_time_in_seconds()
{
    struct timeval tod_record;
    gettimeofday(&tod_record, 0);
    return double(tod_record.tv_sec) + double(tod_record.tv_usec) / 1.0e6;
}

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

float* vtk_at(float* origin,int x,int y,int z,int X,int Y)
{
    // single-component vtkImageData scalars are stored as: float,float,... for consecutive x, then y, then z
    return origin + x + X*(y + Y*z);
}

template <> bool from_string<std::string> (const std::string& s,std::string& val) 
{ 
    val = s;
    return true;
} 
