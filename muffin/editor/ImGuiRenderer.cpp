#include "ImGuiRenderer.h"

static std::vector<uint32_t> readShader(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	return buffer;
}

static uint32_t ConvertRGBAToBGRA(uint32_t color)
{
	uint8_t* buf = (uint8_t*)&color;

	std::swap(buf[0], buf[2]);

	return color;
}

ImGuiRenderer::ImGuiRenderer(RHIDriverRef driver)
	: driver(driver)
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();

	RHIShaderRef vert = driver->CreateShader(readShader("imgui-vert.spv"), ShaderType::Vertex);
	RHIShaderRef frag = driver->CreateShader(readShader("imgui-frag.spv"), ShaderType::Fragment);

	GraphicsPipelineCreateInfo pipelineInfo;
	pipelineInfo.fragmentShader = frag;
	pipelineInfo.vertexShader = vert;
	pipelineInfo.depthStencil.depthTestEnable = false;
	pipelineInfo.rasterizer.cullMode = CullMode::None;
	pipelineInfo.rasterizer.faceOrientation = FaceOrientation::CounterClockwise;

	pipeline = driver->CreateGraphicsPipeline(pipelineInfo);

	fontSampler = driver->CreateSampler();

	unsigned char* fontData;
	int32_t textureWidth;
	int32_t textureHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &textureWidth, &textureHeight);

	fontTexture = driver->CreateTexture(textureWidth, textureHeight);

	BufferInfo bufferInfo;
	bufferInfo.usage = BufferUsage::Staging;
	RHIBufferRef textureBuffer = driver->CreateBuffer(4 * textureWidth * textureHeight * sizeof(char), bufferInfo);

	textureBuffer->Write(fontData, 4 * textureWidth * textureHeight * sizeof(char));

	driver->CopyBufferToTexture(textureBuffer, fontTexture, textureWidth, textureHeight);
}

void ImGuiRenderer::Render(RHICommandListRef commandList)
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawData* drawData = ImGui::GetDrawData();

	uint32_t posBufferSize = drawData->TotalVtxCount * sizeof(ImVec2);
	uint32_t uvBufferSize = drawData->TotalVtxCount * sizeof(ImVec2);
	uint32_t colBufferSize = drawData->TotalVtxCount * sizeof(ImU32);
	uint32_t indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

	std::vector<ImVec2> pos;
	std::vector<ImVec2> uv;
	std::vector<ImU32> col;
	std::vector<ImDrawIdx> index;

	for (int32_t cmdListIndex = 0; cmdListIndex < drawData->CmdListsCount; ++cmdListIndex) {
		const ImDrawList* cmdList = drawData->CmdLists[cmdListIndex];

		for (int i = 0; i < cmdList->VtxBuffer.Size; i++) {
			pos.push_back(cmdList->VtxBuffer.Data[i].pos);
			uv.push_back(cmdList->VtxBuffer.Data[i].uv);
			col.push_back(ConvertRGBAToBGRA(cmdList->VtxBuffer.Data[i].col));
		}
		for (int i = 0; i < cmdList->IdxBuffer.Size; i++) {
			index.push_back(cmdList->IdxBuffer.Data[i]);
		}
	}

	if (drawData->TotalVtxCount == 0) {
		return;
	}

	RHIBufferRef posBuffer = driver->CreateBuffer(posBufferSize, BufferInfo{ .usage = BufferUsage::Vertex });
	posBuffer->Write(pos.data(), posBufferSize);

	RHIBufferRef uvBuffer = driver->CreateBuffer(uvBufferSize, BufferInfo{ .usage = BufferUsage::Vertex });
	uvBuffer->Write(uv.data(), uvBufferSize);

	RHIBufferRef colBuffer = driver->CreateBuffer(colBufferSize, BufferInfo{ .usage = BufferUsage::Vertex });
	colBuffer->Write(col.data(), colBufferSize);

	RHIBufferRef indexBuffer = driver->CreateBuffer(indexBufferSize, BufferInfo{ .usage = BufferUsage::Index });
	indexBuffer->Write(index.data(), indexBufferSize);

	struct UBO
	{
		glm::vec2 scale;
		glm::vec2 translate;
	};

	RHIBufferRef uboBuffer = driver->CreateBuffer(sizeof(UBO), BufferInfo{ .usage = BufferUsage::Uniform });

	UBO ubo;
	ubo.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
	ubo.translate = glm::vec2(-1.f);

	uboBuffer->Write(&ubo, sizeof(ubo));

	commandList->BindPipeline(pipeline);
	commandList->SetViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	commandList->BindVertexBuffer(posBuffer, 2);
	commandList->BindVertexBuffer(uvBuffer, 0);
	commandList->BindVertexBuffer(colBuffer, 1);
	commandList->BindIndexBuffer(indexBuffer);

	commandList->BindTexture("fontSampler", fontTexture, fontSampler);
	commandList->BindUniformBuffer("ubo", uboBuffer, sizeof(UBO));

	int32_t globalIndexOffset = 0;
	int32_t globalVertexOffset = 0;

	for (int cmdListIndex = 0; cmdListIndex < drawData->CmdListsCount; cmdListIndex++) {
		ImDrawList* cmdList = drawData->CmdLists[cmdListIndex];
		for (int32_t cmdBufferIndex = 0; cmdBufferIndex < cmdList->CmdBuffer.Size; ++cmdBufferIndex) {
			ImDrawCmd* cmdBuffer = &cmdList->CmdBuffer[cmdBufferIndex];
			uint32_t firstIndex = cmdBuffer->IdxOffset + globalIndexOffset;
			int32_t vertexOffset = cmdBuffer->VtxOffset + globalVertexOffset;

			commandList->SetScissors(cmdBuffer->ClipRect.x, cmdBuffer->ClipRect.y, cmdBuffer->ClipRect.z - cmdBuffer->ClipRect.x, cmdBuffer->ClipRect.w - cmdBuffer->ClipRect.y);
			commandList->DrawIndexed(cmdBuffer->ElemCount, 1, firstIndex, vertexOffset, 0);
		}
		globalIndexOffset += cmdList->IdxBuffer.Size;
		globalVertexOffset += cmdList->VtxBuffer.Size;
	}
}