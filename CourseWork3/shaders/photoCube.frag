#version 330 core
out vec4 FragColor;

in vec3 Normal;  
in vec3 FragPos;  
in vec2 TexCoords;
  
uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;

uniform sampler2D firstTexture; 
uniform sampler2D secondTexture;

uniform float alpha; // coefficient of interpolation

void main()
{
    vec3 textureColor = TexCoords.x < alpha 
        ? texture(firstTexture, TexCoords).rgb 
        : texture(secondTexture, TexCoords).rgb;
    
    // ambient
    float ambientStrength = 1;
    vec3 ambient = ambientStrength * textureColor;
  	
    
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * textureColor;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
    specular = vec3(0.0,0.0,0.0);
        
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
} 