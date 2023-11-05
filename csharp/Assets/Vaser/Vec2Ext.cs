using UnityEngine;

namespace Vaser
{
    public static class Vec2Ext
    {
        public static float Length(this Vector2 a)
        {
            return (float) System.Math.Sqrt(a.x*a.x+a.y*a.y);
        }
        public static float SignedArea(Vector2 P1, Vector2 P2, Vector2 P3)
        {
            return (P2.x-P1.x)*(P3.y-P1.y) - (P3.x-P1.x)*(P2.y-P1.y);
        }
        public static void Dot(Vector2 a, Vector2 b, ref Vector2 o) //dot product: o = a dot b
        {
            o.x = a.x * b.x;
            o.y = a.y * b.y;
        }
        public static void Opposite(ref Vector2 a)
        {
            a.x = -a.x;
            a.y = -a.y;
        }
        public static float Normalize(ref Vector2 a)
        {
            float L = a.Length();
            if (L > 0.0000001f)
            {
                a.x /= L;
                a.y /= L;
            }
            return L;
        }
        public static void Perpen(ref Vector2 a) //perpendicular: anti-clockwise 90 degrees
        {
            float y_value=a.y;
            a.y=a.x;
            a.x=-y_value;
        }
        public static void FollowSigns(ref Vector2 a, Vector2 b)
        {
            if ((a.x>0) != (b.x>0)) a.x = -a.x;
            if ((a.y>0) != (b.y>0)) a.y = -a.y;
        }

        //judgements
        public static bool Negligible(float M)
        {
            const float vaser_min_alw = 0.000000001f;
            return -vaser_min_alw < M && M < vaser_min_alw;
        }
        public static bool Negligible(double M)
        {
            const double vaser_min_alw = 0.0000000001;
            return -vaser_min_alw < M && M < vaser_min_alw;
        }
        public static bool Negligible(this Vector2 a)
        {
            return Negligible(a.x) && Negligible(a.y);
        }
        public static bool IsZero(this Vector2 a)
        {
            return a.x==0.0 && a.y==0.0;
        }
        public static bool Intersecting(Vector2 A, Vector2 B, Vector2 C, Vector2 D)
        {   //return true if AB intersects CD
            return SignedArea(A,B,C)>0 != SignedArea(A,B,D)>0;
        }
        public static bool OppositeQuadrant(Vector2 P1, Vector2 P2)
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

        public static bool AnchorOutward(ref Vector2 V, Vector2 b, Vector2 c, bool reverse=false)
        {   //put the correct outward vector at V, with V placed on b, comparing distances from c
            bool determinant = (b.x*V.x - c.x*V.x + b.y*V.y - c.y*V.y) > 0;
            if (determinant == (!reverse)) {
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

        public static int Intersect(
                Vector2 P1, Vector2 P2, //line 1
                Vector2 P3, Vector2 P4, //line 2
                ref Vector2 Pout,     //the output point
                float[] u_out = null)
        {   //Determine the intersection point of two line segments
            float mua, mub;
            double denom, numera, numerb;

            denom  = (P4.y-P3.y) * (P2.x-P1.x) - (P4.x-P3.x) * (P2.y-P1.y);
            numera = (P4.x-P3.x) * (P1.y-P3.y) - (P4.y-P3.y) * (P1.x-P3.x);
            numerb = (P2.x-P1.x) * (P1.y-P3.y) - (P2.y-P1.y) * (P1.x-P3.x);

            if (Negligible(numera) &&
                Negligible(numerb) &&
                Negligible(denom)) {
                Pout.x = (P1.x + P2.x) * 0.5f;
                Pout.y = (P1.y + P2.y) * 0.5f;
                return 2; //meaning the lines coincide
            }

            if (Negligible(denom)) {
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

        public static int Octant(this Vector2 a)
        {
            if (a.x == 0 && a.y == 0) {
                return 0;
            }
            if (a.x == 0) {
                return a.y > 0 ? 3 : 7;
            }
            if (a.y == 0) {
                return a.x > 0 ? 1 : 5;
            }
            if (a.x > 0 && a.y > 0) {
                if (a.y/a.x < 0.5f) {
                    return 1;
                } else if (a.y/a.x > 2f) {
                    return 3;
                } else {
                    return 2;
                }
            } else if (a.x < 0 && a.y > 0) {
                if (a.y/a.x < -2f) {
                    return 3;
                } else if (a.y/a.x > -0.5f) {
                    return 5;
                } else {
                    return 4;
                }
            } else if (a.x < 0 && a.y < 0) {
                if (a.y/a.x < 0.5f) {
                    return 5;
                } else if (a.y/a.x > 2f) {
                    return 7;
                } else {
                    return 6;
                }
            } else if (a.x > 0 && a.y < 0) {
                if (a.y/a.x < -2f) {
                    return 7;
                } else if (a.y/a.x > -0.5f) {
                    return 1;
                } else {
                    return 8;
                }
            }
            return 0;
        }
    }
}