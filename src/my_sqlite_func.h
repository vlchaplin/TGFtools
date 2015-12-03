/*
 *  my_sqlite_func.h
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 8/26/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */
 
#ifndef MY_SQLITE_FUNC
#define MY_SQLITE_FUNC


#ifdef __cplusplus
extern "C" {
#endif

#include "sqlite3.h"
#include "math.h"



int sqlite3AddMyFunc_linbin(sqlite3 * db);
int sqlite3AddMyFunc_sepang(sqlite3 * db);
int sqlite3AddMyFunc_wavg(sqlite3 * db);
int sqlite3AddMyFunc_wvar(sqlite3 * db);
int sqlite3AddMyFunc_wavgerr(sqlite3 * db);

#ifdef __cplusplus
};
#endif

#endif