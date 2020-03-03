using UnityEngine;
using System.Collections.Generic;

namespace Vaser
{
    public class Polyline
    {
        public VertexArrayHolder holder;

        public class Opt
        {
            public char joint = PLJmiter;
            public char cap = PLCbutt;
            public bool feather = false;
            public float feathering = 0.0f;
            public bool noFeatherAtCap = false;
            public bool noFeatherAtCore = false;
            public float worldToScreenRatio = 1.0f;
            public bool triangulation = false;

            //for Opt.joint
            public const char PLJmiter = (char) 0; //default
            public const char PLJbevel = (char) 1;
            public const char PLJround = (char) 2;
            //for Opt.cap
            public const char PLCbutt   = (char) 0; //default
            public const char PLCround  = (char) 1;
            public const char PLCsquare = (char) 2;
            public const char PLCrect   = (char) 3;
            public const char PLCboth   = (char) 0; //default
            public const char PLCfirst  = (char) 10;
            public const char PLClast   = (char) 20;
            public const char PLCnone   = (char) 30;
        }

        public class Inopt
        {
            public bool constColor = false;
            public bool constWeight = false;
            public bool noCapFirst = false;
            public bool noCapLast = false;
            public bool joinFirst = false;
            public bool joinLast = false;
            public List<float> segmentLength = null; // length of each segment; optional
            public VertexArrayHolder holder = new VertexArrayHolder();

            public Inopt ShallowCopy()
            {
                return (Inopt) this.MemberwiseClone();
            }
        }

        public Polyline()
        {
            holder = new VertexArrayHolder();
        }

        public Polyline(
            List<Vector2> P, List<Color> C, List<float> W, Opt opt)
            : this(P, C, W, opt, null)
        {
            // empty
        }

        public Polyline(
            List<Vector2> P, Color C, float W, Opt opt)
            : this(P, new List<Color> { C }, new List<float> { W }, opt, new Inopt {
                constColor = true, constWeight = true,
            })
        {
            // empty
        }

        public Polyline(
            List<Vector2> P, List<Color> C, List<float> W,
            Opt opt, Inopt inopt)
        {
            int length = P.Count;
            if (opt == null) {
                opt = new Opt();
            }
            if (inopt == null) {
                inopt = new Inopt();
            }

            if (opt.cap >= 10) {
                char dec = (char)((int) opt.cap - ((int) opt.cap % 10));
                if (dec == Opt.PLCfirst || dec == Opt.PLCnone) {
                    inopt.noCapLast = true;
                }
                if (dec == Opt.PLClast || dec == Opt.PLCnone) {
                    inopt.noCapFirst = true;
                }
                opt.cap -= dec;
            }

            int A = 0, B = 0;
            bool on = false;
            for (int i = 1; i < length - 1; i++) {
                Vector2 V1 = P[i] - P[i - 1];
                Vector2 V2 = P[i + 1] - P[i];
                float len = 0;
                if (inopt.segmentLength != null) {
                    V1 *= 1 / inopt.segmentLength[i];
                    V2 *= 1 / inopt.segmentLength[i + 1];
                    len += (inopt.segmentLength[i] + inopt.segmentLength[i + 1]) * 0.5f;
                } else {
                    len += Vec2Ext.Normalize(ref V1) * 0.5f;
                    len += Vec2Ext.Normalize(ref V2) * 0.5f;
                }
                float costho = V1.x * V2.x + V1.y * V2.y;
                const float m_pi = (float) System.Math.PI;
                //float angle = acos(costho)*180/m_pi;
                float cos_a = (float) System.Math.Cos(15 * m_pi / 180);
                float cos_b = (float) System.Math.Cos(10 * m_pi / 180);
                float cos_c = (float) System.Math.Cos(25 * m_pi / 180);
                float weight = W[inopt.constWeight ? 0 : i];
                bool approx = false;
                if ((weight * opt.worldToScreenRatio < 7 && costho > cos_a) || (costho > cos_b) || //when the angle difference at an anchor is smaller than a critical degree, do polyline approximation
                    (len < weight && costho > cos_c)) { //when vector length is smaller than weight, do approximation
                    approx = true;
                }
                if (approx && !on) {
                    A = i;
                    on = true;
                    if (A == 1) {
                        A = 0;
                    }
                    if (A > 1) {
                        PolylineRange(P, C, W, opt, inopt, B, A, false);
                    }
                } else if (!approx && on) {
                    B = i;
                    on = false;
                    PolylineRange(P, C, W, opt, inopt, A, B, true);
                }
            }
            if (on && B < length - 1) {
                B = length - 1;
                PolylineRange(P, C, W, opt, inopt, A, B, true);
            } else if (!on && A < length - 1) {
                A = length - 1;
                PolylineRange(P, C, W, opt, inopt, B, A, false);
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

        public void Append(Polyline polyline)
        {
            holder.Push(polyline.holder);
        }

        private struct StPolyline
        // to hold info for AnchorLate() to perform triangulation
        {
            //for all joints
            public Vector2 vP; //vector to intersection point; outward

            //for djoint==PLJbevel
            public Vector2 T; //core thickness of a line
            public Vector2 R; //fading edge of a line
            public Vector2 bR; //out stepping vector, same direction as cap
            public Vector2 T1; //alternate vectors, same direction as T21
            // all T,R,T1 are outward

            //for djoint==PLJround
            public float t,r;

            //for degeneration case
            public bool degenT; //core degenerated
            public bool degenR; //fade degenerated
            public bool preFull; //draw the preceding segment in full
            public Vector2 PT;
            public float pt; //parameter at intersection

            public char djoint; //determined joint
            // e.g. originally a joint is PLJmiter. but it is smaller than critical angle, should then set djoint to PLJbevel
        }

        private class StAnchor
        // to hold memory for the working of anchor()
        {
            public Vector2[] P = new Vector2[3]; //point
            public Color[] C = new Color[3]; //color
            public float[] W = new float[3]; //weight

            public StPolyline[] SL = new StPolyline[3];
            public VertexArrayHolder vah = new VertexArrayHolder();
        }

        private static void PolyPointInter(
            List<Vector2> P, List<Color> C, List<float> W,
            Inopt inopt, ref Vector2 p, ref Color c, ref float w, int at, float t)
        {
            Color color(int I) {
                return C[inopt != null && inopt.constColor ? 0 : I];
            }
            float weight(int I) {
                return W[inopt != null && inopt.constWeight ? 0 : I];
            }

            if (t == 0.0) {
                p = P[at];
                c = color(at);
                w = weight(at);
            } else if (t == 1.0) {
                p = P[at + 1];
                c = color(at + 1);
                w = weight(at + 1);
            } else {
                p = (P[at] + P[at + 1]) * t;
                c = Gradient.ColorBetween(color(at), color(at + 1), t);
                w = (weight(at) + weight(at + 1)) * t;
            }
        }

        private static void PolylineApprox(
            List<Vector2> P, List<Color> C, List<float> W,
            Opt opt, Inopt inopt, int from, int to)
        {
            if (to - from + 1 < 2) {
                return;
            }
            bool capFirst = !(inopt != null && inopt.noCapFirst);
            bool capLast = !(inopt != null && inopt.noCapLast);
            bool joinFirst = inopt != null && inopt.joinFirst;
            bool joinLast = inopt != null && inopt.joinLast;

            VertexArrayHolder vcore = new VertexArrayHolder(); //curve core
            vcore.SetGlDrawMode(VertexArrayHolder.GL_TRIANGLE_STRIP);

            Color color(int I) {
                return C[inopt != null && inopt.constColor ? 0: I];
            }
            float weight(int I) {
                return W[inopt != null && inopt.constWeight ? 0: I];
            }
            void poly_step(int i, Vector2 pp, float ww, Color cc) {
                float t = 0, r = 0;
                DetermineTr(ww, ref t, ref r, opt.worldToScreenRatio);
                if (opt.feather && !opt.noFeatherAtCore) {
                    r *= opt.feathering;
                }
                float rr = (t + r) / r;
                Vector2 V = P[i] - P[i - 1];
                Vec2Ext.Perpen(ref V);
                Vec2Ext.Normalize(ref V);
                V *= (t + r);
                vcore.Push(pp - V, cc, rr);
                vcore.Push(pp + V, cc, 0);
            }

            for (int i = from + 1; i < to; i++) {
                poly_step(i, P[i], weight(i), color(i));
            }
            Vector2 P_las = new Vector2(), P_fir = new Vector2();
            Color C_las = new Color(), C_fir = new Color();
            float W_las = 0, W_fir = 0;
            PolyPointInter(P, C, W, inopt, ref P_las, ref C_las, ref W_las, to - 1, 0.5f);
            poly_step(to, P_las, W_las, C_las);

            StAnchor SA = new StAnchor();
            {
                PolyPointInter(P, C, W, inopt, ref P_fir, ref C_fir, ref W_fir, from, joinFirst ? 0.5f : 0.0f);
                SA.P[0] = P_fir;
                SA.P[1] = P[from + 1];
                SA.C[0] = C_fir;
                SA.C[1] = color(from + 1);
                SA.W[0] = W_fir;
                SA.W[1] = weight(from + 1);
                Segment(SA, opt, capFirst, false, true);
            }
            if (!joinLast) {
                SA.P[0] = P_las;
                SA.P[1] = P[to];
                SA.C[0] = C_las;
                SA.C[1] = color(to);
                SA.W[0] = W_las;
                SA.W[1] = weight(to);
                Segment(SA, opt, false, capLast, true);
            }

            inopt.holder.Push(vcore);
            inopt.holder.Push(SA.vah);

            if (opt.triangulation) {
                DrawTriangles(vcore, inopt.holder, opt.worldToScreenRatio);
                DrawTriangles(SA.vah, inopt.holder, opt.worldToScreenRatio);
            }
        }

        private static void PolylineExact(
            List<Vector2> P, List<Color> C, List<float> W,
            Opt opt, Inopt inopt, int from, int to)
        {
            bool capFirst = !(inopt != null && inopt.noCapFirst);
            bool capLast = !(inopt != null && inopt.noCapLast);
            bool joinFirst = inopt != null && inopt.joinFirst;
            bool joinLast = inopt != null && inopt.joinLast;

            Color color(int I) {
                return C[inopt != null && inopt.constColor ? 0: I];
            }
            float weight(int I) {
                return W[inopt != null && inopt.constWeight ? 0: I];
            }

            Vector2 mid_l = new Vector2(), mid_n = new Vector2(); //the last and the next mid point
            Color c_l = new Color(), c_n = new Color();
            float w_l = 0, w_n = 0;

            //init for the first anchor
            PolyPointInter(P, C, W, inopt, ref mid_l, ref c_l, ref w_l, from, joinFirst ? 0.5f : 0);

            StAnchor SA = new StAnchor();
            if (to - from + 1 == 2) {
                SA.P[0] = P[from];
                SA.P[1] = P[from + 1];
                SA.C[0] = color(from);
                SA.C[1] = color(from + 1);
                SA.W[0] = weight(from);
                SA.W[1] = weight(from + 1);
                Segment(SA, opt, capFirst, capLast, true);
            } else {
                for (int i = from + 1; i < to; i++) {
                    if (i == to - 1 && !joinLast) {
                        PolyPointInter(P, C, W, inopt, ref mid_n, ref c_n, ref w_n, i, 1.0f);
                    } else {
                        PolyPointInter(P, C, W, inopt, ref mid_n, ref c_n, ref w_n, i, 0.5f);
                    }

                    SA.P[0] = mid_l;
                    SA.C[0] = c_l;
                    SA.W[0] = w_l;
                    SA.P[2] = mid_n;
                    SA.C[2] = c_n;
                    SA.W[2] = w_n;

                    SA.P[1] = P[i];
                    SA.C[1] = color(i);
                    SA.W[1] = weight(i);

                    Anchor(SA, opt, (i == 1) && capFirst, (i == to - 1) && capLast);

                    mid_l = mid_n;
                    c_l = c_n;
                    w_l = w_n;
                }
            }

            inopt.holder.Push(SA.vah);
            if (opt.triangulation) {
                DrawTriangles(SA.vah, inopt.holder, opt.worldToScreenRatio);
            }
        }

        private static void PolylineRange(
            List<Vector2> P, List<Color> C, List<float> W,
            Opt opt, Inopt ininopt, int from, int to, bool approx)
        {
            Inopt inopt;
            if (ininopt == null) {
                inopt = new Inopt();
            } else {
                inopt = ininopt.ShallowCopy();
            }
            if (from > 0) {
                from -= 1;
            }

            inopt.joinFirst = from != 0;
            inopt.joinLast = to != (P.Count - 1);
            inopt.noCapFirst = ininopt.noCapFirst || inopt.joinFirst;
            inopt.noCapLast = ininopt.noCapLast || inopt.joinLast;

            if (approx) {
                PolylineApprox(P, C, W, opt, inopt, from, to);
            } else {
                PolylineExact(P, C, W, opt, inopt, from, to);
            }
        }

        private static void DrawTriangles(VertexArrayHolder triangles, VertexArrayHolder holder, float scale)
        {
            Opt opt = new Opt();
            opt.cap = Opt.PLCnone;
            opt.joint = Opt.PLJbevel;
            opt.worldToScreenRatio = scale;
            if (triangles.glmode == VertexArrayHolder.GL_TRIANGLES) {
                for (int i = 0; i < triangles.GetCount(); i++) {
                    List<Vector2> P = new List<Vector2> ();
                    P.Add(triangles.Get(i));
                    i += 1;
                    P.Add(triangles.Get(i));
                    i += 1;
                    P.Add(triangles.Get(i));
                    P.Add(P[0]);
                    Polyline polyline = new Polyline(P, Color.red, 1 / scale, opt);
                    holder.Push(polyline.holder);
                }
            } else if (triangles.glmode == VertexArrayHolder.GL_TRIANGLE_STRIP) {
                for (int i = 2; i < triangles.GetCount(); i++) {
                    List<Vector2> P = new List<Vector2> ();
                    P.Add(triangles.Get(i - 2));
                    P.Add(triangles.Get(i));
                    P.Add(triangles.Get(i - 1));
                    Polyline polyline = new Polyline(P, Color.red, 1 / scale, opt);
                    holder.Push(polyline.holder);
                }
            }
        }

        private static void DetermineTr(float w, ref float t, ref float R, float scale)
        {
            //efficiency: can cache one set of w,t,R values
            // i.e. when a polyline is of uniform thickness, the same w is passed in repeatedly
            w *= scale;
            float f = w - (float) System.Math.Floor(w);
            // resolution dependent
            if (w >= 0.0 && w < 1.0) {
                t = 0.05f;
                R = 0.768f;
            } else if (w >= 1.0 && w < 2.0) {
                t = 0.05f + f * 0.33f;
                R = 0.768f + 0.312f * f;
            } else if (w >= 2.0 && w < 3.0) {
                t = 0.38f + f * 0.58f;
                R = 1.08f;
            } else if (w >= 3.0 && w < 4.0) {
                t = 0.96f + f * 0.48f;
                R = 1.08f;
            } else if (w >= 4.0 && w < 5.0) {
                t = 1.44f + f * 0.46f;
                R = 1.08f;
            } else if (w >= 5.0 && w < 6.0) {
                t = 1.9f + f * 0.6f;
                R = 1.08f;
            } else if (w >= 6.0) {
                t = 2.5f + (w - 6.0f) * 0.50f;
                R = 1.08f;
            }
            t /= scale;
            R /= scale;
        }

        private static void MakeTrc(
            ref Vector2 P1, ref Vector2 P2, ref Vector2 T, ref Vector2 R, ref Vector2 C,
            float w, Opt opt, ref float rr, ref float tt, ref float dist, bool anchorMode)
        {
            float t = 1.0f, r = 0.0f;

            //calculate t,r
            DetermineTr(w, ref t, ref r, opt.worldToScreenRatio);
            if (opt.feather && !opt.noFeatherAtCore) {
                r *= opt.feathering;
            }
            if (anchorMode) {
                t += r;
            }

            //output
            tt = t;
            rr = r;
            Vector2 DP = P2 - P1;
            dist = Vec2Ext.Normalize(ref DP);
            C = DP * (1 / opt.worldToScreenRatio);
            Vec2Ext.Perpen(ref DP);
            T = DP * t;
            R = DP * r;
        }

        private static void Segment(StAnchor SA, Opt opt, bool capFirst, bool capLast, bool core)
        {
            float[] weight = SA.W;

            Vector2[] P = new Vector2[2];
            P[0] = SA.P[0];
            P[1] = SA.P[1];
            Color[] C = new Color[3];
            C[0] = SA.C[0];
            C[1] = SA.C[1];

            Vector2 T2 = new Vector2();
            Vector2 R2 = new Vector2();
            Vector2 bR = new Vector2();
            float t = 0, r = 0;

            bool varying_weight = weight[0] != weight[1];

            Vector2 capStart = new Vector2(), capEnd = new Vector2();
            StPolyline[] SL = new StPolyline[2];

            /*for (int i = 0; i < 2; i++) {
                //lower the transparency for weight < 1.0
                float actualWeight = weight[i] * opt.worldToScreenRatio;
                if (actualWeight >= 0.0 && actualWeight < 1.0) {
                    C[i].a *= actualWeight;
                }
            }*/

            {
                int i = 0;
                float dd = 0f;
                MakeTrc(ref P[i], ref P[i + 1], ref T2, ref R2, ref bR, weight[i], opt, ref r, ref t, ref dd, false);

                if (capFirst) {
                    if (opt.cap == Opt.PLCsquare) {
                        P[0] -= bR * (t + r);
                    }
                    capStart = bR;
                    Vec2Ext.Opposite(ref capStart);
                    if (opt.feather && !opt.noFeatherAtCap) {
                        capStart *= opt.feathering;
                    }
                }

                SL[i].djoint = opt.cap;
                SL[i].t = t;
                SL[i].r = r;
                SL[i].T = T2;
                SL[i].R = R2;
                SL[i].bR = bR;
                SL[i].degenT = false;
                SL[i].degenR = false;
            }

            {
                int i = 1;
                float dd = 0f;
                if (varying_weight) {
                    MakeTrc(ref P[i - 1], ref P[i], ref T2, ref R2, ref bR, weight[i], opt, ref r, ref t, ref dd, false);
                }

                if (capLast) {
                    if (opt.cap == Opt.PLCsquare) {
                        P[1] += bR * (t + r);
                    }
                    capEnd = bR;
                    if (opt.feather && !opt.noFeatherAtCap) {
                        capEnd *= opt.feathering;
                    }
                }

                SL[i].djoint = opt.cap;
                SL[i].t = t;
                SL[i].r = r;
                SL[i].T = T2;
                SL[i].R = R2;
                SL[i].bR = bR;
                SL[i].degenT = false;
                SL[i].degenR = false;
            }

            SegmentLate(opt, P, C, SL, SA.vah, capStart, capEnd, core);
        }

        private static void SegmentLate(
            Opt opt, Vector2[] P, Color[] C, StPolyline[] SL,
            VertexArrayHolder tris, Vector2 cap1, Vector2 cap2, bool core)
        {
            tris.SetGlDrawMode(VertexArrayHolder.GL_TRIANGLES);

            Vector2 P_0, P_1;
            P_0 = P[0];
            P_1 = P[1];

            Vector2 P1, P2, P3, P4; //core
            Vector2 P1c, P2c, P3c, P4c; //cap
            Vector2 P1r, P2r, P3r, P4r; //fade

            float s0 = 1, s1 = 1;
            if (SL[0].t < SL[1].t) {
                s0 = (SL[0].t + SL[0].r) / (SL[1].t + SL[1].r);
            }
            if (SL[1].t < SL[0].t) {
                s1 = (SL[1].t + SL[1].r) / (SL[0].t + SL[0].r);
            }
            P1 = P_0 + SL[0].T + SL[0].R;
            P1r = P1 - SL[0].R * s0;
            P1c = P1 + cap1 * s0;
            P2 = P_0 - SL[0].T - SL[0].R;
            P2r = P2 + SL[0].R * s0;
            P2c = P2 + cap1 * s0;
            P3 = P_1 + SL[1].T + SL[1].R;
            P3r = P3 - SL[1].R * s1;
            P3c = P3 + cap2 * s1;
            P4 = P_1 - SL[1].T - SL[1].R;
            P4r = P4 + SL[1].R * s1;
            P4c = P4 + cap2 * s1;
            float rr = System.Math.Max((SL[0].t + SL[0].r) / SL[0].r, (SL[1].t + SL[1].r) / SL[1].r);
            float rc = (P_1 - P_0).Length() / System.Math.Max(SL[0].t + SL[0].r, SL[1].t + SL[1].r);

            //core
            if (core) {
                float rc0 = 0, rc1 = 0;
                if (SL[0].djoint == Opt.PLCbutt && !cap1.IsZero()) {
                    rc0 = rc;
                }
                if (SL[1].djoint == Opt.PLCbutt && !cap2.IsZero()) {
                    rc1 = rc;
                }
                tris.Push3(P1, P3, P2, C[0], C[1], C[0], rr,  0, rc0);
                tris.Push3(P2, P3, P4, C[0], C[1], C[1], 0, rc1,  rr);
            }

            //caps
            for (int j = 0; j < 2; j++) {
                VertexArrayHolder cap = new VertexArrayHolder();
                cap.SetGlDrawMode(VertexArrayHolder.GL_TRIANGLE_STRIP);
                Vector2 cur_cap = j == 0 ? cap1 : cap2;
                if (cur_cap.IsZero()) {
                    continue;
                }

                if (SL[j].djoint == Opt.PLCround) {
                    //round cap
                    Vector2 O = P[j];
                    float dangle = GetPljRoundDangle(SL[j].t, SL[j].r, opt.worldToScreenRatio);

                    VectorsToArc(
                        cap, O, C[j], C[j], SL[j].T + SL[j].R, -SL[j].T - SL[j].R,
                        dangle, SL[j].t + SL[j].r, 0.0f, false, O, j == 0 ? cap1 : cap2, rr, false);
                    //cap.Push(O-SL[j].T-SL[j].R, C[j]);
                    //cap.Push(O, C[j]);
                    tris.Push(cap);
                } else if (SL[j].djoint == Opt.PLCrect || SL[j].djoint == Opt.PLCsquare) {
                    //rectangular cap
                    Vector2 Pj, Pjr, Pjc, Pk, Pkr, Pkc;
                    if (j == 0) {
                        Pj = P1;
                        Pjr = P1r;
                        Pjc = P1c;

                        Pk = P2;
                        Pkr = P2r;
                        Pkc = P2c;
                    }
                    else {
                        Pj = P4;
                        Pjr = P4r;
                        Pjc = P4c;

                        Pk = P3;
                        Pkr = P3r;
                        Pkc = P3c;
                    }

                    tris.PushF(Pk, C[j]);
                    tris.PushF(Pkc, C[j]);
                    tris.Push(Pkr, C[j]);
                    tris.Push(Pkr, C[j]);
                    tris.PushF(Pkc, C[j]);
                    tris.Push(Pjr, C[j]);
                    tris.Push(Pjr, C[j]);
                    tris.PushF(Pkc, C[j]);
                    tris.PushF(Pjc, C[j]);
                    tris.Push(Pjr, C[j]);
                    tris.PushF(Pjc, C[j]);
                    tris.PushF(Pj, C[j]);
                }
            }
        }

        private static void Anchor(StAnchor SA, Opt opt, bool capFirst, bool capLast)
        {
            Vector2[] P = SA.P;
            Color[] C = SA.C;
            float[] weight = SA.W;
            StPolyline[] SL = SA.SL;
            if (Vec2Ext.SignedArea(P[0], P[1], P[2]) > 0) {
                // rectify clockwise
                Vector2 P0 = P[0];
                P[0] = P[2];
                P[2] = P0;
                Color C0 = C[0];
                C[0] = C[2];
                C[2] = C0;
                float weight0 = weight[0];
                weight[0] = weight[2];
                weight[2] = weight0;
                bool capCap = capFirst;
                capFirst = capLast;
                capLast = capCap;
            }

            //const float critical_angle=11.6538;
            //  critical angle in degrees where a miter is force into bevel
            //  it is _similar_ to cairo_set_miter_limit () but cairo works with ratio while VASEr works with included angle
            const float cos_cri_angle = 0.979386f; //cos(critical_angle)

            bool varying_weight = !(weight[0] == weight[1] & weight[1] == weight[2]);

            Vector2 T1 = new Vector2(), T2 = new Vector2(), T21 = new Vector2(), T31 = new Vector2(), RR = new Vector2();

            /*for (int i = 0; i < 3; i++) {
                //lower the transparency for weight < 1.0
                float actualWeight = weight[i] * opt.worldToScreenRatio;
                if (actualWeight >= 0.0 && actualWeight < 1.0) {
                    C[i].a *= actualWeight;
                }
            }*/

            {
                int i = 0;

                Vector2 cap0 = new Vector2(), cap1 = new Vector2();
                float r = 0f, t = 0f, d = 0f;
                MakeTrc(ref P[i], ref P[i + 1], ref T2, ref RR, ref cap1, weight[i], opt, ref r, ref t, ref d, true);
                if (varying_weight) {
                    MakeTrc(ref P[i], ref P[i + 1], ref T31, ref RR, ref cap0, weight[i + 1], opt, ref d, ref d, ref d, true);
                } else {
                    T31 = T2;
                }
                SL[i].djoint = opt.cap;
                SL[i].T = T2;
                SL[i].t = t;
                SL[i].r = r;
                SL[i].degenT = false;

                SL[i + 1].T1 = T31;
            }

            {
                int i = 1;

                float r = 0f, t = 0f;
                Vector2 P_cur = P[i]; //current point
                Vector2 P_nxt = P[i + 1]; //next point
                Vector2 P_las = P[i - 1]; //last point

                {
                    Vector2 cap0 = new Vector2(), bR = new Vector2();
                    float length_cur = 0f, length_nxt = 0f, d = 0f;
                    MakeTrc(ref P_las, ref P_cur, ref T1, ref RR, ref cap0, weight[i - 1], opt, ref d, ref d, ref length_cur, true);
                    if (varying_weight) {
                        MakeTrc(ref P_las, ref P_cur, ref T21, ref RR, ref cap0, weight[i], opt, ref d, ref d, ref d, true);
                    } else {
                        T21 = T1;
                    }

                    MakeTrc(ref P_cur, ref P_nxt, ref T2, ref RR, ref bR, weight[i], opt, ref r, ref t, ref length_nxt, true);
                    if (varying_weight) {
                        MakeTrc(ref P_cur, ref P_nxt, ref T31, ref RR, ref cap0, weight[i + 1], opt, ref d, ref d, ref d, true);
                    } else {
                        T31 = T2;
                    }

                    SL[i].T = T2;
                    SL[i].bR = bR;
                    SL[i].t = t;
                    SL[i].r = r;
                    SL[i].degenT = false;

                    SL[i + 1].T1 = T31;
                }

                {   //2nd point
                    //find the angle between the 2 line segments
                    Vector2 ln1 = new Vector2(), ln2 = new Vector2(), V = new Vector2();
                    ln1 = P_cur - P_las;
                    ln2 = P_nxt - P_cur;
                    Vec2Ext.Normalize(ref ln1);
                    Vec2Ext.Normalize(ref ln2);
                    Vec2Ext.Dot(ln1, ln2, ref V);
                    float cos_tho = V.x + V.y;
                    bool zero_degree = System.Math.Abs(cos_tho - 1.0f) < 0.0000001f;
                    bool d180_degree = cos_tho < -1.0f + 0.0001f;
                    bool intersection_fail = false;
                    int result3 = 1;

                    SL[i].djoint = opt.joint;
                    if (SL[i].djoint == Opt.PLJmiter) {
                        if (System.Math.Abs(cos_tho) >= cos_cri_angle) {
                            SL[i].djoint = Opt.PLJbevel;
                        }
                    }

                    Vec2Ext.AnchorOutward(ref T1, P_cur, P_nxt);
                    Vec2Ext.AnchorOutward(ref T21, P_cur, P_nxt);
                    Vec2Ext.FollowSigns(ref SL[i].T1, T21);
                    Vec2Ext.AnchorOutward(ref T2, P_cur, P_las);
                    Vec2Ext.FollowSigns(ref SL[i].T, T2);
                    Vec2Ext.AnchorOutward(ref T31, P_cur, P_las);

                    {   //must do intersection
                        Vector2 interP = new Vector2(), vP = new Vector2();
                        float[] pts = new float[2];
                        result3 = Vec2Ext.Intersect(
                            P_las + T1, P_cur + T21, P_nxt + T31, P_cur + T2, ref interP, pts);

                        if (result3 != 0) {
                            vP = interP - P_cur;
                            SL[i].vP = vP;
                            if (SL[i].djoint == Opt.PLJmiter) {
                                if (pts[0] > 2 || pts[1] > 2 || vP.Length() > 2 * SL[i].t) {
                                    SL[i].djoint = Opt.PLJbevel;
                                }
                            }
                        } else {
                            intersection_fail = true;
                            Debug.Log(System.String.Format("intersection failed: cos(angle)={0}, angle={1} (degree)", cos_tho, System.Math.Acos(cos_tho) * 180 / 3.14159));
                        }
                    }

                    if (zero_degree || intersection_fail) {
                        Segment(SA, opt, capFirst, false, true);
                        SA.P[0] = SA.P[1];
                        SA.P[1] = SA.P[2];
                        SA.C[0] = SA.C[1];
                        SA.C[1] = SA.C[2];
                        SA.W[0] = SA.W[1];
                        SA.W[1] = SA.W[2];
                        Segment(SA, opt, false, capLast, true);
                        return;
                    }

                    Vec2Ext.Opposite(ref T1);
                    Vec2Ext.Opposite(ref T21);
                    Vec2Ext.Opposite(ref T2);
                    Vec2Ext.Opposite(ref T31);

                    //make intersections
                    Vector2 PT1 = new Vector2(), PT2 = new Vector2();
                    float pt1 = 0f, pt2 = 0f;
                    int result1t, result2t;
                    {
                        float[] pts = new float[2];
                        result1t = Vec2Ext.Intersect(
                        P_nxt - T31, P_nxt + T31, P_las + T1, P_cur + T21, //knife1_a
                        ref PT1, pts); //core
                        pt1 = pts[1];
                        result2t = Vec2Ext.Intersect(
                        P_las - T1, P_las + T1, P_nxt + T31, P_cur + T2, //knife2_a
                        ref PT2, pts);
                        pt2 = pts[1];
                    }
                    bool is_result1t = result1t == 1;
                    bool is_result2t = result2t == 1;

                    if (is_result1t | is_result2t) { //core degeneration
                        SL[i].degenT = true;
                        SL[i].preFull = is_result1t;
                        SL[i].PT = is_result1t ? PT1: PT2;
                        SL[i].pt = is_result1t ? pt1: pt2;
                    }

                    if (d180_degree | result3 == 0) { //to solve visual bugs 3 and 1.1
                        SL[i].vP = SL[i].T;
                        Vec2Ext.FollowSigns(ref SL[i].T1, SL[i].T);
                        SL[i].djoint = Opt.PLJmiter;
                    }
                }
            }

            {
                int i = 2;

                Vector2 cap0 = new Vector2();
                float r = 0f, t = 0f, d = 0f;
                MakeTrc(ref P[i - 1], ref P[i], ref T2, ref RR, ref cap0, weight[i], opt, ref r, ref t, ref d, true);

                SL[i].djoint = opt.cap;
                SL[i].T = T2;
                SL[i].t = t;
                SL[i].r = r;
                SL[i].degenT = false;
            }

            AnchorLate(opt, P, C, SA.SL, SA.vah, capFirst, capLast);
            if (capFirst && SL[0].djoint != Opt.PLCbutt && SL[0].djoint != Opt.PLCnone) {
                Segment(SA, opt, true, false, false);
            }
            if (capLast && SL[1].djoint != Opt.PLCbutt && SL[1].djoint != Opt.PLCnone) {
                SA.P[0] = SA.P[1];
                SA.P[1] = SA.P[2];
                SA.C[0] = SA.C[1];
                SA.C[1] = SA.C[2];
                SA.W[0] = SA.W[1];
                SA.W[1] = SA.W[2];
                Segment(SA, opt, false, true, false);
            }
        } //Anchor

        private static void AnchorLate(
            Opt opt, Vector2[] P, Color[] C, StPolyline[] SL,
            VertexArrayHolder tris, bool capFirst, bool capLast)
        {
            Vector2 P_0 = P[0], P_1 = P[1], P_2 = P[2];
            Vector2 P0, P1, P2, P3, P4, P5, P6, P7;

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
            float rc1 = 0, rc2 = 0;
            if (capFirst && SL[0].djoint == Opt.PLCbutt) {
                rc1 = (P_1 - P_0).Length() / (SL[0].r) / 2;
            }
            if (capLast && SL[2].djoint == Opt.PLCbutt) {
                rc2 = (P_2 - P_1).Length() / (SL[2].r) / 2;
            }

            if (SL[1].degenT) {
                P1 = SL[1].PT;
                tris.Push3(P1, P3, P2, C[1], C[0], C[1], 0, rr, 0); //fir seg
                tris.Push3(P1, P5, P6, C[1], C[1], C[2], 0, rr, 0); //las seg

                if (SL[1].preFull) {
                    tris.Push3(P3, P1, P4, C[0], C[1], C[0], 0, rr, 0);
                } else {
                    tris.Push3(P1, P6, P7, C[1], C[2], C[2], 0, 0, rr);
                }
            } else {
                // normal first segment
                tris.Push3(P2, P4, P3, C[1], C[0], C[0], 0, rc1, rr);
                tris.Push3(P4, P2, P1, C[0], C[1], C[1], 0, 0, rr);
                // normal last segment
                tris.Push3(P5, P7, P1, C[1], C[2], C[1], 0, rr, 0);
                tris.Push3(P7, P5, P6, C[2], C[1], C[2], 0, rr, rc2);
            }

            if (normal_line_core_joint != 0) {
                switch (SL[1].djoint) {
                case Opt.PLJmiter:
                    tris.Push3(P2, P0, P1, C[1], C[1], C[1], rr, 0, 0);
                    tris.Push3(P1, P0, P5, C[1], C[1], C[1], 0, rr, 0);
                    break;

                case Opt.PLJbevel:
                    if (normal_line_core_joint == 1) tris.Push3(P2, P5, P1, C[1], C[1], C[1], rr, 0, 0);
                    break;

                case Opt.PLJround:
                    {
                        VertexArrayHolder strip = new VertexArrayHolder();
                        strip.SetGlDrawMode(VertexArrayHolder.GL_TRIANGLE_STRIP);

                        if (normal_line_core_joint == 1) {
                            VectorsToArc(strip, P_1, C[1], C[1], SL[1].T1, SL[1].T, GetPljRoundDangle(SL[1].t, SL[1].r, opt.worldToScreenRatio), SL[1].t, 0.0f, false, P1, new Vector2(), rr, true);
                        } else if (normal_line_core_joint == 2) {
                            VectorsToArc(strip, P_1, C[1], C[1], SL[1].T1, SL[1].T, GetPljRoundDangle(SL[1].t, SL[1].r, opt.worldToScreenRatio), SL[1].t, 0.0f, false, P5, new Vector2(), rr, true);
                        }
                        tris.Push(strip);
                    }
                    break;
                }
            }
        } //AnchorLate

        private static void VectorsToArc(
            VertexArrayHolder hold, Vector2 P, Color C, Color C2,
            Vector2 PA, Vector2 PB, float dangle, float r, float r2,
            bool ignorEnds, Vector2 apparentP, Vector2 hint, float rr, bool innerFade)
        {
            // triangulate an inner arc between vectors A and B,
            // A and B are position vectors relative to P
            const float m_pi = (float) System.Math.PI;
            Vector2 A = PA * (1 / r);
            Vector2 B = PB * (1 / r);
            float rrr;
            if (innerFade) {
                rrr = 0;
            } else {
                rrr = -1;
            }

            float angle1 = (float) System.Math.Acos(A.x);
            float angle2 = (float) System.Math.Acos(B.x);
            if (A.y > 0) {
                angle1 = 2 * m_pi - angle1;
            }
            if (B.y > 0) {
                angle2 = 2 * m_pi - angle2;
            }
            if (!hint.IsZero()) {
                // special case when angle1 == angle2,
                //   have to determine which side by hint
                if (hint.x > 0 && hint.y == 0) {
                    angle1 -= (angle1 < angle2 ? 1 : -1) * 0.00001f;
                } else if (hint.x == 0 && hint.y > 0) {
                    angle1 -= (angle1 < angle2 ? 1 : -1) * 0.00001f;
                } else if (hint.x > 0 && hint.y > 0) {
                    angle1 -= (angle1 < angle2 ? 1 : -1) * 0.00001f;
                } else if (hint.x > 0 && hint.y < 0) {
                    angle1 -= (angle1 < angle2 ? 1 : -1) * 0.00001f;
                } else if (hint.x < 0 && hint.y > 0) {
                    angle1 += (angle1 < angle2 ? 1 : -1) * 0.00001f;
                } else if (hint.x < 0 && hint.y < 0) {
                    angle1 += (angle1 < angle2 ? 1 : -1) * 0.00001f;
                }
            }

            // (apparent center) center of fan
            //draw the inner arc between angle1 and angle2 with dangle at each step.
            // -the arc has thickness, r is the outer radius and r2 is the inner radius,
            //    with color C and C2 respectively.
            //    in case when inner radius r2=0.0f, it gives a pie.
            // -when ignorEnds=false, the two edges of the arc lie exactly on angle1
            //    and angle2. when ignorEnds=true, the two edges of the arc do not touch
            //    angle1 or angle2.
            // -P is the mathematical center of the arc.
            // -when use_apparent_P is true, r2 is ignored,
            //    apparentP is then the apparent origin of the pie.
            // -the result is pushed to hold, in form of a triangle strip
            // -an inner arc is an arc which is always shorter than or equal to a half circumference

            if (angle2 > angle1) {
                if (angle2 - angle1 > m_pi) {
                    angle2 = angle2 - 2 * m_pi;
                }
            }
            else {
                if (angle1 - angle2 > m_pi) {
                    angle1 = angle1 - 2 * m_pi;
                }
            }

            bool incremental = true;
            if (angle1 > angle2) {
                incremental = false;
            }

            if (incremental) {
                if (!ignorEnds) {
                    hold.Push(new Vector2(P.x + PB.x, P.y + PB.y), C, rr);
                    hold.Push(apparentP, C2, rrr);
                }
                for (float a = angle2 - dangle; a > angle1; a -= dangle) {
                    inner_arc_push(System.Math.Cos(a), System.Math.Sin(a));
                }
                if (!ignorEnds) {
                    hold.Push(new Vector2(P.x + PA.x, P.y + PA.y), C, rr);
                    hold.Push(apparentP, C2, rrr);
                }
            }
            else //decremental
            {
                if (!ignorEnds) {
                    hold.Push(apparentP, C2, rr);
                    hold.Push(new Vector2(P.x + PB.x, P.y + PB.y), C, rrr);
                }
                for (float a = angle2 + dangle; a < angle1; a += dangle) {
                    inner_arc_push(System.Math.Cos(a), System.Math.Sin(a), true);
                }
                if (!ignorEnds) {
                    hold.Push(apparentP, C2, rr);
                    hold.Push(new Vector2(P.x + PA.x, P.y + PA.y), C, rrr);
                }
            }

            void inner_arc_push(double x, double y, bool reverse = false) {
                Vector2 PP = new Vector2(P.x + (float) x * r, P.y - (float) y * r);
                //hold.Dot(PP, 0.05f); return;
                if (!reverse) {
                    hold.Push(PP, C, rr);
                    hold.Push(apparentP, C2, rrr);
                } else {
                    hold.Push(apparentP, C2, rr);
                    hold.Push(PP, C, rrr);
                }
            }
        }

        private static float GetPljRoundDangle(float t, float r, float scale)
        {
            float dangle;
            float sum = (t + r) * scale;
            if (sum <= 1.44f + 1.08f) { //w<=4.0, feathering=1.0
                dangle = 0.6f / sum;
            } else if (sum <= 3.25f + 1.08f) { //w<=6.5, feathering=1.0
                dangle = 2.8f / sum;
            } else {
                dangle = 4.2f / sum;
            }
            return dangle;
        }
    }
}