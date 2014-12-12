/* epsilon-equal.c: define a error resist compare. */

#include <math.h>

#include "epsilon-equal.h"

/* Numerical errors sometimes make a floating point number just slightly
   larger or smaller than its true value.  When it matters, we need to
   compare with some tolerance, REAL_EPSILON, defined in kbase.h.  */

bool
epsilon_equal(float const v1,
              float const v2) {

    return
        v1 == v2		       /* Usually they'll be exactly equal, anyway.  */
        || fabs(v1 - v2) <= REAL_EPSILON;
}

