/*
 *  tgftools_config.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 4/15/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */


#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "spoccExeUtilities.h"

#ifndef PKG_PREFIX
#define PKG_PREFIX ""
#endif

#ifndef PKG_LDFLAGS 
#define PKG_LDFLAGS ""
#endif

#ifndef PKG_CFLAGS 
#define PKG_CFLAGS ""
#endif

#ifndef DRSPGEN_INSTALL_DIR 
#define DRSPGEN_INSTALL_DIR ""
#endif

#ifndef DGBM_DRMDB_INSTALL_PATH 
#define DGBM_DRMDB_INSTALL_PATH ""
#endif

#ifndef TTEARCH
#define TTEARCH getenv("GBMDB")
#endif

#ifndef TGFDB 
#define TGFDB getenv("TGFDB")
#endif

void printUsage() {
	
	cout << "Usage: " << endl << "tgftools_config ";
	cout << "--prefix --ldflags --cflags --datadb --tgfdb --rspgen --drmdb --ttetemp --pha1temp --pha2temp" << endl;
	cout << endl;
};

const char progname[] = "tgftools_config";

int main (int argc, char * const argv[]) { 

	if (argc == 1) {
		printUsage();
		return 0;
	}

	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--help", 0) != NULL ) {
		printUsage();
		return 0;
	}

	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--prefix", 0) != NULL ) {
		cout << PKG_PREFIX << endl;
		return 0;
	}
	
	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--ldflags", 0) != NULL ) {
		cout << PKG_LDFLAGS << endl;
		return 0;
	}
	
	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--cflags", 0) != NULL ) {
		cout << PKG_CFLAGS << endl;
		return 0;
	}
	
	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--rspgen", 0) != NULL ) {
		cout << RSPGEN_INSTALL_DIR << endl;
		return 0;
	}
	
	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--drmdb", 0) != NULL ) {
		cout << GBM_DRMDB_INSTALL_PATH << endl;
		return 0;
	}

	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--tgfdb", 0) != NULL ) {
		cout << TGFDB << endl;
		return 0;
	}
	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--datadb", 0) != NULL ) {
		cout << TTEARCH << endl;
		return 0;
	}

	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--ttetemp", 0) != NULL ) {
		cout << TTETEMPLATE_FIT << endl;
		return 0;
	}
	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--pha1temp", 0) != NULL ) {
		cout << PHA1TEMPLATE << endl;
		return 0;
	}
	if ( getCmdOption( (char**)argv, (char**)argv+argc, "--pha2temp", 0) != NULL ) {
		cout << PHA2TEMPLATE << endl;
		return 0;
	}


	return 0;
};