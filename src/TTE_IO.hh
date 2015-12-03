/*
 *  TTE_IO.hh
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 4/7/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#ifndef TTEIOUNIT
#define TTEIOUNIT

#include "PHA_IO.hh"
#include "EdgeSet.hh"
#include "PHAStructures.hh"
#include "TTEventTable.hh"

extern "C" {
	#include "fitsio.h"
};

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

//Max size of buffer used to read / write events
#define MAX_BUFF_SIZE 2000L

//If defined, stop loading events from file after this many
//#define MAX_READ_EVENTS 10L

//If the object is recording, this is the max number of records
//in the log for which stack memory will be allocated.
//If the log is greater than this size, heap memory will be used.
#define GUESS_STATIC_LOGLENGTH 300


//Name of the history extension
#define HISTORY_EXTNAME "HISTORY"

//The width of the history table to write in the fits file.
//Log records are truncated.
#define HISTORY_RECORDS_LENGTH 150
#define HISTORY_RECORDS_FORMAT "150A"


using namespace std;

class TTEReader : public PHAReader {

	private:
	
	int binSortedIntersection(long& rowMIN, long& rowMAX, double& tstart, double& tstop );
	
	public:
	TTEReader(){};
	
	int findRowRange( double tstart, double tstop, long& minr, long& maxr, int mode=1 );
	
	template<typename T,typename M, typename E>
	int ReadDataFile(TTEventTable<T,M,E> * newset, bool hdrsOnly=0, long start=1, long endRow=0);

};

class TTEWriter : public PHAWriter {

	private:
	
	//Psuedo-polymorpism:
	//Interface to parent routines.  Can't be virtualized because they are template functions.
	//...static members must be resolved & defined at compile time, and virtualized functions
	//must have same signatures; beacuse of templates there an infinite number of possible signatures,
	//but only callers determine which are realized. Within the class hierarchy there is no way break this
	//degeneracy.
	//Inheritance just lets this class access the protected members of PHAWriter, instead of having to
	//instantiate a PHAWriter object within this class and make this a friend of PHAWriter.
	//
	//Standard PHA methods are overlapped by calling this->PHAWriter::Method after the specific TTE
	//actions are performed. 

	template<typename E>
	int WriteEboundsHDU(fitsfile** fptr, int * status, EdgeSet<E> *& edges);
	template<typename Oclass>
	int WriteStandardKeys(fitsfile** fptr, int * status, Oclass *& fields);
	
	template<typename T,typename M,typename E>
	int WriteEventsHDU(fitsfile** fptr, int * status, TTEventTable<T,M,E>& set);
	template<typename T,typename M,typename E>
	int WriteGTIHDU(fitsfile** fptr, int * status, TTEventTable<T,M,E>& set);
	template<typename T,typename M,typename E>
	int WriteHistoryHDU(fitsfile** fptr, int * status, TTEventTable<T,M,E>& set);

	public:
	TTEWriter(){};
	
	template<typename T,typename M,typename E>
	int WriteDataFile(TTEventTable<T,M,E>& set);

};



template<typename T,typename M,typename E>
int TTEReader::ReadDataFile(TTEventTable<T,M,E> * newset, bool headersOnly, long start, long endRow) {

	if (newset == NULL) return -1;

	int namelength = this->file.size();
	long nchan;
	int status = 0;	
	fitsfile *fptr;
	
	typename TTEventTable<T,M,E>::fields_class * miscfields;
	miscfields = newset->getMiscFields();
	
	//ffopen(&fptr, filename, READONLY, &status);
	
	this->open( 0, READONLY );
	
	if (this->stat.status != 0) {  
		cout << __LINE__ << endl;
		return this->stat.errclose();
	}
	
	fptr = this->stat.fptr;
	
	char detname[72];
	char tempstr[20];
	strcpy(tempstr,"DETNAM");
	ffgky(fptr, TSTRING, tempstr, detname, NULL, &(this->stat.status) );
	if ( status==0 ) {
		string temp( detname );
		miscfields->setDetName( temp );
	}
	
	this->stat.move2hdu( (char*)"EBOUNDS" );
	if (this->stat.status != 0) {
		cout << __LINE__ << endl;
		return this->stat.errclose();
	} else nchan = this->stat.n_rows;
	/*
	strcpy(tempstr,"EBOUNDS");
	ffmnhd(fptr, ANY_HDU, tempstr, 0, &(this->stat.status));
	ffgnrw(fptr,&nchan,&status);
	if (status != 0) {  
		ffrprt( stderr, status);
		ffclos(fptr, &status);
		return status;
	}*/
	
	
	int b;
	float * chdata;
	EdgeSet<E> * ebounds;
	
	if (! headersOnly) {
		chdata = new float[nchan];
		ebounds = new EdgeSet<E>;
		
		ebounds->reinit(nchan, 0.0);
		ffgcv(fptr,TFLOAT,2,1,1,nchan,0,chdata,NULL,&(this->stat.status));
		for (b=0;b<nchan;b++) { (*ebounds)[0][b] = chdata[b]; }
		ffgcv(fptr,TFLOAT,3,1,1,nchan,0,chdata,NULL,&(this->stat.status));
		for (b=0;b<nchan;b++) { (*ebounds)[1][b] = chdata[b]; }
		
		newset->AttachMBins(ebounds);
		
		delete [] chdata;
		//cout << *ebounds;
	}
	
	this->stat.move2hdu( (char*)"EVENTS" );
	if (this->stat.status != 0) {  
		cout << __LINE__ << endl;
		return this->stat.errclose();
	}
	
	double tzero;
	ffgky(fptr, TDOUBLE, (char *)"TZERO1", &tzero, NULL, &(this->stat.status) );
	miscfields->tzero = tzero;
	
	if (! headersOnly) {
	
		int ratecol,errcol;
		long n_eff_rows,n_rows;
		ffgcno(fptr, CASEINSEN, (char *)"TIME", &ratecol, &(this->stat.status) );
		ffgcno(fptr, CASEINSEN, (char *)"PHA", &errcol, &(this->stat.status) );
		
		if (this->stat.status != 0) {  
			return this->stat.errclose();
		}
		
		n_eff_rows = this->stat.n_eff_rows;
		n_rows = this->stat.n_rows;
		
		string mutator(__FUNCTION__);
		mutator += "() ";
		newset->LogMutation( mutator , this->file );
		
		int T_fits = type_to_fitstype< T >();
		int M_fits = type_to_fitstype< M >();
		
		if ( endRow > 0 ) {
			n_rows = endRow - start + 1;
		} else {
			endRow = n_rows;
			start = 1;
		}
		
		
		#ifdef MAX_READ_EVENTS
			n_rows = MAX_READ_EVENTS;
		#endif
		
		if (n_eff_rows > MAX_BUFF_SIZE) n_eff_rows = MAX_BUFF_SIZE;
		if (n_eff_rows > n_rows) n_eff_rows = n_rows;
		
		
		T t_buffer[MAX_BUFF_SIZE];
		M c_buffer[MAX_BUFF_SIZE];
		
		newset->reserve(n_rows);
		
		while( status == 0 && (start < endRow) ) {
		
			if ( (start + n_eff_rows - 1) > endRow) n_eff_rows = endRow - start + 1;

		
			ffgcv(fptr,T_fits ,ratecol,start,1,n_eff_rows,0,t_buffer,NULL,&(this->stat.status) );
			ffgcv(fptr,M_fits ,errcol ,start,1,n_eff_rows,0,c_buffer,NULL,&(this->stat.status) );
		
			if (status != 0) break;	
			newset->Append( t_buffer, c_buffer, n_eff_rows );
			start += n_eff_rows;
		};
		
		newset->LogMutation( "resized to ", start );
	
	}
	
	if (this->stat.status != 0) {  
		cout << __LINE__ << endl;
		return this->stat.errclose();
	} else {
		this->close();
	}
	
	return 0;
};

template<typename T,typename M,typename E>
int TTEWriter::WriteDataFile(TTEventTable<T,M,E>& set) {
	
	this->writephase="WriteDataFile";
	
	string name;
	
	long nevents = set.length();
	if (nevents == 0) return -1;
	
	char * templatefl;
	templatefl = (char *)TTETEMPLATE_FIT;

	int status = 0;
	fitsfile *fptr;
	fitsfile *tplt;

	ffinit(&fptr, this->file.c_str(), &status);
	
	if (status == 104 || status == 105) {
		remove ( this->file.c_str() );
		status = 0;
		ffinit(&fptr, this->file.c_str(), &status);
	} 
	
	if (status != 0) goto WriteTTEDataErrorClosure;

	ffopen(&tplt, templatefl , READONLY, &status);
	
	if (status != 0) {
		goto WriteTTEDataErrorClosure;
	};
	

	//Copy the Primary HDU to fptr
	ffcpfl( tplt, fptr, 1,1,0, &status);
	
	if (status != 0) goto WriteTTEDataErrorClosure;
	
	ffuky( fptr, TSTRING, (char *)"CREATOR", (char *)"TTE_IO.cpp", NULL, &status);
	
	typedef typename TTEventTable<T,M,E>::fields_class fields_class;
	fields_class * fields;
	fields = set.getMiscFields();

	this->WriteStandardKeys(&fptr, &status, fields);
	ffpdat(fptr, &status);
	ffpcks(fptr, &status);

	char* fitsTableName;
	fitsTableName = (char*)"EBOUNDS";
	ffmnhd(tplt, ANY_HDU, fitsTableName, 0, &status);
	ffcphd( tplt, fptr, &status);
	if (status != 0) goto WriteTTEDataErrorClosure;

	EdgeSet< E > * edgetable;
	edgetable = set.getEdgeSet();
	this->WriteEboundsHDU(&fptr, &status, edgetable);
	this->WriteStandardKeys(&fptr, &status, fields);
	if (status != 0) goto WriteTTEDataErrorClosure;
	
	fitsTableName = (char *)"EVENTS";
	ffmnhd( tplt, ANY_HDU, fitsTableName, 0, &status);
	ffcphd( tplt, fptr, &status);
	if (status != 0) goto WriteTTEDataErrorClosure;
	
	long n_rows;
	ffmnhd( fptr, ANY_HDU, fitsTableName, 0, &status);
	ffgnrw( fptr, &n_rows, &status);
	ffdrow( fptr, 1, n_rows, &status);
	
	this->WriteStandardKeys(&fptr, &status, fields);
	this->WriteEventsHDU(&fptr, &status, set);
	if (status != 0) goto WriteTTEDataErrorClosure;
	fitsTableName = (char *)"GTI";
	ffmnhd( tplt, ANY_HDU, fitsTableName, 0, &status);
	ffcphd( tplt, fptr, &status);
	ffmnhd( fptr, ANY_HDU, fitsTableName, 0, &status);
	ffgnrw( fptr, &n_rows, &status);
	ffdrow( fptr, 1, n_rows, &status);
	if (status != 0) goto WriteTTEDataErrorClosure;
	
	this->WriteStandardKeys(&fptr, &status, fields);
	this->WriteGTIHDU(&fptr, &status, set);
	if (status != 0) goto WriteTTEDataErrorClosure;
	
	if ( set.recorder != NULL ) {
		fitsTableName = (char *)HISTORY_EXTNAME;
		char* ttype[1];
		char* tform[1];
		char* tunit[1];
		ttype[0] = (char *)"HISTORY";
		tform[0] = (char *)HISTORY_RECORDS_FORMAT;
		tunit[0] = (char *)"";
		ffcrtb( fptr, BINARY_TBL, 0L, 1, ttype, tform, tunit, fitsTableName, &status);
		if (status == 0) {
			this->WriteStandardKeys(&fptr, &status, fields);
			this->WriteHistoryHDU(&fptr, &status, set);
		}
		
		if (status != 0) goto WriteTTEDataErrorClosure;
	}
	
	cout << "Wrote " << this->file << "("<<set.length()<<" events)\n";
	
	WriteTTEDataNormalClosure:
	this->writephase = "end\n";
	ffclos(fptr, &status);
	ffclos(tplt, &status);

	return status;
	
	WriteTTEDataErrorClosure:
	ffrprt( stderr, status);
	cerr << " at "<< this->writephase << "\n";;
	status = 0;
	goto WriteTTEDataNormalClosure;
};

template<typename E>
int TTEWriter::WriteEboundsHDU(fitsfile** fptr, int * status, EdgeSet<E> *& edges)
{
	this->writephase = "WriteEboundsHDU";
	return this->PHAWriter::WriteEboundsHDU(fptr, status, edges);
}

template<typename Oclass>
int TTEWriter::WriteStandardKeys(fitsfile** fptr, int * status, Oclass *& fields)
{
	this->writephase = "WriteStandardKeys";
	
	int T_fits = type_to_fitstype(fields->tzero);
	ffuky(*fptr, T_fits, (char *)"TRIGTIME", &(fields->tzero), NULL, status);
	ffuky(*fptr, T_fits, (char *)"TSTART", &(fields->tmin), NULL, status);
	ffuky(*fptr, T_fits, (char *)"TSTOP", &(fields->tmax), NULL, status);
	return this->PHAWriter::WriteStandardKeys(fptr, status, fields);
}

template<typename T,typename M,typename E>
int TTEWriter::WriteGTIHDU(fitsfile** fptr, int * status, TTEventTable<T,M,E>& set)
{
	this->writephase = "WriteGTIHDU";
	
	typename TTEventTable<T,M,E>::fields_class * fields;
	fields = set.getMiscFields();
	int T_fits = type_to_fitstype< T >();
	
	ffuky(*fptr, T_fits, (char *)"TZERO1", &(fields->tzero), NULL, status);
	ffuky(*fptr, T_fits, (char *)"TZERO2", &(fields->tzero), NULL, status);
	
	fftscl(*fptr, 1, 1.0f, double(fields->tzero), status);
	fftscl(*fptr, 2, 1.0f, double(fields->tzero), status);
	
	ffpcl(*fptr, T_fits, 1, 1, 1, 1, &(fields->tmin), status);
	ffpcl(*fptr, T_fits, 2, 1, 1, 1, &(fields->tmax), status);
	
	ffpdat(*fptr, status);
	ffpcks(*fptr, status);
	
	if (*status != 0) { 
		ffrprt( stderr, *status);
	}
	return *status;
}

template<typename T,typename M,typename E>
int TTEWriter::WriteEventsHDU(fitsfile** fptr, int * status, TTEventTable<T,M,E>& set)
{
	
	this->writephase = "WriteEventsHDU";

	//cout << "State at Write:"<< set << "\n";
	typename TTEventTable<T,M,E>::fields_class * fields;
	fields = set.getMiscFields();
	int T_fits = type_to_fitstype< T >();
	int M_fits = type_to_fitstype< M >();
		
	long n_eff_rows, n_rows, fits_row, mem_row;


	T t_buffer[MAX_BUFF_SIZE];
	M c_buffer[MAX_BUFF_SIZE];
	n_eff_rows  = MAX_BUFF_SIZE;

	n_rows = set.length();
	
	//Dwindle the buffer size to the right amount
	if (n_eff_rows > MAX_BUFF_SIZE) n_eff_rows = MAX_BUFF_SIZE;
	if (n_eff_rows > n_rows) n_eff_rows = n_rows;
	
	char keyword[20];
	
	//Invert the time shifting. Have to unset TZEROn, overide w/ fftscl, then reset
	//after all column writes are done.
	t_buffer[0] = 0;
	strcpy(keyword,"TZERO1");
	ffuky(*fptr, T_fits, keyword, &t_buffer[0], NULL, status);
	fftscl(*fptr, 1, 1.0f, double(fields->tzero), status);
	
	fits_row = 1L;
	mem_row = 0L;

	set.BufferEvents( t_buffer, c_buffer, fits_row-1L, &n_eff_rows );
	

	while( *status == 0 && (n_eff_rows > 0L) ) {
	
		ffpcl(*fptr, T_fits, 1, fits_row, 1, n_eff_rows, t_buffer, status);
		ffpcl(*fptr, M_fits, 2, fits_row, 1, n_eff_rows, c_buffer, status);
	
		if (*status != 0) break;
		else {
			fits_row += n_eff_rows;
			set.BufferEvents( t_buffer, c_buffer, fits_row-1L, &n_eff_rows );
		}
	};
	
	strcpy(keyword,"TZERO1");
	ffuky(*fptr, T_fits, keyword, &(fields->tzero), NULL, status);
	if (*status != 0) { 
		ffrprt( stderr, *status);
		return *status;
	}
	
	if (fields->response.length() > 0) {
		char rspname[73];
		strcpy(rspname, fields->response.c_str() );
		strcpy(keyword,"RESPFILE");
		ffuky(*fptr, TSTRING, keyword, rspname, NULL, status);
	}
	
	ffpdat(*fptr, status);
	ffpcks(*fptr, status);
	
	return *status;
}
template<typename T,typename M,typename E>
int TTEWriter::WriteHistoryHDU(fitsfile** fptr, int * status, TTEventTable<T,M,E>& set)
{
	this->writephase="WriteHistoryHDU";
	
	if (set.recorder == NULL) return *status;
	
	int logLength = set.recorder->logLength();
	

	for (int i=0;i<logLength;i++) {
		char * rec = (char*)(*set.recorder)(i).c_str();
		ffpcl(*fptr, TSTRING, 1, i+1, 1, 1, &rec, status);
	};
	
	
	ffpdat(*fptr, status);
	ffpcks(*fptr, status);
	return *status;
}
#endif