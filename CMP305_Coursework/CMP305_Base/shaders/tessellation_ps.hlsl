// Tessellation pixel shader
// Output colour passed to stage.

Texture2D textures[5] : register(t0);

SamplerState sampler0 : register(s0);


static const float transitionDistance = 5.0f; // transition distance(height)

cbuffer LightBuffer : register(b0)
{
    float4 diffuseColour;
    float4 ambientColour;
    float3 lightDirection;
    float padding;
};

cbuffer TextureHeightBuffer : register(b1)
{
    float maxHeightTex0; // maximum height where the textures[0] will be used (water texture)
    float maxHeightTex1; // maximum height where the textures[1] will be used (sand texture)
    float maxHeightTex2; // maximum height where the textures[2] will be used (grass texture)
    float maxHeightTex3; // maximum height where the textures[3] will be used (concrete texture)
};

struct InputType
{
    float4 position : SV_POSITION; // screen view position
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float4 pos : POSITION0; // position without applying world, view and projection matrices
};

// Calculate lighting intensity based on direction and normal. Combine with light colour.
float4 calcDifusse(const float3 lightVector, const float3 normal, const float4 diffuse)
{
    float intensity = saturate(dot(normal, lightVector));
    float4 colour = saturate(diffuse * intensity);
    return colour;
}

float4 calcLighting(const float3 lightVector, float3 normal, float4 diffuseColour, float4 ambientColour)
{
    // calculate the diffuse
    float4 diffuse = calcDifusse(lightVector, normal, diffuseColour);
    
    // sum of all the lights components
    float4 lightColour = saturate(saturate(ambientColour) + saturate(diffuse));
    
    return lightColour;
}

// blend two textures
float4 blend(const float4 texColour0, const float4 texColour1, const float weight0, const float weight1)
{
    return saturate((texColour0 * weight0) + (texColour1 * weight1));
}

// determinate what texture colour to use depending the height (position - without have been applied to screen view)
float4 calculateTextureColour(const float height, const float2 uv)
{
    float4 textureColour = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Only texture 0
    if (height < maxHeightTex0 - transitionDistance)
    {
        textureColour = textures[0].Sample(sampler0, uv);
    }
    // smooth transition from texture 0 to texture 1
    else if (height <= maxHeightTex0)
    {
        float minGradingHeight1 = maxHeightTex0 - transitionDistance; // height where the transition to texture 1 starts
        float maxGradingHeight1 = maxHeightTex0; // height where the transition to texture 1 ends
        
        // calculate weight of each texture
        float weight1 = smoothstep(minGradingHeight1, maxGradingHeight1, height);
        float weight0 = 1.0f - weight1; // calculate how much weight is for texture 0, being one the max value and knwoing the weight of texture 1
        
        textureColour = blend(textures[0].Sample(sampler0, uv), textures[1].Sample(sampler0, uv), weight0, weight1);
    }
    // only texture 1
    else if (height < maxHeightTex1 - transitionDistance)
    {
        textureColour = textures[1].Sample(sampler0, uv);
    }
    // smooth transition from texture 1 to texture 2
    else if (height <= maxHeightTex1)
    {
        float minGradingHeight2 = maxHeightTex1 - transitionDistance; // height where the transition to texture 2 starts
        float maxGradingHeight2 = maxHeightTex1; // height where the transition to texture 2 ends
        
        // calculate weight of each texture
        float weight2 = smoothstep(minGradingHeight2, maxGradingHeight2, height);
        float weight1 = 1.0f - weight2; // calculate how much weight is for texture 0, being one the max value and knwoing the weight of texture 2
        
        textureColour = blend(textures[1].Sample(sampler0, uv), textures[2].Sample(sampler0, uv), weight1, weight2);
    }
    // only texture 2
    else if (height < maxHeightTex2 - transitionDistance)
    {
        textureColour = textures[2].Sample(sampler0, uv);
    }
    // smooth transition from texture 2 to texture 3
    else if (height <= maxHeightTex2)
    {
        float minGradingHeight3 = maxHeightTex2 - transitionDistance; // height where the transition to texture 3 starts
        float maxGradingHeight3 = maxHeightTex2; // height where the transition to texture 3 ends
        
        // calculate weight of each texture
        float weight3 = smoothstep(minGradingHeight3, maxGradingHeight3, height);
        float weight2 = 1.0f - weight3; // calculate how much weight is for texture 0, being one the max value and knwoing the weight of texture 2
        
        textureColour = blend(textures[2].Sample(sampler0, uv), textures[3].Sample(sampler0, uv), weight2, weight3);
    }
    // only texture 3
    else if (height < maxHeightTex3 - transitionDistance)
    {
        textureColour = textures[3].Sample(sampler0, uv);
    }
    // smooth transition from texture 3 to texture 4
    else if (height <= maxHeightTex3)
    {
        float minGradingHeight4 = maxHeightTex3 - transitionDistance; // height where the transition to texture 4 starts
        float maxGradingHeight4 = maxHeightTex3; // height where the transition to texture 4 ends
        
        // calculate weight of each texture
        float weight4 = smoothstep(minGradingHeight4, maxGradingHeight4, height);
        float weight3 = 1.0f - weight4; // calculate how much weight is for texture 0, being one the max value and knwoing the weight of texture 2
        
        
        textureColour = blend(textures[3].Sample(sampler0, uv), textures[4].Sample(sampler0, uv), weight3, weight4);
    }
    // only texture 4
    else
    {
        textureColour = textures[4].Sample(sampler0, uv);
    }
    
    return textureColour;

}



float4 main(InputType input) : SV_TARGET
{
    float4 textureColour = calculateTextureColour(input.pos.y, input.tex);
    float4 lightColour = calcLighting(-lightDirection, input.normal, diffuseColour, ambientColour);

    return lightColour * textureColour;
}