using Vaser;
using UnityEngine;
using System.Collections.Generic;

public class Demo : MonoBehaviour {

    private void Start() {

        Polyline.Opt opt = new Polyline.Opt();
        opt.feather = true;
        opt.feathering = 15.0f;
        opt.joint = Polyline.Opt.PLJ_round;
        opt.cap = Polyline.Opt.PLC_round;
        opt.triangulation = false;
        {
            Camera cam = Camera.main;
            Vector3 a = cam.WorldToScreenPoint(new Vector3(0,0,0));
            Vector3 b = cam.WorldToScreenPoint(new Vector3(1,1,0));
            opt.world_to_screen_ratio = Vector3.Distance(a,b) / new Vector3(1,1,0).magnitude;
        }

        int mode = 2;
        Polyline polyline = null;

        if (mode == 1) {
            polyline = new Polyline(
                new List<Point> {
                    new Point(0, .75f),
                    new Point(-.75f, -.1f),
                    new Point(.75f, .1f),
                    new Point(0, -.75f),
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
                new List<Point> {
                    new Point(0, .75f),
                    new Point(-.75f, -.1f),
                    new Point(.75f, .1f),
                    new Point(0, -.75f),
                },
                new Vaser.Gradient (
                    new List<Vaser.Gradient.Stop> {
                        new Vaser.Gradient.Stop(0.0f,  Color.red),
                        new Vaser.Gradient.Stop(0.0f,  0.1f),
                        new Vaser.Gradient.Stop(0.25f, Color.green),
                        new Vaser.Gradient.Stop(0.5f,  Color.blue),
                        new Vaser.Gradient.Stop(1.0f,  Color.red),
                    }
                ),
                new Polybezier.Opt {
                    world_to_screen_ratio = opt.world_to_screen_ratio
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