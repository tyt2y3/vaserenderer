using UnityEngine;
using System.Collections.Generic;

namespace Vaser
{
    public class Polyline
    {
        private VertexArrayHolder holder;

        public class polyline_opt
        {
            public char joint = PLJ_miter;
            public char cap = PLC_butt;
            public bool feather = false;
            public float feathering = 0.0f;
            public bool no_feather_at_cap = false;
            public bool no_feather_at_core = false;

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

            public float world_to_screen_ratio = 1.0f;
            public bool triangulation = false;
        }

        public Polyline(
            List<Point> P,
            List<Color> C,
            List<float> W,
            polyline_opt opt)
                : this (P, C, W, opt, null)
        {
            // empty
        }

        public Polyline(
            List<Point> P,
            Color C,
            float W,
            polyline_opt opt)
                : this (P, new List<Color> {C}, new List<float> {W}, opt, new polyline_inopt {
                    const_color = true, const_weight = true,
                })
        {
            // empty
        }

        private Polyline(
            List<Point> P,
            List<Color> C,
            List<float> W,
            polyline_opt opt,
            polyline_inopt inopt)
        {
            int length = P.Count;
            if (opt == null) {
                opt = new polyline_opt();
            }
            if (inopt == null) {
                inopt = new polyline_inopt();
            }

            /* if( opt.fallback)
            {
                backend::polyline(P,C[0],W[0],length,0);
                return;
            } */

            if (opt.cap >= 10)
            {
                char dec = (char) ((int) opt.cap - ((int) opt.cap % 10));
                if (dec==polyline_opt.PLC_first || dec==polyline_opt.PLC_none) {
                    inopt.no_cap_last=true;
                }
                if (dec==polyline_opt.PLC_last || dec==polyline_opt.PLC_none) {
                    inopt.no_cap_first=true;
                }
                opt.cap -= dec;
            }

            int A=0, B=0;
            bool on=false;
            for (int i=1; i<length-1; i++)
            {
                Point V1 = P[i]-P[i-1];
                Point V2 = P[i+1]-P[i];
                float len=0;
                if (inopt.segment_length != null) {
                    V1 *= 1/inopt.segment_length[i];
                    V2 *= 1/inopt.segment_length[i+1];
                    len += (inopt.segment_length[i]+inopt.segment_length[i+1])*0.5f;
                } else {
                    len += V1.normalize()*0.5f;
                    len += V2.normalize()*0.5f;
                }
                float costho = V1.x*V2.x+V1.y*V2.y;
                const float m_pi = (float) System.Math.PI;
                //float angle = acos(costho)*180/m_pi;
                float cos_a = (float) System.Math.Cos(15*m_pi/180);
                float cos_b = (float) System.Math.Cos(10*m_pi/180);
                float cos_c = (float) System.Math.Cos(25*m_pi/180);
                float weight = W[inopt.const_weight ? 0 : i];
                bool approx = false;
                if ((weight<7 && costho>cos_a) ||
                    (costho>cos_b) || //when the angle difference at an anchor is smaller than a critical degree, do polyline approximation
                    (len<weight && costho>cos_c)) { //when vector length is smaller than weight, do approximation
                    approx = true;
                }
                if (approx && !on) {
                    A=i; if(A==1) A=0;
                    on=true;
                    if (A>1) {
                        polyline_range(P,C,W,opt,inopt,B,A,false);
                    }
                } else if(!approx && on) {
                    B=i;
                    on=false;
                    polyline_range(P,C,W,opt,inopt,A,B,true);
                }
            }
            if (on && B<length-1) {
                B=length-1;
                polyline_range(P,C,W,opt,inopt,A,B,true);
            } else if (!on && A<length-1) {
                A=length-1;
                polyline_range(P,C,W,opt,inopt,B,A,false);
            }
            holder = inopt.holder;
            inopt.holder = null;
        }

        public Mesh GetMesh()
        {
            Mesh mesh = new Mesh();
            mesh.SetVertices(holder.GetVertices());
            mesh.SetUVs(0, holder.GetUVs());
            mesh.SetColors(holder.GetColors());
            mesh.SetTriangles(holder.GetTriangles(), 0);
            return mesh;
        }

        private class polyline_inopt
        {
            public bool const_color = false;
            public bool const_weight = false;
            public bool no_cap_first = false;
            public bool no_cap_last = false;
            public bool join_first = false;
            public bool join_last = false;
            public List<float> segment_length = null; // length of each segment; optional
            public VertexArrayHolder holder = new VertexArrayHolder();
        }

        private struct st_polyline
        // to hold info for AnchorLate() to perform triangulation
        {
            //for all joints
            public Point vP; //vector to intersection point; outward

            //for djoint==PLJ_bevel
            public Point T; //core thickness of a line
            public Point R; //fading edge of a line
            public Point bR; //out stepping vector, same direction as cap
            public Point T1; //alternate vectors, same direction as T21
            // all T,R,T1 are outward

            //for djoint==PLJ_round
            public float t,r;

            //for degeneration case
            public bool degenT; //core degenerated
            public bool degenR; //fade degenerated
            public bool pre_full; //draw the preceding segment in full
            public Point PT;
            public float pt; //parameter at intersection

            public char djoint; //determined joint
            // e.g. originally a joint is PLJ_miter. but it is smaller than critical angle, should then set djoint to PLJ_bevel
        }

        private class st_anchor
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

        private static void poly_point_inter(List<Point> P, List<Color> C, List<float> W, polyline_inopt inopt,
                ref Point p, ref Color c, ref float w, int at, float t)
        {
            Color color(int I) { return C[inopt != null && inopt.const_color ? 0: I]; }
            float weight(int I) { return W[inopt != null && inopt.const_weight ? 0: I]; }

            if (t == 0.0) {
                p = P[at];
                c = color(at);
                w = weight(at);
            } else if (t == 1.0) {
                p = P[at+1];
                c = color(at+1);
                w = weight(at+1);
            } else {
                p = (P[at]+P[at+1]) * t;
                c = ColorBetween(color(at),color(at+1), t);
                w = (weight(at)+weight(at+1)) * t;
            }
        }

        private static void polyline_approx(
            List<Point> P,
            List<Color> C,
            List<float> W,
            polyline_opt opt,
            polyline_inopt inopt,
            int from,
            int to)
        {
            if (to-from+1 < 2) {
                return;
            }
            bool cap_first = !(inopt != null && inopt.no_cap_first);
            bool cap_last  = !(inopt != null && inopt.no_cap_last);

            VertexArrayHolder vcore = new VertexArrayHolder(); //curve core
            vcore.SetGlDrawMode(VertexArrayHolder.GL_TRIANGLE_STRIP);

            Color color(int I) { return C[inopt != null && inopt.const_color ? from: I]; }
            float weight(int I) { return W[inopt != null && inopt.const_weight ? from: I]; }

            void poly_step(int i, Point pp, float ww, Color cc)
            {
                float t=0,r=0;
                determine_t_r(weight(i), ref t, ref r, opt.world_to_screen_ratio);
                float rr = (t+r) / r;
                if (opt.feather && !opt.no_feather_at_core) {
                    r *= opt.feathering;
                }
                Point V = P[i]-P[i-1];
                V.perpen();
                V.normalize();
                V *= (t+r);
                vcore.Push(pp+V, color(i), rr);
                vcore.Push(pp-V, color(i), rr);
            }

            for (int i=from+1; i<to; i++)
            {
                poly_step(i, P[i], weight(i), color(i));
            }
            Point P_las=new Point(), P_fir=new Point();
            Color C_las=new Color(), C_fir=new Color();
            float W_las=0, W_fir=0;
            poly_point_inter(P, C, W, inopt, ref P_las, ref C_las, ref W_las, to-1, 0.5f);
            poly_step(to, P_las, W_las, C_las);

            st_anchor SA = new st_anchor();
            if (cap_first)
            {
                poly_point_inter(P, C, W, inopt, ref P_fir, ref C_fir, ref W_fir, from, inopt != null && inopt.join_first ? 0.5f : 0.0f);
                SA.P[0] = P_fir;
                SA.P[1] = P[from+1];
                SA.C[0] = C_fir;
                SA.C[1] = color(from+1);
                SA.W[0] = W_fir;
                SA.W[1] = weight(from+1);
                Segment(SA, opt, cap_first, false, true);
            }
            if (!(inopt != null && inopt.join_last) && cap_last)
            {
                SA.P[0] = P_las;
                SA.P[1] = P[to];
                SA.C[0] = C_las;
                SA.C[1] = color(to);
                SA.W[0] = W_las;
                SA.W[1] = weight(to);
                Segment(SA, opt, false, cap_last, true);
            }

            inopt.holder.Push(vcore);
            inopt.holder.Push(SA.vah);

            if (opt.triangulation) {
                DrawTriangles(vcore, inopt.holder, opt.world_to_screen_ratio);
                DrawTriangles(SA.vah, inopt.holder, opt.world_to_screen_ratio);
            }
        }

        private static void polyline_exact(
            List<Point> P,
            List<Color> C,
            List<float> W,
            polyline_opt opt,
            polyline_inopt inopt,
            int from,
            int to)
        {
            bool cap_first = !(inopt != null && inopt.no_cap_first);
            bool cap_last  = !(inopt != null && inopt.no_cap_last);
            bool join_first = inopt != null && inopt.join_first;
            bool join_last = inopt != null && inopt.join_last;

            Color color(int I) { return C[inopt != null && inopt.const_color ? from: I]; }
            float weight(int I) { return W[inopt != null && inopt.const_weight ? from: I]; }

            Point mid_l=new Point(), mid_n=new Point(); //the last and the next mid point
            Color c_l=new Color(), c_n=new Color();
            float w_l=0, w_n=0;

            //init for the first anchor
            poly_point_inter(P, C, W, inopt, ref mid_l, ref c_l, ref w_l, from, join_first ? 0.5f : 0);

            st_anchor SA = new st_anchor();
            if (to-from+1 == 2) {
                SA.P[0] = P[from];
                SA.P[1] = P[from+1];
                SA.C[0] = color(from);
                SA.C[1] = color(from+1);
                SA.W[0] = weight(from);
                SA.W[1] = weight(from+1);
                Segment(SA, opt, cap_first, cap_last, true);
            } else {
                for (int i=from+1; i<to; i++)
                {
                    if (i == to-1 && !join_last) {
                        poly_point_inter(P, C, W, inopt, ref mid_n, ref c_n, ref w_n, i, 1.0f);
                    } else {
                        poly_point_inter(P, C, W, inopt, ref mid_n, ref c_n, ref w_n, i, 0.5f);
                    }

                    SA.P[0]=mid_l; SA.C[0]=c_l;  SA.W[0]=w_l;
                    SA.P[2]=mid_n; SA.C[2]=c_n;  SA.W[2]=w_n;

                    SA.P[1]=P[i];
                    SA.C[1]=color(i);
                    SA.W[1]=weight(i);

                    Anchor(SA, opt, i == 1 && cap_first, i == to-1 && cap_last);

                    mid_l = mid_n;
                    c_l = c_n;
                    w_l = w_n;
                }
            }

            inopt.holder.Push(SA.vah);
            if (opt.triangulation) {
                DrawTriangles(SA.vah, inopt.holder, opt.world_to_screen_ratio);
            }
        }

        private static void polyline_range(
            List<Point> P,
            List<Color> C,
            List<float> W,
            polyline_opt opt,
            polyline_inopt inopt,
            int from, int to,
            bool approx)
        {
            int length = P.Count;
            if (inopt == null) {
                inopt = new polyline_inopt();
            }
            if (from>0) {
                from-=1;
            }

            inopt.join_first = from!=0;
            inopt.join_last = to!=(length-1);
            inopt.no_cap_first = inopt.no_cap_first || inopt.join_first;
            inopt.no_cap_last = inopt.no_cap_last || inopt.join_last;

            if (approx) {
                polyline_approx(P, C, W, opt, inopt, from, to);
            } else {
                polyline_exact(P, C, W, opt, inopt, from, to);
            }
        }

        private static void DrawTriangles(VertexArrayHolder triangles, VertexArrayHolder holder, float scale)
        {
            polyline_opt opt = new polyline_opt();
            opt.cap = polyline_opt.PLC_none;
            //opt.joint = polyline_opt.PLJ_bevel;
            opt.world_to_screen_ratio = scale;
            if (triangles.glmode == VertexArrayHolder.GL_TRIANGLES) {
                for (int i=0; i<triangles.GetCount(); i++)
                {
                    List<Point> P = new List<Point>();
                    P.Add(triangles.Get(i)); i+=1;
                    P.Add(triangles.Get(i)); i+=1;
                    P.Add(triangles.Get(i));
                    P.Add(P[0]);
                    Polyline polyline = new Polyline(P, Color.red, 1/scale, opt);
                    holder.Push(polyline.holder);
                }
            } else if (triangles.glmode == VertexArrayHolder.GL_TRIANGLE_STRIP) {
                for (int i=2; i<triangles.GetCount(); i++)
                {
                    List<Point> P = new List<Point>();
                    P.Add(triangles.Get(i-2));
                    P.Add(triangles.Get(i));
                    P.Add(triangles.Get(i-1));
                    Polyline polyline = new Polyline(P, Color.red, 1/scale, opt);
                    holder.Push(polyline.holder);
                }
            }
        }

        private static void determine_t_r(float w, ref float t, ref float R, float scale)
        {
            //efficiency: can cache one set of w,t,R values
            // i.e. when a polyline is of uniform thickness, the same w is passed in repeatedly
            w *= scale;
            float f = w - (float) System.Math.Floor(w);
            // resolution dependent
            if (w>=0.0 && w<1.0) {
                t=0.05f; R=0.768f;
            } else if (w>=1.0 && w<2.0) {
                t=0.05f+f*0.33f; R=0.768f+0.312f*f;
            } else if (w>=2.0 && w<3.0) {
                t=0.38f+f*0.58f; R=1.08f;
            } else if (w>=3.0 && w<4.0) {
                t=0.96f+f*0.48f; R=1.08f;
            } else if (w>=4.0 && w<5.0) {
                t=1.44f+f*0.46f; R=1.08f;
            } else if (w>=5.0 && w<6.0) {
                t=1.9f+f*0.6f; R=1.08f;
            } else if (w>=6.0){
                t=2.5f+(w-6.0f)*0.50f; R=1.08f;
            }
            t /= scale;
            R /= scale;
        }

        private static void make_T_R_C(
            ref Point P1, ref Point P2, ref Point T, ref Point R, ref Point C,
            float w, polyline_opt opt,
            ref float rr, ref float tt, ref float dist,
            bool anchor_mode)
        {
            float t = 1.0f, r = 0.0f;

            //calculate t,r
            determine_t_r(w, ref t, ref r, opt.world_to_screen_ratio);
            if (opt.feather && !opt.no_feather_at_core && opt.feathering != 1.0) {
                r *= opt.feathering;
            }
            if (anchor_mode) {
                t += r;
            }

            //output
            tt = t;
            rr = r;
            Point DP = P2 - P1;
            dist = DP.normalize();
            C = DP*(1/opt.world_to_screen_ratio);
            DP.perpen();
            T = DP*t;
            R = DP*r;
        }

        private static void Segment(st_anchor SA, polyline_opt opt, bool cap_first, bool cap_last, bool core)
        {
            float[] weight = SA.W;

            Point[] P = new Point[2]; P[0]=SA.P[0]; P[1]=SA.P[1];
            Color[] C = new Color[3]; C[0]=SA.C[0]; C[1]=SA.C[1];

            Point T2=new Point();
            Point R2=new Point();
            Point bR=new Point();
            float t=0, r=0;

            bool varying_weight = !(weight[0]==weight[1]);

            Point cap_start=new Point(), cap_end=new Point();
            st_polyline[] SL = new st_polyline[2];

            for (int i=0; i<2; i++)
            {   //lower the transparency for weight < 1.0
                float actualWeight = weight[i] * opt.world_to_screen_ratio;
                if (actualWeight >= 0.0 && actualWeight < 1.0) {
                    C[i].a *= actualWeight;
                }
            }

            {
                int i = 0;
                float dd = 0f;
                make_T_R_C(ref P[i], ref P[i+1], ref T2, ref R2, ref bR, weight[i], opt, ref r, ref t, ref dd, false);

                if (cap_first)
                {
                    if (opt.cap==polyline_opt.PLC_square) {
                        P[0] = new Point(P[0]) - bR * (t+r);
                    }
                    cap_start = bR;
                    cap_start.opposite();
                    if (opt.feather && !opt.no_feather_at_cap) {
                        cap_start *= opt.feathering;
                    }
                }

                SL[i].djoint=opt.cap;
                SL[i].t=t;
                SL[i].r=r;
                SL[i].T=T2;
                SL[i].R=R2;
                SL[i].bR=bR;
                SL[i].degenT = false;
                SL[i].degenR = false;
            }

            {
                int i = 1;
                float dd = 0f;
                if (varying_weight) {
                    make_T_R_C(ref P[i-1], ref P[i], ref T2, ref R2, ref bR, weight[i], opt, ref r, ref t, ref dd, false);
                }

                if (cap_last)
                {
                    if (opt.cap==polyline_opt.PLC_square) {
                        P[1] += bR * (t+r);
                    }
                    cap_end = bR;
                    if (opt.feather && !opt.no_feather_at_cap) {
                        cap_end *= opt.feathering;
                    }
                }

                SL[i].djoint = opt.cap;
                SL[i].t=t;
                SL[i].r=r;
                SL[i].T=T2;
                SL[i].R=R2;
                SL[i].bR=bR;
                SL[i].degenT = false;
                SL[i].degenR = false;
            }

            SegmentLate(opt, P, C, SL, SA.vah, cap_start, cap_end, core);
        }

        private static void SegmentLate(
                polyline_opt opt,
                Point[] P, Color[] C, st_polyline[] SL,
                VertexArrayHolder tris,
                Point cap1, Point cap2, bool core)
        {
            tris.SetGlDrawMode(VertexArrayHolder.GL_TRIANGLES);

            Point P_0, P_1;
            P_0 = new Point(P[0]);
            P_1 = new Point(P[1]);
            if (SL[0].djoint==polyline_opt.PLC_butt || SL[0].djoint==polyline_opt.PLC_square) {
                P_0 -= cap1;
            }
            if (SL[1].djoint==polyline_opt.PLC_butt || SL[1].djoint==polyline_opt.PLC_square) {
                P_1 -= cap2;
            }

            Point P1, P2, P3, P4;  //core
            Point P1c,P2c,P3c,P4c; //cap
            Point P1r,P2r,P3r,P4r; //fade
            float rr = (SL[0].t + SL[0].r) / SL[0].r;

            P1 = P_0 + SL[0].T + SL[0].R;
                P1r = P1 - SL[0].R;
                P1c = P1 + cap1;
            P2 = P_0 - SL[0].T - SL[0].R;
                P2r = P2 + SL[0].R;
                P2c = P2 + cap1;
            P3 = P_1 + SL[1].T + SL[1].R;
                P3r = P3 - SL[1].R;
                P3c = P3 + cap2;
            P4 = P_1 - SL[1].T - SL[1].R;
                P4r = P4 + SL[1].R;
                P4c = P4 + cap2;

            //core
            if (core) {
                push_quad(
                    tris, ref P1, ref P2, ref P3, ref P4,
                    ref C[0], ref C[0], ref C[1], ref C[1],
                    rr, 0, 0, rr);
            }

            //caps
            for (int j=0; j<2; j++)
            {
                VertexArrayHolder cap = new VertexArrayHolder();
                cap.SetGlDrawMode(VertexArrayHolder.GL_TRIANGLE_STRIP);
                Point cur_cap = j==0 ? cap1 : cap2;
                if (cur_cap.is_zero()) {
                    continue;
                }

                if (SL[j].djoint == polyline_opt.PLC_round) {
                    //round cap
                    Point O = P[j];
                    float dangle = get_PLJ_round_dangle(SL[j].t, SL[j].r, opt.world_to_screen_ratio);

                    vectors_to_arc(
                        cap, O, C[j], C[j],
                        SL[j].T+SL[j].R, -SL[j].T-SL[j].R,
                        dangle,
                        SL[j].t+SL[j].r, 0.0f, false, O, j==0 ? cap1 : cap2, rr, false);
                    //cap.Push(O-SL[j].T-SL[j].R, C[j]);
                    //cap.Push(O, C[j]);
                    tris.Push(cap);
                } else if (SL[j].djoint != polyline_opt.PLC_none) {
                    //rectangular cap
                    Point Pj, Pjr, Pjc, Pk, Pkr, Pkc;
                    if (j==0)
                    {
                        Pj = P1;
                        Pjr= P1r;
                        Pjc= P1c;
                        
                        Pk = P2;
                        Pkr= P2r;
                        Pkc= P2c;
                    }
                    else
                    {
                        Pj = P4;
                        Pjr= P4r;
                        Pjc= P4c;
                        
                        Pk = P3;
                        Pkr= P3r;
                        Pkc= P3c;
                    }

                    tris.PushF(Pk,  C[j]);
                    tris.PushF(Pkc, C[j]);
                    tris.Push (Pkr, C[j]);
                    tris.Push (Pkr, C[j]);
                    tris.PushF(Pkc, C[j]);
                    tris.Push (Pjr, C[j]);
                    tris.Push (Pjr, C[j]);
                    tris.PushF(Pkc, C[j]);
                    tris.PushF(Pjc, C[j]);
                    tris.Push (Pjr, C[j]);
                    tris.PushF(Pjc, C[j]);
                    tris.PushF(Pj,  C[j]);
                }
            }
        }

        private static void Anchor(st_anchor SA, polyline_opt opt, bool cap_first, bool cap_last)
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

            Point T1=new Point(), T2=new Point(), T21=new Point(), T31=new Point(), RR=new Point();

            for (int i=0; i<3; i++)
            {   //lower the transparency for weight < 1.0
                float actualWeight = weight[i] * opt.world_to_screen_ratio;
                if (actualWeight >= 0.0 && actualWeight < 1.0) {
                    C[i].a *= actualWeight;
                }
            }

            {
                int i=0;

                Point cap0=new Point(), cap1=new Point();
                float r=0f,t=0f,d=0f;
                make_T_R_C(ref P[i], ref P[i+1], ref T2, ref RR, ref cap1, weight[i], opt, ref r, ref t, ref d, true);
                if (varying_weight) {
                    make_T_R_C(ref P[i], ref P[i+1], ref T31, ref RR, ref cap0, weight[i+1], opt, ref d, ref d, ref d, true);
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
                make_T_R_C(ref P[i-1], ref P[i], ref cap0, ref cap0, ref cap2, weight[i], opt, ref r, ref t, ref d, true);
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
                make_T_R_C(ref P_las, ref P_cur, ref T1, ref RR, ref cap0, weight[i-1], opt, ref d, ref d, ref length_cur, true);
                if (varying_weight) {
                make_T_R_C(ref P_las, ref P_cur, ref T21, ref RR, ref cap0, weight[i], opt, ref d, ref d, ref d, true);
                } else {
                    T21 = T1;
                }

                make_T_R_C(ref P_cur, ref P_nxt, ref T2, ref RR, ref bR, weight[i], opt, ref r, ref t, ref length_nxt, true);
                if (varying_weight) {
                make_T_R_C(ref P_cur, ref P_nxt, ref T31, ref RR, ref cap0, weight[i+1], opt, ref d, ref d, ref d, true);
                } else {
                    T31 = T2;
                }

                SL[i].T=T2;
                SL[i].bR=bR;
                SL[i].t=t;
                SL[i].r=r;
                SL[i].degenT = false;

                SL[i+1].T1=T31;
                }

                {   //2nd point

                    //find the angle between the 2 line segments
                    Point ln1=new Point(), ln2=new Point(), V=new Point();
                    ln1 = P_cur - P_las;
                    ln2 = P_nxt - P_cur;
                    ln1.normalize();
                    ln2.normalize();
                    Point.dot(ln1, ln2, ref V);
                    float cos_tho=V.x-V.y;
                    bool zero_degree = System.Math.Abs(cos_tho-1.0f) < 0.0000001f;
                    bool d180_degree = cos_tho < -1.0f+0.0001f;
                    int result3 = 1;

                    if (zero_degree)
                    {
                        Segment(SA, opt, cap_first, false, true);
                        SA.P[0]=SA.P[1]; SA.P[1]=SA.P[2];
                        SA.C[0]=SA.C[1]; SA.C[1]=SA.C[2];
                        SA.W[0]=SA.W[1]; SA.W[1]=SA.W[2];
                        Segment(SA, opt, false, cap_last, true);
                        return;
                    }

                    SL[i].djoint = opt.joint;
                    if (opt.joint == polyline_opt.PLJ_miter) {
                        if (System.Math.Abs(cos_tho) >= cos_cri_angle) {
                            SL[i].djoint = polyline_opt.PLJ_bevel;
                        }
                    }

                    Point.anchor_outward(ref T1, P_cur,P_nxt);
                    Point.anchor_outward(ref T21, P_cur,P_nxt);
                        SL[i].T1.follow_signs(T21);
                    Point.anchor_outward(ref T2, P_cur,P_las);
                        SL[i].T.follow_signs(T2);
                    Point.anchor_outward(ref T31, P_cur,P_las);

                    {   //must do intersection
                        Point interP=new Point(), vP=new Point();
                        float[] pts = new float[2];
                        result3 = Point.intersect(
                            P_las+T1, P_cur+T21,
                            P_nxt+T31, P_cur+T2,
                            ref interP, pts);

                        if (result3 != 0) {
                            vP = interP - P_cur;
                            SL[i].vP=vP;
                            if (pts[0] > 2 || pts[1] > 2) {
                                SL[i].djoint = polyline_opt.PLJ_bevel;
                            }
                        } else {
                            SL[i].vP=SL[i].T;
                            SL[i].djoint = polyline_opt.PLJ_bevel;
                            Debug.Log(System.String.Format("intersection failed: cos(angle)={0}, angle={1} (degree)", cos_tho, System.Math.Acos(cos_tho)*180/3.14159));
                        }
                    }

                    T1.opposite();
                    T21.opposite();
                    T2.opposite();
                    T31.opposite();

                    //make intersections
                    Point PT1=new Point(), PT2=new Point();
                    float pt1=0f,pt2=0f;
                    int result1t, result2t;
                    {
                        float[] pts = new float[2];
                        result1t = Point.intersect(
                                    P_nxt-T31, P_nxt+T31,
                                    P_las+T1, P_cur+T21, //knife1_a
                                    ref PT1, pts); //core
                        pt1 = pts[1];
                        result2t = Point.intersect(
                                    P_las-T1, P_las+T1,
                                    P_nxt+T31, P_cur+T2, //knife2_a
                                    ref PT2, pts);
                        pt2 = pts[1];
                    }
                    bool is_result1t = result1t == 1;
                    bool is_result2t = result2t == 1;

                    if (is_result1t | is_result2t)
                    {   //core degeneration
                        SL[i].degenT=true;
                        SL[i].pre_full=is_result1t;
                        SL[i].PT = is_result1t? PT1:PT2;
                        SL[i].pt = is_result1t? pt1:pt2;
                    }

                    if (d180_degree | result3 == 0)
                    {   //to solve visual bugs 3 and 1.1
                        SL[i].vP=SL[i].T;
                        SL[i].T1.follow_signs(SL[i].T);
                        SL[i].djoint=polyline_opt.PLJ_miter;
                    }
                }
            }

            {
                int i=2;

                Point cap0=new Point();
                float r=0f,t=0f,d=0f;
                make_T_R_C(ref P[i-1], ref P[i], ref T2, ref RR, ref cap0, weight[i], opt, ref r, ref t, ref d, true);

                SL[i].djoint=opt.cap;
                SL[i].T=T2;
                SL[i].t=(float)t;
                SL[i].degenT = false;
            }

            AnchorLate(opt, P, C, SA.SL, SA.vah, SA.cap_start, SA.cap_end);
            if (cap_first) {
                Segment(SA, opt, true, false, false);
            }
            if (cap_last) {
                SA.P[0] = SA.P[1]; SA.P[1] = SA.P[2];
                SA.C[0] = SA.C[1]; SA.C[1] = SA.C[2];
                SA.W[0] = SA.W[1]; SA.W[1] = SA.W[2];
                Segment(SA, opt, false, true, false);
            }
        } //Anchor

        private static void AnchorLate(
            polyline_opt opt,
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
            float rr = SL[1].t / SL[1].r;

            if (SL[1].degenT)
            {
                P1 = SL[1].PT;
                tris.Push3(P1, P3, P2, C[1], C[0], C[1], 0, rr, 0); //fir seg
                tris.Push3(P1, P5, P6, C[1], C[1], C[2], 0, rr, 0); //las seg

                if (SL[1].pre_full) {
                    tris.Push3(P3, P1, P4, C[0], C[1], C[0], 0, rr, 0);
                } else {
                    tris.Push3(P1, P6, P7, C[1], C[2], C[2], 0, 0, rr);
                }
            }
            else
            {
                // normal first segment
                tris.Push3(P2, P4, P3, C[1], C[0], C[0], 0, 0, rr);
                tris.Push3(P4, P2, P1, C[0], C[1], C[1], 0, 0, rr);
                // normal last segment
                tris.Push3(P5, P7, P1, C[1], C[2], C[1], 0, rr, 0);
                tris.Push3(P7, P5, P6, C[2], C[1], C[2], 0, rr, 0);
            }

            if (normal_line_core_joint != 0)
            {
                switch (SL[1].djoint)
                {
                    case polyline_opt.PLJ_miter:
                        tris.Push3(P2, P0, P1, C[1], C[1], C[1], rr, 0, 0);
                        tris.Push3(P1, P0, P5, C[1], C[1], C[1], 0, rr, 0);
                    break;

                    case polyline_opt.PLJ_bevel:
                        if (normal_line_core_joint==1)
                        tris.Push3(P2, P5, P1, C[1], C[1], C[1], rr, 0, 0);
                    break;

                    case polyline_opt.PLJ_round: {
                        VertexArrayHolder strip = new VertexArrayHolder();
                        strip.SetGlDrawMode(VertexArrayHolder.GL_TRIANGLE_STRIP);

                    if (normal_line_core_joint==1) {
                        vectors_to_arc(strip, P_1, C[1], C[1], SL[1].T1, SL[1].T,
                            get_PLJ_round_dangle(SL[1].t, SL[1].r, opt.world_to_screen_ratio),
                            SL[1].t, 0.0f, false, P1, new Point(), rr, true);
                    } else if (normal_line_core_joint==2) {
                        vectors_to_arc(strip, P_1, C[1], C[1], SL[1].T1, SL[1].T,
                            get_PLJ_round_dangle(SL[1].t, SL[1].r, opt.world_to_screen_ratio),
                            SL[1].t, 0.0f, false, P5, new Point(), rr, true);
                    }
                        tris.Push(strip);
                    } break;
                }
            }
        } //AnchorLate

        private static void vectors_to_arc(
            VertexArrayHolder hold,
            Point P, Color C, Color C2,
            Point PA, Point PB, float dangle, float r, float r2, bool ignor_ends,
            Point apparent_P, Point hint, float rr, bool inner_fade)
        {
            // triangulate an inner arc between vectors A and B,
            // A and B are position vectors relative to P
            const float m_pi = (float) System.Math.PI;
            Point A = PA * (1/r);
            Point B = PB * (1/r);
            float rrr;
            if (inner_fade) {
                rrr = 0;
            } else {
                rrr = -1;
            }

            float angle1 = (float) System.Math.Acos(A.x);
            float angle2 = (float) System.Math.Acos(B.x);
            if (A.y>0) {
                angle1 = 2*m_pi-angle1;
            }
            if (B.y>0) {
                angle2 = 2*m_pi-angle2;
            }
            if (hint.non_zero()) {
                // special case when angle1 == angle2,
                //   have to determine which side by hint
                if (hint.x > 0 && hint.y == 0) {
                    angle1 -= (angle1 < angle2 ? 1:-1) * 0.00001f;
                } else if (hint.x == 0 && hint.y > 0) {
                    angle1 -= (angle1 < angle2 ? 1:-1) * 0.00001f;
                } else if (hint.x > 0 && hint.y > 0) {
                    angle1 -= (angle1 < angle2 ? 1:-1) * 0.00001f;
                } else if (hint.x > 0 && hint.y < 0) {
                    angle1 -= (angle1 < angle2 ? 1:-1) * 0.00001f;
                } else if (hint.x < 0 && hint.y > 0) {
                    angle1 += (angle1 < angle2 ? 1:-1) * 0.00001f;
                } else if (hint.x < 0 && hint.y < 0) {
                    angle1 += (angle1 < angle2 ? 1:-1) * 0.00001f;
                }
            }

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
                if (!ignor_ends) {
                    hold.Push(new Point(P.x+PB.x, P.y+PB.y), C, rr);
                    hold.Push(apparent_P, C2, rrr);
                }
                for (float a=angle2-dangle; a>angle1; a-=dangle)
                {
                    inner_arc_push(System.Math.Cos(a), System.Math.Sin(a));
                }
                if (!ignor_ends) {
                    hold.Push(new Point(P.x+PA.x, P.y+PA.y), C, rr);
                    hold.Push(apparent_P, C2, rrr);
                }
            }
            else //decremental
            {
                if (!ignor_ends) {
                    hold.Push(apparent_P, C2, rr);
                    hold.Push(new Point(P.x+PB.x, P.y+PB.y), C, rrr);
                }
                for (float a=angle2+dangle; a<angle1; a+=dangle) {
                    inner_arc_push(System.Math.Cos(a), System.Math.Sin(a), true);
                }
                if (!ignor_ends) {
                    hold.Push(apparent_P, C2, rr);
                    hold.Push(new Point(P.x+PA.x, P.y+PA.y), C, rrr);
                }
            }

            void inner_arc_push(double x, double y, bool reverse=false)
            {
                Point PP = new Point(P.x+(float)x*r, P.y-(float)y*r);
                //hold.Dot(PP, 0.05f); return;
                if (!reverse) {
                    hold.Push(PP, C, rr);
                    hold.Push(apparent_P, C2, rrr);
                } else {
                    hold.Push(apparent_P, C2, rr);
                    hold.Push(PP, C, rrr);
                }
            }
        }

        private static float get_PLJ_round_dangle(float t, float r, float scale)
        {
            float dangle;
            float sum = (t+r) * scale;
            if (sum <= 1.44f+1.08f) { //w<=4.0, feathering=1.0
                dangle = 0.6f/sum;
            } else if (sum <= 3.25f+1.08f) { //w<=6.5, feathering=1.0
                dangle = 2.8f/sum;
            } else {
                dangle = 4.2f/sum;
            }
            return dangle;
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

        private static void push_quad(VertexArrayHolder tris,
            ref Point P1, ref Point P2, ref Point P3, ref Point P4,
            ref Color C1, ref Color C2, ref Color C3, ref Color C4,
            float r1, float r2, float r3, float r4)
        {
            //interpret P0 to P3 as triangle strip
            tris.Push3(P1, P3, P2, C1, C3, C2, r1, r3, r2);
            tris.Push3(P2, P3, P4, C2, C3, C4, r2, r3, r4);
        }

        private static Color ColorBetween(Color A, Color B, float t=0.5f)
        {
            if (t<0.0f) t = 0.0f;
            if (t>1.0f) t = 1.0f;

            float kt = 1.0f - t;
            return new Color
            (
                A.r *kt + B.r *t,
                A.g *kt + B.g *t,
                A.b *kt + B.b *t,
                A.a *kt + B.a *t
            );
        }
    }
}