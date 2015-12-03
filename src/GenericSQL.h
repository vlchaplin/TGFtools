/*
 *  GenericSQL.h
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 6/3/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */


#ifndef GENERICSQL_HH
#define GENERICSQL_HH

#include <cstdio>
#include <string>
#include <iostream>
#include <vector>

#include "sqlite3.h"

using namespace std;

class GenericSQL {


	public:
	
	sqlite3 * db;
	string table;
	
	GenericSQL();
	
	void set_db_ptr( sqlite3 * db ) { this->db = db; };
	void set_table ( string name ) { this->table = name; };
	
	int get_column( string colname, vector<string>& val, string condition="" );
	int get_column( string colname, vector<int>& val, string condition="" );
	int get_column( string colname, vector<float>& val, string condition="" );
	int get_column( string colname, vector<double>& val, string condition="" );

};

#endif