#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

struct SpotLight
{
    vec3 position;
    vec3 direction;
    vec3 color;

    float constant;
    float linear;
    float quadratic;

    float cutOff;  //cosine actually
    float outerCutOff;
};

uniform sampler2D diffuseTexture;
uniform samplerCube depthMap;

uniform SpotLight spotLight;
uniform vec3 viewPos;

uniform float far_plane;
uniform bool shadows;

float ShadowCalculation(vec3 fragPos)
{
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - spotLight.position;
    // ise the fragment to light vector to sample from the depth map    
    float closestDepth = texture(depthMap, fragToLight).r;
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    closestDepth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // test for shadows
    float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;        
    // display closestDepth as debug (to visualize depth cubemap)
    // FragColor = vec4(vec3(closestDepth / far_plane), 1.0);    
        
    return shadow;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos)
{
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    normal = normalize(normal);
    
    // ambient
    vec3 ambient = 0.3 * color;
    
    // diffuse
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * light.color;
    
    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * light.color;    
    
    // calculate shadow
    float shadow = shadows ? ShadowCalculation(fragPos) : 0.0;    

    // intensity
    float angle = dot(-lightDir, normalize(light.direction));   
    float intensity = clamp((angle - light.outerCutOff) / 
                            (light.cutOff - light.outerCutOff), 
                            0.0, 1.0);  

    vec3 lighting = intensity * (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

    return lighting;
}

void main()
{        
    vec3 lighting = CalcSpotLight(spotLight, fs_in.Normal, fs_in.FragPos);
    FragColor = vec4(lighting, 1.0);
}