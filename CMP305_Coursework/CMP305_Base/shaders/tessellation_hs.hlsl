// Tessellation Hull Shader
// Prepares control points for tessellation

cbuffer TessellationFactorBuffer : register(b0)
{
    float2 dMinMax; // dMinMax.x is dMin, dMinMax.y is dMax
    float2 lvlOfDetail; // lvlOfDetail.x is min lvl of detail and lvlOfDetail.y is the maximum
};

cbuffer CameraBuffer : register(b1)
{
    float3 cameraPos;
    float padding2;
};

cbuffer MatrixBuffer : register(b2)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

struct InputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct ConstantOutputType
{
    float edges[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

struct OutputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

// Find the center of patch in woold space
float CalculateDistanceFromCamera(float3 pos1, float3 pos2, float3 pos3, float3 pos4)
{
    float3 centerL, centerW;
    float3 d;
    
    // find the center of patch
    centerL = 0.25f * (pos1 + pos2 + pos3 + pos4);
    // calculate the center of patch against the world
    centerW = mul(float4(centerL, 1.0f), worldMatrix).xyz;
     
    // calculate the distance from the camera pos to the center of the quad
    d = distance(centerW, cameraPos);
    
    return d;
}

// Calculate tessellation Factor based on distance from eye camera
// The tessellation is 0 if d >= d1 and 64 if d <= d0.
// [d0, d1] defines the range we tessellate in.
float CalculateTessFactor(float3 d) // d is distance from camera
{
    const float dMin = dMinMax.x;
    const float dMax = dMinMax.y;
    const float loDMin = lvlOfDetail.x;
    const float loDMax = lvlOfDetail.y;
    
    // calculate the tessellation
    // formula: a+t*(b?a) -> lerp (interpolation t range [0,1])
    float t = saturate((dMax - d) / (dMax - dMin));
    float tess = loDMin + (t * (loDMax - loDMin));

    return tess;
}

ConstantOutputType PatchConstantFunction(InputPatch<InputType, 12> inputPatch, uint patchId : SV_PrimitiveID)
{
    ConstantOutputType output;
    
    // Calculate the distance from the center of the quad to the camera position
    // distance from the current quad (centre) to the camera
    float d = CalculateDistanceFromCamera(inputPatch[0].position, inputPatch[1].position, inputPatch[2].position, inputPatch[3].position);
    
    // distance from the left neighbour quad to the camera
    float dLeft = CalculateDistanceFromCamera(inputPatch[8].position, inputPatch[9].position, inputPatch[1].position, inputPatch[0].position);
    // distance from the top neighbour quad to the camera
    float dTop = CalculateDistanceFromCamera(inputPatch[6].position, inputPatch[0].position, inputPatch[3].position, inputPatch[7].position);
    // distance from the right neighbour quad to the camera
    float dRight = CalculateDistanceFromCamera(inputPatch[3].position, inputPatch[2].position, inputPatch[4].position, inputPatch[5].position);
    // distance from the bottom neighbour quad to the camera
    float dBottom = CalculateDistanceFromCamera(inputPatch[1].position, inputPatch[10].position, inputPatch[11].position, inputPatch[2].position);
    

    // calculate the tessellation factor for the center of the current quad, based on the distance
    float tessFactorCenter = CalculateTessFactor(d);
    
    // Set the tessellation factor for tessallating inside the quad.
    output.inside[0] = tessFactorCenter; // u-axis (columns)
    output.inside[1] = tessFactorCenter; // v-axis (rows)

    // Set the tessellation factors for the four edges of the quad.
    output.edges[0] = min(tessFactorCenter, CalculateTessFactor(dLeft)); // left edge
    output.edges[1] = min(tessFactorCenter, CalculateTessFactor(dTop)); // top edge
    output.edges[2] = min(tessFactorCenter, CalculateTessFactor(dRight)); // right edge
    output.edges[3] = min(tessFactorCenter, CalculateTessFactor(dBottom)); // bottom edge

    return output;
}


[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(12)]
[patchconstantfunc("PatchConstantFunction")]
[maxtessfactor(64.0f)]
OutputType main(InputPatch<InputType, 12> patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    OutputType output;

    // Set the position, texture coord and normal for this control point as the output
    output.position = patch[pointId].position;
    output.tex = patch[pointId].tex;
    output.normal = patch[pointId].normal;

    return output;
}