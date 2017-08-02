layout(location = 0) in vec2 Position;

out vec2 my_texCoord;

void main()
{
	if (gl_VertexID == 0)
	{
	gl_Position = vec4(Position, 0, 1);
	my_texCoord = vec2(0, 0);
	}else if (gl_VertexID == 1)
	{
	gl_Position = vec4(Position, 0, 1);
	my_texCoord = vec2(1, 0);
	}
	else if (gl_VertexID == 2)
	{
	gl_Position = vec4(Position, 0, 1);
	my_texCoord = vec2(1, 1);
	}
	else if (gl_VertexID == 3)
	{
	gl_Position = vec4(Position, 0, 1);
	my_texCoord = vec2(0, 0);
	}
	else if (gl_VertexID == 4)
	{
	gl_Position = vec4(Position, 0, 1);
	my_texCoord = vec2(1, 1);
	}
	else if (gl_VertexID == 5)
	{
	gl_Position = vec4(Position, 0, 1);
	my_texCoord = vec2(0, 1);
	}
	else
	{
	gl_Position = vec4(Position, 0, 1);
	my_texCoord = vec2(0, 0);
	}
}