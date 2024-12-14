#version 120

uniform float u_Time;
uniform float u_Chaos;
uniform float u_Scale;
uniform float u_ClockTime;
uniform float u_TimeSpace;

float rand(float x) {
    return fract(sin(x * 12.9898) * 43758.5453);
}

float noise(float x) {
    float i = floor(x);
    float f = fract(x);
    return mix(rand(i), rand(i + 1.0), smoothstep(0.0, 1.0, f));
}

vec4 generateCV() {
    float t = u_Time + u_ClockTime;
    float chaos = u_Chaos;
    float scale = u_Scale;
    
    // time-based vs space-based behavior
    float timeInfluence = 1.0 - u_TimeSpace;
    float spaceInfluence = u_TimeSpace;
    
    vec4 cv;
    
    // cv1: smooth sine wave with chaos modulation
    cv.x = sin(t * (1.0 + chaos * 2.0));
    
    // cv2: phase-shifted complex wave
    float phase = chaos * noise(t * 0.1) * 6.28318;
    cv.y = sin(t * 2.0 + phase);
    
    // cv3: noise-based evolution
    cv.z = mix(
        sin(t * 4.0),
        noise(t * (1.0 + chaos * 5.0)) * 2.0 - 1.0, // normalize noise to [-1, 1]
        chaos
    );
    
    // cv4: chaotic attractor-inspired
    float x = sin(t * 3.0 + cos(t * chaos));
    float y = cos(t * 2.0 + sin(t * (1.0 + chaos)));
    cv.w = (x + y) * 0.5;
    
    // apply time/space influence
    cv = mix(
        cv,
        vec4(noise(cv.x * 10.0) * 2.0 - 1.0,  // normalize noise to [-1, 1]
             noise(cv.y * 10.0) * 2.0 - 1.0,
             noise(cv.z * 10.0) * 2.0 - 1.0,
             noise(cv.w * 10.0) * 2.0 - 1.0),
        spaceInfluence
    );
    
    // scale the outputs to [-1, 1] range and apply user scale
    cv = clamp(cv * scale, -1.0, 1.0);
    
    // convert from [-1, 1] to [0, 1] for output
    return cv * 0.5 + 0.5;
}

void main() {
    gl_FragColor = generateCV();
} 
