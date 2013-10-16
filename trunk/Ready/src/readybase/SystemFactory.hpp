/*  Copyright 2011, 2012, 2013 The Ready Bunch

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
class AbstractRD;
class Properties;

// -------------------------------------------------------------------------------------------------------------

/// Methods for creating RD systems when we don't know their type.
namespace SystemFactory {

    /// Load an RD system from file and create the appropriate AbstractRD-derived instance. (User is responsible for deletion.)
    AbstractRD* CreateFromFile(const char *filename,bool is_opencl_available,int opencl_platform,int opencl_device,
                               Properties &render_settings,bool &warn_to_update);

};
