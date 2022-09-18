#include "Renderer.h"

Renderer::Renderer(RHIDriverRef driver)
	: driver(driver)
{
    gui = std::make_shared<ImGuiRenderer>(driver);
}

void Renderer::Enqueue(RenderObjectRef obj)
{
	renderQueue.push_back(obj);
}

void Renderer::Render()
{
	RHICommandListRef commandList = driver->CreateCommandList();
	RHIRenderTargetRef renderTarget = driver->BeginFrame();
	commandList->Begin();
	commandList->BeginRenderPass(renderTarget);
	commandList->SetViewport(200,  200, 800, 600);
	commandList->SetScissors(200,  200, 800, 800);

	for (RenderObjectRef& obj : renderQueue) {
		obj->Draw(commandList);
	}

	gui->Render(commandList);
	commandList->EndRenderPass();
	commandList->End();
	driver->Submit(commandList);
	driver->EndFrame();

	renderQueue.clear();
}
