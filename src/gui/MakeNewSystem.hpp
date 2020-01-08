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

class AbstractRD;
class Properties;

AbstractRD* MakeNewImage1D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewImage2D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewImage3D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewGeodesicSphere(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewTorus(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewTriangularMesh(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewHexagonalMesh(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewRhombilleTiling(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewPenroseP3Tiling(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewPenroseP2Tiling(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewDelaunay2D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewVoronoi2D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewDelaunay3D(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewBodyCentredCubicHoneycomb(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewFaceCentredCubicHoneycomb(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewDiamondHoneycomb(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewHyperbolicPlane(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
AbstractRD* MakeNewHyperbolicSpace(const bool is_opencl_available, const int opencl_platform, const int opencl_device, Properties& render_settings);
