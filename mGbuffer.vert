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

out vec3 vertPos;
out vec3 normalInterp;
out vec2 my_texCoord;
out vec4 tangent;

void main()
{    
    gl_Position = ModelViewProjection*Position;//goes into clip space

	vec4 vertPos4 = ModelWorld * Position;//vertPos4 in view/world space
	//vertPos = vec3(vertPos4) / vertPos4.w;
	vertPos = gl_Position.rgb;

	normalInterp = Normal_ModelWorld * Normal;
	my_texCoord = TexCoord;
	tangent = Tangent;
}  