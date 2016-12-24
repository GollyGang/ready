/*  Copyright 2011-2016 The Ready Bunch

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
#include "overlays.hpp"

// STL:
#include <vector>

/// Generates image/mesh patterns by drawing a series of overlays.
class InitialPatternGenerator
{
    public:

        ~InitialPatternGenerator();

        void ReadFromXML(vtkXMLDataElement* node);
        vtkSmartPointer<vtkXMLDataElement> GetAsXML(bool generate_initial_pattern_when_loading) const;

        size_t GetNumberOfOverlays() const { return this->overlays.size(); }
        const Overlay& GetOverlay(size_t i) const { return *this->overlays[i]; }

        /// Create a generator suitable for Gray-Scott, so that new patterns can start working immediately.
        void CreateDefaultInitialPatternGenerator(size_t num_chemicals);

    private:

        void RemoveAllOverlays();

        std::vector<Overlay*> overlays; // TODO: use unique_ptr when C++11-compatible VTK is available on target platforms
};
