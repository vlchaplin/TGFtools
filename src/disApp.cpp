/*
 *  disApp.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 6/1/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#include "disApp.h"

disApp * disApp::theApp = NULL;

disApp * disApp::Instance () {
	if ( theApp == NULL ) {
		theApp = new disApp;
	}
	return theApp;
};

void disApp::setAppData (void * ptr) {
	this->appData = ptr;
};
void disApp::setAppWidgets (void * ptr) {
	this->appWidgets = ptr;
};
void * disApp::getAppData () {
	return this->appData;
};
void * disApp::getAppWidgets () {
	return this->appWidgets;
};