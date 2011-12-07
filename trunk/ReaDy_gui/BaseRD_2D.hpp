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
#include "BaseRD.hpp"

class BaseRD_2D : public BaseRD
{
    public:

        BaseRD_2D();

        // retrieve the width and height of the system
        void GetSize(int &x,int &y);

        // retrieve the chemical value at an x,y location
        virtual float GetAt(int x,int y,int iChemical)=0;

    protected:

        int X,Y;
};
