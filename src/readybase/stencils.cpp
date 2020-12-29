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

// Local:
#include "stencils.hpp"

// Stdlib:
#include <cmath>
#include <exception>
#include <map>
#include <sstream>
using namespace std;

// ---------------------------------------------------------------------

string Point::GetName() const
{
    ostringstream oss;
    if (x != 0 || y != 0 || z != 0)
    {
        const int order[3] = { 2, 1, 0 }; // output will be e.g. "ne", "usw", "d"
        const std::string dirs[3][2] = { {"e", "w"}, {"n", "s"}, {"u", "d"} };
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

// ---------------------------------------------------------------------

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

// ---------------------------------------------------------------------

pair<InputPoint, InputPoint> InputPoint::GetAlignedBlocks() const
{
    // return the two block-aligned float4's we'll need to assemble this non-block-aligned float4
    InputPoint block_left{ point, chem };
    InputPoint block_right{ point, chem };
    if (point.x >= 0)
    {
        block_left.point.x -= point.x % 4;
        block_right.point.x += 4 - (point.x % 4);
    }
    else
    {
        block_left.point.x -= 4 + (point.x % 4);
        block_right.point.x -= point.x % 4;
    }
    return make_pair(block_left, block_right);
}

// ---------------------------------------------------------------------

string InputPoint::GetSwizzled() const
{
    // assemble a non-block-aligned float4 for the requested point
    const pair<InputPoint, InputPoint> blocks = GetAlignedBlocks();
    const InputPoint& block_left = blocks.first;
    const InputPoint& block_right = blocks.second;
    ostringstream oss;
    switch (point.x % 4)
    {
    case 1:
    case -3:
        oss << block_left.GetName() << ".yzw, " << block_right.GetName() << ".x";
        break;
    case 2:
    case -2:
        oss << block_left.GetName() << ".zw, " << block_right.GetName() << ".xy";
        break;
    case 3:
    case -1:
        oss << block_left.GetName() << ".w, " << block_right.GetName() << ".xyz";
        break;
    }
    return oss.str();
}

// -------------------------------------------------------------------------

string InputPoint::GetDirectAccessCode(bool wrap) const
{
    if (point.x % 4 != 0)
    {
        throw runtime_error("internal error in GetDirectAccessCode: point.x not divisible by 4");
    }
    const string index_x = GetIndexString(point.x / 4, "x", "X", wrap);
    const string index_y = GetIndexString(point.y, "y", "Y", wrap);
    const string index_z = GetIndexString(point.z, "z", "Z", wrap);
    ostringstream oss;
    oss << GetName() << " = " << chem << "_in[X * (Y * " << index_z << " + " << index_y << ") + " << index_x << "]";
    return oss.str();
}

// -------------------------------------------------------------------------

string InputPoint::GetIndexString(int val, const string& coord, const string& coord_capital, bool wrap)
{
    ostringstream oss;
    const string index = "index_" + coord;
    if (val == 0)
    {
        oss << index;
    }
    else if (wrap)
    {
        oss << "((" << index << showpos << val << " + " << coord_capital << ") & (" << coord_capital << " - 1))";
    }
    else
    {
        oss << "min(" << coord_capital << "-1, max(0, " << index << showpos << val << "))";
    }
    return oss.str();
}

// ---------------------------------------------------------------------

string Stencil::GetDivisorCode() const
{
    ostringstream oss;
    if (divisor != 1 || dx_power > 0)
    {
        oss << " / ";
        if (dx_power > 0)
        {
            oss << "(";
        }
        oss << divisor;
        for (int i = 0; i < dx_power; i++)
        {
            oss << " * dx";
        }
        if (dx_power > 0)
        {
            oss << ")";
        }
    }
    return oss.str();
}

// ---------------------------------------------------------------------

set<InputPoint> AppliedStencil::GetInputPoints_Block411() const
{
    set<InputPoint> input_points;
    for (const StencilPoint& stencil_point : stencil.points)
    {
        input_points.insert({ stencil_point.point, chem });
    }
    return input_points;
}

// ---------------------------------------------------------------------

string AppliedStencil::GetCode() const
{
    ostringstream oss;
    oss << GetName() << " = ";
    const string divisor_code = stencil.GetDivisorCode(); 
    if (!divisor_code.empty())
    {
        oss << "(";
    }
    map<int, vector<Point>> weights;
    for (const StencilPoint& stencil_point : stencil.points)
    {
        weights[stencil_point.weight].push_back(stencil_point.point);
    }
    bool is_first_weight_list = true;
    for(const pair<int, vector<Point>>& weight_list : weights)
    {
        if (!is_first_weight_list)
        {
            oss << " + ";
        }
        is_first_weight_list = false;
        const int weight = weight_list.first;
        const vector<Point>& points = weight_list.second;
        if (weight != 1)
        {
            oss << weight << " * ";
            if (points.size() > 1)
            {
                oss << "(";
            }
        }
        bool is_first_point = true;
        for (const Point& point : points)
        {
            if (!is_first_point)
            {
                oss << " + ";
            }
            is_first_point = false;
            oss << InputPoint{ point, chem }.GetName();
        }
        if (weight != 1 && points.size() > 1)
        {
            oss << ")";
        }
    }
    if (!divisor_code.empty())
    {
        oss << ")" << divisor_code;
    }
    return oss.str();
}

// ---------------------------------------------------------------------

template<int N>
Stencil StencilFrom1DArray(const string& label, int const (&arr)[N], int divisor, int dx_power, int dim1)
{
    if (N % 2 != 1)
    {
        throw runtime_error("Internal error: StencilFrom1DArray takes an odd-sized array");
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

// ---------------------------------------------------------------------

template<int M, int N>
Stencil StencilFrom2DArray(const string& label, int const (&arr)[M][N], int divisor, int dx_power, int dim1, int dim2)
{
    if (M % 2 != 1 || N % 2 != 1)
    {
        throw runtime_error("Internal error: StencilFrom2DArray takes an odd-sized array");
    }
    Stencil stencil{ label, {}, divisor, dx_power };
    for (int j = 0; j < M; j++)
    {
        for (int i = 0; i < N; i++)
        {
            if (arr[j][i] != 0)
            {
                Point point{ 0, 0, 0 };
                point.xyz[dim1] = i - (N - 1) / 2;
                point.xyz[dim2] = - j + (M - 1) / 2; // rows are in reading order, heading south, which is negative y
                stencil.points.push_back({ point, arr[j][i] });
            }
        }
    }
    return stencil;
}

// ---------------------------------------------------------------------

template<int L, int M, int N>
Stencil StencilFrom3DArray(const string& label, int const (&arr)[L][M][N], int divisor, int dx_power, int dim1, int dim2, int dim3)
{
    if (L % 2 != 1 || M % 2 != 1 || N % 2 != 1)
    {
        throw runtime_error("Internal error: StencilFrom3DArray takes an odd-sized array");
    }
    Stencil stencil{ label, {}, divisor, dx_power };
    for (int k = 0; k < L; k++)
    {
        for (int j = 0; j < M; j++)
        {
            for (int i = 0; i < N; i++)
            {
                if (arr[k][j][i] != 0)
                {
                    Point point{ 0, 0, 0 };
                    point.xyz[dim1] = i - (N - 1) / 2;
                    point.xyz[dim2] = -j + (M - 1) / 2; // rows are in reading order, heading south, which is negative y
                    point.xyz[dim3] = k - (L - 1) / 2;
                    stencil.points.push_back({ point, arr[k][j][i] });
                }
            }
        }
    }
    return stencil;
}

// ---------------------------------------------------------------------

Stencil GetGaussianStencil(int dimensionality)
{
    switch (dimensionality)
    {
    case 1:
        // from OpenCV
        return StencilFrom1DArray("gaussian", {1,4,6,4,1}, 16, 0, 0); // is dx_power correct?
    case 2:
        // from https://homepages.inf.ed.ac.uk/rbf/HIPR2/gsmooth.htm
        return StencilFrom2DArray("gaussian", {{1,4,7,4,1},{4,16,26,16,4},{7,26,41,26,7},{4,16,26,16,4},{1,4,7,4,1}}, 273, 0, 0, 1); // is dx_power correct?
    case 3:
        // see Scripts/Python/convolve.py
        return StencilFrom3DArray("gaussian", {{{1,4,6,4,1},{4,16,25,16,4},{7,27,44,27,7},{4,16,25,16,4},{1,4,6,4,1}},
                                               {{4,16,25,16,4},{16,62,101,62,16},{26,101,164,101,26},{16,62,101,62,16},{4,16,25,16,4}},
                                               {{7,27,44,27,7},{26,101,164,101,26},{41,159,258,159,41},{26,101,164,101,26},{7,27,44,27,7}},
                                               {{4,16,25,16,4},{16,62,101,62,16},{26,101,164,101,26},{16,62,101,62,16},{4,16,25,16,4}},
                                               {{1,4,6,4,1},{4,16,25,16,4},{7,27,44,27,7},{4,16,25,16,4},{1,4,6,4,1}}}, 4390, 0, 0, 1, 2); // is dx_power correct?
    default:
        throw runtime_error("Internal error: unsupported dimensionality in GetGaussianStencil");
    }
}

// ---------------------------------------------------------------------

Stencil GetLaplacianStencil(int dimensionality)
{
    switch (dimensionality)
    {
    case 1:
        return StencilFrom1DArray("laplacian", {1,-2,1}, 1, 2, 0);
    case 2:
        // Patra, M. & Karttunen, M. (2006) "Stencils with isotropic discretization error for differential operators" Numerical Methods for Partial Differential Equations, 22.
        return StencilFrom2DArray("laplacian", {{1,4,1}, {4,-20,4}, {1,4,1}}, 6, 2, 0, 1); // Known under the name "Mehrstellen"
    case 3:
        // Patra, M. & Karttunen, M. (2006) "Stencils with isotropic discretization error for differential operators" Numerical Methods for Partial Differential Equations, 22.
        int c1, c2, c3, c4, divisor;
        // 27-point stencil:
        c1 = 1; c2 = 3; c3 = 14; c4 = -128; divisor = 30;
        return StencilFrom3DArray("laplacian", {{{c1,c2,c1}, {c2,c3,c2}, {c1,c2,c1}},
                                                {{c2,c3,c2}, {c3,c4,c3}, {c2,c3,c2}},
                                                {{c1,c2,c1}, {c2,c3,c2}, {c1,c2,c1}}}, divisor, 2, 0, 1, 2);
    default:
        throw runtime_error("Internal error: unsupported dimensionality in GetLaplacianStencil");
    }
    // We could use the 3D stencil for all dimensionalities, with the advantage that if the user converts a 1D or 2D formula to a full kernel
    // the stencil will continue to work if they then increase the dimensionality to 3. But it comes at a cost of more complicated and presumably
    // slower code for the more common use-cases.
}

// ---------------------------------------------------------------------

Stencil GetBiLaplacianStencil(int dimensionality)
{
    switch (dimensionality)
    {
    case 1:
        return StencilFrom1DArray("bilaplacian", {1,-4,6,-4,1}, 1, 4, 0);
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
        throw runtime_error("Internal error: unsupported dimensionality in GetBiLaplacianStencil");
    }
    // We could use the 3D stencil for all dimensionalities, with the advantage that if the user converts a 1D or 2D formula to a full kernel
    // the stencil will continue to work if they then increase the dimensionality to 3. But it comes at a cost of more complicated and presumably
    // slower code for the more common use-cases.
}

// ---------------------------------------------------------------------

Stencil GetTriLaplacianStencil(int dimensionality)
{
    switch (dimensionality)
    {
    case 1:
        return StencilFrom1DArray("trilaplacian", {1,-6,15,-20,15,-6,1}, 1, 6, 0);
    case 2:
        // obtained by convolving a 2D Laplacian stencil with a 2D bi-Laplacian stencil - see Scripts/Python/convolve.py
        return StencilFrom2DArray("trilaplacian", { {0,1,5,6,5,1,0}, {1,6,-33,-56,-33,6,1}, {5,-33,6,314,6,-33,5},
                                                   {6,-56,314,-888,314,-56,6}, {5,-33,6,314,6,-33,5}, {1,6,-33,-56,-33,6,1},
                                                   {0,1,5,6,5,1,0} }, 18, 6, 0, 1);
    case 3:
        // obtained by convolving a 3D Laplacian stencil with a 3D bi-Laplacian stencil - see Scripts/Python/convolve.py
        return StencilFrom3DArray("trilaplacian", { {{-1,-3,-1,0,-1,-3,-1},{-3,-4,27,20,27,-4,-3},{-1,27,139,60,139,27,-1},
               {0,20,60,40,60,20,0},{-1,27,139,60,139,27,-1},{-3,-4,27,20,27,-4,-3},{-1,-3,-1,0,-1,-3,-1}},{{-3,-4,27,20,27,-4,-3},
            {-4,198,180,-28,180,198,-4},{27,180,-1719,-396,-1719,180,27},{20,-28,-396,-392,-396,-28,20},{27,180,-1719,-396,-1719,180,27},
            {-4,198,180,-28,180,198,-4},{-3,-4,27,20,27,-4,-3}},{{-1,27,139,60,139,27,-1},{27,180,-1719,-396,-1719,180,27},
            {139,-1719,1827,4816,1827,-1719,139},{60,-396,4816,2680,4816,-396,60},{139,-1719,1827,4816,1827,-1719,139},
            {27,180,-1719,-396,-1719,180,27},{-1,27,139,60,139,27,-1}},{{0,20,60,40,60,20,0},{20,-28,-396,-392,-396,-28,20},
            {60,-396,4816,2680,4816,-396,60},{40,-392,2680,-47536,2680,-392,40},{60,-396,4816,2680,4816,-396,60},{20,-28,-396,-392,-396,-28,20},
            {0,20,60,40,60,20,0}},{{-1,27,139,60,139,27,-1},{27,180,-1719,-396,-1719,180,27},{139,-1719,1827,4816,1827,-1719,139},
            {60,-396,4816,2680,4816,-396,60},{139,-1719,1827,4816,1827,-1719,139},{27,180,-1719,-396,-1719,180,27},{-1,27,139,60,139,27,-1}},
            {{-3,-4,27,20,27,-4,-3},{-4,198,180,-28,180,198,-4},{27,180,-1719,-396,-1719,180,27},{20,-28,-396,-392,-396,-28,20},
            {27,180,-1719,-396,-1719,180,27},{-4,198,180,-28,180,198,-4},{-3,-4,27,20,27,-4,-3}},{{-1,-3,-1,0,-1,-3,-1},{-3,-4,27,20,27,-4,-3},
            {-1,27,139,60,139,27,-1},{0,20,60,40,60,20,0},{-1,27,139,60,139,27,-1},{-3,-4,27,20,27,-4,-3},{-1,-3,-1,0,-1,-3,-1}} }, 1080, 6, 0, 1, 2);
    default:
        throw runtime_error("Internal error: unsupported dimensionality in GetTriLaplacianStencil");
    }
    // We could use the 3D stencil for all dimensionalities, with the advantage that if the user converts a 1D or 2D formula to a full kernel
    // the stencil will continue to work if they then increase the dimensionality to 3. But it comes at a cost of more complicated and presumably
    // slower code for the more common use-cases.
}

// ---------------------------------------------------------------------

vector<Stencil> GetKnownStencils(int dimensionality)
{
    Stencil XGradient = StencilFrom1DArray("x_gradient", { -1, 0, 1 }, 2, 1, 0);
    Stencil YGradient = StencilFrom1DArray("y_gradient", { -1, 0, 1 }, 2, 1, 1);
    Stencil ZGradient = StencilFrom1DArray("z_gradient", { -1, 0, 1 }, 2, 1, 2);
    Stencil SobelN = StencilFrom2DArray("sobelN", { {1, 2, 1},
                                                    {0, 0, 0},
                                                    {-1, -2, -1} }, 1, 0, 0, 1);
    Stencil SobelS = StencilFrom2DArray("sobelS", { {-1, -2, -1},
                                                    {0, 0, 0},
                                                    {1, 2, 1} }, 1, 0, 0, 1);
    Stencil SobelE = StencilFrom2DArray("sobelE", { {-1, 0, 1},
                                                    {-2, 0, 2},
                                                    {-1, 0, 1} }, 1, 0, 0, 1);
    Stencil SobelW = StencilFrom2DArray("sobelW", { {1, 0, -1},
                                                    {2, 0, -2},
                                                    {1, 0, -1} }, 1, 0, 0, 1);
    Stencil SobelNW = StencilFrom2DArray("sobelNW", { {2, 1, 0},
                                                      {1, 0, -1},
                                                      {0, -1, -2} }, 1, 0, 0, 1);
    Stencil SobelNE = StencilFrom2DArray("sobelNE", { {0, 1, 2},
                                                      {-1, 0, 1},
                                                      {-2, -1, 0} }, 1, 0, 0, 1);
    Stencil SobelSW = StencilFrom2DArray("sobelSW", { {0, -1, -2},
                                                      {1, 0, -1},
                                                      {2, 1, 0} }, 1, 0, 0, 1);
    Stencil SobelSE = StencilFrom2DArray("sobelSE", { {-2, -1, 0},
                                                      {-1, 0, 1},
                                                      {0, 1, 2} }, 1, 0, 0, 1);
    Stencil Gaussian = GetGaussianStencil(dimensionality);
    Stencil Laplacian = GetLaplacianStencil(dimensionality);
    Stencil BiLaplacian = GetBiLaplacianStencil(dimensionality);
    Stencil TriLaplacian = GetTriLaplacianStencil(dimensionality);
    return { XGradient, YGradient, ZGradient, Gaussian, Laplacian, BiLaplacian, TriLaplacian, 
        SobelN, SobelS, SobelE, SobelW, SobelNW, SobelNE, SobelSW, SobelSE };
}

// ---------------------------------------------------------------------
