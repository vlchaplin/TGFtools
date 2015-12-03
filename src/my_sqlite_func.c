/*
 *  my_sqlite_func.c
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 8/26/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#include "my_sqlite_func.h"

//From sqlite3.c
typedef sqlite_int64 i64;          /* 8-byte signed integer */
typedef unsigned char u8;             /* 1-byte unsigned integer */

/*
	Calculates the angle in degrees between two input points
	on the unit sphere, whose coordinates are each given in terms
	of an azimuth and elevation angle, in degrees.
	
	Optional fifth parameter determines whether output is in degrees
	or radians.  Omitting it or passing 0 results in degree output.
	Non-zero value results in radians.
	
	Form: angle( az1, el1, az2, el2, radians=0 )
	
*/
void sepang(sqlite3_context * context, int argc, sqlite3_value **argv) {

	if (argc<5) return; 
	
	//static double deg2rad = 4.0*atan(1.0) / 180.0;
	static double deg2rad = 0.0174532925199; //Since C can't initialize static's with non-const type
	static double cosine;
	double th1,th2,ph1,ph2;
	
	ph1 = deg2rad*sqlite3_value_double( argv[0] );
	th1 = deg2rad*sqlite3_value_double( argv[1] );
	ph2 = deg2rad*sqlite3_value_double( argv[2] );
	th2 = deg2rad*sqlite3_value_double( argv[3] );
	
	cosine = sin(th1)*sin(th2) + cos(th1)*cos(th2)*cos(ph1 - ph2);

	if (cosine > 1.0) cosine = 1.0;
	else if (cosine < -1.0) cosine = -1.0;
	
	if (argc==5 && sqlite3_value_int(argv[4]) ) {
		sqlite3_result_double( context, acos(cosine) );
	} else {
		sqlite3_result_double( context, acos(cosine)/deg2rad );
	}
};

void linbin(sqlite3_context * context, int argc, sqlite3_value **argv) {

	if (argc<3) return;
	
	int colType, minType, delType;
	double colData, minv, step;
	
	static long binIndex;
	
	colType = sqlite3_value_numeric_type( argv[0] );
	minType = sqlite3_value_numeric_type( argv[1] );
	delType = sqlite3_value_numeric_type( argv[2] );

	if ( colType == SQLITE_INTEGER) colData = (double)sqlite3_value_int64(argv[0]); 
	else colData = sqlite3_value_double(argv[0]);
	
	if ( minType == SQLITE_INTEGER) minv = (double)sqlite3_value_int64(argv[1]); 
	else minv = sqlite3_value_double(argv[1]);
	
	if ( delType == SQLITE_INTEGER) step = (double)sqlite3_value_int64(argv[2]); 
	else step = sqlite3_value_double(argv[2]);

	
	binIndex = floor( (colData - minv) / step );
	sqlite3_result_int64( context, binIndex );
};



/*
* Implement weighted mean function wavg() for aggregate queries
* 
* INPUTS: 2 Arguments
*		Argument 0 = data point, argument 1 = weight of the point
*
*/

typedef struct WeightQty WeightQty;
struct WeightQty {
	double rQuant;	/* Floating point quantity of interest */
	i64 iQuant;		/* Integer '' */
	
	double rSumWt;	/* Cumulative weight */
	i64 iSumWt;
	i64 cnt;		/* Number of elements in aggregate */
	
	u8 approx;        /* True if non-integer value was input as the quantity */
};


static void wavgStep(sqlite3_context *context, int argc, sqlite3_value **argv){
	WeightQty *p;
	int q_type, w_type;
	
	if (argc<2) return;

	p = sqlite3_aggregate_context( context, sizeof(*p) );
	q_type = sqlite3_value_numeric_type( argv[0] );
	w_type = sqlite3_value_numeric_type( argv[1] );
	
	if (p && (q_type == SQLITE_NULL || w_type == SQLITE_NULL) ) return;
	
	p->cnt++;
	if ( q_type == SQLITE_INTEGER && w_type == SQLITE_INTEGER ){
		i64 q = sqlite3_value_int64(argv[0]);
		i64 w = sqlite3_value_int64(argv[1]);
		p->iQuant += (w*q);
		p->iSumWt += w;
	} else {
		double q,w;
		if (q_type == SQLITE_INTEGER) {
			q = (double)sqlite3_value_int64(argv[0]);
			w = sqlite3_value_double(argv[1]);
		} else if (w_type == SQLITE_INTEGER) {
			q = sqlite3_value_double(argv[0]);
			w = (double)sqlite3_value_int64(argv[1]);
		} else {
			q = sqlite3_value_double(argv[0]);
			w = sqlite3_value_double(argv[1]);
		}
		p->rQuant += (w*q);
		p->rSumWt += w;
		
		if (! p->approx ) p->approx = 1;
	}
};

static void wavgFinalize(sqlite3_context *context) {
	WeightQty *p;
	p = sqlite3_aggregate_context(context, 0);
	
	if( p && p->cnt>0 ){
		if (p->approx) {
			sqlite3_result_double(context, p->rQuant / p->rSumWt);
		} else {
			sqlite3_result_double(context, ((double)p->iQuant) / (double)p->iSumWt );
		}
	}
};


typedef struct WVarTerm WVarTerm;
struct WVarTerm {
	double rTerm;	/* Floating point quantity of interest */	
	double rSumWt;	/* Cumulative weight */
	double rSumEr;	/* Cumulative error for wavgerr */
	i64 iSumWt;
	
	i64 cnt;		/* Number of elements in aggregate */
	u8 approx;        /* True if non-integer value was input as the quantity */
};

//Step the function wvar.
static void wvarStep(sqlite3_context *context, int argc, sqlite3_value **argv){
	WVarTerm *p;
	int q_type, w_type, m_type;
	
	if (argc<3) return;

	p = sqlite3_aggregate_context( context, sizeof(*p) );
	q_type = sqlite3_value_numeric_type( argv[0] );
	w_type = sqlite3_value_numeric_type( argv[1] );
	m_type = sqlite3_value_numeric_type( argv[2] );
	
	if (p && (q_type == SQLITE_NULL || w_type == SQLITE_NULL || m_type == SQLITE_NULL) ) return;
	
	if (p->cnt == 0) {
		p->rTerm=0;
		p->rSumWt=0;
		p->iSumWt=0;
	}
	
	double wavg = sqlite3_value_double(argv[2]);
	p->cnt++;
	
	if ( q_type == SQLITE_INTEGER && w_type == SQLITE_INTEGER ){
		i64 q = (double)sqlite3_value_int64(argv[0]);
		i64 w = (double)sqlite3_value_int64(argv[1]);

		p->rTerm += w*pow( ( q - wavg ), 2.0 );
		p->iSumWt += w;
	} else {
		double q,w;
		if (q_type == SQLITE_INTEGER) {
			q = (double)sqlite3_value_int64(argv[0]);
			w = sqlite3_value_double(argv[1]);
		} else if (w_type == SQLITE_INTEGER) {
			q = sqlite3_value_double(argv[0]);
			w = (double)sqlite3_value_int64(argv[1]);
		} else {
			q = sqlite3_value_double(argv[0]);
			w = sqlite3_value_double(argv[1]);
		}
		
		p->rTerm += ( w*pow( ( q - wavg ), 2.0 ) );
		p->rSumWt += w;
		
		if (! p->approx ) p->approx = 1;
	}
};

static void wvarFinalize(sqlite3_context *context) {
	WVarTerm *p;
	p = sqlite3_aggregate_context(context, 0);
	if( p && p->cnt>0 ){
		if (p->approx) {
			sqlite3_result_double(context, sqrt(p->rTerm / p->rSumWt) );
		} else {
			sqlite3_result_double(context, sqrt( ( p->rTerm) / (double)p->iSumWt)  );
		}
	}
};


//Step the function wavgerr.
//Query form: wavgerr( x, errors_on_x, wavg_of_x )
static void wavgerrStep(sqlite3_context *context, int argc, sqlite3_value **argv){
	WVarTerm *p;
	int q_type, m_type, e_type;
	
	if (argc<3) return;

	p = sqlite3_aggregate_context( context, sizeof(*p) );
	q_type = sqlite3_value_numeric_type( argv[0] );
	e_type = sqlite3_value_numeric_type( argv[1] );
	m_type = sqlite3_value_numeric_type( argv[2] );
	
	if (p && (q_type == SQLITE_NULL || e_type == SQLITE_NULL || m_type == SQLITE_NULL) ) return;
	
	if (p->cnt == 0) {
		p->rTerm=0;
		p->rSumWt=0;
		p->rSumEr=0;
	}
	
	double wavg = sqlite3_value_double(argv[2]);
	p->cnt++;
	
	double q, err;
	
	if ( e_type == SQLITE_INTEGER ) {
		err = (double)sqlite3_value_int64(argv[1]);
	} else {
		err = sqlite3_value_double(argv[1]);
	}
	
	if ( q_type == SQLITE_INTEGER ){
		q = (double)sqlite3_value_int64(argv[0]);
	} 
	else {
		q = sqlite3_value_double(argv[0]);
	}
	
	p->rTerm += ( pow( ( q - wavg ) / err, 2.0 ) );
	p->rSumEr += pow(1.0 / err, 2.0);

};


static void wavgerrFinalize(sqlite3_context *context) {
	WVarTerm *p;
	p = sqlite3_aggregate_context(context, 0);
	
	if( p && p->cnt>1 ){
		double redChisq = p->rTerm / (p->cnt-1);
		if ( redChisq < 1.0 ) redChisq = 1.0;
		double wavgerr = sqrt(1.0 / p->rSumEr) * sqrt( redChisq );
		sqlite3_result_double(context, wavgerr );
	} else {
		sqlite3_result_double(context, sqrt(1.0 / p->rSumEr) );
	}
};





/* An exported routine to simply add the function wavg to 
* the database db.  This way clients need only have a
* database instance, and need not know the details of the wavg function.
*/

int sqlite3AddMyFunc_sepang(sqlite3 * db) {

	const char fname[] = "sepang";
	int nArgs = 5;
	int textEnc = SQLITE_ANY;
	
	int status = sqlite3_create_function(
		db,
		fname,
		nArgs,
		textEnc,
		0,
		&sepang,
		0,
		0
	);

	return status;
};


int sqlite3AddMyFunc_linbin(sqlite3 * db) {

	const char fname[] = "linbin";
	int nArgs = 3;
	int textEnc = SQLITE_ANY;
	
	int status = sqlite3_create_function(
		db,
		fname,
		nArgs,
		textEnc,
		0,
		&linbin,
		0,
		0
	);

	return status;
};


int sqlite3AddMyFunc_wavg(sqlite3 * db) {

	const char fname[] = "wavg";
	int nArgs = 2;
	int textEnc = SQLITE_ANY;
	
	int status = sqlite3_create_function(
		db,
		fname,
		nArgs,
		textEnc,
		0,
		0,
		&wavgStep,
		&wavgFinalize
	);

	return status;
};

int sqlite3AddMyFunc_wvar(sqlite3 * db) {

	const char fname[] = "wvar";
	int nArgs = 3;
	int textEnc = SQLITE_ANY;
	
	int status = sqlite3_create_function(
		db,
		fname,
		nArgs,
		textEnc,
		0,
		0,
		&wvarStep,
		&wvarFinalize
	);

	return status;
};

int sqlite3AddMyFunc_wavgerr(sqlite3 * db) {

	const char fname[] = "wavgerr";
	int nArgs = 3;
	int textEnc = SQLITE_ANY;
	
	int status = sqlite3_create_function(
		db,
		fname,
		nArgs,
		textEnc,
		0,
		0,
		&wavgerrStep,
		&wavgerrFinalize
	);

	return status;
};
