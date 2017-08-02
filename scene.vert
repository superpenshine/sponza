layout(location = SCENE_POSITION_ATTRIB_LOCATION)
in vec4 Position;

layout(location = SCENE_TEXCOORD_ATTRIB_LOCATION)
in vec2 TexCoord;

layout(location = SCENE_NORMAL_ATTRIB_LOCATION)
in vec3 Normal;

layout(location = SCENE_TANGENT_ATTRIB_LOCATION)
in vec4 Tangent;

uniform mat4 ModelWorld;
uniform mat4 ModelViewProjection;
uniform mat3 Normal_ModelWorld;

out vec2 my_texCoord;
out vec3 vertPos;
out vec3 normalInterp;

//for shader
uniform mat4 lightMatrix;
out vec4 shadowMapCoord;

//for normalMap
out vec4 tangent;


void main()
{
    //Set to MVP * P, model space to clip space
    gl_Position =  ModelViewProjection*Position;
    //Pass vertex attributes to fragment shader
	my_texCoord = TexCoord;

	vec4 vertPos4 = ModelWorld * Position; //in world space
	vertPos = vec3(vertPos4) / vertPos4.w;
	normalInterp = Normal_ModelWorld * Normal;//surface normal to  vertex normal

	//for shader
	shadowMapCoord = lightMatrix * Position;
	//for normalMap
	tangent = vec4(Normal_ModelWorld * Tangent.xyz, Tangent.w);

}