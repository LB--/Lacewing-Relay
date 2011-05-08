
/*
    Copyright (C) 2011 James McLaughlin

    This file is part of Lacewing.

    Lacewing is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lacewing is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Lacewing.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "../Common.h"

lw_error* lw_error_new ()
    { return (lw_error *) new Lacewing::Error();
    }
void lw_error_delete (lw_error * error)
    { delete ((Lacewing::Error *) error); 
    }
void lw_error_add (lw_error * error, long code)
    { ((Lacewing::Error *) error)->Add(code);
    }
const char* lw_error_tostring (lw_error * error)
    { return ((Lacewing::Error *) error)->ToString();
    }
lw_error* lw_error_clone (lw_error * error)
    {  return (lw_error *) ((Lacewing::Error *) error)->Clone();
    }

void lw_error_addf (lw_error * error, const char * format, ...)
{
    va_list args;
    va_start (args, format);

    ((Lacewing::Error *) error)->Add(format, args);    

    va_end (args);
}

