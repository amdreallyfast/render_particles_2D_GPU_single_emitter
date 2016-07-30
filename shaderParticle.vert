#version 440

// position in window space (both X and Y on the range [-1,+1])
layout (location = 0) in vec2 pos;  

// velocity also in window space (ex: an X speed of 1.0 would cross the window horizontally in 2 
// seconds)
layout (location = 1) in vec2 vel;  

// must have the same name as its corresponding "in" item in the frag shader
smooth out vec3 particleColor;

void main()
{
    // hard code a white particle color
    particleColor = vec3(1.0f, 1.0f, 1.0f);
	gl_Position = vec4(pos, -1.0f, 1.0f);
}

