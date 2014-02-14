Objectify: A COFF generator
========

Objectify is like objcopy for Linux but with less options.
It converts any input file into the [Microsoft Common Object File Format (COFF)](http://msdn.microsoft.com/en-us/windows/hardware/gg463119.aspx) This is useful for linking data files to your C executables. I've found it particularly useful for embedding compiled lua scripts.

###Building
To build Objectify, you will need:

1. A version of Microsoft Visual Studio or MinGW. 
2. [Lake.exe](https://github.com/bncastle/Lake.exe) or Steve Donovan's original [Lake](https://github.com/stevedonovan/Lake) plus an installation of Lua.

If you have Visual Studio, be sure to open up a Visual Studio Tools command line.
If instead you are using MinGW, just make sure the MinGW bin directory is in your path.
To build using Lake.exe, make sure it is in your path, goto the source directory, and type:

	lake
Or if you instead have the original lake, you should type:

	lua lake
If all is installed correctly, you should see objectify.exe in the release directory.

####Using Objectify
To start making COFFs, put the exe in your path somewhere and type:

	objectify <infile.ext> <output file>
Where <infile.ext> is the file you want to convert and <output file> is the name you want for the generated COFF file.

Once the COFF file is created, you will have access to 2 symbols:

	_binary_<infile_ext>_start
	_binary_<infile_ext>_size

To access them from your C file, declare them as externals and link in the COFF file. For example if you converted a file called **test.luc**, you could add the following declarations to your C file:

	extern char binary_test_luc_start;
	extern char binary_test_luc_size;

One important thing to note is that these symbols aren't really variables. Instead, their address is their value. They can be declared as whatever makes sense in the code. For example, to get the binary blob size just do the following:

	int bin_size = (int) &binary_test_luc_size;
A char pointer to the data can be obtained by:

	const char *pdata = (char *) &binary_test_start;