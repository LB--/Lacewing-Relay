# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
Python Lacewing Library
=======================
New for version 1.0.1:
    1. Fixed some minor bugs

New for version 1.0:
    1. New protocol. Most things are backwards-compatible, but do check the
    documentation to make sure.
    2. Version set to 1.0. I believe pylacewing has proven stable, and this
    version milestone represents this opinion :-)

New for version 0.8.1:
    1. Fixed server-to-client number sending (because the C++ implementation
    doesn't support fixed-size number types yet)

New for version 0.8:
    1. A new table message type has been added, so now, you can send
    dicts (containing only string keys and values).
    2. The server now ignores invalid channel/peer IDs instead of kicking
    clients (this should fix some disconnection problems).
    3. Protocol updated to Beta 3
    4. Version set to 0.8, since pylacewing has been proven to be stable.
    Will be set to 1.0 when the protocol gets to "Release".

New for version 0.2.4:
    1. Fix pinging

New for version 0.2.32:
    1. Various bugfixes

New for version 0.2.31:
    1. Quick bugfix!

New for version 0.2.3:
    1. Protocol was updated. On the server-side, there is a new master
    connection attribute for channels. On the client-side, there is a master
    bool attribute for peers. A autoClose parameter is now available 
    for the joinChannel methods. Also, the server provides a welcome
    message to accepted clients, see the welcomeMessage attribute and
    getWelcomeMessage method of ServerFactory.
    2. The default channel class can now be changed. Have a look at the
    new channelClass attribute in ServerFactory.

New for version 0.2.15:
    1. Flash policy example provided.
    2. Fixed a bug where a ClientEvent packet would be sent to the
    leaving client also

New for version 0.2.1:
    1. Small bugfixes
    2. A ByteReader type has been made. It can be used in the various
    send methods (and it will be used for all incoming binary messages)

New for version 0.2.0:
    1. Refactored: look in the example files to get an idea.

New for version 0.1.986:
    1. Bugfixes

New for version 0.1.985:
    1. Stack features implemented
    2. Moo subpackage included

New for version 0.1.98:
    1. UDP fixes and support for binary transfers

New for version 0.1.97:
    1. New protocol changes that fixes UDP for
    NAT users

New for version 0.1.95:
    1. Bugfixes related to channel parting

New for version 0.1.9:
    1. Channellisting
"""

__author__ = 'Mathias Kaerlev'
__copyright__ = 'Copyright 2011, Mathias Kaerlev'
__credits__ = ['Mathias Kaerlev', 'Jamie McLaughlin', 'Jean Villy Edberg',
    'Lukas Meller']
__version__ = '1.0.1'
__email__ = 'matpow2@gmail.com'