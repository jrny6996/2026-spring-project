#version 100

precision highp float;

#define MAX_LIGHTS              32
#define LIGHT_DIRECTIONAL       0
#define LIGHT_POINT             1
#define LIGHT_SPOT              2
#define PI 3.14159265358979323846

// Lower these for a darker scene (all in roughly 0–1 range).
const float PBR_EXPOSURE = 0.38;
const float PBR_AMBIENT_SCALE = 0.12;
const float PBR_DIRECT_LIGHT_SCALE = 0.58;
const float PBR_POST_GAMMA_DARKEN = 0.92;

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
    float intensity;
    // Spotlight: cos of half-angle at outer edge (smaller) and inner cone (larger).
    float spotOuterCos;
    float spotInnerCos;
};

// Input vertex attributes (from vertex shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec3 fragNormal;
varying mat3 TBN;

// Input uniform values
uniform int numOfLights;
uniform sampler2D albedoMap;
uniform sampler2D mraMap;
uniform sampler2D normalMap;
uniform sampler2D emissiveMap; // r: Hight g:emissive

uniform vec2 tiling;
uniform vec2 offset;

uniform int useTexAlbedo;
uniform int useTexNormal;
uniform int useTexMRA;
uniform int useTexEmissive;

uniform vec4  albedoColor;
uniform vec4  emissiveColor;
uniform float normalValue;
uniform float metallicValue;
uniform float roughnessValue;
uniform float aoValue;
uniform float emissivePower;

// Input lighting values
uniform Light lights[MAX_LIGHTS];
uniform vec3 viewPos;

uniform vec3 ambientColor;
uniform float ambient;

// Reflectivity in range 0.0 to 1.0
// NOTE: Reflectivity is increased when surface view at larger angle
vec3 SchlickFresnel(float hDotV,vec3 refl)
{
    return refl + (1.0 - refl)*pow(1.0 - hDotV, 5.0);
}

float GgxDistribution(float nDotH,float roughness)
{
    float a = roughness*roughness*roughness*roughness;
    float d = nDotH*nDotH*(a - 1.0) + 1.0;
    d = PI*d*d;
    return (a/max(d,0.0000001));
}

float GeomSmith(float nDotV,float nDotL,float roughness)
{
    float r = roughness + 1.0;
    float k = r*r/8.0;
    float ik = 1.0 - k;
    float ggx1 = nDotV/(nDotV*ik + k);
    float ggx2 = nDotL/(nDotL*ik + k);
    return ggx1*ggx2;
}

vec3 ComputePBR()
{
    vec3 albedo = texture2D(albedoMap, vec2(fragTexCoord.x*tiling.x + offset.x, fragTexCoord.y*tiling.y + offset.y)).rgb;
    albedo = vec3(albedoColor.x*albedo.x, albedoColor.y*albedo.y, albedoColor.z*albedo.z);

    float metallic = clamp(metallicValue, 0.0, 1.0);
    float roughness = clamp(roughnessValue, 0.0, 1.0);
    float ao = clamp(aoValue, 0.0, 1.0);

    if (useTexMRA == 1)
    {
        vec2 mraUv = vec2(fragTexCoord.x*tiling.x + offset.x, fragTexCoord.y*tiling.y + offset.y);
        vec4 mra = texture2D(mraMap, mraUv);
        metallic = clamp(mra.b + metallicValue, 0.04, 1.0);
        roughness = clamp(mra.g + roughnessValue, 0.04, 1.0);
        ao = clamp(aoValue, 0.0, 1.0);
    }

    vec3 N = normalize(fragNormal);
    if (useTexNormal == 1)
    {
        N = texture2D(normalMap, vec2(fragTexCoord.x*tiling.x + offset.x, fragTexCoord.y*tiling.y + offset.y)).rgb;
        N = normalize(N*2.0 - 1.0);
        N = normalize(N*TBN);
    }

    vec3 V = normalize(viewPos - fragPosition);

    vec3 emissive = vec3(0);
    emissive = (texture2D(emissiveMap, vec2(fragTexCoord.x*tiling.x + offset.x, fragTexCoord.y*tiling.y + offset.y)).rgb).g*emissiveColor.rgb*emissivePower*float(useTexEmissive);

    // return N;//vec3(metallic,metallic,metallic);
    // If  dia-electric use base reflectivity of 0.04 otherwise ut is a metal use albedo as base reflectivity
    vec3 baseRefl = mix(vec3(0.04), albedo.rgb, metallic);
    vec3 lightAccum = vec3(0.0);  // Acumulate lighting lum

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (i >= numOfLights) continue;
        vec3 L = normalize(lights[i].position - fragPosition);      // Compute light vector
        vec3 H = normalize(V + L);                                  // Compute halfway bisecting vector
        float dist = length(lights[i].position - fragPosition);     // Compute distance to light
        float attenuation = 1.0/(dist*dist*0.23);                   // Compute attenuation
        float spotFactor = 1.0;
        if (lights[i].type == LIGHT_SPOT)
        {
            vec3 spotDir = normalize(lights[i].target - lights[i].position);
            vec3 toFrag = normalize(fragPosition - lights[i].position);
            float c = dot(toFrag, spotDir);
            spotFactor = smoothstep(lights[i].spotOuterCos, lights[i].spotInnerCos, c);
        }
        vec3 radiance = lights[i].color.rgb*lights[i].intensity*attenuation*spotFactor;

        // Cook-Torrance BRDF distribution function
        float nDotV = max(dot(N,V), 0.0000001);
        float nDotL = max(dot(N,L), 0.0000001);
        float hDotV = max(dot(H,V), 0.0);
        float nDotH = max(dot(N,H), 0.0);
        float D = GgxDistribution(nDotH, roughness);    // Larger the more micro-facets aligned to H
        float G = GeomSmith(nDotV, nDotL, roughness);   // Smaller the more micro-facets shadow
        vec3 F = SchlickFresnel(hDotV, baseRefl);       // Fresnel proportion of specular reflectance

        vec3 spec = (D*G*F)/(4.0*nDotV*nDotL);

        // Difuse and spec light can't be above 1.0
        // kD = 1.0 - kS  diffuse component is equal 1.0 - spec comonent
        vec3 kD = vec3(1.0) - F;

        // Mult kD by the inverse of metallnes, only non-metals should have diffuse light
        kD *= 1.0 - metallic;
        lightAccum += ((kD*albedo.rgb/PI + spec)*radiance*nDotL*PBR_DIRECT_LIGHT_SCALE)*float(lights[i].enabled);
    }

    vec3 ambientFinal = (ambientColor + albedo)*ambient*PBR_AMBIENT_SCALE;

    return (ambientFinal + lightAccum*ao + emissive);
}

void main()
{
    vec3 color = ComputePBR();

    const float exposure = 1.0;
    color *= exposure;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    gl_FragColor = vec4(color, 1.0);
}
