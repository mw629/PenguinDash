#pragma once
#include "Graphics/GraphicsDevice.h"
#include "Math/Calculation.h"
#include <fstream>
#include <sstream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/// オブジェクトの読み込み///

/// <summary>
/// マテリアルテンプレートファイル（.mtl）の読み込み
/// </summary>
/// <param name="directoryPath">ディレクトリパス</param>
/// <param name="filename">ファイル名</param>
/// <returns>読み込んだマテリアルデータ</returns>
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

/// <summary>
/// OBJファイルの簡易読み込み
/// </summary>
/// <param name="directoryPath">ディレクトリパス</param>
/// <param name="filename">ファイル名</param>
/// <returns>読み込んだモデルデータ</returns>
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

/// <summary>
/// Assimpを使用したOBJ/GLTFなどのモデル読み込み
/// </summary>
/// <param name="directoryPath">ディレクトリパス</param>
/// <param name="filename">ファイル名</param>
/// <returns>読み込んだモデルデータ</returns>
ModelData AssimpLoadObjFile(const std::string& directoryPath, const std::string& filename);

/// アニメーションの読み込み ///

/// <summary>
/// AssimpのaiNodeからNode構造体へ再帰的に変換
/// </summary>
/// <param name="node">Assimpノード</param>
/// <returns>変換後のNodeデータ</returns>
Node ReadNode(aiNode* node);

/// <summary>
/// アニメーションファイルの読み込み
/// </summary>
/// <param name="directoryPath">ディレクトリパス</param>
/// <param name="filename">ファイル名</param>
/// <returns>読み込んだアニメーションデータ</returns>
Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);

/// テクスチャの読み込み ///

/// <summary>
/// 画像ファイルからDirectX::ScratchImageとしてテクスチャを読み込み
/// </summary>
/// <param name="filePath">画像ファイルパス</param>
/// <returns>読み込んだテクスチャイメージ</returns>
DirectX::ScratchImage LoadTexture(const std::string& filePath);

/// <summary>
/// テクスチャ用のDirectX 12リソースを生成
/// </summary>
/// <param name="device">DirectX 12デバイス</param>
/// <param name="metadata">テクスチャメタデータ</param>
/// <returns>生成されたテクスチャリソース</returns>
Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

/// <summary>
/// テクスチャデータをGPUリソースへアップロード
/// </summary>
/// <param name="texture">転送先テクスチャリソース</param>
/// <param name="mipImages">ミップマップイメージ群</param>
/// <param name="device">DirectX 12デバイス</param>
/// <param name="commandList">コマンドリスト</param>
/// <returns>中間アップロードバッファリソース</returns>
Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

/// <summary>
/// ディスクリプタヒープのCPUハンドルを取得
/// </summary>
/// <param name="descriptorHeap">対象ディスクリプタヒープ</param>
/// <param name="descriprtorSize">ディスクリプタサイズ</param>
/// <returns>CPUハンドル</returns>
D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriprtorSize);

/// <summary>
/// ディスクリプタヒープのGPUハンドルを取得
/// </summary>
/// <param name="descriptorHeap">対象ディスクリプタヒープ</param>
/// <param name="descriprtorSize">ディスクリプタサイズ</param>
/// <returns>GPUハンドル</returns>
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriprtorSize);

