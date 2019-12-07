Shader "Fade" {

    SubShader {
        Tags { "Queue"="Transparent" "RenderType"="Transparent" "IgnoreProjector"="True" }

        Pass
        {
            ZWrite On
            Blend SrcAlpha OneMinusSrcAlpha

            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            #include "UnityCG.cginc"

            struct appdata {
                float4 vertex : POSITION;
                float4 uv : TEXCOORD0;
                fixed4 color : COLOR;
            };

            struct v2f {
                float4 uv : TEXCOORD0;
                float4 vertex : SV_POSITION;
                fixed4 color : COLOR;
            };

            v2f vert (appdata v) {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.uv = v.uv;
                o.color = v.color;
                return o;
            }

            fixed4 frag (v2f i) : SV_Target {
                fixed4 col = i.color;
                float fact;
                if (i.uv.w == 0) {
                    // rectangular fade
                    fact = max(abs(i.uv.x), abs(i.uv.y));
                } else if (i.uv.w == 1) {
                    // circular fade
                    fact = sqrt(pow(i.uv.x, 2) + pow(i.uv.y, 2));
                } else {
                    fact = 0;
                }
                fact = min((1 - fact) * i.uv.z, 1);
                col.a *= fact;
                return col;
            }
            ENDCG
        }
    }
}