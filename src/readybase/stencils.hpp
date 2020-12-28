/*  Copyright 2011-2020 The Ready Bunch

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

// Stdlib:
#include <set>
#include <string>
#include <vector>

// ---------------------------------------------------------------------

union Point
{
    struct { int x, y, z; };
    int xyz[3];

    std::string GetName() const;

    friend bool operator<(const Point& a, const Point& b)
    {
        if (a.x < b.x) return true;
        else if (!(b.x < a.x)) {
            if (a.y < b.y) return true;
            else if (!(b.y < a.y)) return a.z < b.z;
            else return false;
        }
        else return false;
    }
};

// ---------------------------------------------------------------------

struct StencilPoint
{
    Point point;
    int weight;

    std::string GetCode(const std::string& chem) const;
};

// ---------------------------------------------------------------------

struct Stencil
{
    std::string label; // e.g. "laplacian"
    std::vector<StencilPoint> points;
    int divisor;
    int dx_power;

    std::string GetDivisorCode() const;
};

// ---------------------------------------------------------------------

struct InputPoint
{
    Point point;
    std::string chem;

    std::string GetName() const;
    std::string GetCode(bool wrap) const;
    std::string GetSwizzled() const;
    static std::string GetIndexString(int val, const std::string& coord, const std::string& coord_capital, bool wrap);

    friend bool operator<(const InputPoint& a, const InputPoint& b)
    {
        if (a.point < b.point) return true;
        else if (!(b.point < a.point) && a.chem < b.chem) return true;
        else return false;
    }
};

// ---------------------------------------------------------------------

struct AppliedStencil
{
    Stencil stencil;
    std::string chem; // e.g. "a"

    std::string GetName() const { return stencil.label + "_" + chem; }
    std::set<InputPoint> GetInputPoints_Block411() const;
};

// ---------------------------------------------------------------------

std::vector<Stencil> GetKnownStencils(int dimensionality);

// ---------------------------------------------------------------------
