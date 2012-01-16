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

// STL:
#include <string>
#include <sstream>

double get_time_in_seconds();

float frand(float lower,float upper);

double hypot2(double x,double y);

double hypot3(double x,double y,double z);

// http://www.doc.ic.ac.uk/~akf/handel-c/cgi-bin/forum.cgi?msg=551 
#define STRING_FROM_LITERAL(a) #a
#define STR(a) STRING_FROM_LITERAL(a)

template <typename T> std::string to_string(const T& t) 
{ 
    std::ostringstream oss; 
    oss << t; 
    return oss.str(); 
} 

template <typename T> bool from_string(const std::string& s,T& val) 
{ 
    std::istringstream iss(s); 
    iss >> val; 
    return !iss.fail(); 
} 

