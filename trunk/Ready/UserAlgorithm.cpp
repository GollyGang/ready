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
#include "UserAlgorithm.hpp"

// STL:
using namespace std;

UserAlgorithm::UserAlgorithm()
{
    this->need_reload_program = true;
}

void UserAlgorithm::SetProgram(string s)
{
    if(s != this->program_string)
        this->need_reload_program = true;
    this->program_string = s;
}
