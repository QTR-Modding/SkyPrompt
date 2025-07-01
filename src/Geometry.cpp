#include "Geometry.h"
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

namespace {
    UINT GetBufferLength(RE::ID3D11Buffer* reBuffer) {
        const auto buffer = reinterpret_cast<ID3D11Buffer*>(reBuffer);
        D3D11_BUFFER_DESC bufferDesc = {};
        buffer->GetDesc(&bufferDesc);
        return bufferDesc.ByteWidth;
    }


    void EachGeometry(const RE::TESObjectREFR* obj, const std::function<void(RE::BSGeometry* o3d, RE::BSGraphics::TriShape*)>& callback) {
        if (const auto d3d = obj->Get3D()) {

            RE::BSVisit::TraverseScenegraphGeometries(d3d, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {

                const auto& model = a_geometry->GetGeometryRuntimeData();

                if (const auto triShape = model.rendererData) {

                    callback(a_geometry, triShape);
                }

                return RE::BSVisit::BSVisitControl::kContinue;
            });

        } 
    }
}

void Geometry::FetchVertexes(const RE::BSGeometry* o3d, RE::BSGraphics::TriShape* triShape) {
    if (const uint8_t* vertexData = triShape->rawVertexData) {
        const uint32_t stride = triShape->vertexDesc.GetSize();
        const auto numPoints = GetBufferLength(triShape->vertexBuffer);
        const auto numPositions = numPoints / stride;
        positions.reserve(positions.size() + numPositions);
        for (uint32_t i = 0; i < numPoints; i += stride) {
            const uint8_t* currentVertex = vertexData + i;

            const float* position =
                reinterpret_cast<const float*>(currentVertex + triShape->vertexDesc.GetAttributeOffset(
                                                                   RE::BSGraphics::Vertex::Attribute::VA_POSITION));

            auto pos = RE::NiPoint3{position[0], position[1], position[2]};
            pos = o3d->local * pos;
            positions.push_back(pos);
        }
    }
}
void Geometry::FetchIndexes(const RE::BSGraphics::TriShape* triShape) {
    const auto numIndexes = GetBufferLength(triShape->indexBuffer) / sizeof(uint16_t);

    const auto offset = static_cast<uint16_t>(indexes.size()); 
    indexes.reserve(indexes.size() + numIndexes);

    for (auto i = 0; i < numIndexes; i++) {
        const uint16_t* currentIndex = triShape->rawIndexData + i;
        indexes.push_back(currentIndex[0] + offset);
    }
}

RE::NiPoint3 Geometry::Rotate(const RE::NiPoint3& A, const RE::NiPoint3& angles) {
    RE::NiMatrix3 R;
    R.SetEulerAnglesXYZ(angles);
    return R * A;
}

Geometry::~Geometry() = default;

Geometry::Geometry(RE::TESObjectREFR* obj) {

        this->obj = obj;
    EachGeometry(obj, [this](const RE::BSGeometry* o3d, RE::BSGraphics::TriShape* triShape) -> void {
        FetchVertexes(o3d, triShape);
        //FetchIndexes(triShape);
    });

    if (positions.size() == 0) {
        auto from = obj->GetBoundMin();
        auto to = obj->GetBoundMax();

        if ((to - from).Length() < 1) {
            from = {-5, -5, -5};
            to = {5, 5, 5};
        }
        positions.push_back(RE::NiPoint3(from.x, from.y, from.z));
        positions.push_back(RE::NiPoint3(to.x, from.y, from.z));
        positions.push_back(RE::NiPoint3(to.x, to.y, from.z));
        positions.push_back(RE::NiPoint3(from.x, to.y, from.z));

        positions.push_back(RE::NiPoint3(from.x, from.y, to.z));
        positions.push_back(RE::NiPoint3(to.x, from.y, to.z));
        positions.push_back(RE::NiPoint3(to.x, to.y, to.z));
        positions.push_back(RE::NiPoint3(from.x, to.y, to.z));
    }

}

std::pair<RE::NiPoint3, RE::NiPoint3> Geometry::GetBoundingBox(const RE::NiPoint3 angle, const float scale) const {
    auto min = RE::NiPoint3{0, 0, 0};
    auto max = RE::NiPoint3{0, 0, 0};

    for (auto i = 0; i < positions.size(); i++) {
        const auto p1 = Rotate(positions[i] * scale, angle);

        if (p1.x < min.x) {
            min.x = p1.x;
        }
        if (p1.x > max.x) {
            max.x = p1.x;
        }
        if (p1.y < min.y) {
            min.y = p1.y;
        }
        if (p1.y > max.y) {
            max.y = p1.y;
        }
        if (p1.z < min.z) {
            min.z = p1.z;
        }
        if (p1.z > max.z) {
            max.z = p1.z;
        }
    }

    return std::pair(min, max);

}
