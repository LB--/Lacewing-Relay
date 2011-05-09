# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

"""
Perfom flag operations on numbers.
"""

def listFlag(flaglist):
    """
    Takes a list of bools and returns
    a flagbyte.
    """
    flag = 0
    for index, item in enumerate(flaglist):
        flag = setFlag(flag, index, item)
    return flag

def setFlag(flagbyte, pos, status):
    """
    Sets the bit at 'pos' to 'status', and
    returns the modified flagbyte.
    """
    if status:
        return flagbyte | 2**pos
    else:
        return flagbyte & ~2**pos

def getFlag(flagbyte, pos):
    """
    Returns the bit at 'pos' in 'flagbyte'
    """
    mask = 2**pos
    result = flagbyte & mask
    return (result == mask)
    
def getFlags(flagbyte, *positions):
    """
    Returns the bits specified in the arguments
    """
    return [getFlag(flagbyte, pos) for pos in positions]


