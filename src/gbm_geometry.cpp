/*
 *  gbm_geometry.cpp
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 10/20/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */
 
 /* 
	GBM Detector Orientations (0-13):
	
	X-Y-Z:
      0.24485110,      0.25266675,     0.93605953
      0.50173315,      0.50348756,     0.70339471
      0.52398269,      0.85172176,  -0.0034905982
      0.50086576,     -0.50261728,     0.70463420
      0.54755590,     -0.83675273,   -0.0052360171
      0.99823375,     0.059306014,    0.0034905982
     -0.24690749,     -0.24604707,      0.93728199
     -0.51305109,     -0.50593720,      0.69340185
     -0.55047981,     -0.83484665,    0.0017453017
     -0.50610162,      0.50258069,      0.70090926
     -0.55483086,      0.83193388,   -0.0069812869
     -0.99790194,   -0.064531370,   -0.0052360171
       1.0000000,      0.0000000,       0.0000000
      -1.0000000,      0.0000000,       0.0000000

	Az-Zen:
	   45.900002       20.600000
       45.099998       45.299999
       58.400002       90.199997
       314.89999       45.200001
       303.20001       90.300003
       3.4000001       89.800003
       224.89999       20.400000
       224.60001       46.099998
       236.60001       89.900002
       135.20000       45.500000
       123.70000       90.400002
       183.70000       90.300003
       0.0000000       90.000000
       180.00000       90.000000
 */ 
 
#include "gbm_geometry.hh"

using namespace geom;

#ifndef GLOBALPI 
#define GLOBALPI
double geom::PI = 4.0*atan(1.0);
double geom::DTOR = PI / 180.0;
#endif

int gbmDetname2Num(char * name) 
{
	static const char delim[] = "_";
	
	size_t namelength = strlen(name);
	char * nameCopy = new char[namelength+1];
	
	strcpy(nameCopy, name);
	
	int det;
	char * grp=NULL;
	char * num=NULL;
	grp = strtok(nameCopy, delim);
	if (grp == NULL) {
		delete nameCopy;
		return -1;
	}
	else {
		if ( strchr( grp, 'N' ) != NULL ) det = 0;
		else if ( strchr( grp, 'B' ) != NULL ) det = 12;
	}
	
	num = strtok(NULL, delim);
	if (num == NULL) {
		delete nameCopy;
		return -1;
	}else {
		det += atoi( num );
	}
	
	if (det > 13 || det < 0) {
		delete nameCopy;
		return -1;
	}
	
	delete nameCopy;
	return det;
}

string gbmDetShortname(int det) {

	char sname[4];
	if ( det <= 11 ) sprintf(sname, "n%x", det );
	else if ( det <= 13 ) sprintf( sname, "b%1d", det % 12 );
	else sprintf( sname, "d%d", det );
	
	return string(sname);
};

string gbmDetShortname(char * name) 
{
	int det = gbmDetname2Num(name);
	
	return gbmDetShortname(det);
}

Rmatrix geom::I3 = Rmatrix(1.0, 1.0, 1.0);

GBM_Geometry::GBM_Geometry() {
	
	//this->PI = 4.0*atan(1.0);
	//this->DTOR = this->PI / 180.0;
	
	vector<double> v(3);
	
	normals.resize(14,3);
	
	normals[0]  = vec3(v, 0.24485110,      0.25266675,     0.93605953);
	normals[1]  = vec3(v, 0.50173315,      0.50348756,     0.70339471);
	normals[2]  = vec3(v, 0.52398269,      0.85172176,  -0.0034905982);
	normals[3]  = vec3(v, 0.50086576,     -0.50261728,     0.70463420);
	normals[4]  = vec3(v, 0.54755590,     -0.83675273,   -0.0052360171);
	normals[5]  = vec3(v, 0.99823375,     0.059306014,    0.0034905982);
	normals[6]  = vec3(v,-0.24690749,     -0.24604707,      0.93728199);
	normals[7]  = vec3(v,-0.51305109,     -0.50593720,      0.69340185);
	normals[8]  = vec3(v,-0.55047981,     -0.83484665,    0.0017453017);
	normals[9]  = vec3(v,-0.50610162,      0.50258069,      0.70090926);
	normals[10] = vec3(v,-0.55483086,      0.83193388,   -0.0069812869);
	normals[11] = vec3(v,-0.99790194,    -0.064531370,   -0.0052360171);
	normals[12] = vec3(v,  1.0000000,       0.0000000,       0.0000000);
	normals[13] = vec3(v, -1.0000000,       0.0000000,       0.0000000);
	

};

const vector<double>& GBM_Geometry::detNormal( size_type det ) {
	
	if (det < 0 || det > 13) {
		cout << "Error, detector number out of range at detNormal()" << endl;
		return normals[12];
	}
	
	return normals[det];
}

vector<double> GBM_Geometry::copyDetNormal( size_type det ) {
	
	if (det < 0 || det > 13) {
		cout << "Error, detector number out of range at copyDetNormal()" << endl;
		vector<double> empty;
		return empty;
	}
	
	return normals[det];
}

double GBM_Geometry::ang2det(size_type det, double s_x, double s_y, double s_z, int cos) {

	if (det < 0 || det > 13) {
		cout << "Error, detector number out of range at ang2det()" << endl;
		return -1.0;
	}
	
	vector<double> v(3);
	
	double angle = vdot(	
					vec3(v, s_x, s_y, s_z), 
					normals[det],
					cos
				);
	
	return angle;
};

double GBM_Geometry::ang2det(size_type det, double s[2], int cos ) {

	if (det < 0 || det > 13) {
		cout << "Error, detector number out of range at ang2det()" << endl;
		return -1.0;
	}
	
	vector<double> v(3);
	sphere2cart( v, s[0], s[1] );
	
	double angle = vdot(	
					v, 
					normals[det],
					cos
				);
	
	return angle;
};

double GBM_Geometry::ang2det(size_type det, const vector<double>& s, int cos) {
	
	if (det < 0 || det > 13) {
		cout << "Error, detector number out of range at ang2det()" << endl;
		return -1.0;
	}
	
	double angle = vdot(	
					s, 
					normals[det],
					cos
				);
	
	return angle;
};

double GBM_Geometry::ang2det(size_type det, vector<double>& s, int cos) {

	if (det < 0 || det > 13) {
		cout << "Error, detector number out of range at ang2det()" << endl;
		return -1.0;
	}
	
	double angle = vdot(	
					s, 
					normals[det],
					cos
				);
	
	return angle;
};
