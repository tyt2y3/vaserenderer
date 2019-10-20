Shader "Tutorial/034_2D_SDF_Basics/Rectangle"{

	SubShader{
		//the material is completely non-transparent and is rendered at the same time as the other opaque geometry
		Tags{ "RenderType"="Opaque" "Queue"="Geometry"}

		Pass{
			CGPROGRAM

			#include "UnityCG.cginc"

			#pragma vertex vert
			#pragma fragment frag

			float2 rotate(float2 samplePosition, float rotation){
				const float PI = 3.14159;
				float angle = rotation * PI * 2 * -1;
				float sine, cosine;
				sincos(angle, sine, cosine);
				return float2(cosine * samplePosition.x + sine * samplePosition.y, cosine * samplePosition.y - sine * samplePosition.x);
			}

			float2 translate(float2 samplePosition, float2 offset){
				//move samplepoint in the opposite direction that we want to move shapes in
				return samplePosition - offset;
			}

			float2 scale(float2 samplePosition, float scale){
				return samplePosition / scale;
			}

			float circle(float2 samplePosition, float radius){
				//get distance from center and grow it according to radius
				return length(samplePosition) - radius;
			}

			float rectangle(float2 samplePosition, float2 halfSize){
				float2 componentWiseEdgeDistance = abs(samplePosition) - halfSize;
				return max(componentWiseEdgeDistance.x, componentWiseEdgeDistance.y);
			}

			struct appdata{
				float4 vertex : POSITION;
			};

			struct v2f{
				float4 position : SV_POSITION;
				float4 worldPos : TEXCOORD0;
			};

			v2f vert(appdata v){
				v2f o;
				//calculate the position in clip space to render the object
				o.position = UnityObjectToClipPos(v.vertex);
				//calculate world position of vertex
				o.worldPos = mul(unity_ObjectToWorld, v.vertex);
				return o;
			}

			float scene(float2 position) {
				float2 circlePosition = position;
				circlePosition = rotate(circlePosition, _Time.y * 0.01);
				circlePosition = translate(circlePosition, float2(2, 0));
				float sceneDistance = rectangle(circlePosition, float2(1, 2));
				return sceneDistance;
			}

			fixed4 frag(v2f i) : SV_TARGET{
				float dist = scene(i.worldPos.xz);
				fixed4 col = fixed4(dist, dist, dist, 1);
				return col;
			}

			ENDCG
		}
	}
	FallBack "Standard" //fallback adds a shadow pass so we get shadows on other objects
}