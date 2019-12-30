using Vaser;
using UnityEngine;
using System.Collections.Generic;

public class Demo : MonoBehaviour {

    private int mode = 3;
    private Polyline polyline = null;
    private Polyline.Opt opt = new Polyline.Opt();
    private GameObject myGameObject = null;

    private void Start() {

        opt.feather = false;
        opt.feathering = 5f;
        opt.joint = Polyline.Opt.PLJround;
        opt.cap = Polyline.Opt.PLCround;
        opt.triangulation = false;
        {
            Camera cam = Camera.main;
            Vector3 a = cam.WorldToScreenPoint(new Vector3(0,0,0));
            Vector3 b = cam.WorldToScreenPoint(new Vector3(1,1,0));
            opt.worldToScreenRatio = Vector3.Distance(a,b) / new Vector3(1,1,0).magnitude;
        }

        if (mode == 1) {
            polyline = new Polyline(
                new List<Vector2> {
                    new Vector2(0, .75f),
                    new Vector2(-.75f, -.1f),
                    new Vector2(.75f, .1f),
                    new Vector2(0, -.75f),
                },
                new List<Color> {
                    Color.red,
                    Color.green,
                    Color.blue,
                    Color.red,
                },
                new List<float> {
                    0.05f, 0.15f, 0.15f, 0.05f,
                },
                opt
            );
        } else if (mode == 2) {
            Polybezier polybezier = new Polybezier(
                new List<Vector2> {
                    new Vector2(0, .75f),
                    new Vector2(-.75f, -.1f),
                    new Vector2(.75f, .1f),
                    new Vector2(0, -.75f),
                },
                new Vaser.Gradient (
                    new List<Vaser.Gradient.Stop> {
                        new Vaser.Gradient.Stop(0.0f,  Color.red),
                        new Vaser.Gradient.Stop(0.0f,  0.01f),
                        new Vaser.Gradient.Stop(0.25f, Color.green),
                        new Vaser.Gradient.Stop(0.25f, 0.15f),
                        new Vaser.Gradient.Stop(0.5f,  Color.blue),
                        new Vaser.Gradient.Stop(0.5f,  0.15f),
                        new Vaser.Gradient.Stop(1.0f,  Color.red),
                        new Vaser.Gradient.Stop(1.0f,  0.01f),
                    }
                ),
                new Polybezier.Opt {
                    worldToScreenRatio = opt.worldToScreenRatio
                }
            );
            polyline = polybezier.Render(opt);
        }

        myGameObject = new GameObject("Mesh", typeof(MeshFilter), typeof(MeshRenderer));
        myGameObject.transform.localScale = new Vector3(1,1,1);
        myGameObject.GetComponent<MeshRenderer>().material = Resources.Load<Material>("Vaser/Fade");

        if (polyline != null) {
            Mesh mesh = polyline.GetMesh();
            myGameObject.GetComponent<MeshFilter>().mesh = mesh;
        }
    }

    void Update()
    {
        float time = Time.fixedTime;
        if (mode == 3) {
            polyline = new Polyline();
            polyline.holder.Push4(
                new Vector2(-2.3f,  1),
                new Vector2( 2.3f,  1),
                new Vector2(-2.3f, -1),
                new Vector2( 2.3f, -1),
                new Color(0.365f, 0.808f, 0.910f, 1),
                new Color(0.769f, 0.380f, 0.639f, 1),
                new Color(0.365f, 0.808f, 0.910f, 1),
                new Color(0.5f, 0.305f, 0.773f, 1),
                0, 0, 0, 0
            );
            // pink new Color(0.769f, 0.380f, 0.639f, 1)
            // cyan new Color(0.180f, 0.5f, 0.941f, 1)
            for (int k=0; k<10; k++)
            {
                Polybezier.Buffer buffer = new Polybezier.Buffer();
                buffer.AddPoints(ComputeSineCurve(time, 0.5f+0.035f*k));
                buffer.Gradient(
                    new Vaser.Gradient (
                        new List<Vaser.Gradient.Stop> {
                            new Vaser.Gradient.Stop(0.0f,  new Color(0.365f, 0.808f, 0.910f, 1)),
                            new Vaser.Gradient.Stop(0.5f,  new Color(0.914f, 0.914f, 0.369f, 1)),
                            new Vaser.Gradient.Stop(1.0f,  new Color(0.5f, 0.305f, 0.773f, 1)),
                            new Vaser.Gradient.Stop(0.0f,  0.01f),
                            new Vaser.Gradient.Stop(1.0f,  0.01f),
                        }
                    )
                );
                polyline.Append(buffer.Render(opt));
            }
            Mesh mesh = polyline.GetMesh();
            myGameObject.GetComponent<MeshFilter>().mesh = mesh;
        }
    }

    private List<Vector2> ComputeSineCurve(float time, float amplitude)
    {
        List<Vector2> curve = new List<Vector2> ();
        for (float t=-1; t<1; t+=0.02f) {
            float value = 0;
            List<float> harmonics = new List<float> { 0.5f, 0.25f, 0.25f };
            for (int j=0; j<harmonics.Count; j++) {
                value += harmonics[j] * (float) System.Math.Sin(time - System.Math.PI * t * (j+1));
            }
            curve.Add(new Vector2(2.3f * t, -t + amplitude * value));
        }
        return curve;
    }
}