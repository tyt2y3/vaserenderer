using UnityEngine;
using System.Collections.Generic;

namespace Vaser
{
    public class Polyline
    {
        const float cri_core_adapt = 0.0001f;

        public class polyline_opt
        {
            //const tessellator_opt* tess;
            public char joint; //use PLJ_xx
            public char cap;   //use PLC_xx
            public bool feather;
            public float feathering;
            public bool no_feather_at_cap;
            public bool no_feather_at_core;

            //for polyline_opt.joint
            public const char PLJ_miter = (char)0; //default
            public const char PLJ_bevel = (char)1;
            public const char PLJ_round = (char)2;
            //for polyline_opt.cap
            public const char PLC_butt  = (char)0; //default
            public const char PLC_round = (char)1;
            public const char PLC_square= (char)2;
            public const char PLC_rect  = (char)3;
            public const char PLC_both  = (char)0; //default
            public const char PLC_first = (char)10;
            public const char PLC_last  = (char)20;
            public const char PLC_none  = (char)30;
        }

        public struct st_polyline
        // to hold info for AnchorLate() to perform triangluation
        {
            //for all joints
            public Point vP; //vector to intersection point
            public Point vR; //fading vector at sharp end
                //all vP,vR are outward

            //for djoint==PLJ_bevel
            public Point T; //core thickness of a line
            public Point R; //fading edge of a line
            public Point bR; //out stepping vector, same direction as cap
            public Point T1,R1; //alternate vectors, same direction as T21
                //all T,R,T1,R1 are outward

            //for djoint==PLJ_round
            public float t,r;

            //for degeneration case
            public bool degenT; //core degenerated
            public bool degenR; //fade degenerated
            public bool pre_full; //draw the preceding segment in full
            public Point PT,PR;
            public float pt; //parameter at intersection
            public bool R_full_degen;

            public char djoint; //determined joint
                    // e.g. originally a joint is PLJ_miter. but it is smaller than critical angle, should then set djoint to PLJ_bevel
        }

        public class st_anchor
        // to hold memory for the working of anchor()
        {
            public Point[] P = new Point[3]; //point
            public Color[] C = new Color[3]; //color
            public float[] W = new float[3]; //weight

            public Point cap_start = new Point();
            public Point cap_end = new Point();
            public st_polyline[] SL = new st_polyline[3];
            public VertexArrayHolder vah = new VertexArrayHolder();
        }

        private static void determine_t_r(float w, ref float t, ref float R)
        {
            //efficiency: can cache one set of w,t,R values
            // i.e. when a polyline is of uniform thickness, the same w is passed in repeatedly
            float f = w - (float)System.Math.Floor(w);
            t = w; R = 0f;
            /* // resolution dependent
            if (w>=0.0 && w<1.0) {
                t=0.05; R=0.768;//R=0.48+0.32*f;
            } else if (w>=1.0 && w<2.0) {
                t=0.05+f*0.33; R=0.768+0.312*f;
            } else if (w>=2.0 && w<3.0) {
                t=0.38+f*0.58; R=1.08;
            } else if (w>=3.0 && w<4.0) {
                t=0.96+f*0.48; R=1.08;
            } else if (w>=4.0 && w<5.0) {
                t=1.44+f*0.46; R=1.08;
            } else if (w>=5.0 && w<6.0) {
                t=1.9+f*0.6; R=1.08;
            } else if (w>=6.0){
                t=2.5+(w-6.0)*0.50; R=1.08;
            } */
        }

        private static void make_T_R_C(
            ref Point P1, ref Point P2, ref Point T, ref Point R, ref Point C,
            float w, polyline_opt opt,
            ref float rr, ref float tt, ref float dist,
            bool seg_mode=false)
        {
            float t = 1.0f, r = 0.0f;

            //calculate t,r
            determine_t_r(w, ref t, ref r);

            //output
            tt = t;
            Point DP = P2 - P1;
            dist = DP.normalize();
            C = DP;
            DP.perpen();
            T = DP*t;
        }

        public static void Anchor(st_anchor SA, polyline_opt opt, bool cap_first, bool cap_last)
        {
            Point[] P = SA.P;
            Color[] C = SA.C;
            float[] weight = SA.W;
            st_polyline[] SL = SA.SL;
            if (Point.signed_area(P[0], P[1], P[2]) > 0) {
                // rectify clockwise
                P = new Point[3] {P[2], P[1], P[0]};
                C = new Color[3] {C[2], C[1], C[0]};
                weight = new float[3] {weight[2], weight[1], weight[0]};
            }

            //const float critical_angle=11.6538;
            //  critical angle in degrees where a miter is force into bevel
            //  it is _similar_ to cairo_set_miter_limit () but cairo works with ratio while VASEr works with included angle
            const float cos_cri_angle=0.979386f; //cos(critical_angle)

            bool varying_weight = !(weight[0]==weight[1] & weight[1]==weight[2]);

            /* // resolution dependent
            float combined_weight=weight[1]+(opt.feather?opt.feathering:0.0);
            if (combined_weight < cri_segment_approx)
            {
                segment(SA, &opt, cap_first,false, opt.joint==PLJ_round?PLC_round:PLC_butt);
                char ori_cap = opt.cap;
                opt.cap = opt.joint==PLJ_round?PLC_round:PLC_butt;
                SA.P[0]=SA.P[1]; SA.P[1]=SA.P[2];
                SA.C[0]=SA.C[1]; SA.C[1]=SA.C[2];
                SA.W[0]=SA.W[1]; SA.W[1]=SA.W[2];
                segment(SA, &opt, false,cap_last, ori_cap);
                return 0;
            }*/

            Point T1=new Point(), T2=new Point(), T21=new Point(), T31=new Point(), RR=new Point();

            /* // resolution dependent
            for (int i=0; i<3; i++)
            {   //lower the transparency for weight < 1.0
                if (weight[i]>=0.0 && weight[i]<1.0)
                {
                    float f=weight[i];
                    C[i].a *=f;
                }
            }*/

            {
                int i=0;

                Point cap0=new Point(), cap1=new Point();
                float r=0f,t=0f,d=0f;
                make_T_R_C(ref P[i], ref P[i+1], ref T2, ref RR, ref cap1, weight[i], opt, ref r, ref t, ref d);
                if (varying_weight) {
                make_T_R_C(ref P[i], ref P[i+1], ref T31, ref RR, ref cap0, weight[i+1], opt, ref d, ref d, ref d);
                } else {
                    T31 = T2;
                }

                SL[i].bR = cap1;

                if (cap_first)
                {
                    if (opt.cap==polyline_opt.PLC_square)
                    {
                        P[0] = P[0] - cap1 * (t+r);
                    }
                    cap1.opposite(); if (opt.feather && !opt.no_feather_at_cap)
                    cap1*=opt.feathering;
                    SA.cap_start = cap1;
                }

                SL[i].djoint=opt.cap;
                SL[i].T=T2;
                SL[i].t=(float)t;
                SL[i].degenT = false;

                SL[i+1].T1=T31;
            }

            if (cap_last)
            {
                int i=2;

                Point cap0=new Point(), cap2=new Point();
                float t=0f,r=0f,d=0f;
                make_T_R_C(ref P[i-1], ref P[i], ref cap0, ref cap0, ref cap2, weight[i], opt, ref r, ref t, ref d);
                if (opt.cap==polyline_opt.PLC_square)
                {
                    P[2] = P[2] + cap2 * (t+r);
                }

                SL[i].bR=cap2;

                if (opt.feather && !opt.no_feather_at_cap)
                    cap2*=opt.feathering;
                SA.cap_end = cap2;
            }

            {
                int i=1;

                float r=0f,t=0f;
                Point P_cur = P[i]; //current point
                Point P_nxt = P[i+1]; //next point
                Point P_las = P[i-1]; //last point
                if (opt.cap==polyline_opt.PLC_butt || opt.cap==polyline_opt.PLC_square)
                {
                    P_nxt -= SA.cap_end;
                    P_las -= SA.cap_start;
                }

                {
                Point cap0=new Point(), bR=new Point();
                float length_cur=0f, length_nxt=0f, d=0f;
                make_T_R_C(ref P_las, ref P_cur, ref T1, ref RR, ref cap0, weight[i-1], opt, ref d, ref d, ref length_cur);
                if (varying_weight) {
                make_T_R_C(ref P_las, ref P_cur, ref T21, ref RR, ref cap0, weight[i], opt, ref d, ref d, ref d);
                } else {
                    T21 = T1;
                }

                make_T_R_C(ref P_cur, ref P_nxt, ref T2, ref RR, ref bR, weight[i], opt, ref r, ref t, ref length_nxt);
                if (varying_weight) {
                make_T_R_C(ref P_cur, ref P_nxt, ref T31, ref RR, ref cap0, weight[i+1], opt, ref d, ref d, ref d);
                } else {
                    T31 = T2;
                }

                SL[i].T=T2;
                SL[i].bR=bR;
                SL[i].t=t;
                SL[i].degenT = false;

                SL[i+1].T1=T31;
                }

                {   //2nd to 2nd last point

                    //find the angle between the 2 line segments
                    Point ln1=new Point(), ln2=new Point(), V=new Point();
                    ln1 = P_cur - P_las;
                    ln2 = P_nxt - P_cur;
                    ln1.normalize();
                    ln2.normalize();
                    Point.dot(ln1, ln2, ref V);
                    float cos_tho=-V.x-V.y;
                    bool zero_degree = Point.negligible(cos_tho-1.0f);
                    bool d180_degree = cos_tho < -1.0f+0.0001f;
                    bool smaller_than_30_degree = cos_tho > 0.8660254f;
                    int result3 = 1;

                    if ( (cos_tho < 0 && opt.joint==polyline_opt.PLJ_bevel) ||
                         (opt.joint!=polyline_opt.PLJ_bevel && opt.cap==polyline_opt.PLC_round) ||
                         (opt.joint==polyline_opt.PLJ_round) )
                    {   //when greater than 90 degrees
                        SL[i-1].bR *= 0.01f;
                        SL[i]  .bR *= 0.01f;
                        SL[i+1].bR *= 0.01f;
                        //to solve an overdraw in bevel and round joint
                    }

                    Point.anchor_outward(ref T1, P_cur,P_nxt);
                    Point.anchor_outward(ref T21, P_cur,P_nxt);
                        SL[i].T1.follow_signs(T21);
                    Point.anchor_outward(ref T2, P_cur,P_las);
                        SL[i].T.follow_signs(T2);
                    Point.anchor_outward(ref T31, P_cur,P_las);

                    {   //must do intersection
                        Point interP=new Point(), vP=new Point();
                        result3 = Point.intersect(
                                    P_las+T1, P_cur+T21,
                                    P_nxt+T31, P_cur+T2,
                                    ref interP);

                        if (result3 != 0) {
                            vP = interP - P_cur;
                            SL[i].vP=vP;
                        } else {
                            SL[i].vP=SL[i].T;
                            Debug.Log("intersection failed: cos(angle)=%.4f, angle=%.4f(degree)"/*, cos_tho, System.Math.Acos(cos_tho)*180/3.14159*/);
                        }
                    }

                    T1.opposite();
                    T21.opposite();
                    T2.opposite();
                    T31.opposite();

                    //make intersections
                    Point PT1=new Point(), PT2=new Point();
                    float pt1=0f,pt2=0f;
                    float[] pts = new float[2];

                    int result1t = Point.intersect(
                                P_nxt-T31, P_nxt+T31,
                                P_las+T1, P_cur+T21, //knife1_a
                                ref PT1, pts); //core
                    pt1 = pts[1];
                    int result2t = Point.intersect(
                                P_las-T1, P_las+T1,
                                P_nxt+T31, P_cur+T2, //knife2_a
                                ref PT2, pts);
                    pt2 = pts[1];
                    bool is_result1t = result1t == 1;
                    bool is_result2t = result2t == 1;
                    //
                    /*if (zero_degree)
                    {
                        bool pre_full = is_result1t;
                        opt.no_feather_at_cap=true;
                        if (pre_full)
                        {
                            segment(SA, opt, true,cap_last, opt.joint==PLJ_round?PLC_round:PLC_butt);
                        }
                        else
                        {
                            char ori_cap = opt.cap;
                            opt.cap = opt.joint==PLJ_round?PLC_round:PLC_butt;
                            SA.P[0]=SA.P[1]; SA.P[1]=SA.P[2];
                            SA.C[0]=SA.C[1]; SA.C[1]=SA.C[2];
                            SA.W[0]=SA.W[1]; SA.W[1]=SA.W[2];
                            segment(SA, opt, true,cap_last, ori_cap);
                        }
                        return 0;
                    }*/

                    if (is_result1t | is_result2t)
                    {   //core degeneration
                        SL[i].degenT=true;
                        SL[i].pre_full=is_result1t;
                        SL[i].PT = is_result1t? PT1:PT2;
                        SL[i].pt = is_result1t? pt1:pt2;
                    }

                    //make joint
                    SL[i].djoint = opt.joint;
                    if (opt.joint == polyline_opt.PLJ_miter)
                        if (cos_tho >= cos_cri_angle)
                            SL[i].djoint=polyline_opt.PLJ_bevel;

                    if (d180_degree | result3 == 0)
                    {   //to solve visual bugs 3 and 1.1
                        SL[i].vP=SL[i].T;
                        SL[i].T1.follow_signs(SL[i].T);
                        SL[i].djoint=polyline_opt.PLJ_miter;
                    }
                } //2nd to 2nd last point
            }

            {
                int i=2;

                Point cap0=new Point();
                float r=0f,t=0f,d=0f;
                make_T_R_C(ref P[i-1], ref P[i], ref T2, ref RR, ref cap0, weight[i], opt, ref r, ref t, ref d);

                SL[i].djoint=opt.cap;
                SL[i].T=T2;
                SL[i].t=(float)t;
                SL[i].degenT = false;
            }

            /*if (cap_first || cap_last) {
                anchor_cap(SA.P,SA.C, SA.SL,SA.vah, SA.cap_start,SA.cap_end);
            }*/
            AnchorLate(P, C, SA.SL, SA.vah, SA.cap_start, SA.cap_end);
        } //anchor

        public static void AnchorLate(
                Point[] P, Color[] C, st_polyline[] SL,
                VertexArrayHolder tris,
                Point cap1, Point cap2)
        {
            Point P_0 = P[0], P_1 = P[1], P_2 = P[2];
            if (SL[0].djoint==polyline_opt.PLC_butt || SL[0].djoint==polyline_opt.PLC_square)
                P_0 -= cap1;
            if (SL[2].djoint==polyline_opt.PLC_butt || SL[2].djoint==polyline_opt.PLC_square)
                P_2 -= cap2;

            Point P0, P1, P2, P3, P4, P5, P6, P7;

            P0 = P_1 + SL[1].vP;
            P1 = P_1 - SL[1].vP;

            P2 = P_1 + SL[1].T1;
            P3 = P_0 + SL[0].T;
            P4 = P_0 - SL[0].T;

            P5 = P_1 + SL[1].T;
            P6 = P_2 + SL[2].T;
            P7 = P_2 - SL[2].T;

            int normal_line_core_joint = 1; //0:dont draw, 1:draw, 2:outer only

            if (SL[1].degenT)
            {
                P1 = SL[1].PT;
                tris.Push3(P1, P3, P2, C[1], C[0], C[1], 0, 1, 0); //fir seg
                tris.Push3(P1, P5, P6, C[1], C[1], C[2], 0, 1, 0); //las seg

                if (SL[1].pre_full) {
                    tris.Push3(P3, P1, P4, C[0], C[1], C[0], 0, 1, 0);
                } else {
                    tris.Push3(P1, P6, P7, C[1], C[2], C[2], 0, 0, 1);
                }
            }
            else
            {
                // normal first segment
                tris.Push3(P2, P4, P3, C[1], C[0], C[0], 0, 0, 1);
                tris.Push3(P4, P2, P1, C[0], C[1], C[1], 0, 0, 1);
                // normal last segment
                tris.Push3(P5, P7, P1, C[1], C[2], C[1], 0, 1, 0);
                tris.Push3(P7, P5, P6, C[2], C[1], C[2], 0, 1, 0);
            }

            if (normal_line_core_joint != 0)
            {
                switch (SL[1].djoint)
                {
                    case polyline_opt.PLJ_miter:
                        tris.Push3(P2, P0, P1, C[1], C[1], C[1], 1, 0, 0);
                        tris.Push3(P1, P0, P5, C[1], C[1], C[1], 0, 1, 0);
                    break;

                    case polyline_opt.PLJ_bevel:
                        if (normal_line_core_joint==1)
                        tris.Push3(P2, P5, P1, C[1], C[1], C[1], 1, 0, 0);
                    break;

                    case polyline_opt.PLJ_round: {
                        VertexArrayHolder strip = new VertexArrayHolder();
                        strip.SetGlDrawMode(VertexArrayHolder.GL_TRIANGLE_STRIP);

                    if (normal_line_core_joint==1) {
                        vectors_to_arc(strip, P_1, C[1], C[1], SL[1].T1, SL[1].T,
                            get_PLJ_round_dangle(SL[1].t,SL[1].r),
                            SL[1].t, 0.0f, false, P1);
                    } else if (normal_line_core_joint==2) {
                        vectors_to_arc(strip, P_1, C[1], C[1], SL[1].T1, SL[1].T,
                            get_PLJ_round_dangle(SL[1].t,SL[1].r),
                            SL[1].t, 0.0f, false, P5);
                    }
                        tris.Push(strip);
                    } break;
                }
            }
        } //AnchorLate

        private static void inner_arc(
                VertexArrayHolder hold,
                Point P, Color C, Color C2,
                float dangle, float angle1, float angle2,
                float r, float r2, bool ignor_ends,
                Point apparent_P, bool use_apparent_P)
        {
            // (apparent center) center of fan
            //draw the inner arc between angle1 and angle2 with dangle at each step.
            // -the arc has thickness, r is the outer radius and r2 is the inner radius,
            //    with color C and C2 respectively.
            //    in case when inner radius r2=0.0f, it gives a pie.
            // -when ignor_ends=false, the two edges of the arc lie exactly on angle1
            //    and angle2. when ignor_ends=true, the two edges of the arc do not touch
            //    angle1 or angle2.
            // -P is the mathematical center of the arc.
            // -when use_apparent_P is true, r2 is ignored,
            //    apparent_P is then the apparent origin of the pie.
            // -the result is pushed to hold, in form of a triangle strip
            // -an inner arc is an arc which is always shorter than or equal to a half circumference

            void inner_arc_push(double x, double y)
            {
                //hold.Dot(new Point(P.x+(float)x*r, P.y-(float)y*r), 0.05f);
                hold.Push(new Point(P.x+(float)x*r, P.y-(float)y*r), C, 1);
                if (use_apparent_P) {
                    hold.Push(apparent_P, C2, 1);
                } else {
                    hold.Push(new Point(P.x+(float)x*r2, P.y-(float)y*r2), C2, 1);
                }
            }

            const float m_pi = (float) System.Math.PI;

            if (angle2 > angle1)
            {
                if (angle2-angle1>m_pi)
                {
                    angle2=angle2-2*m_pi;
                }
            }
            else
            {
                if (angle1-angle2>m_pi)
                {
                    angle1=angle1-2*m_pi;
                }
            }

            bool incremental = true;
            if (angle1 > angle2)
            {
                incremental = false;
            }

            if (incremental)
            {
                if (ignor_ends)
                {
                    for (float a=angle2-dangle; a>angle1; a-=dangle)
                    {
                        inner_arc_push(System.Math.Cos(a), System.Math.Sin(a));
                    }
                }
                else
                {
                    for (float a=angle2; ; a-=dangle)
                    {
                        if (a<angle1) {
                            a=angle1;
                        }

                        inner_arc_push(System.Math.Cos(a), System.Math.Sin(a));

                        if (a<=angle1) {
                            break;
                        }
                    }
                }
            }
            else //decremental
            {
                if (ignor_ends)
                {
                    for (float a=angle2+dangle; a<angle1; a+=dangle)
                    {
                        inner_arc_push(System.Math.Cos(a), System.Math.Sin(a));
                    }
                }
                else
                {
                    for (float a=angle2; ;a+=dangle)
                    {
                        if (a>angle1) {
                            a=angle1;
                        }

                        inner_arc_push(System.Math.Cos(a), System.Math.Sin(a));

                        if (a>=angle1) {
                            break;
                        }
                    }
                }
            }
        } // inner_arc

        private static void vectors_to_arc(
                VertexArrayHolder hold,
                Point P, Color C, Color C2,
                Point A, Point B, float dangle, float r, float r2, bool ignor_ends,
                Point apparent_P)
        {
            // triangulate an inner arc between vectors A and B,
            // A and B are position vectors relative to P
            const float m_pi = (float) System.Math.PI;
            const float vaser_min_alw = 0.00000000001f;
            A *= 1/r;
            B *= 1/r;
            if (A.x > 1.0f-vaser_min_alw) A.x = 1.0f-vaser_min_alw;
            if (A.x <-1.0f+vaser_min_alw) A.x =-1.0f+vaser_min_alw;
            if (B.x > 1.0f-vaser_min_alw) B.x = 1.0f-vaser_min_alw;
            if (B.x <-1.0f+vaser_min_alw) B.x =-1.0f+vaser_min_alw;

            float angle1 = (float) System.Math.Acos(A.x);
            float angle2 = (float) System.Math.Acos(B.x);
            if (A.y>0) {
                angle1 = 2*m_pi-angle1;
            }
            if (B.y>0) {
                angle2 = 2*m_pi-angle2;
            }

            inner_arc(hold, P, C, C2, dangle, angle1, angle2, r, r2, ignor_ends, apparent_P, true);
        }

        private static float get_PLJ_round_dangle(float t, float r)
        {
            float dangle = 0.25f;
            float sum = t+r;
            /* // resolution dependent
            if (sum <= 1.44f+1.08f) //w<=4.0, feathering=1.0
                dangle = 0.6f/(t+r);
            else if (sum <= 3.25f+1.08f) //w<=6.5, feathering=1.0
                dangle = 2.8f/(t+r);
            */
            return dangle;
        }

        private static void push_quad(VertexArrayHolder core,
                Point P0, Point P1, Point P2, Point P3,
                Color c0, Color c1, Color c2, Color c3)
        {
            core.Push3(P0, P1, P2, c0, c1, c2);
            core.Push3(P2, P1, P3, c2, c1, c3);
        }

        private static void same_side_of_line(ref Point V, Point R, Point a, Point b)
        {
            float sign1 = Point.signed_area(a+R, a,b);
            float sign2 = Point.signed_area(a+V, a,b);
            if ((sign1>=0) != (sign2>=0))
            {
                V.opposite();
            }
        }
    }
}