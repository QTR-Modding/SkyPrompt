#pragma once

class Geometry {
    std::vector<RE::NiPoint3> positions;
    std::vector<uint16_t> indexes;
    RE::TESObjectREFR* obj;

    void FetchVertexes(const RE::BSGeometry* o3d, RE::BSGraphics::TriShape* triShape);
    void FetchIndexes(const RE::BSGraphics::TriShape* triShape);

    static RE::NiPoint3 Rotate(const RE::NiPoint3& A, const RE::NiPoint3& angles);

public:
    ~Geometry();
    Geometry(RE::TESObjectREFR* obj);
    std::pair<RE::NiPoint3, RE::NiPoint3> GetBoundingBox(RE::NiPoint3 angle, float scale) const;
};