layout (location = 0) out vec4 FragColor;//vertex position
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedoSpec;

in vec3 vertPos;
in vec3 normalInterp;
in vec2 my_texCoord;
in vec4 tangent;
uniform int HasBumpMap;
uniform int HasDiffuseMap;
uniform vec3 Diffuse;
uniform sampler2D DiffuseMap;
uniform sampler2D BumpMap;

void main(){
	FragColor = vec4(vertPos, 1.0);
	gNormal = normalInterp;
	vec3 diffuse = vec3(1.0);
	gAlbedoSpec.rgb = vec3(1.0);

	if (HasDiffuseMap != 0)
		diffuse = Diffuse * texture(DiffuseMap, my_texCoord).rgb;

	if (HasBumpMap != 0)
	{
		vec3 normalMap = normalize(texture(BumpMap, my_texCoord).rgb * 2.0 - vec3(1));
		vec3 bitangent = normalize(cross(gNormal, normalize(vec3(tangent)))) * tangent.w;
		mat3 tangent_space = mat3(normalize(vec3(tangent)), normalize(bitangent), gNormal);
		gNormal = tangent_space * normalMap;//now in world space
		gAlbedoSpec.rgb = texture(DiffuseMap, my_texCoord).rgb;
	}
}