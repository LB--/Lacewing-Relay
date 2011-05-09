# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

import struct
from struct import unpack_from, pack

from lacewing.moo.packetloaders.common import _MooLoader

STRING = 7
NUMBER = 8
BINARY = 9
SET_NUMBER = 11
SET_STRING = 12
GET_NUMBER = 13
GET_STRING = 14
NUMBER_LOADED = 15
STRING_LOADED = 16

def detectType(value):
    if isinstance(value, str):
        return STRING
    elif isinstance(value, int):
        return NUMBER
    else:
        raise NotImplementedError('could not detect type for %r' % value)

class Message(_MooLoader):
    type = None
    value = None
    def _load(self, data):
        mooGame = self.settings.get('mooGame', True)
        type = None
        if mooGame:
            try:
                type, = unpack_from('<B', data)
                self.type = type
                if type in (STRING, BINARY, STRING_LOADED):
                    self.value = data[1:]
                elif type in (NUMBER, NUMBER_LOADED):
                    self.value, = unpack_from('<I', data, 1)
                elif type in (SET_NUMBER, SET_STRING, GET_NUMBER, GET_STRING):
                    value = {}
                    self.value = value
                    filenameSize = unpack_from('<H', data, 1)
                    value['Filename'] = str(buffer(data, 3, filenameSize))
                    offset = 3 + filenameSize
                    groupSize = unpack_from('<H', data, offset)
                    offset += 2
                    value['Group'] = str(buffer(data, offset, groupSize))
                    offset += groupSize
                    itemSize = unpack_from('<H', data, offset)
                    offset += 2
                    value['Item'] = str(buffer(data, offset, itemSize))
                    offset += itemSize
                    if type == SET_NUMBER:
                        value['Value'] = unpack_from('<I', data, offset)
                    elif type == SET_STRING:
                        valueSize = unpack_from('<H', data, offset)
                        offset += 2
                        value['Value'] = str(buffer(data, offset, valueSize))
                else:
                    raise NotImplementedError('datatype %s not implemented' % type)
            except struct.error:
                raise NotImplementedError('data (%r) for type %s invalid' % (data, type))

        else:
            self.type = STRING
            self.value = data
            
    def generate(self):
        type = self.type
        mooGame = self.settings.get('mooGame', True)
        if mooGame:
            if type in (STRING, BINARY, STRING_LOADED):
                return pack('<B', type) + self.value
            elif type in (NUMBER, NUMBER_LOADED):
                return pack('<BI', type, self.value)
            elif type in (SET_NUMBER, SET_STRING, GET_NUMBER, GET_STRING):
                value = self.value
                filename = value['Filename']
                group = value['Group']
                item = value['Item']
                
                data = ''.join([
                    pack('<H', len(filename)), filename,
                    pack('<H', len(group)), group,
                    pack('<H', len(item)), item
                ])
                
                if type == SET_NUMBER:
                    return data + pack('<I', value['Value'])
                elif type == SET_STRING:
                    valueString = value['Value']
                    return data + pack('<H', len(valueString)) + valueString
                return data
            else:
                raise NotImplementedError('datatype %s not implemented' % type)
        else:
            return self.value

__all__ = ['Message', 'STRING', 'NUMBER', 'BINARY', 'SET_NUMBER', 'SET_STRING',
    'GET_NUMBER', 'GET_STRING', 'NUMBER_LOADED', 'STRING_LOADED', 'detectType']