using UnityEngine;
using System.Collections.Generic;

namespace Vaser
{
    public class VertexArrayHolder
    {
        public const int GL_POINTS = 1;
        public const int GL_LINES = 2;
        public const int GL_TRIANGLES = 3;
        public const int GL_LINE_STRIP = 4;
        public const int GL_LINE_LOOP = 5;
        public const int GL_TRIANGLE_STRIP = 6;
        public const int GL_TRIANGLE_FAN = 7;

        public int glmode { get; set; } = GL_TRIANGLES; //drawing mode in opengl
        private bool jumping = false;
        private List<Vector3> vert = new List<Vector3>();
        private List<Color> color = new List<Color>();
        private List<int> fade = new List<int>();

        public void SetGlDrawMode(int gl_draw_mode)
        {
            glmode = gl_draw_mode;
        }

        public int Push(Point P, Color cc, int fade0=0)
        {
            int cur = vert.Count;
            vert.Add(new Vector3(P.x, P.y, 0));
            color.Add(cc);
            fade.Add(fade0);

            if (jumping)
            {
                jumping = false;
                RepeatLastPush();
            }
            return cur;
        }

        public void Push3(
                Point P1, Point P2, Point P3,
                Color C1, Color C2, Color C3,
                int fade1=0, int fade2=0, int fade3=0)
        {
            Push(P1, C1, fade1);
            Push(P2, C2, fade2);
            Push(P3, C3, fade3);
        }

        public void Push(VertexArrayHolder hold)
        {
            if (glmode == hold.glmode)
            {
                vert.AddRange(hold.vert);
                color.AddRange(hold.color);
            }
            else if (glmode == GL_TRIANGLES &&
                hold.glmode == GL_TRIANGLE_STRIP)
            {
                for (int b=3; b < hold.vert.Count; b+=2)
                {
                    vert.Add(hold.vert[b-3]); color.Add(hold.color[b-3]); fade.Add(0);
                    vert.Add(hold.vert[b-2]); color.Add(hold.color[b-2]); fade.Add(0);
                    vert.Add(hold.vert[b-1]); color.Add(hold.color[b-1]); fade.Add(hold.fade[b-1]);
                    vert.Add(hold.vert[b-1]); color.Add(hold.color[b-1]); fade.Add(0);
                    vert.Add(hold.vert[b-2]); color.Add(hold.color[b-2]); fade.Add(hold.fade[b-0]);
                    vert.Add(hold.vert[b-0]); color.Add(hold.color[b-0]); fade.Add(0);
                }
            }
        }

        public Point Get(int i)
        {
            Point P = new Point();
            P.x = vert[i].x;
            P.y = vert[i].y;
            return P;
        }

        public Color GetColor(int b)
        {
            return color[b];
        }

        public Point GetRelativeEnd(int di = -1)
        {
            //di=-1 is the last one
            int count = vert.Count;
            int i = count+di;
            if (i<0) i=0;
            if (i>=count) i=count-1;
            return Get(i);
        }

        public void RepeatLastPush()
        {
            int i = vert.Count-1;
            Push(Get(i), GetColor(i));
        }

        public void Jump() //to make a jump in triangle strip by degenerated triangles
        {
            if (glmode == GL_TRIANGLE_STRIP)
            {
                RepeatLastPush();
                jumping = true;
            }
        }

        public List<Vector3> GetVertices()
        {
            return vert;
        }

        public List<int> GetTriangles()
        {
            List<int> indices = new List<int>();
            if (glmode == GL_TRIANGLES)
            {
                for (int i=0; i < vert.Count; i++)
                {
                    indices.Add(i);
                }
            }
            return indices;
        }

        public List<Vector4> GetUVs()
        {
            List<Vector4> uvs = new List<Vector4>();
            Vector4[] UVS = new Vector4[]
            {
                new Vector4(0, 0, 1, 0),
                new Vector4(1, 0, 1, 0),
                new Vector4(1, 1, 1, 0),
                new Vector4(0, 1, 1, 0),
                new Vector4(-1, 1, 1, 0),
                new Vector4(-1, 0, 1, 0),
                new Vector4(-1, -1, 1, 0),
                new Vector4(0, -1, 1, 0),
                new Vector4(1, -1, 1, 0),
                new Vector4(1, 0, 2, 0),
                new Vector4(1, 0, 0.25f, 0),
            };

            if (glmode == GL_TRIANGLES)
            {
                for (int i=2; i < fade.Count; i+=3)
                {
                    int count = 0;
                    if (fade[i-2] > 0) count++;
                    if (fade[i-1] > 0) count++;
                    if (fade[i-0] > 0) count++;
                    if (count == 0) {
                        uvs.Add(UVS[0]);
                        uvs.Add(UVS[0]);
                        uvs.Add(UVS[0]);
                    } else if (count == 3) {
                        uvs.Add(UVS[1]);
                        uvs.Add(UVS[5]);
                        uvs.Add(UVS[3]);
                    } else if (count == 2) {
                        int fadeU = 0;
                        int fadeV = 0;
                        for (int j=0; j<3; j++)
                        {
                            if (j == 0) {
                                fadeU = fade[i-2];
                                fadeV = fade[i-0];
                            } else if (j == 1) {
                                fadeU = fade[i-1];
                                fadeV = fade[i-2];
                            } else if (j == 2) {
                                fadeV = fade[i-1];
                                fadeU = fade[i-0];
                            }
                            if (fadeU > 0 && fadeV > 0) {
                                Debug.Log("fade both");
                                uvs.Add(UVS[8]);
                            } else if (fadeU > 0) {
                                Debug.Log("fadeU");
                                uvs.Add(UVS[2]);
                            } else if (fadeV > 0) {
                                Debug.Log("fadeV");
                                uvs.Add(UVS[6]);
                            }
                        }
                    } else if (count == 1 &&
                        fade[i-2] >= 0 && fade[i-1] >= 0 && fade[i-0] >= 0) {
                        int fadeU = 0;
                        int fadeV = 0;
                        Point A = Get(i-2);
                        Point B = Get(i-1);
                        Point C = Get(i-0);
                        Point U = B - A;
                        Point V = C - B;
                        for (int j=0; j<3; j++)
                        {
                            float ratio = 0;
                            if (j == 0) {
                                U = B - A;
                                V = C - A;
                                if (V.length() > 0) {
                                    ratio = U.length() / V.length();
                                }
                                fadeU = fade[i-2];
                                fadeV = fade[i-0];
                            } else if (j == 1) {
                                V = -U;
                                U = C - B;
                                if (V.length() > 0) {
                                    ratio = U.length() / V.length();
                                }
                                fadeU = fade[i-1];
                                fadeV = fade[i-2];
                            } else if (j == 2) {
                                V = -U;
                                U = A - C;
                                if (V.length() > 0) {
                                    ratio = U.length() / V.length();
                                }
                                fadeV = fade[i-1];
                                fadeU = fade[i-0];
                            }
                            if (fadeU > 0 || fadeV > 0) {
                                Debug.Log("fade side");
                                if ((fadeU > 0 && ratio > 4.0) ||
                                    (fadeV > 0 && ratio < 0.25)) {
                                    // this triangle is so short that we want to fade more
                                    uvs.Add(UVS[10]);
                                } else {
                                    uvs.Add(UVS[1]);
                                }
                            } else {
                                Debug.Log("corner fade");
                                uvs.Add(UVS[5]);
                            }
                        }
                    } else if (count == 1) {
                        int fadeU = 0;
                        int fadeV = 0;
                        for (int j=0; j<3; j++)
                        {
                            if (j == 0) {
                                fadeU = fade[i-2];
                                fadeV = fade[i-0];
                            } else if (j == 1) {
                                fadeU = fade[i-1];
                                fadeV = fade[i-2];
                            } else if (j == 2) {
                                fadeV = fade[i-1];
                                fadeU = fade[i-0];
                            }
                            if (fadeU > 0 || fadeV > 0) {
                                Debug.Log("fan fade");
                                uvs.Add(UVS[9]);
                            } else {
                                Debug.Log("zero fade");
                                uvs.Add(UVS[0]);
                            }
                        }
                    }
                }
            }
            return uvs;
        }

        public List<Color> GetColors()
        {
            return color;
        }
    }
}
