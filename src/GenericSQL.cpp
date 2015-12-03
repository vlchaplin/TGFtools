/*
 *  GenericSQL.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 6/3/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#include "GenericSQL.h"

using namespace std;

GenericSQL::GenericSQL() {};

int GenericSQL::get_column( string colname, vector<string>& val, string condition )
{

	sqlite3_stmt * ppStmt;
	
	string qry = "select "+colname+" from "+table+" "+condition;
	
	sqlite3_prepare_v2( db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	int rc = sqlite3_step(ppStmt);

	if ( rc == SQLITE_DONE ) {
		sqlite3_finalize(ppStmt);
		return SQLITE_DONE;
	} else if ( rc != SQLITE_ROW ) {
		cout << sqlite3_errmsg( this->db ) << "   ("<<rc<<")" << endl;
		sqlite3_finalize(ppStmt);
		return rc;
	}

	while ( rc == SQLITE_ROW ) {
		
		val.push_back( string((char*)sqlite3_column_text( ppStmt, 0) ) );
		
		rc = sqlite3_step(ppStmt);
	}
	
	sqlite3_finalize(ppStmt);
	
	return SQLITE_ROW;
}
int GenericSQL::get_column( string colname, vector<int>& val, string condition )
{

	sqlite3_stmt * ppStmt;
	
	string qry = "select "+colname+" from "+table+" "+condition;
	
	sqlite3_prepare_v2( db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	int rc = sqlite3_step(ppStmt);

	if ( rc == SQLITE_DONE ) {
		sqlite3_finalize(ppStmt);
		return SQLITE_DONE;
	} else if ( rc != SQLITE_ROW ) {
		sqlite3_finalize(ppStmt);
		cout << sqlite3_errmsg( this->db ) << "   ("<<rc<<")" << endl;
		return rc;
	}

	while ( rc == SQLITE_ROW ) {
		
		val.push_back( sqlite3_column_int( ppStmt, 0) );
		
		rc = sqlite3_step(ppStmt);
	}
	sqlite3_finalize(ppStmt);
	return SQLITE_ROW;
}
int GenericSQL::get_column( string colname, vector<float>& val, string condition )
{

	sqlite3_stmt * ppStmt;
	
	string qry = "select "+colname+" from "+table+" "+condition;
	
	sqlite3_prepare_v2( db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	int rc = sqlite3_step(ppStmt);

	if ( rc == SQLITE_DONE ) {
		sqlite3_finalize(ppStmt);
		return SQLITE_DONE;
	} else if ( rc != SQLITE_ROW ) {
		sqlite3_finalize(ppStmt);
		cout << sqlite3_errmsg( this->db ) << "   ("<<rc<<")" << endl;
		return rc;
	}

	while ( rc == SQLITE_ROW ) {
		
		val.push_back( sqlite3_column_double( ppStmt, 0) );
		
		rc = sqlite3_step(ppStmt);
	}
	sqlite3_finalize(ppStmt);
	return SQLITE_ROW;
}
int GenericSQL::get_column( string colname, vector<double>& val, string condition )
{

	sqlite3_stmt * ppStmt;
	
	string qry = "select "+colname+" from "+table+" "+condition;
	
	sqlite3_prepare_v2( db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	int rc = sqlite3_step(ppStmt);

	if ( rc == SQLITE_DONE ) {
		sqlite3_finalize(ppStmt);
		return SQLITE_DONE;
	} else if ( rc != SQLITE_ROW ) {
		sqlite3_finalize(ppStmt);
		cout << sqlite3_errmsg( this->db ) << "   ("<<rc<<")" << endl;
		return rc;
	}

	while ( rc == SQLITE_ROW ) {
		
		val.push_back( sqlite3_column_double( ppStmt, 0) );
		
		rc = sqlite3_step(ppStmt);
	}
	sqlite3_finalize(ppStmt);
	return SQLITE_ROW;
}