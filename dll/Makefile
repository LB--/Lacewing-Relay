
LIBS = ws2_32.lib mswsock.lib mpr.lib crypt32.lib secur32.lib advapi32.lib
FLAGS = /D_lacewing_library 

liblacewing.dll: ../liblacewing.a libgcc.a
	CL ../src/cxx/*.cc ../liblacewing.a libgcc.a $(LIBS) /MD /LD /Feliblacewing.dll exports.def

