
LINK_DYNAMIC = false

--Directory where all the intermediate files go
OBJ_DIR = 'obj'

--Output directory
LOC = 'release/'
if(DEBUG) then LOC = 'debug/' end

--Library directories
LIBDIR = ""

--Include directories
INCDIR = ""

default
{
	c.program{LOC..'objectify',src='*', dynamic = false, odir=OBJ_DIR}
}