#include <cstdio>
#include <cstdlib>

#include "ExecSolver.h"

using namespace std;

static const char * z3solverCmdline = "C:\\Projects\\z3\\z3\\build\\z3.exe /smt2 ";

std::string runSolver(const std::string& file)
{
    string cmd = z3solverCmdline + file;
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buffer[128];
    std::string result = "";
    while (!feof(pipe))
    {
        if (fgets(buffer, 128, pipe) != NULL)
        {
            result += buffer;
        }
    }
    _pclose(pipe);
    return result;
}
 