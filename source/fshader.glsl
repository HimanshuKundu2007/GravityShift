#version 150

in vec3 fPositionEye;
in vec3 fNormalEye;

out vec4 fColor;

uniform vec4 AmbientProduct;
uniform vec4 DiffuseProduct;
uniform vec4 SpecularProduct;
uniform vec4 LightPosition; // Eye-space position of overhead sector luminaire
uniform float Shininess;

void main()
{
    // Re-normalize interpolated surface normal
    vec3 N = normalize(fNormalEye);
    if (!gl_FrontFacing) {
        N = -N;
    }

    // Eye direction pointing towards the camera at origin in eye space
    vec3 V = normalize(-fPositionEye);

    // -------------------------------------------------------------
    // 1. PRIMARY OVERHEAD STATION LUMINAIRE (Point Light with Attenuation)
    // -------------------------------------------------------------
    vec3 lightVec = LightPosition.xyz - fPositionEye;
    float lightDist = length(lightVec);
    vec3 L = (lightDist > 1e-4) ? (lightVec / lightDist) : vec3(0.0, 1.0, 0.0);
    vec3 H = normalize(L + V); // Blinn-Phong halfway vector

    // Physically-inspired quadratic distance attenuation
    float attenuation = 1.0 / (1.0 + 0.024 * lightDist + 0.0018 * lightDist * lightDist);

    // Primary Diffuse Reflection (Lambertian)
    float nDotL = max(dot(N, L), 0.0);
    vec4 diffuseTerm = nDotL * DiffuseProduct * attenuation;

    // Primary Specular Reflection (Blinn-Phong)
    float nDotH = max(dot(N, H), 0.0);
    float specCoeff = (nDotL > 0.0) ? pow(nDotH, max(Shininess, 1.0)) : 0.0;
    vec4 specTerm = specCoeff * SpecularProduct * attenuation;

    // -------------------------------------------------------------
    // 2. SECONDARY OMNIDIRECTIONAL FILL LIGHT (Soft Eye-Space Radiance)
    // Prevents surfaces turned away from the overhead luminaire from becoming pitch black
    // -------------------------------------------------------------
    vec3 fillDir = normalize(vec3(0.25, 0.45, 0.85));
    float fillNDotL = max(dot(N, fillDir), 0.0) * 0.22;
    vec4 fillDiffuse = fillNDotL * DiffuseProduct;

    // -------------------------------------------------------------
    // 3. AMBIENT TERM
    // -------------------------------------------------------------
    vec4 ambientTerm = AmbientProduct;

    // -------------------------------------------------------------
    // 4. SCI-FI FRESNEL SILHOUETTE RIM LIGHT
    // Accentuates astronauts, crates, and portal edges against darker background bulkheads
    // -------------------------------------------------------------
    float rimFactor = 1.0 - max(dot(N, V), 0.0);
    rimFactor = pow(clamp(rimFactor, 0.0, 1.0), 2.5);
    vec4 rimTerm = rimFactor * vec4(0.05, 0.12, 0.22, 0.0);

    // Composite local illumination
    vec3 litColor = ambientTerm.rgb + diffuseTerm.rgb + fillDiffuse.rgb + specTerm.rgb + rimTerm.rgb;

    // -------------------------------------------------------------
    // 5. ATMOSPHERIC SPATIAL DEPTH CUEING (Distance Fog)
    // Enhances visual depth separation between near interactive objects and distant walls
    // -------------------------------------------------------------
    float depth = -fPositionEye.z;
    const float fogNear = 12.0;
    const float fogFar = 55.0;
    float fogFactor = clamp((depth - fogNear) / (fogFar - fogNear), 0.0, 0.68);
    vec3 spaceNavyFog = vec3(0.016, 0.022, 0.038); // Deep sci-fi atmosphere

    vec3 finalColor = mix(litColor, spaceNavyFog, fogFactor);

    fColor = vec4(finalColor, 1.0);
}
