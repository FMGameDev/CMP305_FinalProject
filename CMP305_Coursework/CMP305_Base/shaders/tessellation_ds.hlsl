// Tessellation domain shader
// After tessellation the domain shader processes the all the vertices

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer waterBuffer : register(b1)
{
    float fakeWaterLevel;
    float3 padding;
};

struct ConstantOutputType
{
    float edges[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

struct InputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct OutputType
{
    float4 position : SV_POSITION; // screen view position
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float4 pos : POSITION0; // position without applying world, view and projection matrices
};

[domain("quad")]
OutputType main(ConstantOutputType input, float2 uvCoord : SV_DomainLocation, const OutputPatch<InputType, 4> patch)
{
    float3 vertexPosition, normalVal;
    float2 texVal;
    OutputType output;
 
    // Determine the position of the new vertex (Bilinear Interpolation)
	// Deal with y coords
    // Linearly interpolate(lerp) between 0 and 1
    float3 v1 = lerp(patch[0].position, patch[1].position, uvCoord.y); 
    // Linearly interpolate(lerp) between 3 and 2
    float3 v2 = lerp(patch[3].position, patch[2].position, uvCoord.y);
    // The final vertex position will be the lerp result between v1, v2 based on our x-coords
    vertexPosition = lerp(v1, v2, uvCoord.x); 
    
    float2 t1 = lerp(patch[0].tex, patch[1].tex, uvCoord.y); 
    // Linearly interpolate(lerp) between 3 and 2
    float2 t2 = lerp(patch[3].tex, patch[2].tex, uvCoord.y);
    // The final vertex position will be the lerp result between v1, v2 based on our x-coords
    texVal = lerp(t1, t2, uvCoord.x);
    
    float3 n1 = lerp(patch[0].normal, patch[1].normal, uvCoord.y);
    // Linearly interpolate(lerp) between 3 and 2
    float3 n2 = lerp(patch[3].normal, patch[2].normal, uvCoord.y);
    // The final vertex position will be the lerp result between v1, v2 based on our x-coords
    normalVal= lerp(n1, n2, uvCoord.x);
    
    // ouput the position without applying world, view and projection matrices
    output.pos = float4(vertexPosition, 1.0f);
    
    // fake the water level by setting the height to 0 where it is lower
    if (fakeWaterLevel == 1.0f && vertexPosition.y < 0.0f)
    {
        vertexPosition.y = 0.0f;
        normalVal = float3(0.0f, 1.0f, 0.0f);
    }
		    
    // Calculate the position of the new vertex against the world, view, and projection matrices. 
    output.position = mul(float4(vertexPosition, 1.0f), worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // Send the input color into the pixel shader.
    output.normal = mul(normalVal, (float3x3) worldMatrix);
    output.normal = normalize(output.normal);
    
    // Send texture
    output.tex = texVal;

    return output;
}
