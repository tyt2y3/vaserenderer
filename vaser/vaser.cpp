#ifndef VASER_CPP
#define VASER_CPP

#include "vaser.h"

#ifdef VASER_DEBUG
	#define DEBUG printf
#else
	#define DEBUG ;//
#endif

#include <math.h>
#include <vector>
#include <stdlib.h>

namespace VASEr
{
namespace VASErin
{	//VASEr internal namespace
const double vaser_min_alw=0.00000000001; //smallest value not regarded as zero
const Color default_color = {0,0,0,1};
const double default_weight = 1.0;
#include "point.h"
#include "color.h"
class vertex_array_holder;
#include "backend.h"
#include "vertex_array_holder.h"
#include "agg_curve4.cpp"
}
#include "opengl.cpp"
#include "polyline.cpp"
#include "gradient.cpp"
#include "curve.cpp"

} //namespace VASEr

#undef DEBUG
#endif
