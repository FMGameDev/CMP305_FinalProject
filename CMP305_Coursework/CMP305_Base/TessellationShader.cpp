#include "TessellationShader.h"

TessellationShader::TessellationShader(ID3D11Device* device, HWND hwnd) : BaseShader(device, hwnd)
{
	initShader(L"tessellation_vs.cso", L"tessellation_hs.cso", L"tessellation_ds.cso", L"tessellation_ps.cso");
}


TessellationShader::~TessellationShader()
{
	if (sampleState)
	{
		sampleState->Release();
		sampleState = 0;
	}
	if (matrixBuffer)
	{
		matrixBuffer->Release();
		matrixBuffer = 0;
	}
	if (tessellationFactorBuffer)
	{
		tessellationFactorBuffer->Release();
		tessellationFactorBuffer = 0;
	}
	if (cameraBuffer)
	{
		cameraBuffer->Release();
		cameraBuffer = 0;
	}
	if (layout)
	{
		layout->Release();
		layout = 0;
	}
	// Release the light constant buffer.
	if (lightBuffer)
	{
		lightBuffer->Release();
		lightBuffer = 0;
	}


	//Release base shader components
	BaseShader::~BaseShader();
}

void TessellationShader::initShader(const wchar_t* vsFilename, const wchar_t* psFilename)
{
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_BUFFER_DESC tessellationFactorBufferDesc;
	D3D11_BUFFER_DESC cameraBufferDesc;
	D3D11_BUFFER_DESC fakeWaterLvlBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC lightBufferDesc;
	D3D11_BUFFER_DESC textureHeightBufferDesc;

	// Load (+ compile) shader files
	loadVertexShader(vsFilename);
	loadPixelShader(psFilename);

	// Setup the description of the dynamic matrix constant buffer that is in the vertex, hull, domain shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);

	// Setup the description of the dynamic tessellation factor buffer that is in the hull shader
	tessellationFactorBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	tessellationFactorBufferDesc.ByteWidth = sizeof(TessellationFactorBufferType);
	tessellationFactorBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	tessellationFactorBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	tessellationFactorBufferDesc.MiscFlags = 0;
	tessellationFactorBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&tessellationFactorBufferDesc, NULL, &tessellationFactorBuffer);

	// Setup the description of the dynamic camera buffer that is in the hull shader
	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof(CameraBufferType);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraBufferDesc.MiscFlags = 0;
	cameraBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&cameraBufferDesc, NULL, &cameraBuffer);

	// Setup the description of the dynamic fake water level buffer that is in the domain shader
	fakeWaterLvlBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	fakeWaterLvlBufferDesc.ByteWidth = sizeof(FakeWaterLvlBufferType);
	fakeWaterLvlBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	fakeWaterLvlBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	fakeWaterLvlBufferDesc.MiscFlags = 0;
	fakeWaterLvlBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&fakeWaterLvlBufferDesc, NULL, &fakeWaterLvlBuffer);

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	renderer->CreateSamplerState(&samplerDesc, &sampleState);

	// Setup light buffer
	// Setup the description of the light dynamic constant buffer that is in the pixel shader.
	// Note that ByteWidth always needs to be a multiple of 16 if using D3D11_BIND_CONSTANT_BUFFER or CreateBuffer will fail.
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightBufferType);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&lightBufferDesc, NULL, &lightBuffer);

	// Set up texture height buffer
	// Set uo the description of the max texture height for each texture dynamic contstant buffer that is in the pixel shader
	textureHeightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	textureHeightBufferDesc.ByteWidth = sizeof(TextureHeightBufferType);
	textureHeightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	textureHeightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	textureHeightBufferDesc.MiscFlags = 0;
	textureHeightBufferDesc.StructureByteStride = 0;
	renderer->CreateBuffer(&textureHeightBufferDesc, NULL, &textureHeightBuffer);
}

void TessellationShader::initShader(const wchar_t* vsFilename, const wchar_t* hsFilename, const wchar_t* dsFilename, const wchar_t* psFilename)
{
	// InitShader must be overwritten and it will load both vertex and pixel shaders + setup buffers
	initShader(vsFilename, psFilename);

	// Load other required shaders.
	loadHullShader(hsFilename);
	loadDomainShader(dsFilename);
}


void TessellationShader::setShaderParameters(ID3D11DeviceContext* deviceContext, const XMMATRIX& worldMatrix, const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix,
	ID3D11ShaderResourceView* textures[5], std::vector<float> maxTextureHeight, bool fakeWaterLevel,
	Light* light,
	float* dMinMax, float* lvlOfDetail, Camera* camera)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Transpose the matrices to prepare them for the shader.
	XMMATRIX tworld = XMMatrixTranspose(worldMatrix);
	XMMATRIX tview = XMMatrixTranspose(viewMatrix);
	XMMATRIX tproj = XMMatrixTranspose(projectionMatrix);

	// send tessellation factor buffer to Dull Shader
	result = deviceContext->Map(tessellationFactorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	TessellationFactorBufferType* tessellationFactorPtr = (TessellationFactorBufferType*)mappedResource.pData;
	tessellationFactorPtr->dMaxMin = XMFLOAT2(dMinMax[0], dMinMax[1]);
	tessellationFactorPtr->lvlOfDetail = XMFLOAT2(lvlOfDetail[0], lvlOfDetail[1]);
	deviceContext->Unmap(tessellationFactorBuffer, 0);
	deviceContext->HSSetConstantBuffers(0, 1, &tessellationFactorBuffer);

	// send camera buffer to Dull Shader
	result = deviceContext->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	CameraBufferType* cameraPtr = (CameraBufferType*)mappedResource.pData;
	cameraPtr->cameraPos = camera->getPosition();
	cameraPtr->padding2 = 0.0f;
	deviceContext->Unmap(cameraBuffer, 0);
	deviceContext->HSSetConstantBuffers(1, 1, &cameraBuffer);

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	MatrixBufferType* dataPtr = (MatrixBufferType*)mappedResource.pData;
	dataPtr->world = tworld;// worldMatrix;
	dataPtr->view = tview;
	dataPtr->projection = tproj;
	deviceContext->Unmap(matrixBuffer, 0);
	deviceContext->HSSetConstantBuffers(2, 1, &matrixBuffer);
	deviceContext->DSSetConstantBuffers(0, 1, &matrixBuffer);

	// send if the user want to fake the water level
	result = deviceContext->Map(fakeWaterLvlBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	FakeWaterLvlBufferType* fakeWaterLvlPtr = (FakeWaterLvlBufferType*)mappedResource.pData;
	fakeWaterLvlPtr->fakeWaterLvl = (float)fakeWaterLevel;
	fakeWaterLvlPtr->padding = XMFLOAT3(0.0f, 0.0f, 0.0f);
	deviceContext->Unmap(fakeWaterLvlBuffer, 0);
	deviceContext->DSSetConstantBuffers(1, 1, &fakeWaterLvlBuffer);

	// Send light data to pixel shader
	LightBufferType* lightPtr;
	deviceContext->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	lightPtr = (LightBufferType*)mappedResource.pData;
	lightPtr->diffuse = light->getDiffuseColour();
	lightPtr->ambient = light->getAmbientColour();
	lightPtr->direction = light->getDirection();
	lightPtr->padding = 0.0f;
	deviceContext->Unmap(lightBuffer, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &lightBuffer);

	// Send max height per texture data to pixel shader
	TextureHeightBufferType* texHeightPtr;
	deviceContext->Map(textureHeightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	texHeightPtr = (TextureHeightBufferType*)mappedResource.pData;
	texHeightPtr->maxHeightTex0 = maxTextureHeight[0];
	texHeightPtr->maxHeightTex1 = maxTextureHeight[1];
	texHeightPtr->maxHeightTex2 = maxTextureHeight[2];
	texHeightPtr->maxHeightTex3 = maxTextureHeight[3];
	deviceContext->Unmap(textureHeightBuffer, 0);
	deviceContext->PSSetConstantBuffers(1, 1, &textureHeightBuffer);

	// Set shader texture resource in the pixel shader.
	deviceContext->PSSetShaderResources(0, 5, textures);
	deviceContext->PSSetSamplers(0, 1, &sampleState);
}
