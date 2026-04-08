#include <cassert>
#include <algorithm>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/math.hpp>
#include "GltfImporter.h"
#include "Math/Vector3.h"
#include "Render/Vertex.h"
#include "Util/Log.h"

static MeshAsset ExtractMesh(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh)
{
    MeshAsset out;

    static uint32_t meshIdx = 0;
	out.Name = mesh.name.empty() ? "Mesh_" + std::to_string(meshIdx++) : std::string(mesh.name);

    for (auto& prim : mesh.primitives)
    {
        // --- Positions ---
        auto* posAttr = prim.findAttribute("POSITION");
        if (posAttr == prim.attributes.end())
            continue;

        auto& posAccessor = asset.accessors[posAttr->accessorIndex];
        uint32_t vertexOffset = static_cast<uint32_t>(out.Vertices.size());
        out.Vertices.resize(vertexOffset + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, posAccessor,
            [&](const fastgltf::math::fvec3& pos, size_t i) {
                auto& v = out.Vertices[vertexOffset + i];
                v.Position = Vector3(pos.x(), pos.y(), pos.z());
                // update AABB
                out.BoundsMin.x = std::min(out.BoundsMin.x, pos.x());
                out.BoundsMin.y = std::min(out.BoundsMin.y, pos.y());
                out.BoundsMin.z = std::min(out.BoundsMin.z, pos.z());
                out.BoundsMax.x = std::max(out.BoundsMax.x, pos.x());
                out.BoundsMax.y = std::max(out.BoundsMax.y, pos.y());
                out.BoundsMax.z = std::max(out.BoundsMax.z, pos.z());
            });

        // --- Normals ---
        if (auto* attr = prim.findAttribute("NORMAL"); attr != prim.attributes.end())
        {
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, asset.accessors[attr->accessorIndex],
                [&](const fastgltf::math::fvec3& n, size_t i) {
                    out.Vertices[vertexOffset + i].Normal = Vector3(n.x(), n.y(), n.z());
                });
        }  

        // --- UVs ---
        if (auto* attr = prim.findAttribute("TEXCOORD_0"); attr != prim.attributes.end())
        {
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, asset.accessors[attr->accessorIndex],
                [&](const fastgltf::math::fvec2& uv, size_t i) {
                    out.Vertices[vertexOffset + i].TexCoord = Vector2(uv.x(), uv.y());
                });
        }
        
        // --- Tangents ---
        if (auto* attr = prim.findAttribute("TANGENT"); attr != prim.attributes.end())
        {
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, asset.accessors[attr->accessorIndex],
                [&](const fastgltf::math::fvec4& t, size_t i) {
                    out.Vertices[vertexOffset + i].Tangent = Vector3(t.x(), t.y(), t.z());
                });
        }
        
        // --- Indices ---
        if (prim.indicesAccessor.has_value())
        {
            auto& indexAccessor = asset.accessors[*prim.indicesAccessor];
            uint32_t indexOffset = static_cast<uint32_t>(out.Indices.size());
            out.Indices.resize(indexOffset + indexAccessor.count);

            // copyFromAccessor writes directly into the destination, handling u8/u16/u32 source types
            // We offset the pointer manually since it writes from index 0
            fastgltf::copyFromAccessor<uint32_t>(asset, indexAccessor, &out.Indices[indexOffset]);

            // Offset indices to account for vertices from previous primitives
            for (size_t i = indexOffset; i < out.Indices.size(); ++i)
                out.Indices[i] += vertexOffset;
        }
        else
        {
            // Non-indexed primitive � generate sequential indices
            uint32_t indexOffset = static_cast<uint32_t>(out.Indices.size());
            uint32_t vertCount = static_cast<uint32_t>(posAccessor.count);
            out.Indices.resize(indexOffset + vertCount);
            for (uint32_t i = 0; i < vertCount; ++i)
                out.Indices[indexOffset + i] = vertexOffset + i;
        }
    }

    return out;
}

RawGltfPackage FastGltfImporter::Import(const std::filesystem::path& path)
{
	auto data = fastgltf::GltfDataBuffer::FromPath(path);
	if (data.error() != fastgltf::Error::None)
	{
		LOG_WARN_TO("asset",  "Failed to read GLTF file : {}", path.string());
		return {};
	}

	fastgltf::Parser parser;
	auto assetResult = parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::LoadExternalBuffers);

	if (assetResult.error() != fastgltf::Error::None)
	{
		LOG_WARN_TO("asset", "Failed to parse GLTF : {}", fastgltf::getErrorMessage(assetResult.error()));
		return {};
	}

    const fastgltf::Asset& asset = assetResult.get();
	RawGltfPackage out;

    for (auto& mesh : asset.meshes)
    {
		out.Meshes.push_back(ExtractMesh(asset, mesh));
    }

    return out;
}
