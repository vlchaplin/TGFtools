/*
 *  GeoTransform.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 3/8/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#include "GeoTransform.h"

x3& xyz_vec(vGeographic& vgeo, x3& xyz) {
	xyz[0] = vgeo.r*cos(vgeo.lat)*cos(vgeo.lon);
	xyz[1] = vgeo.r*cos(vgeo.lat)*sin(vgeo.lon);
	xyz[2] = vgeo.r*sin(vgeo.lat);
	
	return xyz;
};

x3& xyz_vec(vGeodetic& vgeo, x3& xyz) {
	double N = N_crv(vgeo.lat);
	
	xyz[0] = (N + vgeo.h)*cos(vgeo.lat)*cos(vgeo.lon);
	xyz[1] = (N + vgeo.h)*cos(vgeo.lat)*sin(vgeo.lon);
	xyz[2] = (N*(1.0 - earth_ee) + vgeo.h)*sin(vgeo.lat);
	
	return xyz;
};


vGeoInertial geo2gei( long double met, vGeodetic& point )
{
		x3 pos;
		jtime jt = met2jcent(met);
		Rz rm( gst(jt) );
		
		rm.tr();
		
		#ifdef CORR_PRECESS_NUTATE
		Rzyz prcsnCorr;
		precession_op( prcsnCorr, jt );
		prcsnCorr.tr();
		rm *= prcsnCorr;
		#endif
		
		rm.rot( xyz_vec( point, pos ) );
		return pos;
}

vGeoInertial geo2gei( long double met, vGeographic& point )
{
		x3 pos;
		jtime jt = met2jcent(met);
		Rz rm( gst(jt) );
		
		rm.tr();
		
		#ifdef CORR_PRECESS_NUTATE
		Rzyz prcsnCorr;
		precession_op( prcsnCorr, jt );
		prcsnCorr.tr();
		rm *= prcsnCorr;
		#endif
		
		rm.rot( xyz_vec( point, pos ) );
		return pos;
}

vGeographic gei2geo( long double met, vGeoInertial pos )
{
		jtime jt = met2jcent(met);
		Rz rm( gst(jt) );
		
		#ifdef CORR_PRECESS_NUTATE
		Rzyz prcsnCorr;
		precession_op( prcsnCorr, jt );
		prcsnCorr.rot( pos );
		#endif
		
		rm.rot( pos );
		
		vGeographic point;
		cart2sphere( pos, point.r, point.lon, point.lat );
		
		return point;
}

Rzyz& precession_op( Rzyz& m, jtime& mt ) 
{
	double jcn_sq = mt.jcn*mt.jcn;
	double euler_z, euler_yp, euler_zpp;
	euler_z = -0.64062*mt.jcn - 0.0003*jcn_sq;
	euler_yp = -0.55675*mt.jcn + 0.00012*jcn_sq;
	euler_zpp = -0.64062*mt.jcn - 0.00003*jcn_sq;
	
	euler_z = deg2rad( euler_z );
	euler_yp = deg2rad( euler_yp );
	euler_zpp = deg2rad( euler_zpp );
	
	euler_z*=-1;
	euler_zpp*=-1;
	m.set( euler_z, euler_yp, euler_zpp );
	return m;
}