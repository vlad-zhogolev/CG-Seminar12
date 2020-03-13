#version 330 core

struct PointLight 
{
    vec3 position;
    vec3 color;  
    bool isOn; 

    float constant;
    float linear;
    float quadratic;    
};

struct DirLight 
{
    vec3 direction;
    vec3 color;
    bool isOn;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    vec3 color;
    bool isOn;

    float constant;
    float linear;
    float quadratic;

    float cutOff;  //cosine actually
    float outerCutOff;
};

struct Material 
{        
    vec3 albedo;
    vec3 normal;    
    float metallic;
    float roughness;
};

// output color
out vec4 FragColor;

const float PI                      = 3.14159265359;
const int   MAX_DIR_LIGHTS_NUMBER   = 4;
const int   MAX_POINT_LIGHTS_NUMBER = 32;
const int   MAX_SPOT_LIGHTS_NUMBER  = 32;

// input data
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// material maps
uniform sampler2D texture_albedo1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_metallic1;
uniform sampler2D texture_roughness1;
uniform float opacityRatio;
uniform float refractionRatio;

uniform vec3 cameraPos;

uniform samplerCube skybox;

uniform DirLight sun;
uniform int dirLightsNumber;
uniform DirLight dirLights[MAX_DIR_LIGHTS_NUMBER];
uniform int pointLightsNumber;
uniform PointLight pointLights[MAX_POINT_LIGHTS_NUMBER];
uniform int spotLightsNumber;
uniform SpotLight spotLights[MAX_SPOT_LIGHTS_NUMBER];

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(texture_normal1, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   =  normalize(Normal);
    vec3 T   =  normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN =  mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(vec3 N, vec3 directionToView, vec3 L, float roughness)
{
    float NdotV = max(dot(N, directionToView), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 calcPointLight(PointLight light, Material material, vec3 fragmentPositon, vec3 directionToView, vec3 F0)
{  
    vec3 directionToLight = normalize(light.position - fragmentPositon);
    vec3 halfway = normalize(directionToView + directionToLight);    
    
    // Cook-Torrance BRDF
    float D = distributionGGX(material.normal, halfway, material.roughness);   
    float G = geometrySmith(material.normal, directionToView, directionToLight, material.roughness);      
    vec3  F = fresnelSchlick(max(dot(halfway, directionToView), 0.0), F0);
      
    vec3 nominator    = D * G * F; 
    float NdotV = max(dot(material.normal, directionToView), 0.0);
    float NdotL = max(dot(material.normal, directionToLight), 0.0);
    float denominator = 4 * NdotV * NdotL + 0.001; // 0.001  for preventing division by zero.
    vec3 specular = nominator / denominator;
       
    // kS is equal to Fresnel 
    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - material.metallic;     

    float distance = length(light.position - fragmentPositon);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    // scale light by NdotL add to outgoing radiance Lo 
    return (kD * material.albedo / PI + specular) *  light.color * attenuation * NdotL;
}

vec3 calcDirLight(DirLight light, Material material, vec3 directionToView, vec3 F0)
{   
    vec3 halfway = normalize(directionToView - light.direction);
    
    // Cook-Torrance BRDF
    float D = distributionGGX(material.normal, halfway, material.roughness);   
    float G = geometrySmith(material.normal, directionToView, -light.direction, material.roughness);      
    vec3  F = fresnelSchlick(max(dot(halfway, directionToView), 0.0), F0);
      
    vec3 nominator    = D * G * F; 
    float NdotV = max(dot(material.normal, directionToView), 0.0);
    float NdotL = max(dot(material.normal, -light.direction), 0.0);
    float denominator = 4 * NdotV * NdotL + 0.001; // 0.001 for preventing division by zero.
    vec3 specular = nominator / denominator;
      
    // kS is equal to Fresnel     
    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - material.metallic;     

    // scale light by NdotL add to outgoing radiance Lo 
    return (kD * material.albedo / PI + specular) * light.color * NdotL;      
}

vec3 calcSpotLight(SpotLight light, Material material, vec3 fragmentPositon, vec3 directionToView, vec3 F0)
{
    vec3 directionToLight = normalize(light.position - fragmentPositon);
    vec3 halfway = normalize(directionToView + directionToLight);
       
    // Cook-Torrance BRDF
    float D = distributionGGX(material.normal, halfway, material.roughness);   
    float G = geometrySmith(material.normal, directionToView, directionToLight, material.roughness);      
    vec3  F = fresnelSchlick(max(dot(halfway, directionToView), 0.0), F0);
      
    vec3 nominator    = D * G * F; 
    float NdotV = max(dot(material.normal, directionToView), 0.0);
    float NdotL = max(dot(material.normal, directionToLight), 0.0);
    float denominator = 4 * NdotV * NdotL + 0.001; // 0.001 for preventing division by zero.
    vec3 specular = nominator / denominator;
        
    // kS is equal to Fresnel
    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - material.metallic;     

    // attenuation
    float distance = length(light.position - fragmentPositon);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    // intensity
    float angle = dot(directionToLight, normalize(-light.direction));   
    float intensity = clamp((angle - light.outerCutOff) / 
                            (light.cutOff - light.outerCutOff), 
                            0.0, 1.0);

    // scale light by NdotL add to outgoing radiance Lo 
    return (kD * material.albedo / PI + specular) * intensity * light.color * NdotL;
}

void main()
{		
    Material material;
    material.albedo    = pow(texture(texture_albedo1, TexCoords).rgb, vec3(2.2));
    material.metallic  = texture(texture_metallic1, TexCoords).r;
    material.roughness = texture(texture_roughness1, TexCoords).r;
    material.normal    = getNormalFromMap();

    vec3 directionToView = normalize(cameraPos - WorldPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, material.albedo, material.metallic);

    // reflectance equation
    vec3 Lo = calcDirLight(sun, material, directionToView, F0);
    for(int i = 0; i < pointLightsNumber; ++i)
    {
        if (pointLights[i].isOn)
        {
            Lo += calcPointLight(pointLights[i], material, WorldPos, directionToView, F0);   
        }
    }
    for(int i = 0; i < dirLightsNumber; ++i)
    {
        if (dirLights[i].isOn)
        {
            Lo += calcDirLight(dirLights[i], material, directionToView, F0);
        }
    }
    for(int i = 0; i < spotLightsNumber; ++i)
    {
        if (spotLights[i].isOn)
        {
            Lo += calcSpotLight(spotLights[i], material, WorldPos, directionToView, F0);
        }
    }

    vec3 ambient = vec3(0.03) * material.albedo;
    vec3 color = ambient + Lo;

    vec3 reflected = reflect(-directionToView, material.normal);
    vec3 reflectedColor = texture(skybox, reflected).xyz;
    color += reflectedColor * material.metallic;

    vec3 refracted = refract(-directionToView, material.normal, 1.0 / refractionRatio);
    vec3 refractedColor = texture(skybox, refracted).xyz;
    //color = mix(refractedColor, color, opacityRatio);
    color += refractedColor * (1.0 - opacityRatio);

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}