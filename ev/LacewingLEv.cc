
/*
    Copyright (C) 2011 James McLaughlin

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE. 
*/

#include "LacewingLEv.h"

#include <stdio.h>

struct Watcher
{
    ev_io watcher;
    Lacewing::LEvPump * pump;
    void * tag;
};

void Lacewing::LEvPump::callback (struct ev_loop * loop, ev_io * _watcher, int events)
{
    Watcher * watcher = (Watcher *) _watcher;
    
    watcher->pump->Ready (watcher->tag, false,
                    events & EV_READ, events & EV_WRITE);
}

void Lacewing::LEvPump::AddRead (int FD, void * Tag)
{
    Watcher * watcher = new Watcher;
    ev_io_init (&watcher->watcher, callback, FD, EV_READ);
    
    watcher->pump = this;
    watcher->tag = Tag;
    
    ev_io_start (EV_DEFAULT, &watcher->watcher);
}

void Lacewing::LEvPump::AddReadWrite (int FD, void * Tag)
{
    Watcher * watcher = new Watcher;
    ev_io_init (&watcher->watcher, callback, FD, EV_READ | EV_WRITE);
    
    watcher->pump = this;
    watcher->tag = Tag;
    
    ev_io_start (EV_DEFAULT, &watcher->watcher);
}

