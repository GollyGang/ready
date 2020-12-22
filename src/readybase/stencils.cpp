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

string OffsetToCode(int iSlot, const Point& point, const string& chem)
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
    if (weight != 1)
    {
        oss << " * " << weight;
    }
    return oss.str();
    // TODO: collect all the stencil points with the same weight and use a single multiply on them
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

set<InputPoint> AppliedStencil::GetInputPoints_Block411() const
{
    set<InputPoint> input_points;
    for (const StencilPoint& stencil_point : stencil.points)
    {
        for (int iSlot = 0; iSlot < 4; iSlot++)
        {
            Point block_point{ CellSlotToBlock(iSlot + stencil_point.point.x), stencil_point.point.y, stencil_point.point.z };
            input_points.insert({ block_point, chem });
        }
    }
    return input_points;
}

template<int N>
Stencil StencilFrom1DArray(const string& label, int const (&arr)[N], int divisor, int dx_power, int dim1)
{
    if (N % 2 != 1)
    {
        throw exception("Internal error: StencilFrom1DArray takes an odd-sized array");
    }
    Stencil stencil{ label, {}, divisor, dx_power };
    for (int i = 0; i < N; i++)
    {
        if (arr[i] != 0)
        {
            Point point{ 0, 0, 0 };
            point.xyz[dim1] = i - (N - 1) / 2;
            stencil.points.push_back({ point, arr[i] });
        }
    }
    return stencil;
}

template<int M, int N>
Stencil StencilFrom2DArray(const string& label, int const (&arr)[M][N], int divisor, int dx_power, int dim1, int dim2)
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
            if (arr[i][j] != 0)
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

template<int L, int M, int N>
Stencil StencilFrom3DArray(const string& label, int const (&arr)[L][M][N], int divisor, int dx_power, int dim1, int dim2, int dim3)
{
    if (L % 2 != 1 || M % 2 != 1 || N % 2 != 1)
    {
        throw exception("Internal error: StencilFrom3DArray takes an odd-sized array");
    }
    Stencil stencil{ label, {}, divisor, dx_power };
    for (int k = 0; k < N; k++)
    {
        for (int j = 0; j < M; j++)
        {
            for (int i = 0; i < L; i++)
            {
                if (arr[i][j][k] != 0)
                {
                    Point point{ 0, 0, 0 };
                    point.xyz[dim1] = i - (L - 1) / 2;
                    point.xyz[dim2] = j - (M - 1) / 2;
                    point.xyz[dim3] = k - (N - 1) / 2;
                    stencil.points.push_back({ point, arr[i][j][k] });
                }
            }
        }
    }
    return stencil;
}

Stencil GetLaplacianStencil(int dimensionality)
{
    switch (dimensionality)
    {
    case 1:
        return StencilFrom1DArray("laplacian", {1,-2,1}, 1, 1, 0);
    case 2:
        // Patra, M. & Karttunen, M. (2006) "Stencils with isotropic discretization error for differential operators" Numerical Methods for Partial Differential Equations, 22. 
        return StencilFrom2DArray("laplacian", {{1,4,1}, {4,-20,4}, {1,4,1}}, 6, 2, 0, 1); // Known under the name "Mehrstellen"
    case 3:
        // O'Reilly and Beck (2006) "A Family of Large-Stencil Discrete Laplacian Approximations in Three Dimensions" Int. J. Num. Meth. Eng. (Equation 22)
        return StencilFrom3DArray("laplacian", {{{2,3,2}, {3,6,3}, {2,3,2}}, {{3,6,3}, {6,-88,6}, {3,6,3}}, {{2,3,2}, {3,6,3}, {2,3,2}}}, 26, 3, 0, 1, 2);
        // TODO: compare with Patra2006, esp the dx_power
    default:
        throw exception("Internal error: unsupported dimensionality in GetLaplacianStencil");
    }
    // We could use the 3D stencil for all dimensionalities, with the advantage that if the user converts a 1D or 2D formula to a full kernel
    // the stencil will continue to work if they then increase the dimensionality to 3. But it comes at a cost of more complicated and presumably
    // slower code for the more common use-cases.
}

Stencil GetBiLaplacianStencil(int dimensionality)
{
    switch (dimensionality)
    {
    case 1:
        return StencilFrom1DArray("bilaplacian", {1,-4,6,-4,1}, 1, 2, 0);
    case 2:
        // Patra, M. & Karttunen, M. (2006) "Stencils with isotropic discretization error for differential operators" Numerical Methods for Partial Differential Equations, 22. 
        return StencilFrom2DArray("bilaplacian", {{0,1,1,1,0}, {1,-2,-10,-2,1}, {1,-10,36,-10,1}, {1,-2,-10,-2,1}, {0,1,1,1,0}}, 3, 4, 0, 1);
    case 3:
        // Patra, M. & Karttunen, M. (2006) "Stencils with isotropic discretization error for differential operators" Numerical Methods for Partial Differential Equations, 22. 
        int c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, divisor;
        // 52-point stencil:
        c2 = c3 = c5 = c8 = c9 = 0;
        c1 = -1; c4 = 10; c6 = -20; c7 = -36; c10 = 360; divisor = 36;
        return StencilFrom3DArray("bilaplacian", {{{c1,c2,c3,c2,c1}, {c2,c4,c5,c4,c2}, {c3,c5,c8, c5,c3}, {c2,c4,c5,c4,c2}, {c1,c2,c3,c2,c1}},
                                                  {{c2,c4,c5,c4,c2}, {c4,c6,c7,c6,c4}, {c5,c7,c9, c7,c5}, {c4,c6,c7,c6,c4}, {c2,c4,c5,c4,c2}},
                                                  {{c3,c5,c8,c5,c3}, {c5,c7,c9,c7,c5}, {c8,c9,c10,c9,c8}, {c5,c7,c9,c7,c5}, {c3,c5,c8,c5,c3}},
                                                  {{c2,c4,c5,c4,c2}, {c4,c6,c7,c6,c4}, {c5,c7,c9, c7,c5}, {c4,c6,c7,c6,c4}, {c2,c4,c5,c4,c2}},
                                                  {{c1,c2,c3,c2,c1}, {c2,c4,c5,c4,c2}, {c3,c5,c8, c5,c3}, {c2,c4,c5,c4,c2}, {c1,c2,c3,c2,c1}}}, divisor, 4, 0, 1, 2);
    default:
        throw exception("Internal error: unsupported dimensionality in GetBiLaplacianStencil");
    }
    // We could use the 3D stencil for all dimensionalities, with the advantage that if the user converts a 1D or 2D formula to a full kernel
    // the stencil will continue to work if they then increase the dimensionality to 3. But it comes at a cost of more complicated and presumably
    // slower code for the more common use-cases.
}

/*Stencil GetTriLaplacianStencil(int dimensionality)
{
    switch (dimensionality)
    {
    case 1:
        return StencilFrom1DArray("trilaplacian", 
    case 2:
        return StencilFrom2DArray("trilaplacian",
    case 3:
        return StencilFrom3DArray("trilaplacian",
    default:
        throw exception("Internal error: unsupported dimensionality in GetTriLaplacianStencil");
    }
    // We could use the 3D stencil for all dimensionalities, with the advantage that if the user converts a 1D or 2D formula to a full kernel
    // the stencil will continue to work if they then increase the dimensionality to 3. But it comes at a cost of more complicated and presumably
    // slower code for the more common use-cases.
}*/

vector<Stencil> GetKnownStencils(int dimensionality)
{
    Stencil XGradient = StencilFrom1DArray("x_gradient", { -1, 0, 1 }, 2, 1, 0);
    Stencil YGradient = StencilFrom1DArray("y_gradient", { -1, 0, 1 }, 2, 1, 1);
    Stencil ZGradient = StencilFrom1DArray("z_gradient", { -1, 0, 1 }, 2, 1, 2);
    Stencil Laplacian = GetLaplacianStencil(dimensionality);
    Stencil BiLaplacian = GetBiLaplacianStencil(dimensionality);
    return { XGradient, YGradient, ZGradient, Laplacian, BiLaplacian };
}

