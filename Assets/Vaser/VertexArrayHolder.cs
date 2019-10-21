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

        public int count { get; private set; } = 0; //counter
        public int glmode { get; set; } = GL_TRIANGLES; //drawing mode in opengl
        private bool jumping = false;
        private List<float> vert = new List<float>(); //because it holds 2d vectors
        private List<Color> color = new List<Color>();

        public void SetGlDrawMode(int gl_draw_mode)
        {
            glmode = gl_draw_mode;
        }

        public int Push(Point P, Color cc, bool trans = false)
        {
            int cur = count;
            vert.Add(P.x);
            vert.Add(P.y);
            if (!trans) {
                color.Add(cc);
            } else {
                Color ccc = new Color();
                ccc.r = cc.r;
                ccc.g = cc.g;
                ccc.b = cc.b;
                ccc.a = 0;
                color.Add(ccc);
            }

            count++;
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
                bool trans1 = false, bool trans2 = false, bool trans3 = false)
        {
            Push(P1, C1, trans1);
            Push(P2, C2, trans2);
            Push(P3, C3, trans3);
        }

        public void Push(VertexArrayHolder hold)
        {
            if (glmode == hold.glmode)
            {
                count += hold.count;
                vert.AddRange(hold.vert);
                color.AddRange(hold.color);
            }
            else if (glmode == GL_TRIANGLES &&
                hold.glmode == GL_TRIANGLE_STRIP)
            {
                for (int b=2; b < hold.count; b++)
                {
                    for ( int k=0; k<3; k++, count++)
                    {
                        int B = b-2 + k;
                        vert.Add(hold.vert[B*2]);
                        vert.Add(hold.vert[B*2+1]);
                        color.Add(hold.color[B*4]);
                        color.Add(hold.color[B*4+1]);
                        color.Add(hold.color[B*4+2]);
                        color.Add(hold.color[B*4+3]);
                    }
                }
            }
        }

        public Point Get(int i)
        {
            Point P = new Point();
            P.x = vert[i*2];
            P.y = vert[i*2+1];
            return P;
        }

        public Color GetColor(int b)
        {
            return color[b];
        }

        public Point GetRelativeEnd(int di = -1)
        {
            //di=-1 is the last one
            int i = count+di;
            if (i<0) i=0;
            if (i>=count) i=count-1;
            return Get(i);
        }

        public void RepeatLastPush()
        {
            Point P = new Point();
            Color cc = new Color();

            int i = count-1;

            P.x = vert[i*2];
            P.y = vert[i*2+1];
            Push(P, GetColor(i));
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
            List<Vector3> vertices = new List<Vector3>();
            if (glmode == GL_TRIANGLES)
            {
                for (int i=1; i<vert.Count; i+=2)
                {
                    vertices.Add(new Vector3(vert[i-1], vert[i], 0));
                }
            }
            return vertices;
        }

        public List<int> GetTriangles()
        {
            List<int> indices = new List<int>();
            if (glmode == GL_TRIANGLES)
            {
                int count = vert.Count / 2;
                for (int i = 0; i < count; i++)
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
            Point X = new Point();
            Point Y = new Point();
            Point Z = new Point();
            Point UV = new Point();
            if (glmode == GL_TRIANGLES)
            {
                for (int i = 5; i < vert.Count; i += 6)
                {
                    A.x = vert[i-5]; A.y = vert[i-4];
                    B.x = vert[i-3]; B.y = vert[i-2];
                    C.x = vert[i-1]; C.y = vert[i-0];
                    X = B - A;
                    Y = C - B;
                    Z = A - C;
                    UV = X - Z;
                    UV.normalize();
                    uvs.Add(new Vector4(UV.x, UV.y, 0, 0));
                    UV = Y - X;
                    UV.normalize();
                    uvs.Add(new Vector4(UV.x, UV.y, 0, 0));
                    UV = Z - Y;
                    UV.normalize();
                    uvs.Add(new Vector4(UV.x, UV.y, 0, 0));
                }
            }
            return uvs;
        }

        public List<Color> GetColors()
        {
            return color;
        }

        /*public void draw_triangles()
        {
            Color col={1 , 0, 0, 0.5};
            if (glmode == GL_TRIANGLES)
            {
                for (int i=0; i<count; i++)
                {
                    Point P[4];
                    P[0] = get(i); i++;
                    P[1] = get(i); i++;
                    P[2] = get(i);
                    P[3] = P[0];
                    polyline((Vec2*)P,col,1.0,4,0);
                }
            }
            else if (glmode == GL_TRIANGLE_STRIP)
            {
                for (int i=2; i<count; i++)
                {
                    Point P[3];
                    P[0] = get(i-2);
                    P[1] = get(i);
                    P[2] = get(i-1);
                    polyline((Vec2*)P,col,1.0,3,0);
                }
            }
        }*/
    }
}
