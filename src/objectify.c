#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Objectify, a tiny COFF file generator (like objcopy for Linux, but not as full-featured)
// This converts any file to a COFF format so you can link it to a binary file and use it from your c code
//
// Once the COFF file is created, you will have access to 2 symbols:
// _binary_<inputfilename_ext>_start
// _binary_<inputfilename_ext>_size
//
// To access them from your C file, simply declare them. Ex:
// extern char binary_test_luc_start[];
// extern char binary_test_luc_size[];
//
// After you get them in, you can cast them to whatever type you want.
//
//Author: Bryan Castleberry
//Date: Feb 2014
//Copyright 2014 Pixelbyte Studios LLC
//

#define SYMBOL_PREFIX	"_binary_"

typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned char  uchar;

//COFF Header [20 Bytes]
//
#define COFF_HEADER_SIZE  20
typedef struct
{
	ushort machine_type;
	ushort num_sections;
	uint time_stamp;
	uint symbol_table_pointer;
	uint num_symbols;
	ushort optional_header_size;
	ushort characteristics;
} COFFHeader;

//Section Header [40 bytes]
//
#define SECTION_HEADER_SIZE	40
typedef struct
{
	char name[8];
	uint virtual_size;
	uint virtual_address;
	uint raw_data_size;
	uint raw_data_pointer;
	uint relocations_pointer;
	uint line_numbers_pointer;
	ushort num_relocations;
	ushort num_line_numbers;
	uint characteristics;
} SectionHeader;

long file_size(FILE *fp)
{
	long current = ftell(fp);
	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);
	fseek(fp,current, SEEK_SET);
	return size;
}

void write_short(FILE *fw, ushort val)
{
	fwrite(&val, sizeof(ushort), 1, fw);
}

int main(int argc, char **argv)
{
	int i, offset,string_table_size;
	char z[8];
	FILE *fin, *fout;
	char *fname;
	char *outname;
	char symbols[2][80];
	int symbol_val[2] = {0, 0};
	int symbol_sec[2] = {1, 0xFFFF};

	if(argc < 3)
	{
		fprintf(stdout, "\n== Objectify (Copyright 2014 Pixelbyte Studios) ==\n");
		fprintf(stdout, "Usage: objectify.exe <input file> <output name>\n");
		fprintf(stdout, "Purpose: Produces a COFF file that can be linked to.\n");
		fprintf(stdout, "================================================\n");

		return EXIT_SUCCESS;
	}

	fname = argv[1];
	outname = argv[2];

	memset(z, 0, 8);

	//Pull in the data from the file to be objectified
	fin = fopen(fname, "rb");

	if(fin == NULL)
	{
		fprintf(stderr, "Error: %s does not exist!\n", fname);
		return EXIT_FAILURE;
	}

	int obj_size = file_size(fin);                //Get the size of the file for later
	uchar *data = (uchar *) malloc(obj_size);
	fread(data, 1, obj_size, fin);
	fclose(fin);
	///////////////////////////////////////////////////

	//Create our COFF header
	//Any settings not set are either supposed to be 0 or will be set later
	COFFHeader coff_header;
	memset(&coff_header, 0, sizeof(COFFHeader));  // Zero out the structure
	coff_header.machine_type = 0x14c; //Our machine type is I386
	coff_header.num_sections = 1;                 // We only have 1 section
	//Symbol table location = coff header size + all section headers + section header data size
	//note: for now i only have 1 section so my calcs are a bit simpler
	coff_header.symbol_table_pointer = COFF_HEADER_SIZE + SECTION_HEADER_SIZE + obj_size;
	coff_header.num_symbols = 2;                  // hardcoded to 2
	coff_header.characteristics = 0x105;          // 32-bit machine, line #s stripped, relocs stripped

	//Setup our one and only section
	//Any settings not set are either supposed to be 0 or will be set later
	SectionHeader sec_header;
	memset(&sec_header, 0, sizeof(SectionHeader)); //Zero out the structure
	memset(&sec_header.name, 0, 8);
	strcpy(sec_header.name,".data");               //Our section name will always be ".data"
	sec_header.raw_data_size =  obj_size;

	//Since we only have 1 section, the raw data pointer will simply be the coff header + section header size
	sec_header.raw_data_pointer = COFF_HEADER_SIZE + SECTION_HEADER_SIZE;
	sec_header.characteristics = 0xC0100040;        //Mem read/write, initialized data, 1byte align

	fout = fopen(outname, "wb");
	fwrite(&coff_header, 1, sizeof(COFFHeader), fout);
	fwrite(&sec_header, 1, sizeof(SectionHeader), fout);
	fwrite(data, 1, obj_size, fout);
	free(data); //Free up the data memory

	//Make the basename for our symbols
	i = 0;
	while(fname[i] != '\0') 
	{
		if(fname[i] == '.') fname[i] = '_';
		i++;
	}

	//Write the symbol tables for each of our 3 symbols
	strcpy(symbols[0], SYMBOL_PREFIX);
	strcat(symbols[0], fname);
	strcat(symbols[0], "_start");

	// strcpy(symbols[1], SYMBOL_PREFIX);
	// strcat(symbols[1], fname);
	// strcat(symbols[1], "_end");
	//symbol_val[1] = obj_size;

	strcpy(symbols[1], SYMBOL_PREFIX);
	strcat(symbols[1], fname);
	strcat(symbols[1], "_size");
	symbol_val[1] = obj_size;


	//We'll keep track of the offset into the COFF string table as we'll need it
	//Start it at 4 since it includes the 4 bytes for the string table length
	offset = 4;
	//we need the size of all strings combined (include the 4 byte offset location)
	string_table_size = 4;
	for(i = 0; i < 2; i++)
	{
		string_table_size += strlen(symbols[i]) + 1;

		//If the symbol name is <= 8, then we put it in, otherwise
		//we put its offset into the string table
		if(strlen(symbols[i]) <= 8)
		{
			fwrite(symbols[i], 1, strlen(symbols[i]), fout);
			if(8 - strlen(symbols[i]) > 0) 
				fwrite(z, 1, 8 - strlen(symbols[i]), fout);
		}
		else
		{
			fwrite(z, 1, 4, fout);  //first 4 bytes are 0
			fwrite(&offset, sizeof(int), 1, fout);
		}

		fwrite(&symbol_val[i], sizeof(int), 1, fout);  //Symbol value
		write_short(fout, symbol_sec[i]); 
		write_short(fout, 0); //Type
		fputc(2, fout); //Storage class (2 for us)
		fputc(0, fout); //#of aux symbols (0 for us)

		offset += strlen(symbols[i]) + 1;  //Offset by the string size + 1
	}
	
	//Now write the COFF string table
	fwrite(&string_table_size, sizeof(int), 1, fout);
	for(i = 0; i < 3; i++)
	{
		fwrite(symbols[i],1, strlen(symbols[i]), fout);
		fputc(0, fout); //0 terminate the symbols
	}

	fclose(fout);
	return EXIT_SUCCESS;
}