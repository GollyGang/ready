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
#include "BaseOverlay.hpp"
#include "utils.hpp"

void RectangleOverlay::Apply(int iChemical,const PointND& at,float& value) const
{
    if(!at.InRect(this->corner1,this->corner2)) return; // leave points outside untouched
    
    float f;
    switch(this->fill_mode)
    {
        case Constant: f = this->value1; break;
        case WhiteNoise: f = frand(this->value1,this->value2); break;
    }
    switch(this->paste_mode)
    {
        case Copy: value = f; break;
        case Add: value += f; break;
    }
}
