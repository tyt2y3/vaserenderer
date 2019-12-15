using Vaser;
using UnityEngine;
using System.Collections.Generic;

public class Demo : MonoBehaviour {

    private void Start() {

        Polyline.Opt opt = new Polyline.Opt();
        opt.feather = false;
        opt.feathering = 15.0f;
        opt.joint = Polyline.Opt.PLJround;
        opt.cap = Polyline.Opt.PLCround;
        opt.triangulation = false;
        {
            Camera cam = Camera.main;
            Vector3 a = cam.WorldToScreenPoint(new Vector3(0,0,0));
            Vector3 b = cam.WorldToScreenPoint(new Vector3(1,1,0));
            opt.worldToScreenRatio = Vector3.Distance(a,b) / new Vector3(1,1,0).magnitude;
        }

        int mode = 2;
        Polyline polyline = null;

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

        Mesh mesh = polyline.GetMesh();

        GameObject gameObject = new GameObject("Mesh", typeof(MeshFilter), typeof(MeshRenderer));
        gameObject.transform.localScale = new Vector3(1,1,1);
        gameObject.GetComponent<MeshFilter>().mesh = mesh;
        gameObject.GetComponent<MeshRenderer>().material = Resources.Load<Material>("Vaser/Fade");
    }
}