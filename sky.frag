in vec3 TexCoord;
uniform samplerCube skybox;
layout (location = 0) out vec4 color;

void main()
{
    color = vec4(texture(skybox, TexCoord).rgb, 1.0);
}