/*
Copyright(c) 2016-2023 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES =================================
#include "Component.h"
#include <vector>
#include "../../Math/Matrix.h"
#include "../../Math/BoundingBox.h"
#include "../Rendering/Renderer_Definitions.h"
//============================================

namespace Spartan
{
    class Mesh;
    class Material;
    class RHI_VertexBuffer;

    class SP_CLASS Renderable : public Component
    {
    public:
        Renderable(std::weak_ptr<Entity> entity);
        ~Renderable();

        // IComponent
        void Serialize(FileStream* stream) override;
        void Deserialize(FileStream* stream) override;

        // geometry/mesh
        void SetGeometry(
            Mesh* mesh,
            const Math::BoundingBox aabb = Math::BoundingBox::Undefined,
            uint32_t index_offset  = 0, uint32_t index_count  = 0,
            uint32_t vertex_offset = 0, uint32_t vertex_count = 0
        );
        void SetGeometry(const Renderer_MeshType mesh_type);
        void GetGeometry(std::vector<uint32_t>* indices, std::vector<RHI_Vertex_PosTexNorTan>* vertices) const;

        // properties
        uint32_t GetIndexOffset()                 const { return m_geometry_index_offset; }
        uint32_t GetIndexCount()                  const { return m_geometry_index_count; }
        uint32_t GetVertexOffset()                const { return m_geometry_vertex_offset; }
        uint32_t GetVertexCount()                 const { return m_geometry_vertex_count; }
        Mesh* GetMesh()                           const { return m_mesh; }
        const Math::BoundingBox& GetBoundingBox() const { return m_bounding_box_local; }
        const Math::BoundingBox& GetAabb();

        //= MATERIAL ====================================================================
        // Sets a material from memory (adds it to the resource cache by default)
        std::shared_ptr<Material> SetMaterial(const std::shared_ptr<Material>& material);

        // Loads a material and the sets it
        std::shared_ptr<Material> SetMaterial(const std::string& file_path);

        void SetDefaultMaterial();
        std::string GetMaterialName() const;
        Material* GetMaterial()       const { return m_material; }
        auto HasMaterial()            const { return m_material != nullptr; }
        //===============================================================================

        // shadows
        void SetCastShadows(const bool cast_shadows) { m_cast_shadows = cast_shadows; }
        auto GetCastShadows() const                  { return m_cast_shadows; }

        // instancing
        bool HasInstancing()                  const { return !m_instances.empty(); }
        RHI_VertexBuffer* GetInstanceBuffer() const { return m_instance_buffer.get(); }
        uint32_t GetInstanceCount()           const { return static_cast<uint32_t>(m_instances.size()); }
        void SetInstances(const std::vector<Math::Matrix>& instances);

    private:
        // geometry/mesh
        uint32_t m_geometry_index_offset  = 0;
        uint32_t m_geometry_index_count   = 0;
        uint32_t m_geometry_vertex_offset = 0;
        uint32_t m_geometry_vertex_count  = 0;
        Mesh* m_mesh                      = nullptr;
        bool m_bounding_box_dirty         = true;
        Math::BoundingBox m_bounding_box_local;
        Math::BoundingBox m_bounding_box;

        // material
        bool m_material_default = false;
        Material* m_material    = nullptr;

        // instancing
        std::vector<Math::Matrix> m_instances;
        std::shared_ptr<RHI_VertexBuffer> m_instance_buffer;

        // misc
        Math::Matrix m_last_transform = Math::Matrix::Identity;
        bool m_cast_shadows           = true;
    };
}
