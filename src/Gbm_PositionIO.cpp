/*
 *  Gbm_PositionIO.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 3/30/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#include "Gbm_PositionIO.h"

long GbmPosReader::findRow( double time ) {
	vector<double> sTimes;
	vector<double> eTimes;
	long row = -1;
	if ( this->getTimes( sTimes, eTimes ) == 0 ) {
		row = findNearestElement( sTimes, time );
		
		
		
	}
	
	
	return row;
};

int TrigdatReader::getTimes( double *& tstarts, double *& tstops, long * ntimes )
{
	if ( this->stat.move2hdu((char*)"EVNTRATE" ) ) return this->stat.status;
	
	*ntimes = stat.n_rows;
	
	tstarts = new double[stat.n_rows];
	tstops = new double[stat.n_rows];
	
	double * ptr1 = &(tstarts[0]);
	double * ptr2 = &(tstops[0]);
	long i=1;
	
	while( stat.status == 0 && i <= stat.n_rows ) {
	
		if ( (i + stat.n_eff_rows - 1) > stat.n_rows) stat.n_eff_rows = stat.n_rows - i + 1;
	
		ffgcv(stat.fptr, TDOUBLE, 1, i, 1, stat.n_eff_rows,0, tstarts,NULL,&(this->stat.status) );
		ffgcv(stat.fptr, TDOUBLE, 2, i, 1, stat.n_eff_rows,0, tstops, NULL,&(this->stat.status) );
	
	//	cout << *ptr1 << "," << *ptr2 << endl;
	//	cout << tstarts[0] << "," << tstops[0] << endl;
	
		tstarts+=stat.n_eff_rows;
		tstops+=stat.n_eff_rows;
		
		i+=stat.n_eff_rows;
		
		if (stat.status != 0) break;	

	};
	
	tstarts = ptr1;
	tstops = ptr2;
	
	return stat.status;

}

int TrigdatReader::getTimes( vector<double>& tstart, vector<double>& tstop, long startRow, long endRow )
{
	if ( this->stat.move2hdu((char*)"EVNTRATE" ) ) return this->stat.status;
	
	long stride = stat.n_eff_rows;
	long n_rows = stat.n_rows;
	
	if ( endRow > 0 && startRow > 0 ) {
		n_rows = endRow - startRow + 1;
	} else {
		endRow = n_rows;
		startRow = 1;
	}
	
	tstart.reserve(n_rows);
	tstop.reserve(n_rows);
	
	double * tstarts = new double[stride];
	double * tstops = new double[stride];
	
	long i=startRow;
	
	while( stat.status == 0 && i <= endRow ) {
	
		if ( (i + stride - 1) > endRow) stride = endRow - i + 1;
	
		ffgcv(stat.fptr, TDOUBLE, 1, i, 1, stride,0, tstarts,NULL,&(this->stat.status) );
		ffgcv(stat.fptr, TDOUBLE, 2, i, 1, stride,0, tstops, NULL,&(this->stat.status) );
	
		for (int k=0;k<stride;k++) {
			tstart.push_back( tstarts[k] );
			tstop.push_back( tstops[k] );
		}
				
		i+=stride;
		
		if (stat.status != 0) break;	

	};
	delete tstarts;
	delete tstops;
	
	return stat.status;

}

int TrigdatReader::getScAttPos( double * tstart, double * tstop, geom::gquat * quat, geom::x3 * pos, long rownum )
{
	if ( this->stat.move2hdu((char*)"EVNTRATE" ) ) return this->stat.status;
	if (stat.status != 0) return this->stat.status;
	
	double scattit[4];
	double ecipos[3];
	
	long i=rownum;
	
	if ( i > stat.n_rows  || i < 1 ) return -1;
	
	ffgcv(stat.fptr, TDOUBLE, 1, i, 1, 1,0, tstart,NULL,&(this->stat.status) );
	ffgcv(stat.fptr, TDOUBLE, 2, i, 1, 1,0, tstop, NULL,&(this->stat.status) );
	ffgcv(stat.fptr, TDOUBLE, 3, i, 1, 4,0, scattit, NULL,&(this->stat.status) );
	ffgcv(stat.fptr, TDOUBLE, 4, i, 1, 3,0, ecipos, NULL,&(this->stat.status) );
	
	quat->set(scattit);
	pos->set(ecipos);
	
	return stat.status;
}

int TrigdatReader::getScAttPos( vector<double>& tstart, vector<double>& tstop, vector<geom::gquat>& quats, vector<geom::x3>& pos, long startRow, long endRow )
{
	if ( this->stat.move2hdu((char*)"EVNTRATE" ) ) return this->stat.status;
	
	long stride = stat.n_eff_rows;
	long n_rows = stat.n_rows;
	
	if ( endRow > 0 && startRow > 0 ) {
		n_rows = endRow - startRow + 1;
	} else {
		endRow = n_rows;
		startRow = 1;
	}
	
	tstart.reserve(n_rows);
	tstop.reserve(n_rows);
	quats.reserve(n_rows);
	pos.reserve(n_rows);
	
	double * tstarts = new double[stride];
	double * tstops = new double[stride];
	double * scattit = new double[4*stride];
	double * ecipos3 = new double[3*stride];
	
	long i=startRow;
	
	while( stat.status == 0 && i <= endRow ) {
	
		if ( (i + stride - 1) > endRow) stride = endRow - i + 1;
	
		ffgcv(stat.fptr, TDOUBLE, 1, i, 1, stride,0, tstarts,NULL,&(this->stat.status) );
		ffgcv(stat.fptr, TDOUBLE, 2, i, 1, stride,0, tstops, NULL,&(this->stat.status) );
		ffgcv(stat.fptr, TDOUBLE, 3, i, 1, 4*stride,0, scattit, NULL,&(this->stat.status) );
		ffgcv(stat.fptr, TDOUBLE, 4, i, 1, 3*stride,0, ecipos3, NULL,&(this->stat.status) );
		
	
		for (int k=0;k<stride;k++) {
			tstart.push_back( tstarts[k] );
			tstop.push_back( tstops[k] );
			
			quats.push_back( geom::gquat( scattit + k ) );
			pos.push_back( geom::x3( ecipos3 + k ) );
		}
				
		i+=stride;
		
		if (stat.status != 0) break;	

	};
	delete tstarts;
	delete tstops;
	delete scattit;
	delete ecipos3;
	
	return stat.status;
}


int PoshistReader::getTimes( vector<double>& tstart, vector<double>& tstop, long startRow, long endRow )
{
	
	if ( this->stat.move2hdu((char*)attExtName.c_str()) ) {
		cout << " at " << __func__ << endl;
		return this->stat.status;
	}
	
	long stride = stat.n_eff_rows;
	long n_rows = stat.n_rows;
	
	if ( endRow > 0 && startRow > 0 ) {
		n_rows = endRow - startRow + 1;
	} else {
		endRow = n_rows;
		startRow = 1;
	}
	
	tstart.reserve(n_rows);
	tstop.reserve(n_rows);
	
	double * tstarts = new double[stride];
	
	long i=startRow;
	
	while( stat.status == 0 && i <= endRow ) {
	
		if ( (i + stride - 1) > endRow) stride = endRow - i + 1;
	
		ffgcv(stat.fptr, TDOUBLE, 1, i, 1, stride,0, tstarts,NULL,&(this->stat.status) );
	
		for (int k=0;k<stride;k++) tstart.push_back( tstarts[k] );

		i+=stride;
		
		if (stat.status != 0) break;	

	};
	
	std::copy( tstart.begin(), tstart.end()-1, tstop.begin() );
	//The last entry has tstart=tstop
	tstop.push_back( tstarts[stride-1] );
	
	
	delete tstarts;
	
	return stat.status;

}

int PoshistReader::getScAttPos( double * tstart, double * tstop, geom::gquat * quat, geom::x3 * pos, long rownum )
{
	if ( this->stat.move2hdu((char*)attExtName.c_str() ) ) return this->stat.status;
	if (stat.status != 0) return this->stat.status;
	
	double scattit[4];
	double ecipos[3];
	
	long i=rownum;
	
	if ( i > stat.n_rows  || i < 1 ) return -1;
	
	ffgcv(stat.fptr, TDOUBLE, 1, i, 1, 1,0, tstart,NULL,&(this->stat.status) );
	*tstop=*tstart;
	
	ffgcv(stat.fptr, TDOUBLE, 2, i, 1, 1,0, scattit, NULL,&(this->stat.status) );
	ffgcv(stat.fptr, TDOUBLE, 3, i, 1, 1,0, scattit+1, NULL,&(this->stat.status) );
	ffgcv(stat.fptr, TDOUBLE, 4, i, 1, 1,0, scattit+2, NULL,&(this->stat.status) );
	ffgcv(stat.fptr, TDOUBLE, 5, i, 1, 1,0, scattit+3, NULL,&(this->stat.status) );
	ffgcv(stat.fptr, TDOUBLE, 9, i, 1, 1,0, ecipos, NULL,&(this->stat.status) );
	ffgcv(stat.fptr, TDOUBLE, 10, i, 1, 1,0, ecipos+1, NULL,&(this->stat.status) );
	ffgcv(stat.fptr, TDOUBLE, 11, i, 1, 1,0, ecipos+2, NULL,&(this->stat.status) );
	
	quat->set(scattit);
	pos->set(ecipos);
	
	return stat.status;
}

int PoshistReader::getScAttPos( vector<double>& tstart, vector<double>& tstop, vector<geom::gquat>& quats, vector<geom::x3>& pos, long startRow, long endRow )
{
	if ( this->stat.move2hdu( (char*)attExtName.c_str()) ) return this->stat.status;
	
	long stride = stat.n_eff_rows;
	long n_rows = stat.n_rows;
	
	if ( endRow > 0 && startRow > 0 ) {
		n_rows = endRow - startRow + 1;
	} else {
		endRow = n_rows;
		startRow = 1;
	}
	
	tstart.reserve(n_rows);
	tstop.reserve(n_rows);
	quats.reserve(n_rows);
	pos.reserve(n_rows);
	
	double * tstarts = new double[stride];
	double * scattit = new double[4*stride];
	double * ecipos = new double[3*stride];
	
	long i=startRow;
	
	while( stat.status == 0 && i <= endRow ) {
	
		if ( (i + stride - 1) > endRow) stride = endRow - i + 1;
	
		ffgcv(stat.fptr, TDOUBLE, 1, i, 1, stride,0, tstarts,NULL,&(this->stat.status) );
		
		ffgcv(stat.fptr, TDOUBLE, 2, i, 1, stride,0, scattit, NULL,&(this->stat.status) );
		ffgcv(stat.fptr, TDOUBLE, 3, i, 1, stride,0, scattit+1*stride, NULL,&(this->stat.status) );
		ffgcv(stat.fptr, TDOUBLE, 4, i, 1, stride,0, scattit+2*stride, NULL,&(this->stat.status) );
		ffgcv(stat.fptr, TDOUBLE, 5, i, 1, stride,0, scattit+3*stride, NULL,&(this->stat.status) );
		
		ffgcv(stat.fptr, TDOUBLE, 9, i, 1, stride,0, ecipos, NULL,&(this->stat.status) );
		ffgcv(stat.fptr, TDOUBLE, 10, i, 1, stride,0, ecipos+1*stride, NULL,&(this->stat.status) );
		ffgcv(stat.fptr, TDOUBLE, 11, i, 1, stride,0, ecipos+2*stride, NULL,&(this->stat.status) );
		
	
		for (int k=0;k<stride;k++) {
			tstart.push_back( tstarts[k] );
			
			quats.push_back( geom::gquat( scattit[k], scattit[k+1*stride], scattit[k+2*stride], scattit[k+3*stride] ) );
			pos.push_back( geom::x3( ecipos[k], ecipos[k+1*stride], ecipos[k+2*stride]) );
		}
				
		i+=stride;
		
		if (stat.status != 0) break;	

	};
	
	
	std::copy( tstart.begin(), tstart.end()-1, tstop.begin() );
	//The last entry has tstart=tstop
	tstop.push_back( tstarts[stride-1] );
	
	delete tstarts;
	delete scattit;
	delete ecipos;
	
	return stat.status;
}