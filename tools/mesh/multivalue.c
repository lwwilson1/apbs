/*
* Name:	multivalue.c
* Author(s):	Adrian Kaats
*		Effectively, Nathan Baker as well (the majority of this program is directly copied from
*		value.c distributed in apbs/tools/mesh with the source version of apbs-0.4.0 - please see
* 		this function for reference and more importantly for any licensing issues).
* Date:	November 29, 2006
*
* Description:	Accepts a CSV (with no column titles) file containing 3D coordinates for which the
*			value from a DX-formatted grid is evaluated.  The original coordinates and their its
* 			values are output to a text file formatted exactly like the input but with one extra
*			column containing the calculated value at the point.  This program will also either
*			append an existing io.mc, or create it if it doesn't exist in the directory from which
*			it is called.
*
* Usage:		> multivalue csvCoordinatesFile dxFormattedFile outputFile
*
*			Example of input file contents:	123.234,23.8E03,9.6e-4
*								5.9,6.2,0.3
*								-7e3,91,0.6
*
*				NOTE	-No white space allowed anywhere on a line (ex. no space after commas)
*					-No blank lines allowed at the top of the file (blank lines OK at the end)
*					-No column headers/titles
*
* Programming notes:
* 	1)	This program uses the convention that exiting with a 0 represents success (even in the case
*		where the program can't do anything: example --> user input coordinate file contains no data).
*		The program exits with a 1 upon failure (ex. any kind of I/O error, or error code generated by
*		external routines such as Vgrid routines).
* 	2)	Vgrid routines use convention that returning 1 is success.  Also note that Vgrid routines,
* 		I believe, use VASSERT which can terminate the entire program with an abort() signal which is
*		not handled here.
* 	3)	The original 'value.c' routine written by Nathan Baker uses the convention that returning 0
*		represents success - that convention is carried on here...
* 	4)	To compile this program, you will need to have apbscfg.h and vclist.h in an appropriate include
* 		directory - these do not seem to be created by RPM installations of APBS or its tools.
*
*	MANY THANKS to David Gohara without whom I (Adrian) would have spent untold ages trying to compile this
*/

#include "apbs.h"

void multivalue_usage(void) {
    Vnm_print(1,"\n");
    Vnm_print(1,"Usage: > multivalue csvCoordinatesFile dxFormattedFile outputFile"
                "[outputformat]\n\n");
    Vnm_print(1,"csvCoordinatesFile is the input CSV file containing 3D coordinates\n");
    Vnm_print(1,"dxFormattedFile is the input DX grid on which coords are evaluated\n\n");
    Vnm_print(1,"The optional argument outputformat specifies output OpenDX type.\n");
    Vnm_print(1,"Acceptable values include\n\
       dx:  standard OpenDX format\n\
       dxbin:  binary OpenDX format\n");
    Vnm_print(1,"If this arg is not specified, the output is standard OpenDX.\n\n");
    Vnm_print(1,"Example input csvCoordinatesfile:\n\n\t");
    Vnm_print(1,"123.234,23.8E03,9.6e-4\n\t");
    Vnm_print(1,"5.9,6.2,0.3\n\t");
    Vnm_print(1,"-7e3,91,0.6\n\n\t");
    Vnm_print(1,"NOTE\t-No white space allowed anywhere on\n\t\t ");
    Vnm_print(1,"a line (ex. no space after commas)\n\t\t");
    Vnm_print(1,"-No blank lines allowed at the top of\n\t\t ");
    Vnm_print(1,"the file (blank lines OK at the end)\n\t\t");
    Vnm_print(1,"-No column headers/titles\n");
}

int main(int argc, char *argv[]) {
    char *inputFileName, *dxFileName, *outputFileName;
    int scanNum = 0;
    double pt[3], val;
    FILE *inputFileStream, *outputFileStream;
    Vdata_Format format;
    Vgrid *grid;

    /* *************** CHECK INVOCATION ******************* */
    Vio_start();
    Vnm_redirect(1);
    Vnm_print(1, "\n");
    if (argc != 4 && argc != 5) {
        Vnm_print(2,"Invalid number of arguments, # of arguments received: %d\n",argc);
        multivalue_usage();
        exit(1);
    }

    inputFileName = argv[1];
    dxFileName = argv[2];
    outputFileName = argv[3];

    if (argc == 5) {
        if (Vstring_strcasecmp(argv[4], "dx")) {
            format = VDF_DX;
        } else if (Vstring_strcasecmp(argv[4], "dxbin")) {
            format = VDF_DXBIN;
        } else {
            printf("\n*** Argument error: format must be 'dx' or 'dxbin'.\n\n");
            exit(1);
        }
    } else {
        format = VDF_DX;
    }

    Vnm_print(1,"Input file:\t%s\ndx file:\t%s\nOutput file:\t%s\n", inputFileName, dxFileName, outputFileName);

    /* *************** READ DATA ******************* */
    Vnm_print(1, "main:  Reading data from %s...\n", dxFileName);
    grid = Vgrid_ctor(0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, VNULL);
    if (format == VDF_DX) {
        if (!Vgrid_readDX(grid, "FILE", "ASC", VNULL, dxFileName)) {
            Vnm_print(2, "main:  Problem reading standard OpenDX-format grid from %s\n",
              dxFileName);
            exit(1);
        }
    } else if (format == VDF_DXBIN) {
        if (!Vgrid_readDXBIN(grid, "FILE", "ASC", VNULL, dxFileName)) {
            Vnm_print(2, "main:  Problem reading binary OpenDX-format grid from %s\n",
              dxFileName);
            exit(1);
        }
    } else {
        Vnm_print(2, "main:  Format not properly specified. \n");
        exit(1);
    }

    /*
    try opening input file - upon error, exit with error flag
    */
    if((inputFileStream = fopen(inputFileName,"r")) == NULL){
        Vnm_print(2,"Error opening input file: %s\n",inputFileName);
        exit(1);
    }

    /*
    try opening output file - upon error, try closing input file and exit with error flag
    */
    if((outputFileStream = fopen(outputFileName,"w")) == NULL){
        Vnm_print(2,"Error opening output file: %s\n",outputFileName);
        /*
        since an error occured creating an output file, at least try closing input
        file before exiting
        */
        if(fclose(inputFileStream) == EOF){
            Vnm_print(2,"Error closing input file: %s",inputFileName);
        }
        exit(1);
    }

    /*
    read lines of input file placing comma separated values into x, y and z
    NOTE does not check for valid data, stops on EOF returned by scanf which
    can happen if input file lines don't match format string or upon an actual EOF
    */
    scanNum = fscanf(inputFileStream,"%lg%*c%lg%*c%lg",&pt[0],&pt[1],&pt[2]);
    /*
    Exit without error if first scan of input file results in EOF
    */
    if(scanNum == EOF){
        Vnm_print(1,"Invalid data in the input file: %s\n",inputFileName);
        multivalue_usage();
        exit(0);
    }
    else{
        Vnm_print(1,"Getting values...\n");
        Vnm_print(1,"Displayed and written to output file as x,y,z,value\n");
    }
    while(scanNum != EOF){
        /*
        perform Vgrid_value --> 1) check if point is within mesh bounds, if not set value to 0 and return;
        2) if point is actually on a mesh point, give mesh pt value; 3) otherwise use trilinear interpolation
        to get value
        */
        if(Vgrid_value(grid, pt, &val)){
            Vnm_print(1,"%e,%e,%e,%e\n",pt[0],pt[1],pt[2],val);
            /*
            write line of output file (maybe should implement error checking on fprintf,
            but that's seems like overkill)
            */
            fprintf(outputFileStream,"%e,%e,%e,%e\n",pt[0],pt[1],pt[2],val);
        }
        else{
            Vnm_print(1,"%e,%e,%e,%s\n",pt[0],pt[1],pt[2],"NaN");
            /*
            write line of output file with string 'NaN' for value - this is done to avoid
            the case where certain machines may not implement NaN or do so in a weird way?
            */
            fprintf(outputFileStream,"%e,%e,%e,%s\n",pt[0],pt[1],pt[2],"NaN");
        }

        /*
        scan in next line of input file
        */
        scanNum = fscanf(inputFileStream,"%lg%*c%lg%*c%lg",&pt[0],&pt[1],&pt[2]);
    }

    /*
    close input file
    */
    if(fclose(inputFileStream) == EOF){
        Vnm_print(2,"Error closing input file: %s\n",inputFileName);
        exit(1);
    }

    /*
    close output file
    */
    if(fclose(outputFileStream) == EOF){
        Vnm_print(2,"Error closing output file: %s\n",outputFileName);
        exit(1);
    }

    exit(0);
}

