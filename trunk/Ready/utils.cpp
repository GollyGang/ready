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
#include <limits.h>

// STL:
#include <vector>
using namespace std;

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

template <> bool from_string<string> (const string& s,string& val) 
{ 
    val = s;
    return true;
} 

string GetChemicalName(int i)
{ 
    if(i<26)
        return string(1,'a'+i); 
    else if(i<26*27)
        return GetChemicalName(i/26-1)+GetChemicalName(i%26);
    throw runtime_error("GetChemicalName: out of range");
}

int IndexFromChemicalName(const string& s)
{
    for(int i=0;i<26*27;i++)
        if(s==GetChemicalName(i))
            return i;
    throw runtime_error("IndexFromChemicalName: unrecognised chemical name: "+s);
}

// read a multiline string, strip leading whitespace where shared by all lines
string trim_multiline_string(const char* s)
{
    vector<string> vs;
    istringstream iss(s);
    string item;
    int minLeadingWhitespace = INT_MAX,tailtrim = 0;
    while(getline(iss, item, '\n'))
    {
        int leadingWhitespace = (int)item.find_first_not_of(" \t");
        if(leadingWhitespace!=string::npos)
        {
            minLeadingWhitespace = min(minLeadingWhitespace,leadingWhitespace);
            vs.push_back(item);
            tailtrim=0;
        }
        else
        {
            if(!vs.empty()) 
                vs.push_back("\n");
            // (lines with all-whitespace don't contribute to minLeadingWhitespace)
            tailtrim++;
        }
    }
    ostringstream oss;
    for(int i=0;i<(int)vs.size()-tailtrim;i++)
    {
        if(minLeadingWhitespace < (int)vs[i].size())
            oss << vs[i].substr(minLeadingWhitespace);
        if(i < (int)vs.size()-tailtrim-1)
            oss << "\n";
    }
    return oss.str();
}
