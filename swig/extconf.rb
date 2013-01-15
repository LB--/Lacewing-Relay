require 'mkmf'

find_library('lacewing', 'lw_version', '../')
create_makefile('lacewing')

