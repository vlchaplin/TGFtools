/*
 *  DataRecorder.hh
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 4/13/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */
 
#ifndef DATARECORDER_H
#define DATARECORDER_H

#include <cstdio>
#include <cstring>

#include <vector>
#include <iostream>
#include <sstream>
#include <typeinfo>

using namespace std;

class record {
	public:
	string line;
	record(string& text) {
		line.assign(text);
	};
	record(string text) {
		line.assign(text);
	};
	inline record& operator=(string& text) {
		this->line.assign(text);
		return *this;
	};
	inline record& operator+=(string& text) {
		this->line += text;
		return *this;
	};
	inline string operator+(string& text) {
		return this->line + text;
	};
	friend inline string operator+(string& base, record& rec) {
		return (base + rec.line);
	};
	friend inline ostream& operator<<(ostream& output, record& rec) {
		output << rec.line;
		return output;
	}
};

class DataRecorder : public ostream {

	protected:
	vector<record> log;
	
	public:
	string linePr;
	string lineBr;
	
	public:
	
	virtual DataRecorder& operator<<(stringstream& operand){
		//cout << "DataRecorder ss\n";
		log.push_back( record( string(operand.str()) ) );
		operand.str("");
		return *this;
	};
	template<typename O>
	DataRecorder& operator<<(O& operand){
		//cout << "DataRecorder\n";
		stringstream operandreformat;
		operandreformat << operand;
		//string string_operand;
		//string_operand = operandreformat.str();
		log.push_back( record( string(operandreformat.str()) ) );
		return *this;
	};
	DataRecorder& operator<<(DataRecorder& otherLog){		
		vector<record>::iterator otherLine;
		otherLine = otherLog.log.begin();
		while (otherLine != otherLog.log.end() ) {
			this->log.push_back( *otherLine );
			++otherLine;
		}
		return *this;
	};

	DataRecorder() {
		this->linePr = " *this: ";
		this->lineBr = " DD";
	};

	template<typename L,typename T>
	inline DataRecorder( const char *& file, L& line, T& object ) {
		stringstream operandreformat;
		string Lstring;
		operandreformat << line;
		Lstring = operandreformat.str();
	
		this->log.push_back( record(string("Log for: ") + string( typeid(T).name()) ) );
		this->log.push_back( record(string("From ") + string(file) + string(", line ") + Lstring ) );
		
		this->linePr = " *this: ";
		this->lineBr = " XX";
	};
/*	
	void LogMutation( const char * operation, const char * named_operand) {
		log.push_back( string(" *this ") + string(operation) + string(named_operand) );
	};
*/
	template<typename Op>
	inline void LogMutation( string& operation, Op& operand) {
		stringstream operandreformat;
		string string_operand;
		//operandreformat << operand;
		//string_operand = operandreformat.str();
		
		log.push_back( record( string(operation) ) );
		*this << operand;
		//this->operator<< < DataRecorder& >(operand);
	};

	template<typename Op>
	inline void LogMutation( const char * operation, Op& operand) {
		stringstream operandreformat;
		string string_operand;
		//operandreformat << operand;
		//string_operand = operandreformat.str();
		
		log.push_back( record(string(operation) ) );
		*this << operand;
	};

	int logLength() {
		return log.size();
	};
	
	void log2chars(char ** records, int maxRecords) {
		stringstream buffer;
		vector<record>::iterator line;
		int recordnum = 0;
		
		line = this->log.begin();
		while ( (recordnum < maxRecords) && (line != this->log.end()) ) {
			buffer << this->linePr << *line << this->lineBr;
			strcpy( records[recordnum], (buffer.str()).c_str() );
			++recordnum;
			++line;
		}
	
	};
	string& operator[](int i) {
		return log[i].line;
	};
	string operator()(int i) {
		
		return linePr + log[i].line + lineBr;
	};

	friend inline ostream& operator<<(ostream& output, DataRecorder& rec) {
		vector<record>::iterator line;
		line = rec.log.begin();
		while (line != rec.log.end() ) {
			output << rec.linePr + *line + rec.lineBr << endl;
			++line;
		}
		output << endl;
		return output;
	};
	

};



#endif