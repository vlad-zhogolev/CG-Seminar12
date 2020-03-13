#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
layout (location = 3) out vec3 gMaterialAmbient;
layout (location = 4) out vec3 gMaterialDiffuse;
layout (location = 5) out vec4 gMaterialSpecular;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;

    // store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);

    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;

    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;

	// store material properties in textures
	gMaterialAmbient = ambient;
	gMaterialDiffuse = diffuse;
	gMaterialSpecular.rgb = specular;
	gMaterialSpecular.a = shininess;
}