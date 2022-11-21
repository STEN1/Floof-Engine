#version 450

layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec3 WorldPos;

layout (set = 0, binding = 0) uniform samplerCube environmentMap;

const float PI = 3.14159265359;
const float Epsilon = 0.01;

void main()
{		
	// The world vector acts as the normal of a tangent surface
    // from the origin, aligned to WorldPos. Given this normal, calculate all
    // incoming radiance of the environment. The result of this radiance
    // is the radiance of light coming from -Normal direction, which is what
    // we use in the PBR shader to sample irradiance.
   vec3 N = normalize(WorldPos);
//
    vec3 irradiance = vec3(0.0);
//    
//    // tangent space calculation from origin point
    vec3 up    = vec3(0.0, 1.0, 0.0);
//
    if (N.y > 1.0 - Epsilon) {
        up += vec3(Epsilon, 0.0, Epsilon);
    }
//
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
//       
   //float sampleDelta = 0.025;
   float sampleDelta = 0.050; // 0.04 minimum on mac
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
//            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            //vec3 tangentSample = vec3(1.0,1.0,1.0);
//            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
//
            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
       }
   }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
//    
<<<<<<< HEAD
    FragColor = vec4(irradiance, 1.0);
    //FragColor = vec4(1.0);
=======
//    FragColor = vec4(irradiance, 1.0);
    FragColor = vec4(vec3(0.5), 1.0);
>>>>>>> 877815836e8793e7df08d832d04a9c7704c15f67
}