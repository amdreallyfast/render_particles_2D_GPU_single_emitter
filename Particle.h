#pragma once

//#include "glm/vec2.hpp"
#include "glm/vec4.hpp"


/*-----------------------------------------------------------------------------------------------
Description:
    This is a simple structure that says where a particle is, where it is going, and whether it
    has gone out of bounds ("is active" flag).  That flag also serves to prevent all particles
    from going out all at once upon creation by letting the "particle updater" regulate how many
    are emitted every frame.
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
struct Particle
{
    //glm::vec2 _position; float f1[2];
    //glm::vec2 _velocity; float f2[2];
    glm::vec4 _position;
    glm::vec4 _velocity;

    // Note: Booleans cannot be uploaded to the shader 
    // (https://www.opengl.org/sdk/docs/man/html/glVertexAttribPointer.xhtml), so send the 
    // "is active" flag as an integer.  It is understood 
    int _isActive; int iBuffer[3];
};