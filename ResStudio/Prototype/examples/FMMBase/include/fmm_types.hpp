#ifndef FMM_TYPES_HPP
#define FMM_TYPES_HPP
#include <complex>
namespace FMM_3D{
  extern int L_max;
  typedef enum axs {Read,Write} Access;
  typedef enum key {MVP,
		    Interpolation_Key,
		    Green_Translate,
		    Green_Interpolate,
		    Receiving_Key,
		    FarField_key, //5
		    DT_FFL,        DT_C2P,        DT_XLT,        DT_P2C,        DT_NFL,        DT_RCV,//11
		    DT_ffl,        DT_c2p,        DT_xlt,        DT_p2c,        DT_nfl,        DT_rcv,//17
		    SG_ffl,        SG_c2p,        SG_xlt,        SG_p2c,        SG_nfl,        SG_rcv,//23
		    NUM_TASK_KEYS
  } TaskKey;
  struct DTHandle{
    long id;
  };
  typedef std::complex<double> ComplexElementType;
  typedef double ElementType;
  /*---------------------------------------------------------------*/
  typedef struct{
    union{// no. of blocks in first level
      int B1,groups,g,group_count;
    };
    union {// no. of blocks in second level
      int B2,parts,part_count;
    };
    union {// interpolation points
      int interp_points,m;
    };
    double work_min;
    int iter_batch_count;
  }Parameters_t;
  extern Parameters_t Parameters;
  

}//namespace FMM_3D

#endif
