#include "ParticleManager.h"

#include "glm/detail/func_geometric.hpp"    // glm::dot
#include "RandomToast.h"
#include "glload/include/glload/gl_4_4.h"


/*-----------------------------------------------------------------------------------------------
Description:
    An empty constructor.  It exists because I didn't know if anything had to go in it, and then
    I kept it around even when everything was moved to the Init(...) method.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (7-26-2016)
-----------------------------------------------------------------------------------------------*/
ParticleManager::ParticleManager()
{

}

/*-----------------------------------------------------------------------------------------------
Description:
    Calls Cleanup() in the event that the user forgot to call it themselves.
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (7-30-2016)
-----------------------------------------------------------------------------------------------*/
ParticleManager::~ParticleManager()
{
    this->Cleanup();
}

/*-----------------------------------------------------------------------------------------------
Description:
    As the name suggests, this deletes the shader program, vertex buffer, and VAO associated 
    with this object.  Is called in the constructor in the event that someone forgot to call it 
    explicitly.  This method exists so that the user can reset it without deleting the actual 
    object (??why would you want to do this??) .
Parameters: None
Returns:    None
Exception:  Safe
Creator:    John Cox (7-26-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleManager::Cleanup()
{
    glDeleteProgram(_programId);
    glDeleteBuffers(1, &_arrayBufferId);
    glDeleteVertexArrays(1, &_vaoId);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Records the program ID, the maximum number of particles to be emitted per frame, and the 
    emitter center.  Re-sizes the internal collection of particles to accomodate the specified 
    number.  Calculates the boundary at which particles become invalid based on the provided 
    radius.  Calculates the velocity delta from the provided min and max velocities.
Parameters: 
    programId       The shader program must be constructed prior to this.
    numParticles    The maximum number of particles that the manager has to work with.
    maxParticlesEmittedPerFrame     Self-explanatory.
    center          A 2D vector in window coordinates (X and Y bounded by [-1,+1]).
    radius          In window coords.  
    minVelocity     In window coords.
    maxVelocity     In window coords.
Returns:    None
Exception:  Safe
Creator:    John Cox (7-26-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleManager::Init(unsigned int programId,
    unsigned int numParticles, 
    unsigned int maxParticlesEmittedPerFrame,
    const glm::vec2 center,
    float radius,
    float minVelocity,
    float maxVelocity)
{
    _programId = programId;
    _allParticles.resize(numParticles);
    _sizeBytes = sizeof(Particle) * numParticles;
    _drawStyle = GL_POINTS;
    _maxParticlesEmittedPerFrame = maxParticlesEmittedPerFrame;
    _center = center;
    _radiusSqr = radius * radius;   // because only radius squared is used during update
    _velocityMin = minVelocity;
    _velocityDelta = maxVelocity - minVelocity;

    // start all particles at the emission orign
    for (size_t particleCount = 0; particleCount < _allParticles.size(); particleCount++)
    {
        // pointer arithmetic will do
        this->ResetParticle(_allParticles.data() + particleCount);
    }

    // initialize OpenGL objects
    // Note: MUST bind the program beforehand or else the VAO generation and binding will blow 
    // up.
    glUseProgram(programId);

    glGenVertexArrays(1, &_vaoId);
    glGenBuffers(1, &_arrayBufferId);

    // the order of vertex array / buffer array binding doesn't matter so long as both are bound 
    // before setting vertex array attributes
    glBindVertexArray(_vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, _arrayBufferId);

    // just allocate space now, and send updated data at render time
    GLuint bufferSizeBytes = sizeof(Particle);
    bufferSizeBytes *= numParticles;
    glBufferData(GL_ARRAY_BUFFER, bufferSizeBytes, 0, GL_DYNAMIC_DRAW);

    // position appears first in structure and so is attribute 0 
    // velocity appears second and is attribute 1
    // "is active" flag is third and is attribute 2
    unsigned int vertexArrayIndex = 0;
    unsigned int bufferStartOffset = 0;

    unsigned int bytesPerStep = sizeof(Particle);

    // position
    GLenum itemType = GL_FLOAT;
    unsigned int numItems = sizeof(Particle::_position) / sizeof(float);
    glEnableVertexAttribArray(vertexArrayIndex);
    glVertexAttribPointer(vertexArrayIndex, numItems, itemType, GL_FALSE, bytesPerStep, (void *)bufferStartOffset);

    // velocity
    itemType = GL_FLOAT;
    numItems = sizeof(Particle::_velocity) / sizeof(float);
    bufferStartOffset += sizeof(Particle::_position);
    vertexArrayIndex++;
    glEnableVertexAttribArray(vertexArrayIndex);
    glVertexAttribPointer(vertexArrayIndex, numItems, itemType, GL_FALSE, bytesPerStep, (void *)bufferStartOffset);

    // ignoring the "is active" flag by not telling OpenGL that there is an item here 
    // Note: Does this waste bytes? Yes, but it would be more work to pluck out the position and 
    // velocity and make those contiguous in another data structure than it would be to simply 
    // ignore the "is active" flag.
    //// "is active" flag
    //itemType = GL_INT;
    //numItems = sizeof(Particle::_isActive) / sizeof(int);
    //bufferStartOffset += sizeof(Particle::_velocity);
    //vertexArrayIndex++;
    //glEnableVertexAttribArray(vertexArrayIndex);
    //glVertexAttribPointer(vertexArrayIndex, numItems, itemType, GL_FALSE, bytesPerStep, (void *)bufferStartOffset);

    // cleanup
    glBindVertexArray(0);   // unbind this BEFORE the array
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);    // always last
}

/*-----------------------------------------------------------------------------------------------
Description:
    Checks if each particle is out of bounds, and if so, resets it.  If the quota for emitted 
    particles hasn't been reached yet, then the particle is sent back out again.  Lastly, if the 
    particle is active, then its position is updated with its velocity and the provided delta 
    time.
Parameters:
    deltatimeSec        Self-explanatory
Returns:    None
Exception:  Safe
Creator:    John Cox (7-4-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleManager::Update(float deltaTimeSec)
{
    unsigned int emitCounter = 0;
    for (size_t particleIndex = 0; particleIndex < _allParticles.size(); particleIndex++)
    {
        Particle &particleRef = _allParticles[particleIndex];
        if (this->OutOfBounds(particleRef))
        {
            particleRef._isActive = false;
            Particle pCopy = (_allParticles[particleIndex]);
            this->ResetParticle(&pCopy);
            _allParticles[particleIndex] = pCopy;
        }

        // TODO: ?a way to make these conditions into assignments to avoid the pipeline thrashing? perhaps take advantage of "is active" being an integer??

        // if vs else-if()? eh
        if (!particleRef._isActive && emitCounter < _maxParticlesEmittedPerFrame)
        {
            particleRef._isActive = true;
            emitCounter++;
        }

        if (particleRef._isActive)
        {
            particleRef._position = particleRef._position +
                (particleRef._velocity * deltaTimeSec);
        }
    }
}

void ParticleManager::Render()
{
    glUseProgram(_programId);
    glBindVertexArray(_vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, _arrayBufferId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, _sizeBytes, _allParticles.data());
    glDrawArrays(_drawStyle, 0, _allParticles.size());
    glUseProgram(0);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Checks if the provided particle has gone outside the circle.
Parameters:
    p   A const reference to a Particle object.
Returns:
    True if the particle's position is outside the circle's boundaries, otherwise false.
    Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
bool ParticleManager::OutOfBounds(const Particle &p) const
{
    glm::vec2 centerToParticle = p._position - _center;
    float distSqr = glm::dot(centerToParticle, centerToParticle);
    if (distSqr > _radiusSqr)
    {
        return true;
    }
    else
    {
        return false;
    }

}

/*-----------------------------------------------------------------------------------------------
Description:
    Sets the given particle's starting position and velocity.  Does NOT alter the "is active"
    flag.  That flag is altered during Update(...).
Parameters:
    resetThis   Self-explanatory.
Returns:    None
Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
void ParticleManager::ResetParticle(Particle *resetThis) const
{
    resetThis->_position = _center;
    resetThis->_velocity = this->GetNewVelocityVector();
}

/*-----------------------------------------------------------------------------------------------
Description:
    Generates a new velocity vector between the previously provided minimum and maximum values 
    and in the provided direction.
Parameters: None
Returns:    
    A 2D vector whose magnitude is between the initialized "min" and "max" values and whose
    direction is random.
Exception:  Safe
Creator:    John Cox (7-2-2016)
-----------------------------------------------------------------------------------------------*/
glm::vec2 ParticleManager::GetNewVelocityVector() const
{
    // randomize between the min and max velocities to get a little variation
    float velocityVariation = RandomOnRange0to1() * _velocityDelta;
    float velocityMagnitude = _velocityMin + velocityVariation;

    // this demo particle "manager" emits in a circle, so get a random 2D direction
    float newX = (float)(RandomPosAndNeg() % 100);
    float newY = (float)(RandomPosAndNeg() % 100);
    glm::vec2 randomVelocityVector = glm::normalize(glm::vec2(newX, newY));
    
    return randomVelocityVector * velocityMagnitude;
}
