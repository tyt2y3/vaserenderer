using UnityEngine;
using System.Collections.Generic;

namespace Vaser {

    public class Gradient {
        public char unit = GD_ratio;
        public List<Stop> stops = new List<Stop> (); //array must be sorted in ascending order of t

        public const char GD_ratio  = (char) 0;
        public const char GD_length = (char) 1;

        public const char GS_none   = (char) 0;
        public const char GS_rgba   = (char) 1;
        public const char GS_rgb    = (char) 2; //rgb only
        public const char GS_alpha  = (char) 3;
        public const char GS_weight = (char) 4;

        public struct Stop {
            public float t; //position
            public char type; //GS_xx
            public Color color;
            public float weight;
        }

        public void Apply(List<Color> C, List<float> W, List<float> L, int limit, float pathLength) {
            if (stops.Count == 0) {
                return;
            }

            //current stops
            int las_c = -1, las_a = -1, las_w = -1, //last
                cur_c = -1, cur_a = -1, cur_w = -1, //current
                nex_c =  0, nex_a =  0, nex_w =  0; //next

            float lengthAlong = 0.0f;

            if (stops.Count <= 1) {
                return;
            }
            for (int i = 0; i < limit; i += 1) {
                lengthAlong += L[i];
                float p = 0.0f;
                if (unit == GD_ratio) {
                    p = lengthAlong / pathLength;
                } else if (unit == GD_length) {
                    p = lengthAlong;
                } else {
                    break;
                }

                //lookup for cur
                for (nex_w = cur_w; nex_w < stops.Count; nex_w += 1) {
                    if (stops[nex_w].type == GS_weight && p <= stops[nex_w].t) {
                        cur_w = nex_w;
                        break;
                    }
                }
                for (nex_c = cur_c; nex_c < stops.Count; nex_c += 1) {
                    if ((stops[nex_c].type == GS_rgba || stops[nex_c].type == GS_rgb) && p <= stops[nex_c].t) {
                        cur_c = nex_c;
                        break;
                    }
                }
                for (nex_a = cur_a; nex_a < stops.Count; nex_a += 1) {
                    if ((stops[nex_a].type == GS_rgba || stops[nex_a].type == GS_alpha) && p <= stops[nex_a].t) {
                        cur_a = nex_a;
                        break;
                    }
                }
                //look for las
                for (nex_w = cur_w; nex_w >= 0; nex_w -= 1) {
                    if (stops[nex_w].type == GS_weight && p >= stops[nex_w].t) {
                        las_w = nex_w;
                        break;
                    }
                }
                for (nex_c = cur_c; nex_c >= 0; nex_c -= 1) {
                    if ((stops[nex_c].type == GS_rgba || stops[nex_c].type == GS_rgb) && p >= stops[nex_c].t) {
                        las_c = nex_c;
                        break;
                    }
                }
                for (nex_a = cur_a; nex_a >= 0; nex_a -= 1) {
                    if ((stops[nex_a].type == GS_rgba || stops[nex_a].type == GS_alpha) && p >= stops[nex_a].t) {
                        las_a = nex_a;
                        break;
                    }
                }

                if (cur_c == las_c) {
                    C[i] = Sc(cur_c);
                } else {
                    C[i] = ColorBetween(Sc(las_c), Sc(cur_c), (p - St(las_c)) / (St(cur_c) - St(las_c)));
                }
                if (cur_w == las_w) {
                    W[i] = Sw(cur_w);
                } else {
                    W[i] = GetStep(Sw(las_w), Sw(cur_w), p - St(las_w), St(cur_w) - St(las_w));
                }
                if (cur_a == las_a) {
                    C[i] = SaC(i, cur_a);
                } else {
                    C[i] = GetStepColor(i, Sa(las_a), Sa(cur_a), p - St(las_a), St(cur_a) - St(las_a));
                }
            }

            Color Sc(int x) {
                return stops[x].color;
            }
            float Sw(int x) {
                return stops[x].weight;
            }
            float Sa(int x) {
                return stops[x].color.a;
            }
            float St(int x) {
                return stops[x].t;
            }
            Color SaC(int i, int x) {
                Color c = new Color(C[i].r, C[i].g, C[i].b, C[i].a);
                c.a = stops[x].color.a;
                return c;
            }
            float GetStep(float A, float B, float t, float T) {
                return ((T - t) * A + t * B) / T;
            }
            Color GetStepColor(int i, float A, float B, float t, float T) {
                Color c = new Color(C[i].r, C[i].g, C[i].b, C[i].a);
                c.a = GetStep(A, B, t, T);
                return c;
            }
        }

        public static Color ColorBetween(Color A, Color B, float t) {
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            float kt = 1.0f - t;
            return new Color(
                A.r * kt + B.r * t,
                A.g * kt + B.g * t,
                A.b * kt + B.b * t,
                A.a * kt + B.a * t
            );
        }
    }
}