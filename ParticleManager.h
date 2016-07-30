#pragma once

#include "Particle.h"
#include "glm/vec2.hpp"

#include <vector>

/*-----------------------------------------------------------------------------------------------
Description:
    I don't like the idea of a "manager" because it is a vague description that seems to be used
    when a OO approach isn't thought out, but for this demo program, it will suffice to 
    encapsulate the basic functionality of particle storage, emission, and a region in which they
    are valid.
Creator:    John Cox (7-27-2016)
-----------------------------------------------------------------------------------------------*/
class ParticleManager
{
public:
    ParticleManager();
    ~ParticleManager();
    void Init(unsigned int programId,
        unsigned int computeProgramId,
        unsigned int numParticles, 
        unsigned int maxParticlesEmittedPerFrame,
        const glm::vec2 center,
        float radius,
        float minVelocity,
        float maxVelocity);
    void Cleanup();
    void Update(float deltaTimeSec);

    void Render();

private:
    bool OutOfBounds(const Particle &p) const;
    void ResetParticle(Particle *resetThis) const;
    glm::vec2 GetNewVelocityVector() const;

    glm::vec2 _center;
    float _radiusSqr;   // because radius is never used
    float _velocityMin;
    float _velocityDelta;

    // save on the large header inclusion of OpenGL and write out these primitive types instead 
    // of using the OpenGL typedefs
    // Note: IDs are GLuint (unsigned int), draw style is GLenum (unsigned int), GLushort is 
    // unsigned short.
    unsigned int _programId;
    unsigned int _computeProgramId;
    unsigned int _vaoId;
    unsigned int _arrayBufferId;
    unsigned int _drawStyle;    // GL_TRIANGLES, GL_LINES, etc.
    unsigned int _sizeBytes;    // useful for glBufferSubData(...)
    std::vector<Particle> _allParticles;
    unsigned int _maxParticlesEmittedPerFrame;

    // these are associated with the compute shader
    // Note: To be honest, the only one that needs to be kept around in this demo is the one for 
    // delta time.  That needs to be updated potentially every frame (this demo hard codes it, 
    // but eh?).  Nevertheless, if the user wants to expand and shrink the circle or move the 
    // emitter or flucuate how many particles can be emitted per frame, then these uniforms 
    // could be useful.
    unsigned int _unifLocDeltaTimeSec;
    unsigned int _unifLocRadiusSqr;
    unsigned int _unifLocEmitterCenter;
    unsigned int _unifLocMaxParticlesEmittedPerFrame;
};
