#include "pm.h"

void 
parse_foveon(FILE * const ifp);

void  
foveon_interpolate(float coeff[3][4]);

void 
foveon_load_raw(void);

void  
foveon_coeff(bool * const useCoeffP,
             float        coeff[3][4]);
