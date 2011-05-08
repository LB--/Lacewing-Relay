#!/usr/bin/env python

from distutils.core import setup

setup(
    name = 'pylacewing',
    version = '1.0.1',
    description = 'Python Lacewing Library',
    author = 'Mathias Kaerlev',
    author_email = 'matpow2@gmail.com',
    url = 'http://code.google.com/p/pylacewing/',
    license = 'MIT license',
    packages = [
        'lacewing', 
        'lacewing.packetloaders',
        'lacewing.moo',
        'lacewing.moo.packetloaders'
    ],
    classifiers = [
        'Development Status :: 4 - Beta',
        'Framework :: Twisted',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 2',
        'Topic :: Communications',
        'Topic :: Internet'
    ]
)