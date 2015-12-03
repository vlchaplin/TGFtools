/*
 *  PHA_IO.hh
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 3/30/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#ifndef PHAIOUNIT
#define PHAIOUNIT

#include "EdgeSet.hh"
#include "PHAStructures.hh"
#include "TTEventTable.hh"
#include "DBStringUtilities.hh"


#include "fitsio.h"

#include <string>
#include <vector>
#include <iostream>

using namespace std;

struct FitsKeyLocation {
	string key;
	int ftype;
	int hdunum;
	void * ptr;
};

inline void check_for_archive(char * inputFile, bool * isTar, bool * isZip, char ** archive, char ** relativeFile) {


	//cout << "Checking for archive" << endl;

	char * temp;
	char * temp2;
	size_t arcbase_nchars;
	
	(*archive)[0]='\0';
	(*relativeFile)[0]='\0';
	*isTar=0;
	*isZip=0;

	temp = strstr(inputFile, (char*)".tar");
	
	if ( temp != NULL ) {
		
		//cout << ".tar";
		
		temp += 4;
		*isTar=1;
		
		temp2 = strstr(temp, (char*)".gz");
		
		if ( temp2 != NULL ) {
			
			//cout << ".gz";
			
			temp2 += 3;
			*isZip=1;
			
			arcbase_nchars = (size_t)(temp2 - inputFile);
			
			//copy from temp+1 since the path seperator is there...
			if ( temp2[0] != '\0' ) strcpy( *relativeFile, temp2+1 );
			
		} 
		else
		{
			arcbase_nchars = (size_t)(temp - inputFile);
			
			if ( temp[0] != '\0' ) strcpy( *relativeFile, temp+1 );
		}
		
		strncpy( *archive, inputFile, arcbase_nchars );
		
		(*archive)[arcbase_nchars]='\0';
		
		return;
	}
	
	
	temp = strstr(inputFile, (char*)".tgz");
	
	if ( temp != NULL ) {
		
		//cout << ".tgz";
		
		*isTar=1;
		*isZip=1;
		temp += 4;
		
		if ( temp[0] != '\0' ) strcpy( *relativeFile, temp+1 );
		
		arcbase_nchars = (size_t)(temp - inputFile);
		
		strncpy( *archive, inputFile, arcbase_nchars );
		
		(*archive)[arcbase_nchars]='\0';
		
		return;
	}
	
	temp = strstr(inputFile, (char*)".gz");
	
	if ( temp != NULL ) {
		
		*isTar=0;
		*isZip=1;
		temp += 3;
		
		if ( temp[0] != '\0' ) strcpy( *relativeFile, temp );
		
		arcbase_nchars = (size_t)(temp - inputFile);
		
		strncpy( *archive, inputFile, arcbase_nchars );
		
		(*archive)[arcbase_nchars]='\0';
		
		return;
	}
	
	
};

class FxbReadStat {
	public:
	fitsfile * fptr;
	int status;
	long n_eff_rows;
	long n_rows;
	long n_cols;
	
	bool isTar;
	bool isZip;
	bool unlinkOnClose;
	
	FxbReadStat() {
		fptr=NULL;
		status=0;
		n_eff_rows=0;
		n_rows=0;
		n_cols=0;
		isTar=0;
		isZip=0;
		unlinkOnClose=0;

	};
	
	inline int open(char * file, int extno=0, int mode=READONLY ) {
	
		this->status=0;
	
		ffopen( &(this->fptr), file, mode, &(this->status) );
		
		if (this->status != 0) {
			
			char basename[200];
			char relative[100];
			char command[310];
		
			char * b_ptr = basename;
			char * r_ptr = relative;
			char ** b_ptrptr = &b_ptr;
			char ** r_ptrptr = &r_ptr;
		
			check_for_archive( file, &isTar, &isZip, b_ptrptr, r_ptrptr );
			
			if ( isTar || isZip ) {
			
				if ( isTar && strlen(relative) == 0 ) {
					cout << "Unable to extract from a tar archive without a relative filename appened" << endl;
					return -1;
				}
			
				if ( FileExists((const char *)relative) ) unlinkOnClose=0;
				else if ( isTar && isZip ) {
					unlinkOnClose=1;
					sprintf( command, (char*)"tar -xzvf %s %s", basename, relative );
					cout << command << endl;
					
					system( command );
				} else if ( isTar ) {
					unlinkOnClose=1;
					sprintf( command, (char*)"tar -xvf %s %s", basename, relative );
					cout << command << endl;
					
					system( command );
				}
				
				this->status = 0;
				ffopen( &(this->fptr), relative, mode, &(this->status) );
				
			}
			
		
		}
		
		
		
		if (this->status != 0) {  
			return this->errclose();
		} else if (extno != 0) {
			ffmahd(this->fptr, extno, NULL, &(this->status) );
			ffgnrw(this->fptr,&(this->n_rows),&(this->status) );
			ffgrsz(this->fptr,&(this->n_eff_rows),&(this->status) );
			
			if ( n_eff_rows > n_rows ) n_eff_rows = n_rows;
			
			if (this->status != 0) return this->errclose();
		}
		return 0;
	};
	inline int move2hdu(char * extname) {
	
		if (this->fptr == NULL)  return -1;
	
		if (this->status != 0) {  
			return this->errclose();
		} else {
			ffmnhd(this->fptr, ANY_HDU, extname, 0, &(this->status) );
			ffgnrw(this->fptr,&(this->n_rows),&(this->status) );
			ffgrsz(this->fptr,&(this->n_eff_rows),&(this->status) );
			
			if ( n_eff_rows > n_rows ) n_eff_rows = n_rows;
			
			if (this->status != 0) return this->errclose();
		}
		return 0;
	};
	inline int move2hdu(int extno) {
		if (this->status != 0 || this->fptr == NULL) {  
			return this->errclose();
		} else {
			ffmahd(this->fptr, extno, NULL, &(this->status) );
			ffgnrw(this->fptr,&(this->n_rows),&(this->status) );
			ffgrsz(this->fptr,&(this->n_eff_rows),&(this->status) );
			
			if ( n_eff_rows > n_rows ) n_eff_rows = n_rows;
			
			if (this->status != 0) return this->errclose();
		}
		return 0;
	};
	
	inline int close() {
		unlinkOnClose=0;
		
		if ( unlinkOnClose ) {
			int st=0;
			char file[300];
			file[0]='\0';
			ffflnm(fptr, file, &st );
			
			if ( strlen(file) > 0 && FileExists(file) ) remove( file );
		}
		
		
		ffclos(fptr, &status );
		
		
		fptr = NULL;
		isTar=0;
		isZip=0;
		unlinkOnClose=0;
		
		return status;
	
	};
	
	inline int errclose() {
		if (this->fptr == NULL)  return -1;
		ffrprt( stderr, status);		
		return this->close();
	};
	
};


class PHAReader {

	
	protected:
	string file;
	vector< FitsKeyLocation > fileToMemLink;
	
	template<typename T>
	void bindKeysToMem( OGIP_PHA_misc<T>& keys ) {
		
		FitsKeyLocation links[] = { {"TRIGTIME",TDOUBLE,1,(void*)&keys.tzero},
									{"TSTART",TDOUBLE, 1,(void*)&keys.tmin}, 
									{"TSTOP",TDOUBLE,  1,(void*)&keys.tmax},
									{"DETNAM",TSTRING, 1,keys.charDetName},
									{"RESPFILE",TSTRING, 3,keys.charRmfFile} };
		
		this->fileToMemLink.assign( links, links+5 );
	};
	
	FxbReadStat stat;
	
	public:
	PHAReader(){};
	
	FxbReadStat * getFxb() { return &(this->stat); };
	
	
	int set_status(int newstatus) {stat.status=newstatus; return newstatus;};
	
	int open( int extno=0, int mode=READONLY ) {
		return stat.open( (char*)file.c_str(), extno, mode );
	};
	
	int close() {

		stat.close();
		
		return 0;
	};

	void setFile(char * filename) {
		this->file.assign(filename);
	};
	void setFile(string filename) {
		this->file.assign(filename);
	};
	string getFile() {
		return this->file;
	};
	
	template<typename T>
	int ReadPHAKeys( OGIP_PHA_misc<T> * keys );

	template<typename C,typename E,typename T>
	int ReadDataFile(SinglePHA<C,E,T> * newset);

};

class PHAWriter {

	private:
	
	template<typename C,typename E,typename T>
	int WriteSpectrumHDU(fitsfile** fptr, int * status, SinglePHA<C,E,T>& set);
	
	protected:
	string file;
	string writephase;
	unsigned int specCompositionLevel;
	int pha_col;
	int err_col;
	
	
	template<typename E>
	int WriteEboundsHDU(fitsfile** fptr, int * status, EdgeSet<E> *& edges);
	template<typename Oclass>
	int WriteStandardKeys(fitsfile** fptr, int * status, Oclass *& fields);
	template<typename C,typename E,typename T>
	int formatSpecHDUCLAS(fitsfile** fptr, int * status, SinglePHA<C,E,T> * data);
	
	
	public:
	
	PHAWriter(){
		specCompositionLevel=2;
		pha_col=2;
		err_col=3;
	};
	
	bool setSpecComposition(unsigned int setting) {
		//Effects HDUCLAS2: 0= BGK, 1= NET, 2= TOTAL
		if (setting <= 2) {
			this->specCompositionLevel = setting;
			return 1;
		} else return 0;
		
	};
	
	void setFile(char * filename) {
		this->file.assign(filename);
	};
	/*
	void setFile(string& filename) {
		this->file.assign(filename);
	};*/
	void setFile(string filename) {
		this->file.assign(filename);
	};
	
	string getFile() {
		return this->file;
	};
	
	template<typename C,typename E,typename T>
	int WriteDataFile(SinglePHA<C,E,T>& set);

};

#include "PHA_IO.cpp"
#endif