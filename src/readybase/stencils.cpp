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
#include <sstream>
using namespace std;

#include "stencils.hpp"

string Point::GetName() const
{
    ostringstream oss;
    if (x != 0 || y != 0 || z != 0)
    {
        const int order[3] = { 2, 1, 0 }; // output will be e.g. "ne", "usw", "d"
        const std::string dirs[3][2] = { {"e", "w"}, {"s", "n"}, {"u", "d"} };
        for (const int i : order)
        {
            if (xyz[i] > 0)
            {
                oss << dirs[i][0];
                if (xyz[i] > 1)
                {
                    oss << xyz[i];
                }
            }
            else if (xyz[i] < 0)
            {
                oss << dirs[i][1];
                if (xyz[i] < -1)
                {
                    oss << -xyz[i];
                }
            }
        }
    }
    return oss.str();
}

int CellSlotToBlock(int x)
{
    if (x > 0)
    {
        return x / 4;
    }
    return -int(ceil(-x / 4.0));
}

int CellSlotToSlot(int x)
{
    while (x < 0)
    {
        x += 4;
    }
    return x % 4;
}

std::string OffsetToCode(int iSlot, const Point& point, const string& chem)
{
    ostringstream oss;
    int source_block_x = CellSlotToBlock(iSlot + point.x);
    int iSourceSlot = CellSlotToSlot(iSlot + point.x);
    InputPoint sourceBlock{ { source_block_x, point.y, point.z }, chem };
    oss << sourceBlock.GetName() << "." << "xyzw"[iSourceSlot];
    return oss.str();
}

std::string StencilPoint::GetCode(int iSlot, const string& chem) const
{
    ostringstream oss;
    oss << OffsetToCode(iSlot, point, chem);
    if (weight != 1.0f)
    {
        oss << " * " << weight;
    }
    return oss.str();
}

string InputPoint::GetName() const
{
    ostringstream oss;
    oss << chem;
    const string dir_name = point.GetName();
    if (!dir_name.empty())
    {
        oss << "_" << dir_name;
    }
    return oss.str();
}

string Stencil::GetDivisorCode() const
{
    ostringstream oss;
    oss << divisor << " * dx";
    for (int i = 0; i < dx_power - 1; i++)
    {
        oss << " * dx";
    }
    return oss.str();
}

Point AppliedStencil::CellPointToBlockPoint(const Point& point)
{
    // convert cell coord to block coord, by dividing x by 4 and rounding away from zero
    Point p{ point.x, point.y, point.z };
    if (p.x > 0)
    {
        p.x = int(ceil(p.x / 4.0));
    }
    else if (p.x < 0)
    {
        p.x = -int(ceil(-p.x / 4.0));
    }
    return p;
}

set<InputPoint> AppliedStencil::GetInputPoints_Block411() const
{
    set<InputPoint> input_points;
    for (const StencilPoint& stencil_point : stencil.points)
    {
        Point block_point = AppliedStencil::CellPointToBlockPoint(stencil_point.point);
        input_points.insert({ block_point, chem });
    }
    return input_points;
}

template<int N>
Stencil StencilFrom1DArray(const string& label, float const (&arr)[N], int divisor, int dx_power, int dim1)
{
    if (N % 2 != 1)
    {
        throw exception("Internal error: StencilFrom1DArray takes an odd-sized array");
    }
    Stencil stencil{ label, {}, divisor, dx_power };
    for (int i = 0; i < N; i++)
    {
        if (arr[i] != 0.0f)
        {
            Point point{ 0, 0, 0 };
            point.xyz[dim1] = i - (N - 1) / 2;
            stencil.points.push_back({ point, arr[i] });
        }
    }
    return stencil;
}

template<int M, int N>
Stencil StencilFrom2DArray(const string& label, float const (&arr)[M][N], int divisor, int dx_power, int dim1, int dim2)
{
    if (M % 2 != 1 || N % 2 != 1)
    {
        throw exception("Internal error: StencilFrom2DArray takes an odd-sized array");
    }
    Stencil stencil{ label, {}, divisor, dx_power };
    for (int j = 0; j < N; j++)
    {
        for (int i = 0; i < M; i++)
        {
            if (arr[i][j] != 0.0f)
            {
                Point point{ 0, 0, 0 };
                point.xyz[dim1] = i - (M - 1) / 2;
                point.xyz[dim2] = j - (N - 1) / 2;
                stencil.points.push_back({ point, arr[i][j] });
            }
        }
    }
    return stencil;
}

std::vector<Stencil> GetKnownStencils()
{
    //int a[3] = { -1, 0, 1 };
    Stencil XGradient = StencilFrom1DArray("x_gradient", { -1, 0, 1 }, 2, 1, 0);
    Stencil YGradient = StencilFrom1DArray("y_gradient", { -1, 0, 1 }, 2, 1, 1);
    Stencil ZGradient = StencilFrom1DArray("z_gradient", { -1, 0, 1 }, 2, 1, 2);
    // TODO: 1D Laplacian 3-point stencil: [ 1,-2,1 ] / dx
    // TODO: 1D bi-Laplacian 5-point stencil: [ 1,-4,6,-4,1 ] / dx^2
    // TODO: 1D tri-Laplacian
    Stencil Laplacian2D = StencilFrom2DArray("laplacian", { {1, 4, 1}, {4, -20, 4}, {1, 4, 1} }, 6, 2, 0, 1);
    // TODO: 2D bi-Laplacian
    // TODO: 2D tri-Laplacian
    // TODO: 3D Laplacian 27-point stencil: [ [ 2,3,2; 3,6,3; 2,3,2 ], [ 3,6,3; 6,-88,6; 3,6,3 ], [ 2,3,2; 3,6,3; 2,3,2 ] ] / 26dx^3
        // Following: O'Reilly and Beck (2006) "A Family of Large-Stencil Discrete Laplacian Approximations in
        // Three Dimensions" Int. J. Num. Meth. Eng. (Equation 22)
    // TODO: 3D bi-Laplacian
    // TODO: 3D tri-Laplacian
    return { XGradient, YGradient, ZGradient, Laplacian2D };
}
