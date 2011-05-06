
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

#ifndef LacewingTimeHelper
#define LacewingTimeHelper

#include <time.h>

const char * const Weekdays [] =
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

const char * const Months [] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

inline int LookupDayOfWeek(const char * String)
{
    for(int i = 0; i < 7; ++i)
        if(!stricmp(Weekdays[i], String))
            return i;

    return -1;
}

inline int LookupMonth(const char * String)
{
    for(int i = 0; i < 12; ++i)
        if(!stricmp(Months[i], String))
            return i;

    return -1;
}

inline void ParseTime(char * Time, tm &TimeStructure)
{
    if(strlen(Time) < 8)
        return;

    Time[2] = 0;
    Time[5] = 0;

    TimeStructure.tm_hour = atoi(Time);
    TimeStructure.tm_min  = atoi(Time + 3);
    TimeStructure.tm_sec  = atoi(Time + 6);
}

inline time_t ParseTimeString(const char * StringC)
{
    char String[32];
    strcpy(String, StringC);

    int Length = strlen(String);

    if(Length < 8)
        return 0;

    char * Month, * Time;
    int Day, Year;

    if(String[3] == ',')
    {
        /* RFC 822/RFC 1123 - Sun, 06 Nov 1994 08:49:37 GMT */

        if(Length < 29)
            return 0;

        String[ 7] = 0;
        String[11] = 0;
        String[16] = 0;
        
        Day = atoi(String + 4);
        Month = String + 8;
        Year = atoi(String + 12);
        Time = String + 17;

    }
    else if(String[3] == ' ' || String[3] == '	')
    {
        /* ANSI C's asctime() format - Sun Nov  6 08:49:37 1994 */

        if(Length < 24)
            return 0;

        String[ 7] = 0;
        String[10] = 0;
        String[19] = 0;

        Month = String + 4;
        Day = atoi(String + 8);
        Time = String + 11;
        Year = atoi(String + 20);
    }
    else
    {
        /* RFC 850 date (Sunday, 06-Nov-94 08:49:37 GMT) - unsupported */
        
        return 0;
    }

    tm TM;

    TM.tm_mday  = Day;
    TM.tm_wday  = 0;
    TM.tm_year  = Year - 1900;
    TM.tm_isdst = 0;
    TM.tm_mon   = LookupMonth(Month);
    TM.tm_yday  = 0;

    ParseTime(Time, TM);

    #ifdef LacewingWindows
        return _mkgmtime(&TM);
    #else
        #if HAVE_TIMEGM
            return timegm(&TM);
        #else
            #pragma error "Can't find a suitable way to convert a tm to a UTC UNIX time"
        #endif
    #endif
}

#endif 

