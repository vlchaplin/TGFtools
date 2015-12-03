AUTOMAKE_OPTIONS = foreign

if DEF_DATADB
gbmdatabase = $(prefix)/GbmDataArc.db
else
gbmdatabase = @DATADB@
endif

if GBMRSP_DEF
rsgenpdir = "getenv(\"GBMRSPDIR\")"
else
rsgenpdir = "\"@GBMRSP@/\""
endif

if DRMDB_DEF
drmdbdir = "getenv(\"GBMDRMDB002\")"
else
drmdbdir = "\"@DRMDB@/\""
endif

fits_templatesdir = $(prefix)/fits_templates
initdir = $(prefix)
perllibdir = $(prefix)/lib/perl
scriptsdir = $(prefix)/scripts

if USE_HEADAS
HEASOFT_CFLAGS= -I@HEADAS@/include
HEASOFT_LFLAGS= -L@HEADAS@/lib -lcfitsio
else
HEASOFT_CFLAGS= @LHEAS_CFLAGS@
HEASOFT_LFLAGS= @LHEAS_LDFLAGS@ -lcfitsio
endif

if USE_DISLIN
DISLIN_CFLAGS= -DHAVE_DISLIN=1 -I@DISLIN@
DISLIN_LFLAGS= -L@DISLIN@ -ldislin
else

endif

if PUTVERSN
test=putversn
else
test=dontputversn
endif

ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

mylinkerflags = $(HEASOFT_LFLAGS) @SQLITE_LDFLAGS@ -lsqlite3

mymacros = \
	 \
	-DPKG_PREFIX="\"$(prefix)\"" \
	-DPKG_LDFLAGS="\"$(mylinkerflags)\"" \
	-DPKG_CFLAGS="\"-Wall $(ACLOCAL_AMFLAGS) $(DISLIN_CFLAGS) $(HEASOFT_CFLAGS) @SQLITE_CFLAGS@\"" \
	-DUTC_FMT_DEFAULT="\"%Y-%m-%d %H:%M:%S.nnnnnn\"" \
	-DPHA1TEMPLATE=\"$(fits_templatesdir)/pha1template.pha\" \
	-DPHA2TEMPLATE=\"$(fits_templatesdir)/pha2template.pha\" \
	-DTTETEMPLATE_FIT=\"$(fits_templatesdir)/tte.fit\" \
	-DRSPGEN_INSTALL_DIR=$(rsgenpdir) \
	-DGBM_DRMDB_INSTALL_PATH=$(drmdbdir) \
	-DWHICH_RM="\"@WHICH_RM@\"" \
	-DWHICH_ENV="\"@WHICH_ENV@\"" \
	-DTTE_DEFAULT_PAD=10.0
	
#mymacros += -DTGFDB=\"$(prefix)/GbmTgfs.db\"
#mymacros += -DTTEARCH=\"$(gbmdatabase)\"

AM_CPPFLAGS= -Wall \
	$(mymacros) \
	$(DISLIN_CFLAGS) $(HEASOFT_CFLAGS) @SQLITE_CFLAGS@

EXTRA_DIST = README

dist_init_SCRIPTS = tgftools_init.sh tgftools_init.csh
dist_data_DATA = \
	data/gbmtrig_tgfs1.txt \
	data/lat_tgfs.txt \
	data/other_tgfs.txt \
	data/wwlln_pos_trig_for_msb.txt \
	data/wwlln_pos_untrig_for_msb.txt \
	data/wwlln_strikes1.txt

dist_fits_templates_DATA = data/fits_templates/pha1template.pha data/fits_templates/pha2template.pha data/fits_templates/tte.fit
dist_scripts_SCRIPTS = \
	src/scripts/add_strokes.pl \
	src/scripts/add_tgfs.pl \
	src/scripts/posharchive.pl \
	src/scripts/ttearchive.pl \
	src/scripts/show_posdata.sh \
	src/scripts/show_ttedata.sh \
	src/scripts/show_strokes.sh \
	src/scripts/show_tgfs.pl \
	src/scripts/show_matches.pl \
	src/scripts/remove_strokes.pl \
	src/scripts/remove_tgfs.pl \
	src/scripts/remove_matches.pl \
	src/scripts/tgf_data_report.pl
	
dist_perllib_SCRIPTS = \
	src/scripts/GbmTime.pm \
	src/scripts/FITSUtils.pm

mysrccodebase = \
	src/DBStringUtilities.cpp \
	src/DBStringUtilities.hh \
	src/DataRecorder.hh \
	src/DynMatrix.h \
	src/EdgeSet.hh \
	src/PHA_IO.cpp \
	src/PHA_IO.hh \
	src/PHA2_IO.cpp \
	src/PHA2_IO.hh \
	src/PHAElements.hh \
	src/PHAStructures.cpp \
	src/PHAStructures.hh \
	src/TTEventTable.hh \
	src/VectorSpan.h \
	src/gbm_geometry.cpp \
	src/gbm_geometry.hh \
	src/gbmrsp_interface.hh \
	src/my_sqlite_func.h \
	src/my_sqlite_func.c \
	src/GenericSQL.h \
	src/GenericSQL.cpp \
	src/spoccExeUtilities.cpp \
	src/spoccExeUtilities.h \
	src/GeoTransform.h \
	src/GeoTransform.cpp \
	src/Gbm_PositionIO.h \
	src/Gbm_PositionIO.cpp \
	src/tgfsdb_io.h \
	src/tgfsdb_io.cpp \
	src/algorithms_t.h \
	src/algorithms_t.cpp \
	src/TTE_IO.hh \
	src/TTE_IO.cpp
	
bin_PROGRAMS = get_tgf_coords get_match_coords get_fermi_nadir make_match_data integrate_tgf_times tgftools_config make_nadir_drms time2met

get_tgf_coords_LDADD = $(mylinkerflags)
get_match_coords_LDADD = $(mylinkerflags)
get_fermi_nadir_LDADD = $(mylinkerflags)
make_match_data_LDADD = $(mylinkerflags)
make_nadir_drms_LDADD = $(mylinkerflags)
integrate_tgf_times_LDADD = $(mylinkerflags)

get_tgf_coords_SOURCES = $(mysrccodebase) src/get_tgf_direction.cpp
get_match_coords_SOURCES = $(mysrccodebase) src/get_match_coords.cpp
get_fermi_nadir_SOURCES = $(mysrccodebase) src/get_fermi_nadir.cpp
make_match_data_SOURCES = $(mysrccodebase) src/make_match_data.cpp
make_nadir_drms_SOURCES = $(mysrccodebase) src/make_geocenter_drms.cpp
integrate_tgf_times_SOURCES = $(mysrccodebase) src/integrate_tgf_times.cpp

tgftools_config_LDADD = $(mylinkerflags)
tgftools_config_SOURCES = $(mysrccodebase) src/tgftools_config.cpp


time2met_SOURCES = src/time2met.hh src/spoccExeUtilities.cpp src/DBStringUtilities.cpp src/time2met.cpp


if USE_DISLIN
bin_PROGRAMS+=make_data_plot set_tgf_bounds
make_data_plot_LDADD = $(mylinkerflags) $(DISLIN_LFLAGS)
make_data_plot_SOURCES = $(mysrccodebase) src/disApp.cpp src/make_data_plot.cpp

set_tgf_bounds_LDADD = $(mylinkerflags) $(DISLIN_LFLAGS)
set_tgf_bounds_SOURCES = $(mysrccodebase) src/disApp.cpp src/set_tgf_bounds.cpp

endif





