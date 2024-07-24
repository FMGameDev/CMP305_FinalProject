// Tessellation Shader Plane.h
// Basic single light shader setup
#pragma once

#include "DXF.h"

using namespace std;
using namespace DirectX;


class TessellationShader : public BaseShader
{

private:
	struct FakeWaterLvlBufferType
	{
		float fakeWaterLvl;
		XMFLOAT3 padding;
	};

	struct LightBufferType
	{
		XMFLOAT4 diffuse;
		XMFLOAT4 ambient;
		XMFLOAT3 direction;
		float padding;
	};
	struct TextureHeightBufferType
	{
		float maxHeightTex0; // maximum height where the textures[0] will be used (water texture)
		float maxHeightTex1; // maximum height where the textures[1] will be used (sand texture)
		float maxHeightTex2; // maximum height where the textures[2] will be used (grass texture)
		float maxHeightTex3; // maximum height where the textures[3] will be used (concrete texture)
	};
	struct TessellationFactorBufferType
	{
		XMFLOAT2 dMaxMin;
		XMFLOAT2 lvlOfDetail;
	};
	struct CameraBufferType
	{
		XMFLOAT3 cameraPos;
		float padding2;
	};

public:

	TessellationShader(ID3D11Device* device, HWND hwnd);
	~TessellationShader();

	void setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection,
		ID3D11ShaderResourceView* textures[5], std::vector<float> maxTextureHeight, bool fakeWaterLevel,
		Light* light,
		float* dMinMax, float* lvlOfDetail, Camera* camera);

private:
	void initShader(const wchar_t* vsFilename, const wchar_t* psFilename);
	void initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* psFilename);

private:
	ID3D11Buffer* matrixBuffer;
	ID3D11Buffer* tessellationFactorBuffer;
	ID3D11Buffer* cameraBuffer;
	ID3D11Buffer* lightBuffer;
	ID3D11Buffer* textureHeightBuffer;
	ID3D11Buffer* fakeWaterLvlBuffer;
};