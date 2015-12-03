/*
 *  gbmrsp_interface.hh
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 6/18/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#ifndef GBMRSP_INTERFACE_HH
#define GBMRSP_INTERFACE_HH

#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "EdgeSet.hh"
#include "DBStringUtilities.hh"

#ifndef GBM_DRMDB_INSTALL_PATH
#define GBM_DRMDB_INSTALL_PATH getenv("GBMDRMDB002")
#endif

#ifndef RSPGEN_INSTALL_DIR
#define RSPGEN_INSTALL_DIR "/gbm/fastcopy/software/"
#endif

using namespace std;

static EdgeSet<float> NaIPhotonPoints = EdgeSet<float>::logbingen(5.0, 50000.0, 140);
static EdgeSet<float> BGOPhotonPoints = EdgeSet<float>::logbingen(150.0, 60000.0, 140);

template<typename E>
inline void WriteNMLFile( char * caller, string& file, string& rspfile, string& object, EdgeSet<E>& edges, 
	int det, double az, double el, double tstart, double tstop)
{
	
	if ( RSPGEN_INSTALL_DIR == NULL || strlen(RSPGEN_INSTALL_DIR) == 0 ) {
		return;
	}
	if ( GBM_DRMDB_INSTALL_PATH == NULL || strlen(GBM_DRMDB_INSTALL_PATH) == 0 ) {
		return;
	}
	
	ofstream fstr;
	fstr.open( file.c_str(), ofstream::out );
	
	size_t nobins_out = edges.size();
	
	string endl = "\n";
	
	fstr << "&gbmrsp_inputs debug=0, detector="<<det<<", triflag=0," << endl;
	fstr << " read_one_drm=0, one_drm_path='none', one_drm_file='none'," << endl;
	fstr << "infile='"<< RSPGEN_INSTALL_DIR << "/inputs/gridpos_InitialGBMDRMdb001.dat'," << endl;
	fstr << "tripath='"<< RSPGEN_INSTALL_DIR << "/inputs/'," << endl;
	fstr << "trifile='InitialGBMDRM_triangleinfo_db001.fits'," << endl;
	fstr << "calling_code='" << caller << "'," << endl;
	
	fstr << "drmdbpath='"<< GBM_DRMDB_INSTALL_PATH << "',"<<endl;
	fstr << "nobins_in=140"<< ", nobins_out="<<nobins_out<<","<<endl;
	
	if (det < 12) 
	{
	fstr << "ebin_edge_in = " << NaIPhotonPoints[0][0] << "  " << str_join( NaIPhotonPoints[1], "  " ) << " ," << endl;
/*	fstr << "ebin_edge_in = " <<
  "  5  5.34  5.70312  6.09094  6.50513  6.94748            " << endl <<
  "  7.41991  7.92447  8.46333  9.03884  9.65349  10.3099   " << endl <<
  "  11.011  11.7598  12.5594  13.4135  14.3256  15.2997    " << endl <<
  "  16.3401  17.4513  18.638  19.9054  21.2589  22.7045    " << endl <<
  "  24.2485  25.8974  27.6584  29.5392  31.5479  33.6931   " << endl <<
  "  35.9843  38.4312  41.0446  43.8356  46.8164  50	    " << endl <<
  "  53.4  57.0312  60.9094  65.0513  69.4748  74.1991	    " << endl <<
  "  79.2446  84.6333  90.3884  96.5349  103.099  110.11    " << endl <<
  "  117.598  125.594  134.135  143.256  152.997  163.40    " << endl <<
  "  174.513  186.38  199.054  212.589  227.045  242.48     " << endl <<
  "  258.974  276.584  295.392  315.479  336.931  359.843   " << endl <<
  "  384.312  410.446  438.356  468.164  500  534           " << endl <<
  "  570.312  609.094  650.512  694.748  741.991  792.446   " << endl <<
  "  846.333  903.884  965.349  1030.99  1101.1  1175.98    " << endl <<
  "  1255.94  1341.35  1432.56  1529.97  1634.01  1745.13   " << endl <<
  "  1863.8  1990.54  2125.89  2270.45  2424.85  2589.74    " << endl <<
  "  2765.84  2953.92  3154.79  3369.31  3598.43  3843.12   " << endl <<
  "  4104.46  4383.56  4681.65  5000  5340  5703.12         " << endl <<
  "  6090.94  6505.12  6947.48  7419.91  7924.46  8463.33   " << endl <<
  "  9038.84  9653.49  10309.9  11011  11759.8  12559.4     " << endl <<
  "  13413.5  14325.6  15299.7  16340.1  17451.3  18637.9   " << endl <<
  "  19905.3  21258.9  22704.5  24248.5  25897.3  27658.4   " << endl <<
  "  29539.2  31547.8  33693.1  35984.3  38431.2  41044.6   " << endl <<
  "  43835.6  46816.4  50000  ,"<< endl;
*/

	}
	else 
	{
	fstr << "ebin_edge_in = " << BGOPhotonPoints[0][0] << "  " << str_join( BGOPhotonPoints[1], "  " ) << " ," << endl;
/*	fstr << "ebin_edge_in = " <<
	"   1000.0000       1029.6772       1060.2351       1091.6998 " << endl <<     
    "   1124.0984       1157.4584       1191.8085       1227.1780 " << endl <<
    "   1263.5972       1301.0971       1339.7100       1379.4688 " << endl <<
    "   1420.4075       1462.5612       1505.9659       1550.6586 " << endl <<
    "   1596.6778       1644.0627       1692.8538       1743.0929 " << endl <<
    "   1794.8229       1848.0882       1902.9342       1959.4079 " << endl <<
    "   2017.5575       2077.4329       2139.0852       2202.5672 " << endl <<
    "   2267.9332       2335.2390       2404.5422       2475.9022 " << endl <<
    "   2549.3800       2625.0383       2702.9420       2783.1577 " << endl <<
    "   2865.7539       2950.8013       3038.3728       3128.5430 " << endl <<
    "   3221.3893       3316.9910       3415.4299       3516.7902 " << endl <<
    "   3621.1585       3728.6242       3839.2792       3953.2181 " << endl <<
    "   4070.5384       4191.3404       4315.7275       4443.8061 " << endl <<
    "   4575.6856       4711.4790       4851.3023       4995.2752 " << endl <<
    "   5143.5208       5296.1659       5453.3411       5615.1808 " << endl <<
    "   5781.8234       5953.4115       6130.0919       6312.0156 " << endl <<
    "   6499.3383       6692.2202       6890.8263       7095.3265 " << endl <<
    "   7305.8956       7522.7139       7745.9667       7975.8450 " << endl <<
    "   8212.5454       8456.2705       8707.2286       8965.6344 " << endl <<
    "   9231.7090       9505.6799       9787.7816       10078.255 " << endl <<
    "   10377.349       10685.319       11002.429       11328.950 " << endl <<
    "   11665.161       12011.350       12367.813       12734.855 " << endl <<
    "   13112.789       13501.939       13902.639       14315.229 " << endl <<
    "   14740.065       15177.508       15627.933       16091.726 " << endl << 
   "   16569.283       17061.012       17567.335       18088.683 " << endl << 
   "   18625.504       19178.256       19747.412       20333.460 " << endl << 
   "   20936.899       21558.247       22198.034       22856.809 " << endl <<  
   "   23535.134       24233.590       24952.774       25693.302 " << endl <<  
   "   26455.806       27240.939       28049.373       28881.799 " << endl <<  
   "    29738.929       30621.496       31530.255       32465.983" << endl << 
   "    33429.482       34421.574       35443.108       36494.959" << endl <<
   "    37578.026       38693.235       39841.541       41023.925" << endl << 
   "    42241.398       43495.003       44785.811       46114.927" << endl <<
   "    47483.487       48892.663       50343.658       51837.715" << endl << 
   "    53376.111       54960.163       56591.224       58270.691" << endl <<
   "    60000.000 , " << endl;
*/
	}

	fstr << "ebin_edge_out = " << edges[0][0] << "  " << str_join( edges[1], "  " ) << " ," << endl;
	fstr << "npos=1," << endl;
	fstr << "src_az=" << az << "," << endl;
	fstr << "src_el=" << el << "," << endl;
	fstr << "tstart=" << tstart << "," << endl;
	fstr << "tstop= " << tstop << "," << endl;
	fstr << "history_name='"<< rspfile << "'," << endl;
	fstr << "object_class='"<< object << "'," << endl;
	fstr << "energy_calib_scheme='data', gain_cor=1.0," << endl;
	fstr << "lut_filename='', lut_checksum='0'," << endl;
	fstr << "use_coslat_corr=1, leaf_ver='v10'," << endl;
	fstr << "atscat_path='" << RSPGEN_INSTALL_DIR << "/inputs/'," << endl;
	fstr << "atscat_file='test_atscatfile_preeinterp_db002.fits'," << endl;
	fstr << "matrix_type=0 /" << endl;

	fstr.close( );

	cout << "Wrote NML:" << file << endl; 

};




#endif