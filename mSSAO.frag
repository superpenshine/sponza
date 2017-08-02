out float FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;//rotation

uniform float Height;
uniform float Width;

uniform vec3 samples[64];
uniform mat4 projection;

// tile noise texture over screen based on screen dimensions divided by noise size

vec2 noiseScale = vec2(Width/4.0, Height/4.0);
int kernelSize = 64;
float radius = 0.2;
float bias = 0.025;

void main()
{//vec3(1.0, 1.0, 1.0);
    vec3 fragPos   = texture(gPosition, TexCoords).xyz;
	vec3 normal    = texture(gNormal, TexCoords).rgb;
	vec3 randomVec = texture(texNoise, TexCoords * noiseScale).xyz;
	vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	mat4 projection2 = projection;
	float occlusion = 0.0;
	//64 is kernel size
	for(int i = 0; i < kernelSize; ++i){
		vec3 Sample = TBN * samples[i]; // From tangent to view space
		Sample = fragPos + Sample * radius; //0.5radius
		vec4 offset = vec4(Sample, 1.0);
		offset = projection * offset;    // from view to clip-space
		offset.xyz /= offset.w;               // perspective divide
		offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0 
		float sampleDepth = texture(gPosition, offset.xy).z;
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= Sample.z + bias ? 1.0 : 0.0) * rangeCheck;  //0.025 bias
	}
	occlusion = 1.0 - (occlusion / kernelSize);
	FragColor = occlusion;    
}