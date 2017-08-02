layout(location = 0) in vec3 position;

uniform mat4 worldProjection;
out vec3 TexCoord;
void main()
{
    gl_Position = worldProjection * vec4(position, 1.0); 
	gl_Position.w = gl_Position.z;
    TexCoord = position;
}