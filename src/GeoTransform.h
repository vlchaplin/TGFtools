/*
 *  GeoTransform.h
 *
 *  Created by Vandiver L. Chapin on 3/8/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#ifndef GEOXFORM
#define GEOXFORM

#include <math.h>
#include <iostream>
#include <iomanip>

#include "fitsio.h"
#include "gbm_geometry.hh"
#include "DBStringUtilities.hh"


using namespace std;
using namespace geom;


/*
* Constants, formulas and transformation equations from
* M. Hapgood, Rutherford Appleton Labratory Space Science and Technology Dept.
* http://sspg1.bnsc.rl.ac.uk/Share/Coordinates/geo_tran.htm
*/


//WGS 84 ellipsoid and 1/f. Axes in meters
#define EARTH_SEMI_MAJ 6378137.0
#define EARTH_SEMI_MIN 6356752.3
#define EARTH_AVG_RAD 6378000.0
#define EARTH_INV_F 298.257

//#define CORR_PRECESS_NUTATE

const double earth_ecc = sqrt( EARTH_SEMI_MAJ*EARTH_SEMI_MAJ - EARTH_SEMI_MIN*EARTH_SEMI_MIN ) / EARTH_SEMI_MAJ;
const double earth_ee = earth_ecc*earth_ecc;
//const double earth_med_rad = (EARTH_SEMI_MAJ + EARTH_SEMI_MIN) / 2.0

inline double N_crv (double& lat) {
	return EARTH_SEMI_MAJ*1.0 / sqrt(1.0 - (1.0/EARTH_INV_F)*(2.0 - (1.0/EARTH_INV_F))*sin(lat)*sin(lat) );
};

struct jtime {
	double jcn;
	double mjd;
	friend ostream& operator<<(ostream& os, jtime& obj) {
		os << setw(8) << "MJD: ";
		os << setprecision(8) << obj.mjd << endl;
		os << setw(8) << "J.Cent: ";
		os << setw(8) << obj.jcn << endl;
		os << setw(8) << "SoD: ";
		os << setw(8) << (obj.mjd - floor(obj.mjd))*86400.0 << endl;
		return os;
	};
};

typedef x3 vGeoInertial;

struct vGeodetic {
	double h; //meters
	double lat; //radians
	double lon;
};
x3& xyz_vec(vGeodetic& vgeo, x3& xyz);

struct vGeographic {
	double r; //meters
	double lat;
	double lon;
	inline vGeographic& operator=(vGeodetic& vgd) {
		
		x3 temp;
		cart2sphere( xyz_vec( vgd, temp ), r, lon, lat );
		
		return *this;
	};
};
x3& xyz_vec(vGeographic& vgeo, x3& xyz);


inline jtime met2jcent( long double met ) {
	jtime mt;
	mt.mjd = glast_met2mjd(met);
	//2451545.0
	mt.jcn = (mt.mjd - 51544.5) / 36525.0;
	return mt;
};

inline double gst( jtime& mt ) {
	//cout << mt;
	#ifdef USE_HAPGOOD_GST_FORMULA 
	double hrOfDay = (mt.mjd - floor(mt.mjd))*24.0;
	double siderealDeg = ( 100.461 + 36000.770 * mt.jcn + 15.04107 * hrOfDay);
	#endif

	#ifndef USE_HAPGOOD_GST_FORMULA
	double siderealDeg = 280.46061837  +  360.98564736629 * ( mt.jcn * 36525.0 ) + 0.000387933 * mt.jcn * mt.jcn;
	#endif
	//cout << "Sidereal angle: " << siderealDeg << endl;
	
	return deg2rad(siderealDeg);
};

vGeoInertial geo2gei( long double met, vGeodetic& point );
vGeoInertial geo2gei( long double met, vGeographic& point );
vGeographic gei2geo( long double met, vGeoInertial point );


Rzyz& precession_op( Rzyz& m, jtime& mt );

#endif