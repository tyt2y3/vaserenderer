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
        private List<bool> fade = new List<bool>();

        public void SetGlDrawMode(int gl_draw_mode)
        {
            glmode = gl_draw_mode;
        }

        public int Push(Point P, Color cc, bool fade0=false)
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
                bool fade1=false, bool fade2=false, bool fade3=false)
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
                    vert.Add(hold.vert[b-3]); color.Add(hold.color[b-3]); fade.Add(false);
                    vert.Add(hold.vert[b-2]); color.Add(hold.color[b-2]); fade.Add(false);
                    vert.Add(hold.vert[b-1]); color.Add(hold.color[b-1]); fade.Add(hold.fade[b-1]);
                    vert.Add(hold.vert[b-1]); color.Add(hold.color[b-1]); fade.Add(false);
                    vert.Add(hold.vert[b-2]); color.Add(hold.color[b-2]); fade.Add(hold.fade[b-0]);
                    vert.Add(hold.vert[b-0]); color.Add(hold.color[b-0]); fade.Add(false);
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
            Point A = new Point();
            Point B = new Point();
            Point C = new Point();
            Point U = new Point();
            Point V = new Point();
            Point X = new Point();
            Vector4 SE = new Vector4(1, -1, 0, 0);
            Vector4 SW = new Vector4(-1, -1, 0, 0);
            Vector4 NE = new Vector4(1, 1, 0, 0);
            Vector4 NW = new Vector4(-1, 1, 0, 0);
            Vector4 N = new Vector4(0, 1, 0, 0);
            Vector4 E = new Vector4(1, 0, 0, 0);
            Vector4 S = new Vector4(0, -1, 0, 0);
            Vector4 W = new Vector4(-1, 0, 0, 0);
            Vector4 O = new Vector4(0, 0, 0, 0);
            if (glmode == GL_TRIANGLES)
            {
                for (int i=2; i < fade.Count; i+=3)
                {
                    if (!fade[i-2] && !fade[i-1] && !fade[i-0]) {
                        uvs.Add(O);
                        uvs.Add(O);
                        uvs.Add(O);
                        continue;
                    }
                    A = Get(i-2);
                    B = Get(i-1);
                    C = Get(i-0);
                    bool fadeU = false;
                    bool fadeV = false;
                    for (int j=0; j<3; j++)
                    {
                        if (j == 0) {
                            U = B - A;
                            V = C - A;
                            fadeU = fade[i-2];
                            fadeV = fade[i-0];
                        } else if (j == 1) {
                            V = -U;
                            U = C - B;
                            fadeU = fade[i-1];
                            fadeV = fade[i-2];
                        } else if (j == 2) {
                            V = -U;
                            U = A - C;
                            fadeV = fade[i-1];
                            fadeU = fade[i-0];
                        }
                        if (fadeU && fadeV) {
                            Debug.Log("fade both");
                            X = U + V;
                            if (X.x > 0 && X.y > 0) {
                                uvs.Add(NE);
                            } else if (X.x < 0 && X.y > 0) {
                                uvs.Add(NW);
                            } else if (X.x > 0 && X.y < 0) {
                                uvs.Add(SE);
                            } else if (X.x < 0 && X.y < 0) {
                                uvs.Add(SW);
                            }
                        } else if (fadeU) {
                            Debug.Log("fadeU");
                            if (V.x > 0 && V.y > 0) {
                                uvs.Add(NE);
                            } else if (V.x < 0 && V.y > 0) {
                                uvs.Add(NW);
                            } else if (V.x > 0 && V.y < 0) {
                                uvs.Add(SE);
                            } else if (V.x < 0 && V.y < 0) {
                                uvs.Add(SW);
                            }
                        } else if (fadeV) {
                            Debug.Log("fadeV");
                            if (U.x > 0 && U.y > 0) {
                                uvs.Add(NE);
                            } else if (U.x < 0 && U.y > 0) {
                                uvs.Add(NW);
                            } else if (U.x > 0 && U.y < 0) {
                                uvs.Add(SE);
                            } else if (U.x < 0 && U.y < 0) {
                                uvs.Add(SW);
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
