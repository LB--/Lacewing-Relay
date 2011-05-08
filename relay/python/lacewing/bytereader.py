# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
Implements a way to easily read and write binary types
"""

from cStringIO import StringIO
import struct

BYTE = struct.Struct('b')
UBYTE = struct.Struct('B')
SHORT = struct.Struct('h')
USHORT = struct.Struct('H')
FLOAT = struct.Struct('f')
INT = struct.Struct('i')
UINT = struct.Struct('I')

class ByteReader(object):
    buffer = None

    def __init__(self, input = None):
        """
        @param input: The input to initially fill the reader with
        @type input: file/str/buffer
        """
        if isinstance(input, file):
            buffer = input
            self.write = buffer.write
        else:
            if input is not None:
                buffer = StringIO(input)
            else:
                buffer = StringIO()
                self.write = buffer.write
            self.data = buffer.getvalue

        self.buffer = buffer
        self.tell = buffer.tell
    
    def data(self):
        currentPosition = self.tell()
        self.seek(0)
        data = self.read()
        self.seek(currentPosition)
        return data
    
    def seek(self, *arg, **kw):
        self.buffer.seek(*arg, **kw)
    
    def read(self, *arg, **kw):
        return self.buffer.read(*arg, **kw)
    
    def size(self):
        currentPosition = self.tell()
        self.seek(0, 2)
        size = self.tell()
        self.seek(currentPosition)
        return size
    
    def __len__(self):
        return self.size()
    
    def __str__(self):
        return self.data()
    
    def __repr__(self):
        return repr(str(self))

    def readByte(self, unsigned = False):
        format = UBYTE if unsigned else BYTE
        value, = self.readStruct(format)
        return value
        
    def readShort(self, unsigned = False):
        format = USHORT if unsigned else SHORT
        value, = self.readStruct(format)
        return value

    def readFloat(self):
        value, = self.readStruct(FLOAT)
        return value
        
    def readInt(self, unsigned = False):
        format = UINT if unsigned else INT
        value, = self.readStruct(format)
        return value
        
    def readString(self):
        currentPosition = self.tell()
        store = ''
        while 1:
            readChar = self.read(1)
            if readChar in ('\x00', ''):
                break
            store = ''.join([store, readChar])
        return store
    
    def readStruct(self, structType):
        return structType.unpack(self.read(structType.size))
        
    def writeByte(self, value, unsigned = False):
        format = UBYTE if unsigned else BYTE
        self.writeStruct(format, value)
        
    def writeShort(self, value, unsigned = False):
        format = USHORT if unsigned else SHORT
        self.writeStruct(format, value)

    def writeFloat(self, value):
        self.writeStruct(FLOAT, value)
        
    def writeInt(self, value, unsigned = False):
        format = UINT if unsigned else INT
        self.writeStruct(format, value)
        
    def writeString(self, value):
        self.write(value + "\x00")
    
    def writeStruct(self, structType, *values):
        self.write(structType.pack(*values))