using Vaser;
using UnityEngine;
using System.Collections.Generic;

public class Demo : MonoBehaviour {

    private void Start() {

        Polyline.polyline_opt opt = new Polyline.polyline_opt();
        opt.feather = false;
        opt.feathering = 15.0f;
        opt.joint = Polyline.polyline_opt.PLJ_round;
        opt.cap = Polyline.polyline_opt.PLC_round;
        opt.triangulation = false;
        {
            Camera cam = Camera.main;
            Vector3 a = cam.WorldToScreenPoint(new Vector3(0,0,0));
            Vector3 b = cam.WorldToScreenPoint(new Vector3(1,1,0));
            opt.world_to_screen_ratio = Vector3.Distance(a,b) / new Vector3(1,1,0).magnitude;
        }

        Polyline polyline = new Polyline(
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

        Mesh mesh = polyline.GetMesh();

        GameObject gameObject = new GameObject("Mesh", typeof(MeshFilter), typeof(MeshRenderer));
        gameObject.transform.localScale = new Vector3(1,1,1);
        gameObject.GetComponent<MeshFilter>().mesh = mesh;
        gameObject.GetComponent<MeshRenderer>().material = Resources.Load<Material>("Vaser/Fade");
    }
}