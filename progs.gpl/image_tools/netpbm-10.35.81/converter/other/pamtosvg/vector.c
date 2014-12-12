/* vector.c: vector/point operations. */

#include <math.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include "pm_c_util.h"

#include "vector.h"
#include "message.h"
#include "epsilon-equal.h"

static float acos_d (float, at_exception_type * excep);


/* Given the point COORD, return the corresponding vector.  */

vector_type
make_vector (const float_coord c)
{
  vector_type v;

  v.dx = c.x;
  v.dy = c.y;
  v.dz = c.z;

  return v;
}


/* And the converse: given a vector, return the corresponding point.  */

float_coord
vector_to_point (const vector_type v)
{
  float_coord coord;

  coord.x = v.dx;
  coord.y = v.dy;
  coord.z = v.dz;

  return coord;
}


float
magnitude (const vector_type v)
{
  return (float) sqrt (v.dx * v.dx + v.dy * v.dy + v.dz * v.dz);
}


vector_type
normalize (const vector_type v)
{
  vector_type new_v;
  float m = magnitude (v);

  /* assert (m > 0.0); */

  if (m > 0.0)
  {
    new_v.dx = v.dx / m;
    new_v.dy = v.dy / m;
    new_v.dz = v.dz / m;
  }
  else
  {
	new_v.dx = v.dx;
    new_v.dy = v.dy;
    new_v.dz = v.dz;
  }

  return new_v;
}


vector_type
Vadd (const vector_type v1, const vector_type v2)
{
  vector_type new_v;

  new_v.dx = v1.dx + v2.dx;
  new_v.dy = v1.dy + v2.dy;
  new_v.dz = v1.dz + v2.dz;

  return new_v;
}


float
Vdot (const vector_type v1, const vector_type v2)
{
  return v1.dx * v2.dx + v1.dy * v2.dy + v1.dz * v2.dz;
}


vector_type
Vmult_scalar (const vector_type v, const float r)
{
  vector_type new_v;

  new_v.dx = v.dx * r;
  new_v.dy = v.dy * r;
  new_v.dz = v.dz * r;

  return new_v;
}


/* Given the IN_VECTOR and OUT_VECTOR, return the angle between them in
   degrees, in the range zero to 180.  */

float
Vangle (const vector_type in_vector, 
	const vector_type out_vector,
	at_exception_type * exp)
{
  vector_type v1 = normalize (in_vector);
  vector_type v2 = normalize (out_vector);

  return acos_d (Vdot (v2, v1), exp);
}


float_coord
Vadd_point (const float_coord c, const vector_type v)
{
  float_coord new_c;

  new_c.x = c.x + v.dx;
  new_c.y = c.y + v.dy;
  new_c.z = c.z + v.dz;
  return new_c;
}


float_coord
Vsubtract_point (const float_coord c, const vector_type v)
{
  float_coord new_c;

  new_c.x = c.x - v.dx;
  new_c.y = c.y - v.dy;
  new_c.z = c.z - v.dz;
  return new_c;
}


pm_pixelcoord
Vadd_int_point(pm_pixelcoord const c,
               vector_type   const v) {

    pm_pixelcoord a;

    a.col = ROUND ((float) c.col + v.dx);
    a.row = ROUND ((float) c.row + v.dy);
    
    return a;
}


vector_type
Vabs (const vector_type v)
{
  vector_type new_v;

  new_v.dx = (float) fabs (v.dx);
  new_v.dy = (float) fabs (v.dy);
  new_v.dz = (float) fabs (v.dz);
  return new_v;
}


/* Operations on points.  */

float_coord
Padd (const float_coord coord1, const float_coord coord2)
{
  float_coord sum;

  sum.x = coord1.x + coord2.x;
  sum.y = coord1.y + coord2.y;
  sum.z = coord1.z + coord2.z;

  return sum;
}


float_coord
Pmult_scalar (const float_coord coord, const float r)
{
  float_coord answer;

  answer.x = coord.x * r;
  answer.y = coord.y * r;
  answer.z = coord.z * r;

  return answer;
}


vector_type
Psubtract (const float_coord c1, const float_coord c2)
{
  vector_type v;

  v.dx = c1.x - c2.x;
  v.dy = c1.y - c2.y;
  v.dz = c1.z - c2.z;

  return v;
}



/* Operations on integer points.  */

vector_type
IPsubtract(pm_pixelcoord const coord1,
           pm_pixelcoord const coord2) {

    vector_type v;

    v.dx = (int) (coord1.col - coord2.col);
    v.dy = (int) (coord1.row - coord2.row);
    v.dz = 0.0;
    
    return v;
}



static float
acos_d (float v, at_exception_type * excep)
{
  float a;

  if (epsilon_equal (v, 1.0))
    v = 1.0;
  else if (epsilon_equal (v, -1.0))
    v = -1.0;

  errno = 0;
  a = (float) acos (v);
  if (errno == ERANGE || errno == EDOM)
    {
      at_exception_fatal(excep, strerror(errno));
      return 0.0;
    }
  
  
  return a * (float) 180.0 / (float) M_PI;
}
