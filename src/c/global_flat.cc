
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

const char * lw_version ()
    { return Lacewing::Version();
    }
lw_bool lw_start_thread (void * function, void * param)
    { return Lacewing::StartThread(function, param);
    }
void lw_set_current_thread_name (const char * name)
    { Lacewing::SetCurrentThreadName(name);
    }
void lw_pause (long milliseconds)
    { Lacewing::Pause(milliseconds);
    }
void lw_yield ()
    { Lacewing::Yield();
    }
lw_i64 lw_current_thread_id ()
    { return Lacewing::CurrentThreadID();
    }
lw_i64 lw_file_last_modified (const char * filename)
    { return Lacewing::LastModified(filename);
    }
lw_bool lw_file_exists (const char * filename)
    { return Lacewing::FileExists(filename);
    }
lw_i64 lw_file_size (const char * filename)
    { return Lacewing::FileSize(filename);
    }
void lw_int64_to_string (lw_i64 int64, char * buffer)
    { Lacewing::Int64ToString(int64, buffer);
    }
void lw_temp_path (char * buffer, long length)
    { Lacewing::TempPath(buffer, length);
    }
void lw_new_temp_file (char * buffer, long length)
    { Lacewing::NewTempFile(buffer, length);
    }
long lw_count_processors ()
    { return Lacewing::CountProcessors();
    }

