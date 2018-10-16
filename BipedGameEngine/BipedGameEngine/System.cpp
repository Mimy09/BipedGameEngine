#include "System.h"
using namespace bpd;

System *System::instance = nullptr;

System::System() {
	time		= new bpd::Time;
	window		= new bpd::Window;
	direct3D	= new bpd::Direct3D;
	scene		= new bpd::Scene;
	input		= new bpd::Input;
	cam			= new bpd::Camera;
	
	frameCount	= 0;
	fps			= 0;
	frameTime	= 0;

	curr_mouseX = 0;
	curr_mouseY = 0;
	prev_mouseX = 0;
	prev_mouseY = 0;
}
System::~System() {}

bool System::Initialize(HINSTANCE hInstance){
	// Initialize win32 window
	if (!window->Initialize(
		"BPD_ENGINE_WINDOW",
		"Biped Game Engine",
		800,
		800,
		hInstance
	)){
		ErrorLogger::Log("Failed to initialize window");
		return false;
	}

	// DirectX 11 setup
	direct3D->fullscreen	= false;
	direct3D->wireframe		= false;
	direct3D->vsync			= false;

	// Initialize DirectX 11
	if(!direct3D->Initialize(
		hInstance,
		window->GetWindowWidth(),
		window->GetWindowHeight(),
		window->GetHWND()
	)){
		ErrorLogger::Log("Failed to initialize DirectX");
		return false;
	}

	// Initialize keyboard and mouse input
	if(!input->Initialize(hInstance,window->GetHWND(),window->GetWindowWidth(),window->GetWindowHeight())) {
		ErrorLogger::Log("Failed to initialize input");
		return false;
	}

	

	// Initialize the scene
	if (!scene->Initialize(direct3D)) {
		ErrorLogger::Log("Failed to initialize scene");
		return false;
	}

	// Camera information
	cam->camPosition = DirectX::XMVectorSet(0.0f,2.0f,-8.0f,0.0f);
	cam->camUp = DirectX::XMVectorSet(0.0f,1.0f,0.0f,0.0f);
	cam->camView = DirectX::XMMatrixLookAtLH(cam->camPosition,cam->camTarget,cam->camUp);
	cam->camProjection = DirectX::XMMatrixPerspectiveFovLH(0.4f*3.14159f,window->GetWindowWidth() / window->GetWindowHeight(),1.0f,1000.0f);

	Model* light = scene->AddModel(Primitives::CONE,direct3D->GetDevice(),direct3D->GetSwapChain());
	light->transform.position = direct3D->GETLight().pos;
	light->transform.rotation = direct3D->GETLight().dir;

	Model* model = scene->AddModel("Project\\Assets\\Rock_9\\Rock_9.obj", direct3D->GetDevice(), direct3D->GetSwapChain());
	model->transform.scail = DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f);

	// Set the system instance for static access
	instance = this;

	// Start the message loop
	MessageLoop();

	// Shutdown and release all the objects
	Shutdown();

	return true;
}

int System::MessageLoop() {
	MSG msg;
	ZeroMemory(&msg,sizeof(MSG));
	while(true) {
		BOOL PeekMessageL(
			LPMSG lpMsg,
			HWND hWnd,
			UINT wMsgFilterMin,
			UINT wMsgFilterMax,
			UINT wRemoveMsg
		);

		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
			if(msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			frameCount++;
			if(time->GetTime() > 1.0f) {
				fps = frameCount;
				frameCount = 0;
				time->StartTimer();
			}
			frameTime = time->GetFrameTime();

			Update(frameTime);
			Render();
		}
	}
	return (int)msg.wParam;
}

void System::Shutdown(){
	SAFE_SHUTDOWN (input);
	SAFE_SHUTDOWN (scene);

	SAFE_DELETE	(window);
	SAFE_DELETE	(time);
	SAFE_DELETE	(cam);

	SAFE_SHUTDOWN (direct3D);
}

float timer = 0;
void System::Update(double deltaTime){
	timer += 0.01f;

	input->Update();

	direct3D->Update(deltaTime);

	scene->GetModel(1)->transform.rotation = DirectX::XMFLOAT3(0, timer, 0);

	scene->Update(deltaTime);
	
	if(input->GetKeyDown(DIK_ESCAPE))
		PostMessage(window->GetHWND(),WM_DESTROY,0,0);

	if(input->GetMouseKey(1)) {
		float speed = 10.0f * deltaTime;

		if(input->GetKey(DIK_LSHIFT)) speed *= 3;
		if(input->GetKey(DIK_A)) cam->moveLeftRight -= speed;
		if(input->GetKey(DIK_D)) cam->moveLeftRight += speed;
		if(input->GetKey(DIK_W)) cam->moveBackForward += speed;
		if(input->GetKey(DIK_S)) cam->moveBackForward -= speed;
		if(input->GetKey(DIK_E)) cam->moveDownUp += speed;
		if(input->GetKey(DIK_Q)) cam->moveDownUp -= speed;
		if(input->GetKey(DIK_SPACE)) cam->moveDownUp += speed;

		input->GetMouseMovement(curr_mouseX,curr_mouseY);
		if((curr_mouseX != prev_mouseX) || (curr_mouseY != prev_mouseY)) {

			cam->camYaw += curr_mouseX * 0.001f;
			cam->camPitch += curr_mouseY * 0.001f;

			prev_mouseX = curr_mouseX;
			prev_mouseY = curr_mouseY;
		}
	}

	cam->Update();
}

void System::Render(){
	float color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

	// Clear the render target and stencil
	direct3D->ClearScreen(color);

	// Render all the models in the scene
	scene->Render(
		direct3D,
		cam
	);

	// pass everything to the GPU
	direct3D->Present();
}

LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam) {
	switch(msg) {
	case WM_SIZE:
		if(System::instance != nullptr && wParam != SIZE_MINIMIZED) {

			// Recreate the render target with the new window size
			System::instance->GetDirect3D()->CleanupRenderTarget();
			System::instance->GetDirect3D()->GetSwapChain()->ResizeBuffers(
				0,
				(UINT)LOWORD(lParam),
				(UINT)HIWORD(lParam),
				DXGI_FORMAT_UNKNOWN,
				0
			);
			System::instance->GetDirect3D()->CreateRenderTarget();

		}
		return 0;
	case  WM_KEYDOWN:
		if(wParam == VK_ESCAPE)
			PostQuitMessage(0);
		break;
	case WM_SYSCOMMAND:
		if((wParam & 0xfff0) == SC_KEYMENU)
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(
		hwnd,
		msg,
		wParam,
		lParam
	);
}