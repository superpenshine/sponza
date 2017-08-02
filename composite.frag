out vec4 FragColor;
in vec2 my_texCoord;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
//for ssao
uniform sampler2D ssao;
uniform sampler2D DiffuseMap;

//uniform float exposure;

void main()
{             
    const float gamma = 2.2;
	float AmbientOcclusion = texture(ssao, my_texCoord).r;
	vec3 diffuse = vec3(1.0);
	diffuse = texture(DiffuseMap, my_texCoord).rgb;
	vec3 ambient = vec3(0.3 * diffuse * AmbientOcclusion);
    vec3 hdrColor = texture(scene, my_texCoord).rgb;      
    vec3 bloomColor = texture(bloomBlur, my_texCoord).rgb;
    hdrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * 1);//0.5 exposure
    // also gamma correct while we're at it
    //result = pow(result, vec3(1.0 / gamma));
	FragColor = vec4(result, 1.0f);
}  