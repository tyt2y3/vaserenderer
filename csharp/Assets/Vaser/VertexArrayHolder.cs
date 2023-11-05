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

        public int glmode = GL_TRIANGLES; //drawing mode in opengl
        private bool jumping = false;
        private List<Vector3> vert = new List<Vector3>();
        private List<Color> color = new List<Color>();
        private List<float> fade = new List<float>();

        public int GetCount()
        {
            return vert.Count;
        }

        public void SetGlDrawMode(int gl_draw_mode)
        {
            glmode = gl_draw_mode;
        }

        public int Push(Vector2 P, Color cc, float fade0=0)
        {
            int cur = vert.Count;
            vert.Add(P); // A Vector2 can be implicitly converted into a Vector3. (The z is set to zero in the result).
            color.Add(cc);
            fade.Add(fade0);

            if (jumping)
            {
                jumping = false;
                RepeatLastPush();
            }
            return cur;
        }

        public int PushF(Vector2 P, Color C)
        {
            C.a = 0;
            return Push(P, C);
        }

        public void Push3(
                Vector2 P1, Vector2 P2, Vector2 P3,
                Color C1, Color C2, Color C3,
                float fade1=0, float fade2=0, float fade3=0)
        {
            Push(P1, C1, fade1);
            Push(P2, C2, fade2);
            Push(P3, C3, fade3);
        }

        public void Push4(
            Vector2 P1, Vector2 P2, Vector2 P3, Vector2 P4, 
            Color C1, Color C2, Color C3, Color C4,
            float r1, float r2, float r3, float r4)
        {
            //interpret P0 to P3 as triangle strip
            Push3(P1, P2, P3, C1, C2, C3, r1, r2, r3);
            Push3(P3, P2, P4, C3, C2, C4, r3, r2, r4);
        }

        public void Dot(Vector2 P, float size)
        {
            size /= 2;
            if (glmode == GL_TRIANGLES) {
            } else if (glmode == GL_TRIANGLE_STRIP) {
                Push(new Vector2(P.x-size, P.y), Color.red);
                Push(new Vector2(P.x, P.y+size), Color.red);
                Push(new Vector2(P.x, P.y-size), Color.red);
                Push(new Vector2(P.x+size, P.y), Color.red);
                Jump();
            }
        }

        public void Push(VertexArrayHolder hold)
        {
            if (glmode == hold.glmode)
            {
                vert.AddRange(hold.vert);
                color.AddRange(hold.color);
                fade.AddRange(hold.fade);
            }
            else if (glmode == GL_TRIANGLES &&
                hold.glmode == GL_TRIANGLE_STRIP)
            {
                for (int b=3; b < hold.vert.Count; b+=2)
                {
                    vert.Add(hold.vert[b-3]); color.Add(hold.color[b-3]); fade.Add(hold.fade[b-0]);
                    vert.Add(hold.vert[b-2]); color.Add(hold.color[b-2]); fade.Add(hold.fade[b-0]);
                    vert.Add(hold.vert[b-1]); color.Add(hold.color[b-1]); fade.Add(hold.fade[b-1]);
                    vert.Add(hold.vert[b-1]); color.Add(hold.color[b-1]); fade.Add(hold.fade[b-0]);
                    vert.Add(hold.vert[b-2]); color.Add(hold.color[b-2]); fade.Add(hold.fade[b-1]);
                    vert.Add(hold.vert[b-0]); color.Add(hold.color[b-0]); fade.Add(hold.fade[b-0]);
                }
            }
        }

        public Vector2 Get(int i)
        {
            Vector2 P = new Vector2();
            P.x = vert[i].x;
            P.y = vert[i].y;
            return P;
        }

        public Color GetColor(int b)
        {
            return color[b];
        }

        public Vector2 GetRelativeEnd(int di = -1)
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
                new Vector4(0, 0, 0, 0),
                new Vector4(1, 0, 0, 0),
                new Vector4(1, 1, 0, 0),
                new Vector4(0, 1, 0, 0),
                new Vector4(-1, 1, 0, 0),
                new Vector4(-1, 0, 0, 0),
                new Vector4(-1, -1, 0, 0),
                new Vector4(0, -1, 0, 0),
                new Vector4(1, -1, 0, 0),
                new Vector4(-1, 0, 0, 0),
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
                        addUV(0, 1, 1);
                        addUV(0, 1, 1);
                        addUV(0, 1, 1);
                    } else if (count == 3) {
                        addUV(1, fade[i-2], fade[i-2]);
                        addUV(5, fade[i-1], fade[i-1]);
                        addUV(3, fade[i-0], fade[i-0]);
                    } else if (count == 2) {
                        float fadeX = 0, fadeY = 0;
                        for (int j=0; j<3; j++)
                        {
                            float fadeU = 0, fadeV = 0;
                            if (j == 0) {
                                fadeU = fade[i-2];
                                fadeV = fade[i-0];
                            } else if (j == 1) {
                                fadeU = fade[i-1];
                                fadeV = fade[i-2];
                            } else if (j == 2) {
                                fadeU = fade[i-0];
                                fadeV = fade[i-1];
                            }
                            if (fadeU > 0 && fadeV > 0) {
                                fadeX = fadeV;
                                fadeY = fadeU;
                            }
                        }
                        for (int j=0; j<3; j++)
                        {
                            float fadeU = 0, fadeV = 0;
                            if (j == 0) {
                                fadeU = fade[i-2];
                                fadeV = fade[i-0];
                            } else if (j == 1) {
                                fadeU = fade[i-1];
                                fadeV = fade[i-2];
                            } else if (j == 2) {
                                fadeU = fade[i-0];
                                fadeV = fade[i-1];
                            }
                            if (fadeU > 0 && fadeV > 0) {
                                //Debug.Log("fade both");
                                addUV(8, fadeX, fadeY);
                            } else if (fadeU > 0) {
                                //Debug.Log("fadeU");
                                addUV(2, fadeX, fadeY);
                            } else if (fadeV > 0) {
                                //Debug.Log("fadeV");
                                addUV(6, fadeX, fadeY);
                            }
                        }
                    } else if (count == 1 &&
                        fade[i-2] >= 0 && fade[i-1] >= 0 && fade[i-0] >= 0) {
                        float fadeU = 0;
                        float fadeV = 0;
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
                                //Debug.Log("fade side");
                                float fader = fadeU > 0 ? fadeU : fadeV;
                                addUV(1, fader, fader);
                            } else {
                                //Debug.Log("corner fade");
                                float fader = fade[i-2] > 0 ? fade[i-2] : fade[i-1] > 0 ? fade[i-1] : fade[i-0];
                                addUV(5, fader, fader);
                            }
                        }
                    } else if (count == 1) {
                        // may be fade is < 0
                        float fadeU = 0;
                        float fadeV = 0;
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
                                //Debug.Log("fan fade");
                                float fader = fadeU > 0 ? fadeU : fadeV;
                                addUV(9, fader, fader);
                            } else {
                                //Debug.Log("zero fade");
                                float fader = fade[i-2] > 0 ? fade[i-2] : fade[i-1] > 0 ? fade[i-1] : fade[i-0];
                                addUV(0, fader, fader);
                            }
                        }
                    }
                }
            }
            return uvs;

            void addUV(int i, float rx, float ry)
            {
                uvs.Add(new Vector4(UVS[i].x, UVS[i].y, rx, ry));
            }
        }

        public List<Color> GetColors()
        {
            return color;
        }
    }
}
