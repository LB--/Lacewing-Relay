# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
Variables and classes that are common for some packetloaders
"""

from struct import Struct
from lacewing.baseloader import _BaseLoader
from lacewing.bytereader import ByteReader

CONNECT, SET_NAME, JOIN_CHANNEL, LEAVE_CHANNEL, CHANNEL_LIST = xrange(5)

INT_DATA = Struct('<I')

DATA_TYPES = [
    'String', 
    'Number', 
    'Binary'
]

def detectType(value):
    if isinstance(value, str):
        return 'String'
    elif isinstance(value, int):
        return 'Number'
    elif isinstance(value, ByteReader):
        return 'Binary'
    else:
        raise Exception('could not detect message type')

class _DataTypeMixin:
    """
    Mixin for doing various binary read/write
    operations on a message
    """
    def setDataType(self, typeName):
        """
        Sets the kind of data this packet contains
        
        @param typeName: See L{DATA_TYPES} for possible values
        """
        self.settings['variant'] = DATA_TYPES.index(typeName)
        
    def getDataType(self):
        """
        Gets the kind of data this packet contains
        
        @return: See L{DATA_TYPES} for possible values
        """
        return DATA_TYPES[self.settings['variant']]

import json

class _ObjectMessageMixin(_DataTypeMixin):
    isObject = True
    def readMessage(self, data):
        self.value = json.loads(data)
        
    def generateMessage(self):
        return json.dumps(self.value)

class _BinaryMessageMixin(_DataTypeMixin):
    isObject = False
    def readMessage(self, data):
        dataType = self.getDataType()
        if dataType == 'Number':
            self.value, = INT_DATA.unpack_from(data)
        elif dataType == 'Binary':
            self.value = ByteReader(data)
        elif dataType == 'String':
            self.value = str(data)
        
    def generateMessage(self):
        dataType = self.getDataType()
        if dataType == 'Number':
            return INT_DATA.pack(self.value)
        elif dataType in ('Binary', 'String'):
            return str(self.value)