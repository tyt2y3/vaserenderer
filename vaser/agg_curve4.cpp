// this code is isolated from AGG,
// for cubic bezier curve rendering using recursive subdivision

//-----------------------------------------------------------------------
// The Anti-Grain Geometry Project
// A high quality rendering engine for C++
// http://antigrain.com
// 
// Anti-Grain Geometry has dual licensing model. The Modified BSD 
// License was first added in version v2.4 just for convenience.
// It is a simple, permissive non-copyleft free software license, 
// compatible with the GNU GPL. It's well proven and recognizable.
// See http://www.fsf.org/licensing/licenses/index_html#ModifiedBSD
// for details.
// 
// Note that the Modified BSD license DOES NOT restrict your rights 
// if you choose the Anti-Grain Geometry Public License.
// 
// Anti-Grain Geometry Public License
// ====================================================
// 
// Anti-Grain Geometry - Version 2.4 
// Copyright (C) 2002-2005 Maxim Shemanarev (McSeem) 
// 
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
// 
// 
// 
// Modified BSD License
// ====================================================
// Anti-Grain Geometry - Version 2.4 
// Copyright (C) 2002-2005 Maxim Shemanarev (McSeem) 
// 
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   1. Redistributions of source code must retain the above copyright 
//      notice, this list of conditions and the following disclaimer. 
// 
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in 
//      the documentation and/or other materials provided with the 
//      distribution. 
// 
//   3. The name of the author may not be used to endorse or promote 
//      products derived from this software without specific prior 
//      written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
//
//-----------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//-----------------------------------------------------------------------

#include <math.h>
#ifndef M_PI
	#define M_PI 3.141592653589793238462643
#endif

double calc_sq_distance(double x1, double y1, double x2, double y2)
{
	double dx = x2-x1;
	double dy = y2-y1;
	return dx * dx + dy * dy;
}

void recursive_bezier(   double x1, double y1, 
				double x2, double y2, 
				double x3, double y3, 
				double x4, double y4,
				unsigned level,
				double m_angle_tolerance,
				double m_cusp_limit,
				double m_distance_tolerance_square,
				void (*add_point)(void*,double,double),
				void* obj )
{
	const double curve_distance_epsilon                  = 1e-30;
	const double curve_collinearity_epsilon              = 1e-30;
	const double curve_angle_tolerance_epsilon           = 0.01;
	const int curve_recursion_limit = 32;

	if(level > curve_recursion_limit) 
	{
		return;
	}

	// Calculate all the mid-points of the line segments
	//----------------------
	double x12   = (x1 + x2) / 2;
	double y12   = (y1 + y2) / 2;
	double x23   = (x2 + x3) / 2;
	double y23   = (y2 + y3) / 2;
	double x34   = (x3 + x4) / 2;
	double y34   = (y3 + y4) / 2;
	double x123  = (x12 + x23) / 2;
	double y123  = (y12 + y23) / 2;
	double x234  = (x23 + x34) / 2;
	double y234  = (y23 + y34) / 2;
	double x1234 = (x123 + x234) / 2;
	double y1234 = (y123 + y234) / 2;

	// Try to approximate the full cubic curve by a single straight line
	//------------------
	double dx = x4-x1;
	double dy = y4-y1;

	double d2 = fabs(((x2 - x4) * dy - (y2 - y4) * dx));
	double d3 = fabs(((x3 - x4) * dy - (y3 - y4) * dx));
	double da1, da2, k;

	switch((int(d2 > curve_collinearity_epsilon) << 1) +
	int(d3 > curve_collinearity_epsilon))
	{
		case 0:
			// All collinear OR p1==p4
			//----------------------
			k = dx*dx + dy*dy;
			if(k == 0)
			{
				d2 = calc_sq_distance(x1, y1, x2, y2);
				d3 = calc_sq_distance(x4, y4, x3, y3);
			}
			else
			{
				k   = 1 / k;
				da1 = x2 - x1;
				da2 = y2 - y1;
				d2  = k * (da1*dx + da2*dy);
				da1 = x3 - x1;
				da2 = y3 - y1;
				d3  = k * (da1*dx + da2*dy);
				if(d2 > 0 && d2 < 1 && d3 > 0 && d3 < 1)
				{
					// Simple collinear case, 1---2---3---4
					// We can leave just two endpoints
					return;
				}
				if(d2 <= 0) d2 = calc_sq_distance(x2, y2, x1, y1);
				else if(d2 >= 1) d2 = calc_sq_distance(x2, y2, x4, y4);
				else             d2 = calc_sq_distance(x2, y2, x1 + d2*dx, y1 + d2*dy);

				if(d3 <= 0) d3 = calc_sq_distance(x3, y3, x1, y1);
				else if(d3 >= 1) d3 = calc_sq_distance(x3, y3, x4, y4);
				else             d3 = calc_sq_distance(x3, y3, x1 + d3*dx, y1 + d3*dy);
			}
			if(d2 > d3)
			{
				if(d2 < m_distance_tolerance_square)
				{
					add_point(obj, x2,y2);
					return;
				}
			}
			else
			{
				if(d3 < m_distance_tolerance_square)
				{
					add_point(obj, x3,y3);
					return;
				}
			}
		break;

		case 1:
			// p1,p2,p4 are collinear, p3 is significant
			//----------------------
			if(d3 * d3 <= m_distance_tolerance_square * (dx*dx + dy*dy))
			{
				if(m_angle_tolerance < curve_angle_tolerance_epsilon)
				{
					add_point(obj, x23,y23);
					return;
				}

				// Angle Condition
				//----------------------
				da1 = fabs(atan2(y4 - y3, x4 - x3) - atan2(y3 - y2, x3 - x2));
				if(da1 >= M_PI) da1 = 2*M_PI - da1;

				if(da1 < m_angle_tolerance)
				{
					add_point(obj, x2, y2);
					add_point(obj, x3, y3);
					return;
				}

				if(m_cusp_limit != 0.0)
				{
					if(da1 > m_cusp_limit)
					{
						add_point(obj, x3, y3);
						return;
					}
				}
			}
		break;

		case 2:
			// p1,p3,p4 are collinear, p2 is significant
			//----------------------
			if(d2 * d2 <= m_distance_tolerance_square * (dx*dx + dy*dy))
			{
				if(m_angle_tolerance < curve_angle_tolerance_epsilon)
				{
					add_point(obj, x23, y23);
					return;
				}

				// Angle Condition
				//----------------------
				da1 = fabs(atan2(y3 - y2, x3 - x2) - atan2(y2 - y1, x2 - x1));
				if(da1 >= M_PI) da1 = 2*M_PI - da1;

				if(da1 < m_angle_tolerance)
				{
					add_point(obj, x2, y2);
					add_point(obj, x3, y3);
					return;
				}

				if(m_cusp_limit != 0.0)
				{
					if(da1 > m_cusp_limit)
					{
						add_point(obj, x2, y2);
						return;
					}
				}
			}
		break;

		case 3: 
			// Regular case
			//-----------------
			if((d2 + d3)*(d2 + d3) <= m_distance_tolerance_square * (dx*dx + dy*dy))
			{
				// If the curvature doesn't exceed the distance_tolerance value
				// we tend to finish subdivisions.
				//----------------------
				if(m_angle_tolerance < curve_angle_tolerance_epsilon)
				{
					add_point(obj, x23, y23);
					return;
				}

				// Angle & Cusp Condition
				//----------------------
				k   = atan2(y3 - y2, x3 - x2);
				da1 = fabs(k - atan2(y2 - y1, x2 - x1));
				da2 = fabs(atan2(y4 - y3, x4 - x3) - k);
				if(da1 >= M_PI) da1 = 2*M_PI - da1;
				if(da2 >= M_PI) da2 = 2*M_PI - da2;

				if(da1 + da2 < m_angle_tolerance)
				{
					// Finally we can stop the recursion
					//----------------------
					add_point(obj, x23, y23);
					return;
				}

				if(m_cusp_limit != 0.0)
				{
					if(da1 > m_cusp_limit)
					{
						add_point(obj, x2, y2);
						return;
					}

					if(da2 > m_cusp_limit)
					{
						add_point(obj, x3, y3);
						return;
					}
				}
			}
		break;
	}

	// Continue subdivision
	//----------------------
	recursive_bezier(x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1,
		m_angle_tolerance, m_cusp_limit, m_distance_tolerance_square,
		add_point, obj); 
	recursive_bezier(x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1,
		m_angle_tolerance, m_cusp_limit, m_distance_tolerance_square,
		add_point, obj); 
}

int curve4_div(double x1, double y1, 
		double x2, double y2, 
		double x3, double y3,
		double x4, double y4,
		double m_approximation_scale,
		double m_angle_tolerance,
		double m_cusp_limit,
		void (*add_point)(void*,double,double),
		void* obj )
{
	double m_distance_tolerance_square = 0.5 / m_approximation_scale;
	m_distance_tolerance_square *= m_distance_tolerance_square;
	int m_count = 0;
	add_point(obj, x1, y1);
	recursive_bezier(x1, y1, x2, y2, x3, y3, x4, y4, 0,
		m_angle_tolerance, m_cusp_limit, m_distance_tolerance_square,
		add_point, obj);
	add_point(obj, x4, y4);
}

#undef M_PI
