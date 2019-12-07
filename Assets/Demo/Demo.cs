using Vaser;
using UnityEngine;
using System.Collections.Generic;

public class Demo : MonoBehaviour {

    private void Start() {

        Polyline.st_anchor SA = new Polyline.st_anchor();
        VertexArrayHolder vah = null;
        int mode = 1;
        if (mode == 0) {
            Point A = new Point(-0.5f, 0.5f);
            Point B = new Point(0.5f, 0.5f);
            Point C = new Point(-0.5f, -0.5f);
            Point D = new Point(0.5f, -0.5f);
            vah = new VertexArrayHolder();
            vah.Push3(A, B, C, Color.red, Color.red, Color.red, 2, 0, 2);
            vah.Push3(C, B, D, Color.red, Color.red, Color.red, 0, 2, 2);
        } else if (mode == 1) {
            // normal anchor
            SA.P[0] = new Point(-1f, -0.5f);
            SA.P[1] = new Point(0f, 0.75f);
            SA.P[2] = new Point(1f, -0.5f);
        } else if (mode == 2) {
            // degen prefull
            SA.P[0] = new Point(-1f, -0.75f);
            SA.P[1] = new Point(0f, 0.75f);
            SA.P[2] = new Point(-0.5f, -0.5f);
        } else if (mode == 3) {
            // degen !prefull
            SA.P[0] = new Point(-0.5f, -0.5f);
            SA.P[1] = new Point(0f, 0.75f);
            SA.P[2] = new Point(-0.25f, -1f);
        } else if (mode == 4) {
            // segment
            SA.P[0] = new Point(-1f, -0.5f);
            SA.P[1] = new Point(1f, 0.25f);
        }

        if (mode > 0) {
            SA.C[0] = Color.red;
            SA.C[1] = Color.green;
            SA.C[2] = Color.blue;
            SA.W[0] = 0.25f;
            SA.W[1] = 0.25f;
            SA.W[2] = 0.25f;

            Polyline.polyline_opt opt = new Polyline.polyline_opt();
            opt.feather = true;
            opt.feathering = 20.0f;
            {
                Camera cam = Camera.main;
                Vector3 a = cam.WorldToScreenPoint(new Vector3(0,0,0));
                Vector3 b = cam.WorldToScreenPoint(new Vector3(1,1,0));
                opt.world_to_screen_ratio = Vector3.Distance(a,b) / new Vector3(1,1,0).magnitude;
            }
            if (mode <= 3) {
                opt.joint = Polyline.polyline_opt.PLJ_round;
                opt.cap = Polyline.polyline_opt.PLC_round;
                Polyline.Anchor(SA, opt, true, true);
            } else if (mode == 4) {
                opt.cap = Polyline.polyline_opt.PLC_round;
                Polyline.Segment(SA, opt, true, true, true);
            }
            vah = SA.vah;
        }

        Mesh mesh = new Mesh();
        mesh.SetVertices(vah.GetVertices());
        mesh.SetUVs(0, vah.GetUVs());
        mesh.SetColors(vah.GetColors());
        mesh.SetTriangles(vah.GetTriangles(), 0);

        GameObject gameObject = new GameObject("Mesh", typeof(MeshFilter), typeof(MeshRenderer));
        gameObject.transform.localScale = new Vector3(1,1,1);
        gameObject.GetComponent<MeshFilter>().mesh = mesh;
        gameObject.GetComponent<MeshRenderer>().material = Resources.Load<Material>("Vaser/Fade");
    }
}