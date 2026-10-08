#include "TextureLoader.h"
#include "Core/LogHandler.h"

#ifdef _USE_IMGUI
#include <imgui.h>
#endif // _USE_IMGUI

bool TextureLoader::CheckFilePath(const std::string& filePath)
{
	return textureMap_.find(filePath) != textureMap_.end();
}

bool TextureLoader::StockTextureData(const std::string& filePath,
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU, D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU,
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource, Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource)
{
	Texture tex;
	tex.index = static_cast<int>(textureKeys_.size());//0を基準
	tex.filePath = filePath;
	tex.textureSrvHandleCPU = textureSrvHandleCPU;
	tex.textureSrvHandleGPU = textureSrvHandleGPU;
	tex.resource = textureResource;
	tex.intermediateResource = intermediateResource;

	textureMap_[filePath] = tex;
	textureKeys_.push_back(filePath);

	return true;
}

void TextureLoader::Draw() {
#ifdef _USE_IMGUI
	if (ImGui::BeginTable("TextureTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
		ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed, 50.0f);
		ImGui::TableSetupColumn("File Path", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		for (const auto& key : textureKeys_) {
			const auto& tex = textureMap_[key];
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%d", tex.index);
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%s", tex.filePath.c_str());
		}
		ImGui::EndTable();
	}
#endif // _USE_IMGUI
}

int TextureLoader::GetTextureIndex(const std::string& filePath)
{
	auto it = textureMap_.find(filePath);
	if (it != textureMap_.end()) {
		return it->second.index;
	}
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureLoader::GetTexture(const std::string& filePath)
{
	auto it = textureMap_.find(filePath);
	if (it != textureMap_.end()) {
		return it->second.textureSrvHandleGPU;
	}
	LOG_WARN(std::format("Texture '{}' not found in TextureLoader. Falling back to default texture.", filePath));
	if (!textureKeys_.empty()) {
		return textureMap_[textureKeys_[0]].textureSrvHandleGPU;
	}
	return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureLoader::GetTexture(int index)
{
	if (index >= 0 && index < static_cast<int>(textureKeys_.size())) {
		return textureMap_[textureKeys_[index]].textureSrvHandleGPU;
	}
	LOG_WARN(std::format("Texture index {} out of range (count={}). Falling back to default texture.", index, textureKeys_.size()));
	if (!textureKeys_.empty()) {
		return textureMap_[textureKeys_[0]].textureSrvHandleGPU;
	}
	return D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };
}

void TextureLoader::ReleaseIntermediateResources()
{
	for (auto& [path, tex] : textureMap_) {
		if (tex.intermediateResource) {
			tex.intermediateResource.Reset();
		}
	}
}




