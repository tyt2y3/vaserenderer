using UnityEngine;
using System.Collections.Generic;

namespace Vaser
{
    public class Polyline
    {
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
        // to hold info for anchor_late() to perform triangluation
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

            public Point cap_start;
            public Point cap_end;
            public st_polyline[] SL = new st_polyline[3];
            public VertexArrayHolder vah;
        }

        private static void determine_t_r(float w, ref float t, ref float R)
        {
            //efficiency: can cache one set of w,t,R values
            // i.e. when a polyline is of uniform thickness, the same w is passed in repeatedly
            float f = w - (float)System.Math.Floor(w);

            /*   */if ( w>=0.0f && w<1.0f) {
                t=0.05f; R=0.768f; R=0.0f;
            } else if ( w>=1.0f && w<2.0f) {
                t=0.05f+f*0.33f; R=0.768f+0.312f*f;
            } else if ( w>=2.0f && w<3.0f){
                t=0.38f+f*0.58f; R=1.08f;
            } else if ( w>=3.0f && w<4.0f){
                t=0.96f+f*0.48f; R=1.08f;
            } else if ( w>=4.0f && w<5.0f){
                t=1.44f+f*0.46f; R=1.08f;
            } else if ( w>=5.0f && w<6.0f){
                t=1.9f+f*0.6f; R=1.08f;
            } else if ( w>=6.0f){
                float ff=w-6.0f;
                t=2.5f+ff*0.50f; R=1.08f;
            }
        }

        private static void make_T_R_C(
            ref Point P1, ref Point P2, ref Point T, ref Point R, ref Point C,
            float w, polyline_opt opt,
            ref float rr, ref float tt, ref float dist,
            bool seg_mode=false)
        {
            float t = 1.0f, r = 0.0f;
            Point DP = P2 - P1;

            //calculate t,r
            determine_t_r(w, ref t, ref r);

            if (opt.feather && !opt.no_feather_at_core && opt.feathering != 1.0f)
            {
                r *= opt.feathering;
            }
            else if (seg_mode)
            {
                //TODO: handle correctly for hori/vert segments in a polyline
                if (Point.negligible(DP.x) && P1.x==System.Math.Floor(P1.x)) {
                    if (w>0.0f && w<=1.0f) {
                        t=0.5f; r=0.0f;
                        P2.x = P1.x = (float)System.Math.Floor(P1.x+0.5f);
                    }
                } else if (Point.negligible(DP.y) && P1.y==System.Math.Floor(P1.y)) {
                    if (w>0.0f && w<=1.0f) {
                        t=0.5f; r=0.0f;
                        P2.y = P1.y = (float)System.Math.Floor(P1.y+0.5f);
                    }
                }
            }

            //output t,r
            tt = t;
            rr = r;

            //calculate T,R,C
            dist = DP.normalize();
            C = DP;
            DP.perpen();

            T = DP*t;
            R = DP*r;
        }

        public static void Anchor(st_anchor SA, polyline_opt opt, bool cap_first, bool cap_last)
        {
            Point[] P = SA.P;
            Color[] C = SA.C;
            float[] weight = SA.W;
            st_polyline[] SL = SA.SL;
            SA.vah.SetGlDrawMode(VertexArrayHolder.GL_TRIANGLES);

            //const float critical_angle=11.6538;
            //  critical angle in degrees where a miter is force into bevel
            //  it is _similar_ to cairo_set_miter_limit () but cairo works with ratio while VASEr works with included angle
            const float cos_cri_angle=0.979386f; //cos(critical_angle)

            bool varying_weight = !(weight[0]==weight[1] & weight[1]==weight[2]);

            /*float combined_weight=weight[1]+(opt.feather?opt.feathering:0.0);
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

            Point T1=new Point(),T2=new Point(),T21=new Point(),T31=new Point(); //]these are for calculations in early stage
            Point R1=new Point(),R2=new Point(),R21=new Point(),R31=new Point(); //]

            for (int i=0; i<3; i++)
            {   //lower the transparency for weight < 1.0
                if (weight[i]>=0.0 && weight[i]<1.0)
                {
                    float f=weight[i];
                    C[i].a *=f;
                }
            }

            {
                int i=0;

                Point cap0=new Point(),cap1=new Point();
                float r=0f,t=0f,d=0f;
                make_T_R_C(ref P[i], ref P[i+1], ref T2, ref R2, ref cap1, weight[i], opt, ref r, ref t, ref d);
                if (varying_weight) {
                make_T_R_C(ref P[i], ref P[i+1], ref T31, ref R31, ref cap0, weight[i+1], opt, ref d, ref d, ref d);
                } else {
                    T31 = T2;
                    R31 = R2;
                }
                Point.anchor_outward(ref R2, P[i+1], P[i+2]);
                    T2.follow_signs(R2);

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
                SL[i].R=R2;
                SL[i].t=(float)t;
                SL[i].r=(float)r;
                SL[i].degenT = false;
                SL[i].degenR = false;
                
                SL[i+1].T1=T31;
                SL[i+1].R1=R31;
            }

            if (cap_last)
            {
                int i=2;

                Point cap0=new Point(),cap2=new Point();
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
                Point P_cur = P[i]; //current point //to avoid calling constructor repeatedly
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
                make_T_R_C(ref P_las, ref P_cur, ref T1, ref R1, ref cap0, weight[i-1], opt, ref d, ref d, ref length_cur);
                if (varying_weight) {
                make_T_R_C(ref P_las, ref P_cur, ref T21, ref R21, ref cap0, weight[i], opt, ref d, ref d, ref d);
                } else {
                    T21 = T1;
                    R21 = R1;
                }

                make_T_R_C(ref P_cur, ref P_nxt, ref T2, ref R2, ref bR, weight[i], opt, ref r, ref t, ref length_nxt);
                if (varying_weight) {
                make_T_R_C(ref P_cur, ref P_nxt, ref T31, ref R31, ref cap0, weight[i+1], opt, ref d, ref d, ref d);
                } else {
                    T31 = T2;
                    R31 = R2;
                }

                SL[i].T=T2;
                SL[i].R=R2;
                SL[i].bR=bR;
                SL[i].t=t;
                SL[i].r=r;
                SL[i].degenT = false;
                SL[i].degenR = false;

                SL[i+1].T1=T31;
                SL[i+1].R1=R31;
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
                        R1.follow_signs(T1);
                    Point.anchor_outward(ref T21, P_cur,P_nxt);
                        R21.follow_signs(T21);
                        SL[i].T1.follow_signs(T21);
                        SL[i].R1.follow_signs(T21);
                    Point.anchor_outward(ref T2, P_cur,P_las);
                        R2.follow_signs(T2);
                        SL[i].T.follow_signs(T2);
                        SL[i].R.follow_signs(T2);
                    Point.anchor_outward(ref T31, P_cur,P_las);
                        R31.follow_signs(T31);

                    {   //must do intersection
                        Point interP=new Point(), vP=new Point();
                        result3 = Point.intersect(
                                    P_las+T1, P_cur+T21,
                                    P_nxt+T31, P_cur+T2,
                                    ref interP);

                        if (result3 != 0) {
                            vP = interP - P_cur;
                            SL[i].vP=vP;
                            SL[i].vR=vP*(r/t);
                        } else {
                            SL[i].vP=SL[i].T;
                            SL[i].vR=SL[i].R;
                            Debug.Log("intersection failed: cos(angle)=%.4f, angle=%.4f(degree)"/*, cos_tho, System.Math.Acos(cos_tho)*180/3.14159*/);
                        }
                    }

                    T1.opposite();      //]inward
                        R1.opposite();
                    T21.opposite();
                        R21.opposite();
                    T2.opposite();
                        R2.opposite();
                    T31.opposite();
                        R31.opposite();

                    //make intersections
                    Point PT1=new Point(),PT2=new Point();
                    float pt1=0f,pt2=0f;
                    float[] pts = new float[2];
                    //
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
                    bool inner_sec = Point.intersecting(P_las+T1+R1, P_cur+T21+R21,
                                P_nxt+T31+R31, P_cur+T2+R2);
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
                        //efficiency: if color and weight is same as previous and next point
                        // ,do not generate vertices
                        same_side_of_line(ref SL[i].R, SL[i-1].R, P_cur, P_las);
                            SL[i].T.follow_signs(SL[i].R);
                        SL[i].vP=SL[i].T;
                        SL[i].T1.follow_signs(SL[i].T);
                        SL[i].R1.follow_signs(SL[i].T);
                        SL[i].vR=SL[i].R;
                        SL[i].djoint=polyline_opt.PLJ_miter;
                    }
                } //2nd to 2nd last point
            }

            {
                int i=2;

                Point cap0=new Point();
                float r=0f,t=0f,d=0f;
                make_T_R_C(ref P[i-1], ref P[i], ref T2, ref R2, ref cap0, weight[i], opt, ref r, ref t, ref d);
                    same_side_of_line(ref R2, SL[i-1].R, P[i-1], P[i]);
                        T2.follow_signs(R2);

                SL[i].djoint=opt.cap;
                SL[i].T=T2;
                SL[i].R=R2;
                SL[i].t=(float)t;
                SL[i].r=(float)r;
                SL[i].degenT = false;
                SL[i].degenR = false;
            }

            /*if (cap_first || cap_last) {
                anchor_cap(SA.P,SA.C, SA.SL,SA.vah, SA.cap_start,SA.cap_end);
            }
            anchor_late(SA.P,SA.C, SA.SL,SA.vah, SA.cap_start,SA.cap_end);
            */
            return;
        } //anchor

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