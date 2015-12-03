/*
 *  time2met.hh
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 8/16/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#ifndef TIME2MET_HH
#define TIME2MET_HH


#ifdef SP_EXE_UTITILS_H
#undef SP_EXE_UTITILS_H
#endif

#ifdef UTC_FMT_DEFAULT
#undef UTC_FMT_DEFAULT
#endif

#define UTC_FMT_DEFAULT "%Y-%m-%d %H:%M:%S.nnnnnn"

#include "spoccExeUtilities.h"
#include "DBStringUtilities.hh"
#include <iomanip>


#endif