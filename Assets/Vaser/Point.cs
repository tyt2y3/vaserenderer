namespace Vaser
{
    public struct Point
    {
        public float x;
        public float y;

        public Point(Point P) { x = P.x; y = P.y; }
        public Point(float X, float Y) { x = X; y = Y; }

        // attributes
        public float length()
        {
            return (float) System.Math.Sqrt(x*x+y*y);
        }
        public static float signed_area(Point P1, Point P2, Point P3)
        {
            return (P2.x-P1.x)*(P3.y-P1.y) - (P3.x-P1.x)*(P2.y-P1.y);
        }

        // operators    
        public static Point operator + (Point a, Point b)
        {
            return new Point(a.x + b.x, a.y + b.y);
        }
        public static Point operator - (Point a)
        {
            return new Point(-a.x, -a.y);
        }
        public static Point operator - (Point a, Point b)
        {
            return new Point(a.x-b.x, a.y-b.y);
        }
        public static Point operator * (Point a, float k)
        {
            return new Point(a.x*k, a.y*k);
        }

        public static void dot(Point a, Point b, ref Point o) //dot product: o = a dot b
        {
            o.x = a.x * b.x;
            o.y = a.y * b.y;
        }
        public Point dot_prod(Point b) //return dot product
        {
            return new Point(x*b.x, y*b.y);
        }

        // self operations
        public void opposite()
        {
            x = -x;
            y = -y;
        }
        public void opposite_of(Point a)
        {
            x = -a.x;
            y = -a.y;
        }
        public float normalize()
        {
            float L = length();
            if ( L > 0.0000001)
            {
                x /= L; y /= L;
            }
            return L;
        }
        public int octant()
        {
            if (x == 0 && y == 0) {
                return 0;
            }
            if (x == 0) {
                return y > 0 ? 3 : 7;
            }
            if (y == 0) {
                return x > 0 ? 1 : 5;
            }
            if (x > 0 && y > 0) {
                if (y/x < 0.5f) {
                    return 1;
                } else if (y/x > 2f) {
                    return 3;
                } else {
                    return 2;
                }
            } else if (x < 0 && y > 0) {
                if (y/x < -2f) {
                    return 3;
                } else if (y/x > -0.5f) {
                    return 5;
                } else {
                    return 4;
                }
            } else if (x < 0 && y < 0) {
                if (y/x < 0.5f) {
                    return 5;
                } else if (y/x > 2f) {
                    return 7;
                } else {
                    return 6;
                }
            } else if (x > 0 && y < 0) {
                if (y/x < -2f) {
                    return 7;
                } else if (y/x > -0.5f) {
                    return 1;
                } else {
                    return 8;
                }
            }
            return 0;
        }
        public void perpen() //perpendicular: anti-clockwise 90 degrees
        {
            float y_value=y;
            y=x;
            x=-y_value;
        }
        public void follow_signs(Point a)
        {
            if ((x>0) != (a.x>0)) x = -x;
            if ((y>0) != (a.y>0)) y = -y;
        }

        //judgements
        public static bool negligible(float M)
        {
            const float vaser_min_alw = 0.000000001f;
            return -vaser_min_alw < M && M < vaser_min_alw;
        }
        public static bool negligible(double M)
        {
            const double vaser_min_alw = 0.0000000001;
            return -vaser_min_alw < M && M < vaser_min_alw;
        }
        public bool negligible()
        {
            return negligible(x) && negligible(y);
        }
        public bool non_negligible()
        {
            return !negligible();
        }
        public bool is_zero()
        {
            return x==0.0 && y==0.0;
        }
        public bool non_zero()
        {
            return !is_zero();
        }
        public static bool intersecting(Point A, Point B, Point C, Point D)
        {   //return true if AB intersects CD
            return signed_area(A,B,C)>0 != signed_area(A,B,D)>0;
        }

        //operations require 2 input points
        public static float distance_squared(Point A, Point B)
        {
            float dx=A.x-B.x;
            float dy=A.y-B.y;
            return (dx*dx+dy*dy);
        }
        public static float distance(Point A, Point B)
        {
            return (float) System.Math.Sqrt(distance_squared(A, B));
        }
        public static Point midpoint(Point A, Point B)
        {
            return (A+B)*0.5f;
        }
        public static bool opposite_quadrant(Point P1, Point P2)
        {
            int P1x = P1.x>0? 1:(P1.x<0?-1:0);
            int P1y = P1.y>0? 1:(P1.y<0?-1:0);
            int P2x = P2.x>0? 1:(P2.x<0?-1:0);
            int P2y = P2.y>0? 1:(P2.y<0?-1:0);

            if (P1x != P2x) {
                if (P1y != P2y)
                    return true;
                if (P1y == 0 || P2y == 0) {
                    return true;
                }
            }
            if (P1y != P2y) {
                if (P1x == 0 || P2x == 0) {
                    return true;
                }
            }
            return false;
        }

        //operations of 3 points
        public static bool anchor_outward_D(Point V, Point b, Point c)
        {
            return (b.x*V.x - c.x*V.x + b.y*V.y - c.y*V.y) > 0;
        }
        public static bool anchor_outward(ref Point V, Point b, Point c, bool reverse=false)
        {   //put the correct outward vector at V, with V placed on b, comparing distances from c
            bool determinant = anchor_outward_D ( V,b,c);
            if ( determinant == (!reverse)) {
                //when reverse==true, it means inward
                //positive V is the outward vector
                return false;
            } else {
                //negative V is the outward vector
                V.x=-V.x;
                V.y=-V.y;
                return true; //return whether V is changed
            }
        }
        public static void anchor_inward(ref Point V, Point b, Point c)
        {
            anchor_outward(ref V,b,c,true);
        }

        //operations of 4 points
        public static int intersect(
                Point P1, Point P2, //line 1
                Point P3, Point P4, //line 2
                ref Point Pout,     //the output point
                float[] u_out = null)
        {   //Determine the intersection point of two line segments
            float mua, mub;
            double denom, numera, numerb;

            denom  = (P4.y-P3.y) * (P2.x-P1.x) - (P4.x-P3.x) * (P2.y-P1.y);
            numera = (P4.x-P3.x) * (P1.y-P3.y) - (P4.y-P3.y) * (P1.x-P3.x);
            numerb = (P2.x-P1.x) * (P1.y-P3.y) - (P2.y-P1.y) * (P1.x-P3.x);

            if (negligible(numera) &&
                negligible(numerb) &&
                negligible(denom)) {
                Pout.x = (P1.x + P2.x) * 0.5f;
                Pout.y = (P1.y + P2.y) * 0.5f;
                return 2; //meaning the lines coincide
            }

            if (negligible(denom)) {
                Pout.x = 0;
                Pout.y = 0;
                return 0; //meaning lines are parallel
            }

            mua = (float) (numera / denom);
            mub = (float) (numerb / denom);
            if (u_out != null) {
                u_out[0] = mua;
                u_out[1] = mub;
            }

            Pout.x = P1.x + mua * (P2.x - P1.x);
            Pout.y = P1.y + mua * (P2.y - P1.y);

            bool out1 = mua < 0 || mua > 1;
            bool out2 = mub < 0 || mub > 1;

            if (out1 & out2) {
                return 5; //the intersection lies outside both segments
            } else if (out1) {
                return 3; //the intersection lies outside segment 1
            } else if (out2) {
                return 4; //the intersection lies outside segment 2
            } else {
                return 1; //great
            }
            //http://paulbourke.net/geometry/lineline2d/
        }
    }
}