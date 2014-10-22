#include "gvki/opencl_header.h"
#include <cstdlib>
#include <iostream>
#include "gvki/UnderlyingCaller.h"
#include "gvki/Logger.h"
#include "gvki/Debug.h"
#include <cassert>
#include <cstring>

#ifdef MACRO_LIB
#define API_SUFFIX _hook
#else
#define API_SUFFIX
#endif

// This macro appends API_SUFFIX to its argument
#define __DEFN(name, suffix) name ## suffix
#define _DEFN(name, suffix) __DEFN(name, suffix)
#define DEFN(name) _DEFN(name, API_SUFFIX)

using namespace std;
using namespace gvki;

static UnderlyingCaller uc;

extern "C" {

cl_mem
DEFN(clCreateBuffer)
    (cl_context   context,
     cl_mem_flags flags,
     size_t       size,
     void *       host_ptr,
     cl_int *     errcode_ret
    )
{
    DEBUG_MSG("Intercepted clCreateBuffer()");
    cl_int success = CL_SUCCESS;
    cl_mem buffer = uc.clCreateBufferU(context, flags, size, host_ptr, &success);

    if (success == CL_SUCCESS)
    {
        Logger& l = Logger::Singleton();

        BufferInfo bi;
        bi.size = size;
        bi.data = host_ptr;
        bi.flags = flags;
        bi.bufferType = BufferInfo::MEMORYBUFFER;
        l.buffers[buffer] = bi;
    }

    if (errcode_ret)
        *errcode_ret = success;

    return buffer;
}

cl_mem
DEFN(clCreateSubBuffer)
    (cl_mem                   buffer,
     cl_mem_flags             flags,
     cl_buffer_create_type    buffer_create_type,
     const void *             buffer_create_info,
     cl_int *                 errcode_ret)
{
    DEBUG_MSG("Intercepted clCreateSubBuffer()");
    ERROR_MSG("Not supported!!");
    exit(1);
}

cl_mem
DEFN(clCreateImage2D)
    (cl_context              context,
     cl_mem_flags            flags,
     const cl_image_format * image_format,
     size_t                  image_width,
     size_t                  image_height,
     size_t                  image_row_pitch,
     void *                  host_ptr,
     cl_int *                errcode_ret)
{
    DEBUG_MSG("Intercepted clCreate2DImage()");

    cl_int success = CL_SUCCESS;
    cl_mem img = uc.clCreateImage2DU(context,
                                     flags,
                                     image_format,
                                     image_width,
                                     image_height,
                                     image_row_pitch,
                                     host_ptr,
                                     &success);

    if (success == CL_SUCCESS)
    {
        Logger& l = Logger::Singleton();
        ERROR_MSG("Not supported!");
        exit(1);
    }

    if (errcode_ret)
        *errcode_ret = success;

    return img;
}

cl_mem
DEFN(clCreateImage3D)
    (cl_context               context,
     cl_mem_flags            flags,
     const cl_image_format * image_format,
     size_t                  image_width,
     size_t                  image_height,
     size_t                  image_depth,
     size_t                  image_row_pitch,
     size_t                  image_slice_pitch,
     void *                  host_ptr,
     cl_int *                errcode_ret)
{
    cl_int success = CL_SUCCESS;
    cl_mem img = uc.clCreateImage3DU(context,
                                     flags,
                                     image_format,
                                     image_width,
                                     image_height,
                                     image_depth,
                                     image_row_pitch,
                                     image_slice_pitch,
                                     host_ptr,
                                     &success);

    if (success == CL_SUCCESS)
    {
        Logger& l = Logger::Singleton();
        ERROR_MSG("Not supported!");
        exit(1);
    }

    if (errcode_ret)
        *errcode_ret = success;

    return img;
}

cl_sampler
DEFN(clCreateSampler)
    (cl_context          /* context */,
     cl_bool             /* normalized_coords */,
     cl_addressing_mode  /* addressing_mode */,
     cl_filter_mode      /* filter_mode */,
     cl_int *            /* errcode_ret */)
{
    DEBUG_MSG("Intercepted clCreateSampler()");
    ERROR_MSG("Not supported!!");
    exit(1);
}

/* 5.6 Program objects */
cl_program
DEFN(clCreateProgramWithSource)
    (cl_context        context,
     cl_uint           count,
     const char **     strings,
     const size_t *    lengths,
     cl_int *          errcode_ret
    )
{
    DEBUG_MSG("Intercepted clCreateProgramWithSource()");
    cl_int success = CL_SUCCESS;
    cl_program program = uc.clCreateProgramWithSourceU(context, count, strings, lengths, &success);

    if (success == CL_SUCCESS)
    {
        Logger& l = Logger::Singleton();
        l.programs[program] = ProgramInfo();

        // Make sure we work on the version in the container
        // so we don't do lots of unnecessary copies
        ProgramInfo& pi = l.programs[program];
        
        if (lengths == NULL)
        {
            // All strings are null terminated
            for (int pIndex=0; pIndex < count; ++pIndex)
            {
                pi.sources.push_back(std::string(strings[pIndex]));
            }
        }
        else
        {
            // The user has specified the string length
            for (int pIndex=0; pIndex < count; ++pIndex)
            {
                if (lengths[pIndex] == 0)
                {
                    // This particular string is null terminated
                    pi.sources.push_back(std::string(strings[pIndex]));
                }
                else
                {
                    size_t length = lengths[pIndex];
                    pi.sources.push_back(std::string(strings[pIndex], length));
                }
            }
        }
    }

    if (errcode_ret)
        *errcode_ret = success;

    return program;
}

cl_int
DEFN(clBuildProgram)
    (cl_program           program,
     cl_uint              num_devices,
     const cl_device_id * device_list,
     const char *         options,
     void (*pfn_notify)(cl_program /* program */, void * /* user_data */),
     void *               user_data
    )
{
    DEBUG_MSG("Intercepted clCreateBuildProgram()");
    cl_int success = uc.clBuildProgramU(program,
                                        num_devices,
                                        device_list,
                                        options,
                                        pfn_notify,
                                        user_data);

    if (success == CL_SUCCESS)
    {
        Logger& l = Logger::Singleton();

        assert(l.programs.count(program) == 1 && "Program was not logged!");
        // Make sure we work on the version in the container
        // so we don't do lots of unnecessary copies
        ProgramInfo& pi = l.programs[program];

        if (options != NULL)
        {
            pi.compileFlags = std::string(options);
        }
        else
        {
            pi.compileFlags = std::string("");
        }
    }

    return success;
}

/* 5.7 Kernel objects */

cl_kernel
DEFN(clCreateKernel)
    (cl_program      program,
     const char *    kernel_name,
     cl_int *        errcode_ret
    )
{
    DEBUG_MSG("Intercepted clCreateKernel()");

    cl_int success = CL_SUCCESS;
    cl_kernel kernel = uc.clCreateKernelU(program, kernel_name, errcode_ret);
    if ( success == CL_SUCCESS)
    {
        Logger& l = Logger::Singleton();
        l.kernels[kernel] = KernelInfo();

        assert(l.programs.count(program) == 1 && "Program was not logged!");

        // Make sure we work on the version in the container
        // so we don't do lots of unnecessary copies
        KernelInfo& ki = l.kernels[kernel];
        ki.program = program;
        ki.entryPointName = std::string(kernel_name);
    }

    if (errcode_ret)
        *errcode_ret = success;

    return kernel;
}

cl_int
DEFN(clCreateKernelsInProgram)
    (cl_program     /* program */,
     cl_uint        /* num_kernels */,
     cl_kernel *    /* kernels */,
     cl_uint *      /* num_kernels_ret */
    )
{
    DEBUG_MSG("Intercepted clCreatKernelsInProgram : NOT SUPPORTED!");
    exit(1);
}

cl_int
DEFN(clSetKernelArg)
    (cl_kernel    kernel,
     cl_uint      arg_index,
     size_t       arg_size,
     const void * arg_value)
{
    DEBUG_MSG("Intercepted clSetKernelArg()");
    cl_int success = uc.clSetKernelArgU(kernel,
                                        arg_index,
                                        arg_size,
                                        arg_value
                                       );

    if (success == CL_SUCCESS)
    {
        Logger& l = Logger::Singleton();

        assert (l.kernels.count(kernel) == 1 && "Kernel was not logged");
        KernelInfo& ki = l.kernels[kernel];

        // Resize arguments vector if necessary
        if (ki.arguments.size() <= arg_index)
            ki.arguments.resize(arg_index +1);

        ArgInfo& ai = ki.arguments[arg_index];

        if (ai.argValue != NULL)
        {
            // We must have malloc()'ed from a previous call to this
            // function so free the pointer
            free((void*) ai.argValue);
        }

        // The user is allowed to do whatever
        // they want with the memory pointed
        // to by ``arg_value`` so we need to
        // copy its contents.
        //
        // FIXME: We aren't always free'ing the memory
        // allocated here because we aren't tracking
        // cl_kernel or cl_context destruction
        ai.argValue = malloc(arg_size);
        memcpy((void*) ai.argValue, arg_value, arg_size);

        ai.argSize = arg_size;


        ki.arguments[arg_index] = ai;
    }

    return success;
}

/* 5.8 Executing kernels */
cl_int
DEFN(clEnqueueNDRangeKernel)
    (cl_command_queue command_queue,
     cl_kernel        kernel,
     cl_uint          work_dim,
     const size_t *   global_work_offset,
     const size_t *   global_work_size,
     const size_t *   local_work_size,
     cl_uint          num_events_in_wait_list,
     const cl_event * event_wait_list,
     cl_event *       event
    )
{
    DEBUG_MSG("Intercepted clEnqueueNDRangeKernel()");

    cl_int success = uc.clEnqueueNDRangeKernelU(command_queue,
                                                kernel,
                                                work_dim,
                                                global_work_offset,
                                                global_work_size,
                                                local_work_size,
                                                num_events_in_wait_list,
                                                event_wait_list,
                                                event
                                               );

    if (success == CL_SUCCESS)
    {
        // TODO: Log
        Logger& l = Logger::Singleton();

        assert(l.kernels.count(kernel) == 1 && "kernel was not logged");
        KernelInfo& ki = l.kernels[kernel];

        ki.dimensions = work_dim;

        // Resize recorded NDRange vectors if necessary
        if (ki.globalWorkOffset.size() != work_dim)
            ki.globalWorkOffset.resize(work_dim);

        if (ki.globalWorkSize.size() != work_dim)
            ki.globalWorkSize.resize(work_dim);

        if (ki.localWorkSize.size() != work_dim)
            ki.localWorkSize.resize(work_dim);

        for (int dim=0; dim < work_dim; ++dim)
        {
            if (global_work_offset != NULL)
            {
                ki.globalWorkOffset[dim] = global_work_offset[dim];
            }
            else
            {
                ki.globalWorkOffset[dim] = 0;
            }

            ki.globalWorkSize[dim] = global_work_size[dim];

            if (local_work_size != NULL)
            {
                ki.localWorkSize[dim] = local_work_size[dim];
            }
            else
            {
                // It is implementation defined how to divide the NDRange
                // in this case. We will make the same size as the global_work_size
                // so that there is a single work group.
                ki.localWorkSize[dim] = global_work_size[dim];
            }
        }
        

        // Log stuff now.
        // We need to do this now because the kernel information
        // might be modified later.
        l.dump(kernel);
    }
    
    return success;
}

}