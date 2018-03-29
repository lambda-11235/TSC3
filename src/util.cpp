/*******************************************************************************
 * This file is part of TSC.
 *
 * TSC is a 2-dimensional platform game.
 * Copyright © 2018 The TSC Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "util.hpp"
#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

using namespace std;

void TSC::warn(const std::string& msg)
{
    cerr << "Warning: " << msg << endl;
}

/**
 * A function equivalent to C's sprintf(), but returns a C++ std::string
 * instead so that you don't have to think about the memory management.
 * This function calls vsnprintf() for the actual formatting operation, so
 * look in that function's documentation for the format specifiers.
 */
string TSC::format(const string& spec, ...)
{
    va_list ap;
    int len      = spec.size() + 4096; // 4096 are an educated guess for what's likely needed
    char* target = (char*) malloc(len);

    va_start(ap, spec);
    int result = vsnprintf(target, len, spec.c_str(), ap);
    va_end(ap);

    if (result >= len) { // Guess was to small, reallocate with required space
        len    = result + 1; // +1 for terminating NUL
        target = (char*) realloc(target, len);

        va_start(ap, spec);
        result = vsnprintf(target, len, spec.c_str(), ap);
        va_end(ap);

        if (result >= len) { // Should not happen
            free(target);
            throw(runtime_error("format() failed for unknown reasons in vsnprintf()"));
        }
        else if (result < 0) {
            free(target);
            throw(runtime_error("format() failed with an output eror"));
        }
    }
    else if (result < 0) {
        free(target);
        throw(runtime_error("format() failed with an output eror"));
    }

    string retval(target, result);
    free(target);
    return retval;
}
