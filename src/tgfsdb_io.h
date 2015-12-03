/*
 *  tgfsdb_io.h
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 3/14/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#ifndef TGFSDB_IO_H
#define TGFSDB_IO_H

#include <cstdio>
#include <cmath>
#include <iostream>

#include "sqlite3.h"

#include "my_sqlite_func.h"

#include "GenericSQL.h"
#include "PHA_IO.hh"
#include "VectorSpan.h"
#include "spoccExeUtilities.h"
#include "DBStringUtilities.hh"
#include "gbm_geometry.hh"
#include "GeoTransform.h"

#define TGFTLS_CANON_ID(object, detector) string("tgf"+string(object)+"_")+detector
#define TGFTLS_OUTFILE_CANON_NAME(prefix, object, detector, extension) string(string(prefix)+"_"+TGFTLS_CANON_ID(object, detector)+extension)
#define TTX_CANON_NAME(object, detector) TGFTLS_OUTFILE_CANON_NAME("ttx", object, detector, ".fit")
#define RSP_CANON_NAME(object, detector) TGFTLS_OUTFILE_CANON_NAME("rsp", object, detector, ".rsp")


#define ASSOC_FITS_TABLE (char*)"STROKES"



#define DB_EXPANDED_ERRMSG(db) \
cerr << "Error in " << __FILE__ << ":" << __func__ << "(), ln. " << __LINE__ << endl; \
cerr << sqlite3_errmsg( db ) << endl; 


class GbmDataDb {
	
	/*data model:
	CREATE TABLE IF NOT EXISTS tte (
		filename char(25),
		path char(120),
		utc_start DATETIME,
		utc_stop DATETIME,
		tmin FLOAT,
		tmax FLOAT,
		det char(6),
		type char(12),
		UNIQUE (filename)
	);
	*/
	
	public:
	
	struct result_set {
		double tmin;
		double tmax;
		string file;
		string path;
		string det;
		string type;
	};
	
	private:
	
	sqlite3 * db;
	string dbfile;
	long sqliteflags;
	
	void get_step_data( sqlite3_stmt *& ppStmt, result_set& data);
	
	public:
	
	bool printQry;
	string table;
	
	GbmDataDb();
	GbmDataDb( string file );
	
	string getDbFile() { return dbfile; };
	
	int open_db( string dbfile );
	int close_db() { return sqlite3_close( this->db ); } ;
	inline void set_open_flags(long flags) {
		this->sqliteflags = flags;
	};
	inline sqlite3 * get_db_ptr() {
		return this->db;
	};
	
	int getCoveringData( double metstart, double metstop, vector<result_set>& output, string where="", string postOp="" );
	

};

class TGFdb {

	/* data model for tgflist
	CREATE TABLE IF NOT EXISTS tgflist (
		tgfid char(12),
		utc DATETIME,
		fsec FLOAT,
		met FLOAT,
		PRIMARY KEY(tgfid)
	);
	*/
	public:
	
	bool getLcParams;
	
	struct histGenParams {
		double xs;
		double xe;
		double dx;
	};
	
	struct result_set {
		string tgfid;
		string utc;
		double fsec;
		double met;
		double tstart;
		double tstop;
		int colmask;
		
		histGenParams * lcview;
		
		result_set() {
			colmask = 0;
			lcview = NULL;
		}
	};
	
	static const int tstartcol;
	static const int tstopcol;
	static const int XScol;
	static const int XEcol;
	static const int DXcol;
	static const int tstartmask;
	static const int tstopmask;
	
	

	private:
	sqlite3 * db;
	string dbfile;
	long sqliteflags;
	
	
	void get_step_data( sqlite3_stmt *& ppStmt, result_set& data);
	
	
	public:
	bool printQry;
	string table;
	
	TGFdb();
	TGFdb( string dbfile );
	~TGFdb() {
		close_db();
	};
	
	int open_db( string dbfile );
	int close_db() { return sqlite3_close( this->db ); } ;
	
	inline void set_open_flags(long flags) {
		this->sqliteflags = flags;
	};
	inline sqlite3 * get_db_ptr() {
		return this->db;
	};

	int getEvents(vector<result_set>& output, string whr = "");

};

class StrokeDb {

	/* data model for stroke table
	CREATE TABLE IF NOT EXISTS lightning (
		utc DATETIME,
		fsec FLOAT,
		tgfid char(12),
		lon FLOAT,
		lat FLOAT,
		source char(10) ,
		UNIQUE (utc ASC,fsec ASC)
	);
	*/
	public:
	struct result_set {
		public:
		string tgfid;
		string utc;
		string locsrc;
		double fsec;
		double lon;
		double lat;
		double t_offset;
		double x_offset;
	};

	private:
	sqlite3 * db;
	string dbfile;
	long sqliteflags;
	
	
	void get_step_data( sqlite3_stmt *& ppStmt, result_set& data);
	
	
	public:
	
	string table;
	
	StrokeDb();
	StrokeDb( string dbfile );
	~StrokeDb() {
		close_db();
	};
	
	int open_db( string dbfile );
	int close_db() { return sqlite3_close( this->db ); } ;
	
	inline void set_open_flags(long flags) {
		this->sqliteflags = flags;
	};

	inline sqlite3 * get_db_ptr() {
		return this->db;
	};

	int getEvents(vector<result_set>& output, string whr = "");
	int getNearestEvents(vector<result_set>& output, string utctime, long int tdiff = 0, string whr = "");
	int getSpaceTimeEvents(vector<result_set>& output, string utctime, double refLon, double refLat, 
		long int tdiff = 0, double radSep=300.0, string whr = "");
	int getSpaceTimeEvents(vector<result_set>& output, double evMet, double refLon, double refLat, 
		double tdiff = 0, double radSep=300.0, string whr = "");


};

class AssociationDb {
	
	/* data model for assoc table
	CREATE TABLE IF NOT EXISTS assoc (
		tgfid char(12),
		tmet FLOAT,
		sutc DATETIME,
		sfsec FLOAT,
		tdiff FLOAT, 
		lon FLOAT,
		lat FLOAT,
		az FLOAT,
		el FLOAT,
		RA FLOAT,
		Dec FLOAT,
		bmAng FLOAT,
		dxSrc FLOAT,
		dxBeam FLOAT,
		dxSurf FLOAT,
		pos_x FLOAT,
		pos_y FLOAT,
		pos_z FLOAT,
		qs FLOAT,
		qx FLOAT,
		qy FLOAT,
		qz FLOAT,
		source char(10),
		UNIQUE (tgfid, sutc ASC, sfsec ASC)
	);
	*/
	
	public:
	struct result_set {
	
		TGFdb::result_set tgf;
		StrokeDb::result_set stroke;
		geom::gquat qEv;
		geom::x3 xEv;
		double postime;
		double ra;
		double dec;
		double az;
		double el;
		double bmAng;
		double dxSrc;
		double dxBeam;
		double dxGnd;
		
		int bind(sqlite3_stmt * ppStmt) {
			
			int c=1;
			
			//cout << "Binding '" << tgf.tgfid << "' , " << stroke.utc << "+"<< stroke.fsec << endl;
			
			int rc = sqlite3_bind_text(  ppStmt, c++, tgf.tgfid.c_str(), -1, SQLITE_STATIC );
			
			sqlite3_bind_double(ppStmt,c++, tgf.met );
			sqlite3_bind_text(  ppStmt,c++, stroke.utc.c_str(), -1, SQLITE_STATIC );
			sqlite3_bind_double(ppStmt,c++, stroke.fsec );
			sqlite3_bind_double(ppStmt,c++, stroke.t_offset );
			sqlite3_bind_double(ppStmt,c++, stroke.lon );
			sqlite3_bind_double(ppStmt,c++, stroke.lat );
			
			sqlite3_bind_double(ppStmt,c++, az );
			sqlite3_bind_double(ppStmt,c++, el );
			sqlite3_bind_double(ppStmt,c++, ra );
			sqlite3_bind_double(ppStmt,c++, dec );
			sqlite3_bind_double(ppStmt,c++, bmAng );
			sqlite3_bind_double(ppStmt,c++, dxSrc );
			sqlite3_bind_double(ppStmt,c++, dxBeam );
			sqlite3_bind_double(ppStmt,c++, dxGnd );
			
			sqlite3_bind_double(ppStmt,c++, xEv.x0 );
			sqlite3_bind_double(ppStmt,c++, xEv.x1 );
			sqlite3_bind_double(ppStmt,c++, xEv.x2 );
			sqlite3_bind_double(ppStmt,c++, qEv.qs );
			sqlite3_bind_double(ppStmt,c++, qEv.qx );
			sqlite3_bind_double(ppStmt,c++, qEv.qy );
			sqlite3_bind_double(ppStmt,c++, qEv.qz );
		
			sqlite3_bind_text(ppStmt,  c++, stroke.locsrc.c_str(), -1, SQLITE_STATIC );
		
			return rc;
		};
	};

	private:
	sqlite3 * db;
	string dbfile;
	
	void get_step_data( sqlite3_stmt *& ppStmt, result_set& data);
	
	public:
	
	string table;
	long sqliteflags;
	bool printQry;
	
	AssociationDb(){
	
		char * env = getenv("MATCH_TABLE");
		if ( env == NULL ) table = "assoc"; else table = string(env);
	
		printQry = 0;
		this->sqliteflags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	};
	~AssociationDb() {
		close_db();
	};
	
	int open_db( string dbfile );
	int close_db() { return sqlite3_close( this->db ); } ;
	inline void set_db_ptr(sqlite3 * ptr) {
		this->db = ptr;
	};
	inline sqlite3* get_db_ptr() {
		return this->db;
	};
	
	result_set& data2record ( result_set& data, 
							TGFdb::result_set& tgf, 
							StrokeDb::result_set& stroke, 
							geom::x3& pos, 
							geom::gquat& qEv, 
							double& posTime  );

	int CreateTable();
	int getEntries( vector<result_set>& , string whr="", string groupby="" );
	int getEntries( vector<result_set*>& , string whr="", string groupby="" );
	int getKnownMatches(vector<result_set>& , string );
	int getStrokeCount(string& tgfid);
	int commitAssociation( TGFdb::result_set& tgf, StrokeDb::result_set& stroke, geom::x3 xEv, geom::gquat& qEv, double& posTime );
	int makeEntry( result_set& entry, string on_conflict="" );

};

class CoordConvJoinedSet {
	public:
	TGFdb::result_set    gbmTgfEntry;
	StrokeDb::result_set strokeEntry;
	geom::gquat qEv;
	geom::x3 xEv;
};

class TransfSrcCrd {
	public:
	TGFdb::result_set    gbmTgfEntry;
	StrokeDb::result_set strokeEntry;
	geom::gquat qEv;
	geom::x3 xEv;
	geom::x3 stroke_ScCoords;
	geom::x3 geocen_ScCoords;
	geom::x3 stroke_EciCoords;
	int flag;
	double dataTime;
};


int AddTGFStroke_fitsTable( string file, std::vector<TransfSrcCrd>& events ) ;

int AddTGFStroke_fitsTable( string file, std::vector<AssociationDb::result_set>& events ) ;


#endif