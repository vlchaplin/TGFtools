/*
 *  PHA2_IO.cpp
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 5/18/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#ifndef PHA_2_IODEFINED
#define PHA_2_IODEFINED

#include "PHA2_IO.hh"
#include <typeinfo>


template<typename Oclass>
int PHA2Writer::WriteStandardKeys(fitsfile** fptr, int * status, Oclass *& fields)
{
	this->writephase = "WriteStandardKeys";
	/*
	char date_obs[30];
	char date_end[30];
	
	glast_met_strftime(  fields->tmin, date_obs, (char*)"%Y-%m-%dT%H:%M:%S" );
	glast_met_strftime(  fields->tmax, date_end, (char*)"%Y-%m-%dT%H:%M:%S" );
	
	int T_fits = type_to_fitstype(fields->tzero);
	ffuky(*fptr, T_fits, (char *)"TRIGTIME", &(fields->tzero), NULL, status);
	ffuky(*fptr, T_fits, (char *)"TSTART", &(fields->tmin), NULL, status);
	ffuky(*fptr, T_fits, (char *)"TSTOP", &(fields->tmax), NULL, status);
	ffuky(*fptr, TSTRING, (char *)"DATE-OBS", date_obs, NULL, status);
	ffuky(*fptr, TSTRING, (char *)"DATE-END", date_end, NULL, status);
	*/
	return this->PHAWriter::WriteStandardKeys(fptr, status, fields);
}

template<typename C,typename E,typename T>
int PHA2Writer::formatSpecHDUCLAS(fitsfile** fptr, int * status, PHA2<C,E,T> * data) {
	
	int nchan = data->nchannels();
	this->PHAWriter::formatSpecHDUCLAS( fptr, status, data );
	
	ffmvec( *fptr, pha_col, nchan, status);
	ffmvec( *fptr, err_col, nchan, status);
	return *status; 
}

template<typename C,typename E,typename T>
int PHA2Writer::WriteSpectrumHDU(fitsfile** fptr, int * status, PHA2<C,E,T>& set, 
	typename EdgeSet< E >::size_type& nEdges) 
{
	
	cout << "PHA2 Write Spectrum" << endl;
	
	typedef SinglePHA<C,E,T> pha_type;
	typedef typename pha_type::err_type err_type;
	
	typename PHA2<C,E,T>::size_type i = 0;
	typename PHA2<C,E,T>::size_type nspec;
	typename pha_type::size_type j;
	typename pha_type::size_type nchan;
	typename PHA2<C,E,T>::fields_class * fields;
	
	int c;
	int C_type = type_to_fitstype< C >();
	int T_type = type_to_fitstype< T >();
	int Err_type = type_to_fitstype< err_type >();
	
	long fits_row = 1;

	bool ChAreRates = set.IsDifferential();
	bool poissonErr = set.UsePoissonErrors();
	
	nspec = set.size();
	nchan = set[0].size();
	
	this->formatSpecHDUCLAS( fptr, status, &(set) );
	
	
	
	char tcol[20];
	/*
	if ( ChAreRates ) {
		//If the spectrum is in rates, write a stat_err column
		ttype_to_tform(TFLOAT, tcol);
		ffdcol( *fptr, 1, status );
		fficol( *fptr, 1, (char*)"RATE", tcol, status);
		ffmvec( *fptr, 1, nchan, status);
		
		ffuky(*fptr, TSTRING, (char *)"HDUCLAS3", (char*)"RATE", NULL, status);
		ffuky(*fptr, TSTRING, (char *)"TUNIT1", (char*)"counts/s", NULL, status);
		
		//Add the stat_err column
		ttype_to_tform(TFLOAT, tcol);
		fficol( *fptr, 2, (char*)"STAT_ERR", tcol, status);
		ffmvec( *fptr, 2, nchan, status);
	
	} else {
		//If not, write counts.  Update the column type to be a long integer.
		ttype_to_tform(TLONG, tcol);
		ffdcol( *fptr, 1, status );
		fficol( *fptr, 1, (char*)"COUNTS", tcol, status);
		ffmvec( *fptr, 1, nchan, status);
		
		ffuky(*fptr, TSTRING, (char *)"HDUCLAS3", (char*)"COUNT", NULL, status);
		ffuky(*fptr, TSTRING, (char *)"TUNIT1", (char*)"count", NULL, status);
	}
	if ( poissonErr ) {
		int tf = 1;
		ffuky(*fptr, TLOGICAL, (char *)"POISSERR", &tf, NULL, status);
	} else {
		int tf = 0;
		ffuky(*fptr, TLOGICAL, (char *)"POISSERR", &tf, NULL, status);
	}
	*/
	
	cout << "Nspectra = "<< nspec << endl;

	int exp_c, tstart_c, tstop_c;
	ffgcno(*fptr, CASEINSEN, (char *)"EXPOSURE",    &exp_c, status);
	ffgcno(*fptr, CASEINSEN, (char *)"TIME",  &tstart_c, status);
	ffgcno(*fptr, CASEINSEN, (char *)"ENDTIME",    &tstop_c, status);

	int dchans = nEdges;
	ffuky(*fptr, TINT, (char *)"DETCHANS", &dchans, NULL, status);
	
	fields = set.getMiscFields();
	sprintf(tcol, "TZERO%d", tstart_c);
	ffuky(*fptr, T_type, tcol, &(fields->tzero), NULL, status);
	fftscl(*fptr, tstart_c, 1.0f, double(fields->tzero), status);
	
	sprintf(tcol, "TZERO%d", tstop_c);
	ffuky(*fptr, T_type, tcol, &(fields->tzero), NULL, status);
	fftscl(*fptr, tstop_c, 1.0f, double(fields->tzero), status);
	
	C * c_buffer;
	T tstart, tstop;
	T exp;
	
	err_type * stat_buffer;
	
	c_buffer = new C[nEdges];
	stat_buffer = new err_type[nEdges];
	
	//cout << "PHA2 write: " << endl;
	//cout << set << endl;
	
	while (*status == 0 && ( i < nspec ) ) {
		c = 0;
		
		set(i, &tstart);
		
		nchan = set[i].size();
		exp = set[i].GetExposure();
		tstop = tstart + exp;		
		
		if (nchan != nEdges) {
			i++;
			cout << "Error, wrong edge size"<<endl;
			continue;
		}
		
		for (j=0; j < nEdges; j++) {
			
			c_buffer[c] = set[i][j].cval;
			
			if ( ChAreRates ) {
				//c_buffer[c]*=exp;
			}
			if (! poissonErr ) {
				stat_buffer[c] = set[i][j].stat_err;
			}
			c++;
		};
		
		ffpcl(*fptr, C_type, 1, fits_row, 1, 1*nEdges, c_buffer, status);
		ffpcl(*fptr, T_type, exp_c, fits_row, 1, 1, &exp, status);
		ffpcl(*fptr, T_type, tstart_c, fits_row, 1, 1, &tstart, status);
		ffpcl(*fptr, T_type, tstop_c, fits_row, 1, 1, &tstop, status);
		
		if (! poissonErr ) {
			ffpcl(*fptr, Err_type, 2, fits_row, 1, 1*nEdges, stat_buffer, status);
		}
	
		if (*status != 0) break;
	
		fits_row++;
		i++;
	};
	
	delete [] c_buffer;
	delete [] stat_buffer;
	
	return *status;
};

template<typename C,typename E,typename T>
int PHA2Writer::WriteDataFile(PHA2<C,E,T>& set) {

	typedef typename PHA2<C,E,T>::fields_class fields_class;

	int status = 0;
	long nSpectra = set.size();
	if (nSpectra == 0) return -1;
	
	typename EdgeSet< E >::size_type nchan;
	int namelength;
	long n_rows;
	
	char* fitsTableName;

	string name;
	fitsfile *fptr;
	fitsfile *tplt;
	fields_class * fields;
	
	
	set.getParentFile(name);
	namelength = name.size();
	
	
	char * templatefl;
	if (namelength == 0) templatefl = (char *)PHA2TEMPLATE;
	else {
		templatefl = new char[namelength+1];
		strcpy(templatefl, name.c_str() );
	}

	ffinit(&fptr, this->file.c_str(), &status);
	if (status == 104 || status == 105) {
		remove ( this->file.c_str() );
		status = 0;
		ffinit(&fptr, this->file.c_str(), &status);
	}	
	if (status != 0) goto WritePHA2DataErrorClosure;
	
	//Open the template file
	ffopen(&tplt, templatefl , READONLY, &status);
	if (status != 0) goto WritePHA2DataErrorClosure;
	
	//Copy the Primary HDU to fptr
	ffcpfl( tplt, fptr, 1,1,0, &status);
	ffuky( fptr, TSTRING, (char *)"CREATOR", (char *)"PHA2_IO.cpp", NULL, &status);
	
	fields = set.getMiscFields();
	this->WriteStandardKeys(&fptr, &status, fields);
	
	ffpdat(fptr, &status);
	ffpcks(fptr, &status);
	
	//Copy EBOUNDS extension, clear and write.
	fitsTableName = (char*)"EBOUNDS";
	ffmnhd(tplt, ANY_HDU, fitsTableName, 0, &status);
	ffcphd(tplt, fptr, &status);
	ffmnhd( fptr, ANY_HDU, fitsTableName, 0, &status);
	ffgnrw( fptr, &n_rows, &status);
	ffdrow( fptr, 1, n_rows, &status);
	
	if (status != 0) goto WritePHA2DataErrorClosure;

	EdgeSet< E > * edgetable;
	edgetable = set.getEdgeSet();
	this->WriteEboundsHDU(&fptr, &status, edgetable);
	this->WriteStandardKeys(&fptr, &status, fields);

	if (status != 0) goto WritePHA2DataErrorClosure;
	
	nchan = edgetable->size();

	fitsTableName = (char*)"SPECTRUM";
	ffmnhd( tplt, ANY_HDU, fitsTableName, 0, &status);
	ffcphd( tplt, fptr, &status);
	ffmnhd( fptr, ANY_HDU, fitsTableName, 0, &status);
	ffgnrw( fptr, &n_rows, &status);
	ffdrow( fptr, 1, n_rows, &status);
	
	if (status != 0) goto WritePHA2DataErrorClosure;
	
	this->WriteSpectrumHDU(&fptr, &status, set, nchan);

	ffuky(fptr, TFLOAT, (char *)"AREASCAL", &(set.ogip_fields->areasc), NULL, &status);
	ffuky(fptr, TFLOAT, (char *)"BACKSCAL", &(set.ogip_fields->backsc), NULL, &status);
	ffuky(fptr, TUSHORT,(char *)"GROUPING", &(set.ogip_fields->grpg), NULL, &status);
	
	char rsp[100];
	set.CpyRmf_file(rsp);

	//cout << "RESPFILE=" << rsp << endl;

	if ( rsp == NULL ) ffuky(fptr, TSTRING, (char *)"RESPFILE", (char*)"none", NULL, &status);
	else ffuky(fptr, TSTRING, (char *)"RESPFILE", rsp, NULL, &status);

	
	this->WriteStandardKeys(&fptr, &status, fields);
	if (status != 0) goto WritePHA2DataErrorClosure;
	
	cout << endl<< "----------------------"<<endl;
	cout << "Wrote PHA Type II to:" << this->file << endl;
	cout << "----------------------"<<endl<<endl;

	WritePHA2DataNormalClosure:
	ffpdat(fptr, &status);
	ffpcks(fptr, &status);
	
	ffclos(fptr, &status);
	ffclos(tplt, &status);
	return status;
	
	WritePHA2DataErrorClosure:
	if (namelength > 0) delete [] templatefl;
	cout << endl<< "***"<<endl;
	cout << "*** Error attempting to write PHA Type II file:" << this->file << endl;
	cout << "*** "<<endl;
	ffrprt( stderr, status);
	goto WritePHA2DataNormalClosure;


	cout << "PHA2 Write Spectrum" << endl;
	return status;
};


#endif