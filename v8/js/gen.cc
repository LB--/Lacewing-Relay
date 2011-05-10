
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

#define INPUT "liblacewing.js"
#define OUTPUT "liblacewing.js.inc"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void out (const char * str, FILE * file)
{
    fwrite(str, 1, strlen(str), file);
}

int main (int argc, char * argv[])
{
    FILE * input = fopen(INPUT, "rb");
    FILE * output = fopen(OUTPUT, "w+b");
    
    fseek(input, 0, SEEK_END);
    long input_size = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    char * input_buf = (char *) malloc(input_size);
    fread(input_buf, 1, input_size, input);
    
    fclose(input);
    
    out("unsigned char LacewingJS [] = {", output);
   
    long i = 0;
    int first = 1;
    
    while(i < input_size)
    {
        char buf [16];
        sprintf(buf, first ? "%d" : ",%d", input_buf[i ++]);
        out(buf, output);
        
        first = 0;
    }
    
    out("};", output);
    
    fclose(output);
    
    printf("Wrote %d bytes to %s\n", input_size, OUTPUT);
    
    return 0;
}


