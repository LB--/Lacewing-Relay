# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
The container for packetloaders, which represents
a whole packet
"""

from struct import Struct
import struct

from lacewing.packetloaders import client, server
from lacewing.baseloader import _BaseLoader
from lacewing.exceptions import OutOfData

SC_PACKET_TABLE = [
    # server to client types
    server.Response,
    server.BinaryServerMessage,
    server.BinaryChannelMessage,
    server.BinaryPeerMessage,
    server.ObjectServerMessage,
    server.ObjectChannelMessage,
    server.ObjectPeerMessage,
    server.Peer,
    server.UDPWelcome,
    server.Ping
]

CS_PACKET_TABLE = [
    # client to server types
    client.Request,
    client.BinaryServerMessage,
    client.BinaryChannelMessage,
    client.BinaryPeerMessage,
    client.ObjectServerMessage,
    client.ObjectChannelMessage,
    client.ObjectPeerMessage,
    client.UDPHello,
    client.ChannelMaster,
    client.Pong
]

for items in (CS_PACKET_TABLE, SC_PACKET_TABLE):
    for index, item in enumerate(items):
        item.id = index

HEADER_DATA = Struct('<BB')
FROM_ID = Struct('<H')
HEADER_DATA_UDP = Struct('<B')
SHORT_SIZE = Struct('<H')
INT_SIZE = Struct('<I')

class _Packet(_BaseLoader):
    """
    The base packet.

    @ivar loader: The loader that this packet contains
    @ivar maxSize: The max size of a packet.
    @ivar settings: Settings for this packet.
    """
    loader = None
    fromId = None
    
    def read(self, data):
        """
        Load the specified data with the given settings.
        @type data: buffer or str
        """
        datagram = self.settings.get('datagram', False)
        packetSize = 0
        if datagram:
            headerByte, = HEADER_DATA_UDP.unpack_from(data)
            if self.isClient:
                self.fromId, = FROM_ID.unpack_from(data, 1)
                packetData = buffer(data, 3)
            else:
                packetData = buffer(data, 1)
        else:
            if len(data) < 2:
                raise OutOfData
            headerByte, size = HEADER_DATA.unpack_from(data)
            packetSize += HEADER_DATA.size

        messageType = (headerByte & 0b11110000) >> 4
        variant = headerByte & 0b00001111
        
        if not datagram:
            if size == 254:
                size, = SHORT_SIZE.unpack_from(data, 2)
                packetData = buffer(data, 4, size)
                packetSize += 2 + size
            elif size == 255:
                size, = INT_SIZE.unpack_from(data, 2)
                packetData = buffer(data, 6, size)
                packetSize += 4 + size
            else:
                packetData = buffer(data, 2, size)
                packetSize += size
            if len(packetData) < size:
                raise OutOfData
        loaderClass = self.packetTable[messageType]
        self.loader = self.new(loaderClass, packetData, variant = variant)
        return packetSize

    def generate(self):
        """
        Generates the binary representation of the packet.
        """
        datagram = self.settings.get('datagram', False)
        typeData = self.loader.generate()
        variant = self.loader.settings.get('variant', 0)
        headerByte = variant | (self.loader.id << 4)
        if datagram:
            if self.isClient:
                fromData = FROM_ID.pack(self.fromId)
            else:
                fromData = ''
            return HEADER_DATA_UDP.pack(headerByte) + fromData + typeData
        else:
            size = len(typeData)
            sizeData = ''
            if 65535 > size >= 254:
                sizeData = SHORT_SIZE.pack(size)
                size = 254
            elif 4294967295 > size >= 65535:
                sizeData = INT_SIZE.pack(size)
                size = 255
            return HEADER_DATA.pack(headerByte, size) + sizeData + typeData

class ClientPacket(_Packet):
    packetTable = CS_PACKET_TABLE
    isClient = True

class ServerPacket(_Packet):
    packetTable = SC_PACKET_TABLE
    isClient = False
