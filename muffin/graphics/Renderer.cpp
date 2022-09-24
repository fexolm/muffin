#include "Renderer.h"

Renderer::Renderer(RHIDriverRef driver)
	: driver(driver)
{
}

void Renderer::Enqueue(RenderableRef obj)
{
	renderQueue.push_back(obj);
}

void Renderer::Render()
{
	RHICommandListRef commandList = driver->CreateCommandList();
	RHIRenderTargetRef renderTarget = driver->BeginFrame();
	commandList->Begin();
	commandList->BeginRenderPass(renderTarget);

	for (RenderableRef& obj : renderQueue) {
		commandList->SetViewport(200,  200, 800, 600);
		commandList->SetScissors(200,  200, 800, 600);
		obj->Render(commandList);
	}

	commandList->EndRenderPass();
	commandList->End();
	driver->Submit(commandList);
	driver->EndFrame();

	renderQueue.clear();
}
