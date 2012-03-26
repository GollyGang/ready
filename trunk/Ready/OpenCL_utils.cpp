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
#include "OpenCL_utils.hpp"

// http://www.khronos.org/message_boards/viewtopic.php?f=37&t=2107
const char* GetDescriptionOfOpenCLError(cl_int err)
{
    switch (err) {
        case CL_SUCCESS:                            return "Success!";
        case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
        case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
        case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "Memory object allocation failure";
        case CL_OUT_OF_RESOURCES:                   return "Out of resources";
        case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
        case CL_PROFILING_INFO_NOT_AVAILABLE:       return "Profiling information not available";
        case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
        case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
        case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
        case CL_MAP_FAILURE:                        return "Map failure";
        case CL_INVALID_VALUE:                      return "Invalid value";
        case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
        case CL_INVALID_PLATFORM:                   return "Invalid platform";
        case CL_INVALID_DEVICE:                     return "Invalid device";
        case CL_INVALID_CONTEXT:                    return "Invalid context";
        case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
        case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
        case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
        case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "Invalid image format descriptor";
        case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
        case CL_INVALID_SAMPLER:                    return "Invalid sampler";
        case CL_INVALID_BINARY:                     return "Invalid binary";
        case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
        case CL_INVALID_PROGRAM:                    return "Invalid program";
        case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
        case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
        case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
        case CL_INVALID_KERNEL:                     return "Invalid kernel";
        case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
        case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
        case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
        case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
        case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
        case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
        case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
        case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
        case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
        case CL_INVALID_EVENT:                      return "Invalid event";
        case CL_INVALID_OPERATION:                  return "Invalid operation";
        case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
        case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
        case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
        default: return "Unknown";
    }
}

const char* GetPlatformInfoIdAsString(cl_platform_info i)
{
    switch(i)
    {
        case CL_PLATFORM_PROFILE: return "CL_PLATFORM_PROFILE";
        case CL_PLATFORM_VERSION: return "CL_PLATFORM_VERSION";
        case CL_PLATFORM_NAME: return "CL_PLATFORM_NAME";
        case CL_PLATFORM_VENDOR: return "CL_PLATFORM_VENDOR";
        case CL_PLATFORM_EXTENSIONS: return "CL_PLATFORM_EXTENSIONS";
        default: return "Unknown";
    }
}

const char* GetDeviceInfoIdAsString(cl_device_info i)
{
    switch(i)
    {
        case CL_DEVICE_TYPE:  return "CL_DEVICE_TYPE";
        case CL_DEVICE_VENDOR_ID:  return "CL_DEVICE_VENDOR_ID";
        case CL_DEVICE_MAX_COMPUTE_UNITS:  return "CL_DEVICE_MAX_COMPUTE_UNITS";
        case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:  return "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS";
        case CL_DEVICE_MAX_WORK_GROUP_SIZE:  return "CL_DEVICE_MAX_WORK_GROUP_SIZE";
        case CL_DEVICE_MAX_WORK_ITEM_SIZES:  return "CL_DEVICE_MAX_WORK_ITEM_SIZES";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:  return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:  return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:  return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:  return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:  return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:  return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE";
        case CL_DEVICE_MAX_CLOCK_FREQUENCY:  return "CL_DEVICE_MAX_CLOCK_FREQUENCY";
        case CL_DEVICE_ADDRESS_BITS:  return "CL_DEVICE_ADDRESS_BITS";
        case CL_DEVICE_MAX_READ_IMAGE_ARGS:  return "CL_DEVICE_MAX_READ_IMAGE_ARGS";
        case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:  return "CL_DEVICE_MAX_WRITE_IMAGE_ARGS";
        case CL_DEVICE_MAX_MEM_ALLOC_SIZE:  return "CL_DEVICE_MAX_MEM_ALLOC_SIZE";
        case CL_DEVICE_IMAGE2D_MAX_WIDTH:  return "CL_DEVICE_IMAGE2D_MAX_WIDTH";
        case CL_DEVICE_IMAGE2D_MAX_HEIGHT:  return "CL_DEVICE_IMAGE2D_MAX_HEIGHT";
        case CL_DEVICE_IMAGE3D_MAX_WIDTH:  return "CL_DEVICE_IMAGE3D_MAX_WIDTH";
        case CL_DEVICE_IMAGE3D_MAX_HEIGHT:  return "CL_DEVICE_IMAGE3D_MAX_HEIGHT";
        case CL_DEVICE_IMAGE3D_MAX_DEPTH:  return "CL_DEVICE_IMAGE3D_MAX_DEPTH";
        case CL_DEVICE_IMAGE_SUPPORT:  return "CL_DEVICE_IMAGE_SUPPORT";
        case CL_DEVICE_MAX_PARAMETER_SIZE:  return "CL_DEVICE_MAX_PARAMETER_SIZE";
        case CL_DEVICE_MAX_SAMPLERS:  return "CL_DEVICE_MAX_SAMPLERS";
        case CL_DEVICE_MEM_BASE_ADDR_ALIGN:  return "CL_DEVICE_MEM_BASE_ADDR_ALIGN";
        case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:  return "CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE";
        case CL_DEVICE_SINGLE_FP_CONFIG:  return "CL_DEVICE_SINGLE_FP_CONFIG";
        case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:  return "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE";
        case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:  return "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE";
        case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:  return "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE";
        case CL_DEVICE_GLOBAL_MEM_SIZE:  return "CL_DEVICE_GLOBAL_MEM_SIZE";
        case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:  return "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE";
        case CL_DEVICE_MAX_CONSTANT_ARGS:  return "CL_DEVICE_MAX_CONSTANT_ARGS";
        case CL_DEVICE_LOCAL_MEM_TYPE:  return "CL_DEVICE_LOCAL_MEM_TYPE";
        case CL_DEVICE_LOCAL_MEM_SIZE:  return "CL_DEVICE_LOCAL_MEM_SIZE";
        case CL_DEVICE_ERROR_CORRECTION_SUPPORT:  return "CL_DEVICE_ERROR_CORRECTION_SUPPORT";
        case CL_DEVICE_PROFILING_TIMER_RESOLUTION:  return "CL_DEVICE_PROFILING_TIMER_RESOLUTION";
        case CL_DEVICE_ENDIAN_LITTLE:  return "CL_DEVICE_ENDIAN_LITTLE";
        case CL_DEVICE_AVAILABLE:  return "CL_DEVICE_AVAILABLE";
        case CL_DEVICE_COMPILER_AVAILABLE:  return "CL_DEVICE_COMPILER_AVAILABLE";
        case CL_DEVICE_EXECUTION_CAPABILITIES:  return "CL_DEVICE_EXECUTION_CAPABILITIES";
        case CL_DEVICE_QUEUE_PROPERTIES:  return "CL_DEVICE_QUEUE_PROPERTIES";
        case CL_DEVICE_NAME:  return "CL_DEVICE_NAME";
        case CL_DEVICE_VENDOR:  return "CL_DEVICE_VENDOR";
        case CL_DRIVER_VERSION:  return "CL_DRIVER_VERSION";
        case CL_DEVICE_PROFILE:  return "CL_DEVICE_PROFILE";
        case CL_DEVICE_VERSION:  return "CL_DEVICE_VERSION";
        case CL_DEVICE_EXTENSIONS:  return "CL_DEVICE_EXTENSIONS";
        case CL_DEVICE_PLATFORM:  return "CL_DEVICE_PLATFORM";
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:  return "CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF";
        case CL_DEVICE_HOST_UNIFIED_MEMORY:  return "CL_DEVICE_HOST_UNIFIED_MEMORY";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:  return "CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:  return "CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:  return "CL_DEVICE_NATIVE_VECTOR_WIDTH_INT";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:  return "CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:  return "CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:  return "CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE";
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:  return "CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF";
        case CL_DEVICE_OPENCL_C_VERSION:  return "CL_DEVICE_OPENCL_C_VERSION";
        default: return "Unknown";
    }
}
