#include "gvki/Logger.h"
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <errno.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "gvki/Debug.h"


// For mkdir(). FIXME: Make windows compatible
#include <sys/stat.h>
#define FILE_SEP "/"

using namespace std;
using namespace gvki;

// Maximum number of files or directories
// that can be created
static const int maxFiles = 10000;

Logger& Logger::Singleton()
{
    // FIXME: Read directory from env
    static Logger l("gvki");
    return l;
}

Logger::Logger(std::string directory)
{
    int count = 0;
    bool success= false;

    // Keep trying a directory name with a number as suffix
    // until we find an available one or we exhaust the maximum
    // allowed number
    while (count < maxFiles)
    {
        stringstream ss;
        ss <<  directory << "-" << count;
        ++count;

        // Make the directory
        if (mkdir(ss.str().c_str(), 0770) != 0)
        {
            if (errno != EEXIST)
            {
                perror("Failed to setup directory");
                ERROR_MSG("Directory was :" << directory);
                exit(1);
            }

            // It already exists, try again
            continue;
        }

        this->directory = ss.str();
        success = true;
        break;
    }

    if (!success)
    {
        ERROR_MSG("Exhausted available directory names or couldn't create any");
        exit(1);
    }

    openLog();
}


void Logger::openLog()
{
    // FIXME: We should use mkstemp() or something
    std::stringstream ss;
    ss << directory << FILE_SEP << "log.json";
    output = new std::ofstream(ss.str().c_str(), std::ofstream::out | std::ofstream::ate);

    if (! output->good())
    {
        ERROR_MSG("Failed to create file (" << ss.str() <<  ") to write log to");
        exit(1);
    }

    // Start of JSON array
    *output << "[" << std::endl;
}

void Logger::closeLog()
{
    // End of JSON array
    *output << std::endl << "]";
    output->close();
}

Logger::~Logger()
{
    closeLog();
    delete output;
}

void Logger::dump(cl_kernel k)
{
    // Output JSON format defined by
    // http://multicore.doc.ic.ac.uk/tools/GPUVerify/docs/json_format.html
    KernelInfo& ki = kernels[k];

    static bool isFirst = true;

    if (!isFirst)
    {
        // Emit array element seperator
        // to seperate from previous dump
        *output << "," << endl;
        isFirst = false;
    }

    *output << "{" << endl << "\"language\": \"OpenCL\"," << endl;

    std::string kernelSourceFile = dumpKernelSource(ki);

    *output << "\"kernel_file\": \"" << kernelSourceFile << "\"," << endl;

    // Not officially supported but let's emit it anyway
    *output << "\"global_offset\": ";
    printJSONArray(ki.globalWorkOffset);
    *output << "," << endl;

    *output << "\"global_size\": ";
    printJSONArray(ki.globalWorkSize);
    *output << "," << endl;

    *output << "\"local_size\": ";
    printJSONArray(ki.localWorkSize);
    *output << "," << endl;

    assert(ki.globalWorkOffset.size() == ki.globalWorkSize.size() == ki.localWorkSize.size() &&
            "dimension mismatch");

    *output << "\"entry_point\": \"" << ki.entryPointName << "\"";

    // entry_point might be the last entry is there were no kernel args
    if (ki.arguments.size() == 0)
        *output << endl;
    else
    {
        *output << "," << endl << "\"kernel_arguments\": [ " << endl;
        for (int argIndex=0; argIndex < ki.arguments.size() ; ++argIndex)
        {
            printJSONKernelArgumentInfo(ki.arguments[argIndex]);
            if (argIndex != (ki.arguments.size() -1))
                *output << "," << endl;
        }
        *output << endl << "]" << endl;
    }


    *output << "}";
}

void Logger::printJSONArray(std::vector<size_t>& array)
{
    *output << "[ ";
    for (int index=0; index < array.size(); ++index)
    {
        *output << array[index];

        if (index != (array.size() -1))
            *output << ", ";
    }
    *output << "]";
}

void Logger::printJSONKernelArgumentInfo(ArgInfo& ai)
{
    *output << "{";
    if (ai.argValue == NULL)
    {
        // NULL was passed to clSetKernelArg()
        // That implies its for unallocated memory
        *output << "\"type\": \"array\",";

        // If the arg is for local memory
        if (ai.argSize != sizeof(cl_mem) && ai.argSize != sizeof(cl_sampler))
        {
            // We assume this means this arguments is for local memory
            // where size actually means the sizeof the underlying buffer
            // rather than the size of the type.
            *output << "\"size\" : " << ai.argSize;
        }
        *output << "}";
        return;
    }

    // FIXME:
    // Eurgh... the spec says
    // ```
    // If the argument is a buffer object, the arg_value pointer can be NULL or
    // point to a NULL value in which case a NULL value will be used as the
    // value for the argument declared as a pointer to __global or __constant
    // memory in the kernel.  ```
    //
    // This makes it impossible (seeing as we don't know which address space
    // the arguments are in) to work out the argument type once derefencing the
    // pointer and finding it's equal zero because it could be a scalar constant
    // (of value 0) or it could be an unintialised array!

    DEBUG_MSG("Note sizeof(cl_mem) == " << sizeof(cl_mem));
    // Hack:
    // It's hard to determine what type the argument is.
    // We can't dereference the void* to check if
    // it points to cl_mem we previously stored
    //
    // In some implementations cl_mem will be a pointer
    // which poses a risk if a scalar parameter of the same size
    // as the pointer type.
    if (ai.argSize == sizeof(cl_mem))
    {
       cl_mem mightBecl_mem = *((cl_mem*) ai.argValue);

       // We might be reading invalid data now
       if (buffers.count(mightBecl_mem) == 1)
       {
           // We're going to assume it's cl_mem that we saw before
           *output << "\"type\": \"array\",";
           BufferInfo& bi = buffers[mightBecl_mem];

           *output << "\"size\": " << bi.size << "}";
           return;
       }

    }

    // FIXME: Check for cl_sampler
    //

    // I guess it's scalar???
    *output << "\"type\": \"scalar\",";
    *output << " \"value\": \"0x";
    // Print the value as hex
    uint8_t* asByte = (uint8_t*) ai.argValue;
    for (int byteIndex=0; byteIndex < ai.argSize; ++byteIndex)
    {
       *output << setw(2) << setfill('0') << std::hex << asByte[byteIndex];
    }
    *output << "\"" << std::dec; //std::hex is sticky so switch back to decimal

    *output << "}";
    return;

}

static bool file_exists(std::string& name) {
	ifstream f(name.c_str());
	bool result = f.good();
	f.close();
	return result;
}

std::string Logger::dumpKernelSource(KernelInfo& ki)
{
    int count = 0;
    bool success = false;
    // FIXME: I really want a std::unique_ptr
    std::ofstream* kos = 0;
    std::string theKernelPath;
    while (count < maxFiles)
    {
        stringstream ss;
        ss << ki.entryPointName << "." << count << ".cl";

        ++count;

        std::string withDir = (directory + FILE_SEP) + ss.str();
        if (!file_exists(withDir))
        {
           kos = new std::ofstream(withDir.c_str());

           if (!kos->good())
           {
                kos->close();
                delete kos;
                continue;
           }

           success = true;
           theKernelPath = ss.str();
           break;
        }
    }

    if (!success)
    {
        ERROR_MSG("Failed to log kernel output");
        return std::string("FIXME");
    }

    // Write kernel source
    ProgramInfo& pi = programs[ki.program];

    for (vector<string>::const_iterator b = pi.sources.begin(), e = pi.sources.end(); b != e; ++b)
    {
        *kos << *b;
    }

    // Urgh this is bad, need RAII!
    kos->close();
    delete kos;
    return theKernelPath;
}