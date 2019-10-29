Shader "Fade" {

    Properties
    {
        _Feather ("Feather", Float) = 1.0
    }

    SubShader {
        Tags { "Queue"="Transparent" "RenderType"="Transparent" "IgnoreProjector"="True" }

        Pass
        {
            ZWrite Off
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

            float _Feather;

            fixed4 frag (v2f i) : SV_Target {
                fixed4 col = i.color;
                float fact = max(abs(i.uv.x), abs(i.uv.y));
                if (fact > 1) {
                    fact = (fact-1)*2-1;
                }
                if (_Feather == 0) {
                    return col;
                }
                fact = 1-fact;
                fact = min(fact / _Feather, 1);
                col.a = fact;
                return col;
            }
            ENDCG
        }
    }
}