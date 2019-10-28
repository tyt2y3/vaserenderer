using Vaser;
using UnityEngine;
using System.Collections.Generic;

public class Demo : MonoBehaviour {

    private void Start() {
        Mesh mesh = new Mesh();

        VertexArrayHolder vah = new VertexArrayHolder();

        Point A = new Point(-1.5f, 0.75f);
        Point B = new Point(1f, 1f);
        Point C = new Point(-1f, -1.25f);
        Point D = new Point(0.5f, -1f);
        vah.Push3(A, B, C, Color.red, Color.red, Color.red, true, false, true);
        vah.Push3(C, B, D, Color.red, Color.red, Color.red, false, true, true);

        mesh.SetVertices(vah.GetVertices());
        mesh.SetUVs(0, vah.GetUVs());
        mesh.SetColors(vah.GetColors());
        mesh.SetTriangles(vah.GetTriangles(), 0);

        GameObject gameObject = new GameObject("Mesh", typeof(MeshFilter), typeof(MeshRenderer));
        gameObject.transform.localScale = new Vector3(1,1,1);
        gameObject.GetComponent<MeshFilter>().mesh = mesh;
        gameObject.GetComponent<MeshRenderer>().material = Resources.Load<Material>("Vaser/Rectangle");
    }
}