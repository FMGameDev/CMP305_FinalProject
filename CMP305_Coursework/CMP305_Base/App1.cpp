#include "App1.h"

App1::App1()
{
	m_Terrain = nullptr;
	light = nullptr;
	tessellationShader = nullptr;
}

void App1::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in, bool VSYNC, bool FULL_SCREEN)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in, VSYNC, FULL_SCREEN);

	// Load textures
	// mountain
	textureMgr->loadTexture(L"water", L"res/water.png");
	textureMgr->loadTexture(L"sand", L"res/sand.png");
	textureMgr->loadTexture(L"grass", L"res/grass.png");
	textureMgr->loadTexture(L"concrete", L"res/concrete.png");
	textureMgr->loadTexture(L"snow", L"res/snow.png");

	// for checking texture coords
	textureMgr->loadTexture(L"bunny", L"res/bunny.png");

	// Create Mesh object and shader object
	m_Terrain = new TerrainMesh(renderer->getDevice(), renderer->getDeviceContext(), XMINT2(128, 128));
	tessellationShader = new TessellationShader(renderer->getDevice(), hwnd);
	
	// Initialise light
	light = new Light();
	light->setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	light->setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	light->setDirection(1.0f, -1.0f, 0.0f);

	// get current terrain Resolution
	provisionalResolution[0] = m_Terrain->GetResolution().x;
	provisionalResolution[1] = m_Terrain->GetResolution().y;

	// set the default height offset for diamond-square algorithm
	diamondSquareHeightOffsetRange.min = -30.0f;
	diamondSquareHeightOffsetRange.max = 40.0f;
}


App1::~App1()
{
	// Run base application deconstructor
	BaseApplication::~BaseApplication();

	// Release the Direct3D object.
	if (m_Terrain)
	{
		delete m_Terrain;
		m_Terrain = 0;
	}

	if (light)
	{
		delete light;
		light = 0;
	}

	if (tessellationShader)
	{
		delete tessellationShader;
		tessellationShader = 0;
	}
}


bool App1::frame()
{
	bool result;

	result = BaseApplication::frame();
	if (!result)
	{
		return false;
	}

	// Update the app
	result = update();
	if (!result)
	{
		return false;
	}
	
	// Render the graphics.
	result = render();
	if (!result)
	{
		return false;
	}

	return true;
}

bool App1::update()
{
	// Generate the view matrix based on the camera's position.
	camera->update();

	// get delta time
	float dt = timer->getTime();

	if (renderFrameWaves)
	{
		m_Terrain->BuildSinCosWavesHeightMap(&wavesData, dt);
		m_Terrain->Regenerate(renderer->getDevice(), renderer->getDeviceContext());
	}

	return true;
}

bool App1::render()
{
	XMMATRIX worldMatrix, viewMatrix, projectionMatrix;

	// Get the world, view, projection, and ortho matrices from the camera and Direct3D objects.
	worldMatrix = renderer->getWorldMatrix();
	viewMatrix = camera->getViewMatrix();
	projectionMatrix = renderer->getProjectionMatrix();

	ID3D11ShaderResourceView* textures[5] =
	{
		textureMgr->getTexture(L"water"),
		textureMgr->getTexture(L"sand"),
		textureMgr->getTexture(L"grass"),
		textureMgr->getTexture(L"concrete"),
		textureMgr->getTexture(L"snow")
	};

	// Clear the scene. (default blue colour)
	renderer->beginScene(0.39f, 0.58f, 0.92f, 1.0f);
	{
		// Send geometry data, set shader parameters, render object with shader
		m_Terrain->sendData(renderer->getDeviceContext());
		tessellationShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix,
			textures, maxTextureHeight, fakeWaterLvlToggle, light, dMinMax, lvlOfDetail, camera);
		tessellationShader->render(renderer->getDeviceContext(), m_Terrain->getIndexCount());

		// Render GUI
		gui();
	}
	// Swap the buffers
	renderer->endScene();

	return true;
}

void App1::gui()
{
	// Force turn off unnecessary shader stages.
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->HSSetShader(NULL, NULL, 0);
	renderer->getDeviceContext()->DSSetShader(NULL, NULL, 0);

	// Build UI //
	// FPS
	ImGui::Text("FPS: %.2f", timer->getFPS());

	// Camera Position
	XMFLOAT3 cameraPos = camera->getPosition();
	ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);

	// Title for terrain general settings
	ImGui::Text("\nTerrain General Settings:");
	// Wireframe mode
	ImGui::Checkbox("Wireframe mode", &wireframeToggle);
	// Fake Water Level toggle
	ImGui::Checkbox("Fake Water Lvl mode", &fakeWaterLvlToggle);

	//////////////////////////////  RESOLUTION ////////////////////////////////////////////////////////////////
	if (ImGui::CollapsingHeader("Terrain resolution"))
	{
		ImGui::Text("The resolution will be the number of quads used horizontally\nand vertically in the plane.");
		ImGui::Text("The number of vertices will be (resolution + 1)");
		ImGui::Text("Using tessellation will increase the number of vertices in gpu.");
		ImGui::SliderInt2("Resolution (m*n)", provisionalResolution, 2, 1025);
		if (ImGui::Button("Apply resolution") && (provisionalResolution[0] != m_Terrain->GetResolution().x || provisionalResolution[1] != m_Terrain->GetResolution().y))
		{
			m_Terrain->Resize(renderer->getDevice(), renderer->getDeviceContext(), XMINT2(provisionalResolution[0], provisionalResolution[1]));
		}
		ImGui::Text("\n");
	}

	//////////////////////////////  TESSELLATION ////////////////////////////////////////////////////////////////
	if (ImGui::CollapsingHeader("Tessellation Factor"))
	{
		ImGui::SliderFloat2("Min-Max distance", dMinMax, 0.0f, 100.0f, "%.0f"); //  "%.0f" to fake integer use 
		ImGui::SliderFloat2("Lvl of detail", lvlOfDetail, 0.0f, 64.0f, "%.0f");
		ImGui::Text("\n");
	}

	//////////////////////////////  TEXTURE ////////////////////////////////////////////////////////////////
	if (ImGui::CollapsingHeader("Textures: Max Height"))
	{
		ImGui::SliderFloat("Water Texture", &maxTextureHeight[0], -30, maxTextureHeight[1] - 1); // never higher than the sand
		ImGui::SliderFloat("Sand Texture", &maxTextureHeight[1], maxTextureHeight[0] + 1, maxTextureHeight[2] - 1); // never lower than the water or higher than the grass
		ImGui::SliderFloat("Grass Texture", &maxTextureHeight[2], maxTextureHeight[1] + 1, maxTextureHeight[3] - 1); // never lower than the sand or higher than the concrete
		ImGui::SliderFloat("Concrete Texture", &maxTextureHeight[3], maxTextureHeight[2] + 1, 40); // never lower than the grass
		ImGui::Text("\n");
	}

	////////////////////// PROCEDURAL METHOD FUNCTIONS TO REBUILD HEIGHT MAP COMPLETELY ///////////////////////////////////////////////////////////////////
	ImGui::Text("\n\nProcedural methods to modify height map from scratch:\n");


	//////////////////////////////  WAVES //////////////////////////////////////////////////////////////////////
	if (ImGui::CollapsingHeader("Sin and Cos Waves"))
	{
		ImGui::Checkbox("Render moving waves", &renderFrameWaves);
		ImGui::Checkbox("Apply a deltatime offset to the waves in x", &(wavesData.moveWaves)[0]);
		ImGui::Checkbox("Apply a deltatime offset to the waves in z", &(wavesData.moveWaves)[2]);

		// display info
		float frequency[2] = { wavesData.frequency.x,  wavesData.frequency.z };
		ImGui::SliderFloat2("Frequency (x,z)", frequency, 0.033f, 1.2f);
		float amplitude[2] = { wavesData.amplitude.x,  wavesData.amplitude.z };
		ImGui::SliderFloat2("Amplitude (x,z)", amplitude, 0.0f, 20.0f);

		// check if the wave data has been modified
		if (frequency[0] != wavesData.frequency.x || frequency[1] != wavesData.frequency.z ||
			amplitude[0] != wavesData.amplitude.x || amplitude[1] != wavesData.amplitude.z)
		{
			wavesData.frequency.x = frequency[0];
			wavesData.frequency.z = frequency[1];
			wavesData.amplitude.x = amplitude[0];
			wavesData.amplitude.z = amplitude[1];
		}

		// Button to apply waves
		if (ImGui::Button("Create Waves"))
		{
			wavesData.offset = XMFLOAT3(0.0f, 0.0f, 0.0f);// reset offset
			m_Terrain->BuildSinCosWavesHeightMap(&wavesData);
			m_Terrain->Regenerate(renderer->getDevice(), renderer->getDeviceContext());
		}
		ImGui::Text("\n");
	}

	//////////////////////////////  RANDOM HEIGHTS //////////////////////////////////////////////////////////////////////
	if (ImGui::CollapsingHeader("Random Height"))
	{
		// Set Height Offset Range
		float newRandomeHeight[2] = { randomMinMaxHeight.min, randomMinMaxHeight.max };
		ImGui::SliderFloat2("Min - Max Random Height", newRandomeHeight, -40.0f, 40.0f);
		if (newRandomeHeight[0] != randomMinMaxHeight.min || newRandomeHeight[1] != randomMinMaxHeight.max)
		{
			randomMinMaxHeight.min = newRandomeHeight[0];
			randomMinMaxHeight.max = newRandomeHeight[1];
		}

		// Apply Random Height
		if (ImGui::Button("Apply Random Heights")) {
			// build map height
			m_Terrain->BuildRandomHeightMap(randomMinMaxHeight);
			m_Terrain->Regenerate(renderer->getDevice(), renderer->getDeviceContext());
		}
		ImGui::Text("\n");
	}

	//////////////////////////////  DIAMOND-SQUARE ALGORITHM //////////////////////////////////////////////////////////////////////
	if (ImGui::CollapsingHeader("Diamond-Square Algorithm"))
	{
		ImGui::Text("Resolution needed: (2^n)\nFor example: 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024.");
		ImGui::Text("Number of vertices needed: (2^n)+1\nFor example: 3, 5, 9, 17, 33, 65, 129, 257, 513, 1025.");
		ImGui::Text("The plane m*n needs to be a quads where m=n.");

		// Set Height Offset Range
		float newOffsetRange[2] = { diamondSquareHeightOffsetRange.min, diamondSquareHeightOffsetRange.max };
		ImGui::SliderFloat2("Diamond-Square offset Height", newOffsetRange, -40.0f, 40.0f);
		if (newOffsetRange[0] != diamondSquareHeightOffsetRange.min || newOffsetRange[1] != diamondSquareHeightOffsetRange.max)
		{
			diamondSquareHeightOffsetRange.min = newOffsetRange[0];
			diamondSquareHeightOffsetRange.max = newOffsetRange[1];
		}

		// Apply Diamond-Square algorithm
		if (ImGui::Button("Apply Diamond-Square Algorithm")) {
			m_Terrain->DiamondSquareAlgorithm(diamondSquareHeightOffsetRange);
			m_Terrain->Regenerate(renderer->getDevice(), renderer->getDeviceContext());
		}
		ImGui::Text("\n");
	}

	//////////////////////////////  FLATTEN THE PLANE //////////////////////////////////////////////////////////////////////
	if (ImGui::CollapsingHeader("Flatten"))
	{
		ImGui::Text("Set to 0 the height of every vertex.");
		if (ImGui::Button("Apply Flatten")) {
			m_Terrain->Flatten();
			m_Terrain->Regenerate(renderer->getDevice(), renderer->getDeviceContext());
		}
		ImGui::Text("\n");
	}


	////////////////////// PROCEDURAL METHOD FUNCTIONS TO ALTER THE CURRENT HEIGHT MAP ///////////////////////////////////////////////////////////////////

	// Regenerate completely the height map 
	ImGui::Text("\n\nProcedural methods to adjust the current height map:\n");

	// Smooth
	if (ImGui::CollapsingHeader("Smooth"))
	{
		ImGui::Text("Smooth all the terrain.");
		if (ImGui::Button("Apply Smooth"))
		{
			m_Terrain->Smooth();
			m_Terrain->Regenerate(renderer->getDevice(), renderer->getDeviceContext());
		}
		ImGui::Text("\n");
	}
	// Fault
	if (ImGui::CollapsingHeader("Fault"))
	{
		// Set Height Offset Range
		float newOffsetRange[2] = { faultHeightRange.min, faultHeightRange.max };
		ImGui::SliderFloat2("Fault height offset", newOffsetRange, -40.0f, 40.0f);
		if (newOffsetRange[0] != faultHeightRange.min || newOffsetRange[1] != faultHeightRange.max)
		{
			faultHeightRange.min = newOffsetRange[0];
			faultHeightRange.max = newOffsetRange[1];
		}

		if (ImGui::Button("Apply Fault"))
		{
			m_Terrain->Fault(faultHeightRange);
			m_Terrain->Regenerate(renderer->getDevice(), renderer->getDeviceContext());
		}
		ImGui::Text("\n");
	}

	// Particle Deposition
	if (ImGui::CollapsingHeader("Particle & Anti-Particle Deposition"))
	{
		// Set Height Offset Range
		float newOffsetRange[2] = { particleDepoHeightRange.min, particleDepoHeightRange.max };
		ImGui::SliderFloat2("Particle height range", newOffsetRange, -40.0f, 40.0f);
		if (newOffsetRange[0] != particleDepoHeightRange.min || newOffsetRange[1] != particleDepoHeightRange.max)
		{
			particleDepoHeightRange.min = newOffsetRange[0];
			particleDepoHeightRange.max = newOffsetRange[1];
		}

		if (ImGui::Button("Apply Particle Deposition"))
		{
			m_Terrain->ParticleDeposition(particleDepoHeightRange);
			m_Terrain->Regenerate(renderer->getDevice(), renderer->getDeviceContext());
		}
		// Anti-Particle Deposition
		if (ImGui::Button("Apply Anti-Particle Deposition"))
		{
			m_Terrain->AntiParticleDeposition(particleDepoHeightRange);
			m_Terrain->Regenerate(renderer->getDevice(), renderer->getDeviceContext());
		}
		ImGui::Text("\n");
	}

	ImGui::Text("\nExample with diamond square and smooth:");
	if (ImGui::Button("Make Example Terrain"))
	{
		// Initialise parameters for this example
		// make the plane square and possible to perform the diamond/square algorithm
		provisionalResolution[0] = 128;
		provisionalResolution[1] = 128;
		// set the height offset
		diamondSquareHeightOffsetRange.min = -30.0f;
		diamondSquareHeightOffsetRange.max = 40.0f;
		// faking the water level (levelling up the water to height = 0)
		fakeWaterLvlToggle = true;
		// set default texture max height
		maxTextureHeight[0] = 0.0f;
		maxTextureHeight[1] = 5.0f;
		maxTextureHeight[2] = 15.0f;
		maxTextureHeight[3] = 25.0f;

		// apply procedural methods
		m_Terrain->Resize(renderer->getDevice(), renderer->getDeviceContext(), XMINT2(provisionalResolution[0], provisionalResolution[1]));
		m_Terrain->DiamondSquareAlgorithm(diamondSquareHeightOffsetRange);
		m_Terrain->Smooth();
		m_Terrain->Regenerate(renderer->getDevice(), renderer->getDeviceContext());
	}
	ImGui::Text("\n");

	ImGui::Text("\nPress WASDQE to move the camera along axis \nUse the arrow keys for turning the camera.");

	// Render UI
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

