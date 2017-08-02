uniform vec3 CameraPos;

uniform vec3 Ambient;
uniform vec3 Diffuse;
uniform vec3 Specular;
uniform float Shininess;

uniform int HasDiffuseMap;
uniform int HasAlphaMask;
uniform int HasSpecularMap;
uniform int HasBumpMap;
uniform sampler2D DiffuseMap;

layout (location = 0) out vec4 FragColor;//HDR image, not blurred
layout (location = 1) out vec4 BrightColor;//image to be blurred
in vec2 my_texCoord;

in vec3 vertPos;
in vec3 normalInterp;
in vec4 tangent;

//for shadow
in vec4 shadowMapCoord;
uniform sampler2DShadow ShadowMap;

//for alpha map
uniform sampler2D AlphaMap;

//for specular map
uniform sampler2D SpecularMap;

//for bump map
uniform sampler2D BumpMap;


void main()
{
    // TODO: Replace with Phong shading
	vec3 normal = normalize(normalInterp);

	if (HasAlphaMask  != 0)
	{
		if (texture(AlphaMap, my_texCoord).x < 0.50){
			discard;
		}
	}

	if (HasBumpMap != 0)
	{
		vec3 normalMap = normalize(texture(BumpMap, my_texCoord).rgb * 2.0 - vec3(1));
		vec3 bitangent = normalize(cross(normal, normalize(vec3(tangent)))) * tangent.w;
		mat3 tangent_space = mat3(normalize(vec3(tangent)), normalize(bitangent), normal);
		normal = tangent_space * normalMap;
	}
	vec3 lightDir = normalize(CameraPos - vertPos);
	vec3 viewDir = normalize(lightDir);
	vec3 halfDir = normalize(lightDir + viewDir);

	float specular = 0.0;
	float lambertian = max(dot(lightDir,normal), 0.0);

	float specAngle = max(dot(halfDir, normal), 0.0);
	
	//vec3 ambient = Ambient;
	vec3 diffuse = vec3(1.0);
	specular = pow(specAngle, Shininess);
	vec3 SPECULAR = vec3(1.0);
	
	if (HasSpecularMap  != 0){
		//sp = texture(SpecularMap, my_texCoord).rgb ;
		SPECULAR = texture(SpecularMap, my_texCoord).rgb ;
	}

	if (HasDiffuseMap != 0)
		diffuse = Diffuse * texture(DiffuseMap, my_texCoord).rgb;
	//vec3(0.5, 0.2, 0.3) 
	//vec3 color = vec3 (Specular);
	
	vec3 color = lambertian * diffuse + specular *Specular*SPECULAR;
	float visibility = textureProj(ShadowMap, shadowMapCoord);
	FragColor = vec4(color*(visibility + vec3(0.1, 0.1, 0.1)), 0.4);
	//FragColor = vec4(vertPos, 1.0);

	//bloom
	float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
	if(brightness > 1.0)
		BrightColor = vec4(FragColor.rgb, 1.0);
	
}