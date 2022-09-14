#include "Renderer.h"

Renderer::Renderer(RHIDriverRef driver)
	: driver(driver) {}

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

	for (RenderObjectRef& obj : renderQueue) {
		obj->Draw(commandList);
	}

	commandList->EndRenderPass();
	commandList->End();
	driver->Submit(commandList);
	driver->EndFrame();

	renderQueue.clear();
}
