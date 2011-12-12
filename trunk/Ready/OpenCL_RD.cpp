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
#include "OpenCL_RD.hpp"

void OpenCL_RD::SetPlatform(int i)
{
    if(i != this->iPlatform)
        this->need_reload_opencl = true;
    this->iPlatform = i;
}

void OpenCL_RD::SetDevice(int i)
{
    if(i != this->iDevice)
        this->need_reload_opencl = true;
    this->iDevice = i;
}

int OpenCL_RD::GetPlatform()
{
    return this->iPlatform;
}

int OpenCL_RD::GetDevice()
{
    return this->iDevice;
}

void OpenCL_RD::ReloadOpenCLIfNeeded()
{
    if(!this->need_reload_opencl) return;

    // TODO

    this->need_reload_opencl = false;
}
