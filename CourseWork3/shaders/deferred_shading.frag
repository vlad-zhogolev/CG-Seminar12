#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gMaterialAmbient;
uniform sampler2D gMaterialDiffuse;
uniform sampler2D gMaterialSpecular;

struct DirLight {
	vec3 direction;
	vec3 color;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 color;   

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
    float linear;
    float quadratic;    
};

struct SpotLight {
	vec3 position;
	vec3 direction;
	vec3 color;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;

	float cutOff;  //cosine actually
	float outerCutOff;
};

struct Material{		
	vec3 color;
	float specCoef;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

const int MAX_POINT_LIGHTS_NUMBER = 32;
const int MAX_SPOT_LIGHTS_NUMBER  = 32;
const int MAX_DIR_LIGHTS_NUMBER   = 4;

uniform int pointLightsNumber;
uniform PointLight pointLights[MAX_POINT_LIGHTS_NUMBER];

uniform int spotLightsNumber;
uniform SpotLight spotLights[MAX_SPOT_LIGHTS_NUMBER];

uniform int dirLightsNumber;
uniform DirLight dirLights[MAX_DIR_LIGHTS_NUMBER];

uniform vec3 viewPos;

// Function prototypes
vec3 CalcDirLight(DirLight light, vec3 normal, Material material, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 fragPos, vec3 normal, Material material, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 fragPos, vec3 normal, Material material, vec3 viewDir);

void main()
{          	 
    // retrieve data from gbuffer 
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
	Material material; 
    material.color = texture(gAlbedoSpec, TexCoords).rgb;
    material.specCoef = texture(gAlbedoSpec, TexCoords).a;
	material.ambient = texture(gMaterialAmbient, TexCoords).rgb;
	material.diffuse = texture(gMaterialDiffuse, TexCoords).rgb;
	material.specular = texture(gMaterialSpecular, TexCoords).rgb;
	material.shininess = texture(gMaterialSpecular, TexCoords).a;
    
	vec3 viewDir  = normalize(viewPos - fragPos);

    // calculate global ambient component
    vec3 lighting  = material.color * material.ambient;
    
	// calculate directional lights impact
	for(int i = 0; i < dirLightsNumber; ++i)
		lighting += CalcDirLight(dirLights[i], normal, material, viewDir);

	// calculate point lights impact
	for(int i = 0; i < pointLightsNumber; ++i)
	{
		float distance = length(pointLights[i].position - fragPos);		
		lighting += CalcPointLight(pointLights[i], normal, fragPos, material, viewDir);
	}

	// calculate spot lights impact
	for(int i = 0; i < spotLightsNumber; ++i)
		lighting += CalcSpotLight(spotLights[i], normal, fragPos, material, viewDir);

    FragColor = vec4(lighting, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, Material material, vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);	
    
	// ambient shading
	vec3 ambient = light.color * light.ambient * material.color * material.ambient;

	// diffuse shading
	float diffFactor = max(dot(normal, lightDir), 0.0);
    
	// specular shading  	 
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float specFactor = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    
	// combine results    
	vec3 diffuse  = light.color * light.diffuse * material.color  * material.diffuse * diffFactor;
	vec3 specular = light.color * light.specular * material.specCoef * material.specular * specFactor;
	return ambient + diffuse + specular;		
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, Material material, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos);
	
	// ambient shading
	vec3 ambient = light.color * light.ambient * material.color * material.ambient;

	// diffuse shading
	float diffFactor = max(dot(normal, lightDir), 0.0);
	
	// specular shading
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float specFactor = pow(max(dot(normal, halfwayDir), 0.0), 
					material.shininess);
	
	// attenuation
	float distance	  = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance +
							   light.quadratic * distance * distance);  
	// combined shading
	vec3 diffuse  = light.color * light.diffuse  *
			material.color  * material.diffuse  * diffFactor;
	vec3 specular = light.color * light.specular *
			material.specCoef * material.specular * specFactor;
	return attenuation * (ambient + diffuse + specular);
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, Material material, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos);
	
	// ambient shading
	vec3 ambient = light.color * light.ambient * material.color * material.ambient;

	// diffuse shading
	float diffFactor = max(dot(normal, lightDir), 0.0);
	
	// specular shading
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float specFactor = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
	
	// attenuaton
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance +
							   light.quadratic * distance * distance);  
	// intensity
	float angle = dot(lightDir, normalize(-light.direction));	
	float intensity = clamp((angle - light.outerCutOff) / 
							(light.cutOff - light.outerCutOff), 
							0.0, 1.0);	
	
	// combined shading	
	vec3 diffuse  = light.color * light.diffuse  * 
				material.color * material.diffuse  * 
				diffFactor;
	vec3 specular = light.color * light.specular *
				material.specCoef * material.specular * 
				specFactor;
	return intensity * attenuation * (ambient + diffuse + specular);
}