# Copyright (c) 2011 Mathias Kaerlev.
# See LICENSE for details.

class _BaseLoader(object):
    """
    The base class for all packetloaders.
    """

    def __init__(self, data = None, **settings):
        """
        @type data: str or buffer
        @param settings: Settings set for this packetloader
        """
        self.settings = settings
        self.initialize()
        if data is not None:
            self.read(data)
            
    def new(self, loaderClass, *arg, **kw):
        """
        Creates a new loader from loaderClass and passes the
        current settings to it
        
        @type loaderClass: _BaseLoader
        """
        kw.update(self.settings)
        kw['parent'] = self
        newLoader = loaderClass(*arg, **kw)
        return newLoader
    
    def initialize(self):
        """
        Initializer for this loader
        """

    def read(self, data):
        """
        Parses 'data' and sets the properties of the
        packetloader.
        @type data: str or buffer
        """
        raise NotImplementedError('read() method not implemented (%s)' % self.__class__)
        
    def generate(self):
        """
        Generate the raw representation of the packetloader.
        """
        raise NotImplementedError('generate() method not implemented')

class _EmptyLoader(_BaseLoader):
    def read(self, data):
        pass

    def generate(self):
        return ''