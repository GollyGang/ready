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

class wxVTKRenderWindowInteractor;
class BaseRD;
class BaseRD_2D;
class BaseRD_3D;

void InitializeVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system);
void Initialize2DVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD_2D* system);
void Initialize3DVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD_3D* system);

//void Show2DVTKDemo(wxVTKRenderWindowInteractor* pVTKWindow);
//void Show3DVTKDemo(wxVTKRenderWindowInteractor* pVTKWindow);
