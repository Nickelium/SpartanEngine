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

//= INCLUDES ====================================
#include "Properties.h"
#include "Window.h"
#include "../ImGui/ImGuiExtension.h"
#include "../ImGui/Source/imgui_stdlib.h"
#include "../ImGui/Source/imgui_internal.h"
#include "../WidgetsDeferred/ButtonColorPicker.h"
#include "Core/Engine.h"
#include "World/Entity.h"
#include "World/Components/Transform.h"
#include "World/Components/Renderable.h"
#include "World/Components/PhysicsBody.h"
#include "World/Components/Constraint.h"
#include "World/Components/Light.h"
#include "World/Components/AudioSource.h"
#include "World/Components/AudioListener.h"
#include "World/Components/Environment.h"
#include "World/Components/Terrain.h"
#include "World/Components/ReflectionProbe.h"
#include "Rendering/Mesh.h"
//===============================================

//= NAMESPACES =========
using namespace std;
using namespace Spartan;
using namespace Math;
//======================

weak_ptr<Entity> Properties::m_inspected_entity;
weak_ptr<Material> Properties::m_inspected_material;

namespace
{
    #define column_pos_x 180.0f * Spartan::Window::GetDpiScale()
    #define item_width   120.0f * Spartan::Window::GetDpiScale()

    static string context_menu_id;
    static shared_ptr<Component> copied_component = nullptr;

    static void component_context_menu_options(const string& id, shared_ptr<Component> component, const bool removable)
    {
        if (ImGui::BeginPopup(id.c_str()))
        {
            if (removable)
            {
                if (ImGui::MenuItem("Remove"))
                {
                    if (auto entity = Properties::m_inspected_entity.lock())
                    {
                        if (component)
                        {
                            entity->RemoveComponentById(component->GetObjectId());
                        }
                    }
                }
            }

            if (ImGui::MenuItem("Copy Attributes"))
            {
                copied_component = component;
            }

            if (ImGui::MenuItem("Paste Attributes"))
            {
                if (copied_component && copied_component->GetType() == component->GetType())
                {
                    component->SetAttributes(copied_component->GetAttributes());
                }
            }

            ImGui::EndPopup();
        }
    }

    static bool component_begin(const string& name, const IconType icon_enum, shared_ptr<Component> component_instance, bool options = true, const bool removable = true)
    {
        // Collapsible contents
        const bool collapsed = ImGuiSp::collapsing_header(name.c_str(), ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_DefaultOpen);

        // Component Icon - Top left
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // Component Options - Top right
        if (options)
        {
            const float icon_width = 16.0f;
            const auto original_pen_y = ImGui::GetCursorPosY();

            ImGui::SetCursorPosY(original_pen_y + 5.0f);
            ImGuiSp::image(icon_enum, 15);
            ImGui::SameLine(ImGuiSp::GetWindowContentRegionWidth() - icon_width + 1.0f); ImGui::SetCursorPosY(original_pen_y);
            uint32_t id = static_cast<uint32_t>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY());
            if (ImGuiSp::image_button(id, nullptr, IconType::Component_Options, icon_width, false))
            {
                context_menu_id = name;
                ImGui::OpenPopup(context_menu_id.c_str());
            }

            if (context_menu_id == name)
            {
                component_context_menu_options(context_menu_id, component_instance, removable);
            }
        }

        return collapsed;
    }

    static void component_end()
    {
        ImGui::Separator();
    }
}

Properties::Properties(Editor* editor) : Widget(editor)
{
    m_title          = "Properties";
    m_size_initial.x = 500; // min width

    m_colorPicker_light     = make_unique<ButtonColorPicker>("Light Color Picker");
    m_material_color_picker = make_unique<ButtonColorPicker>("Material Color Picker");
    m_colorPicker_camera    = make_unique<ButtonColorPicker>("Camera Color Picker");
}

void Properties::OnTickVisible()
{
    ImGui::PushItemWidth(item_width);

    if (!m_inspected_entity.expired())
    {
        shared_ptr<Entity> entity_ptr     = m_inspected_entity.lock();
        shared_ptr<Renderable> renderable = entity_ptr->GetComponent<Renderable>();
        Material* material                = renderable ? renderable->GetMaterial() : nullptr;

        ShowTransform(entity_ptr->GetComponent<Transform>());
        ShowLight(entity_ptr->GetComponent<Light>());
        ShowCamera(entity_ptr->GetComponent<Camera>());
        ShowTerrain(entity_ptr->GetComponent<Terrain>());
        ShowEnvironment(entity_ptr->GetComponent<Environment>());
        ShowAudioSource(entity_ptr->GetComponent<AudioSource>());
        ShowAudioListener(entity_ptr->GetComponent<AudioListener>());
        ShowReflectionProbe(entity_ptr->GetComponent<ReflectionProbe>());
        ShowRenderable(renderable);
        ShowMaterial(material);
        ShowPhysicsBody(entity_ptr->GetComponent<PhysicsBody>());
        ShowConstraint(entity_ptr->GetComponent<Constraint>());

        ShowAddComponentButton();
    }
    else if (!m_inspected_material.expired())
    {
        ShowMaterial(m_inspected_material.lock().get());
    }

    ImGui::PopItemWidth();
}

void Properties::Inspect(const shared_ptr<Entity> entity)
{
    m_inspected_entity = entity;

    // If we were previously inspecting a material, save the changes
    if (!m_inspected_material.expired())
    {
        m_inspected_material.lock()->SaveToFile(m_inspected_material.lock()->GetResourceFilePathNative());
    }
    m_inspected_material.reset();
}

void Properties::Inspect(const shared_ptr<Material> material)
{
    m_inspected_entity.reset();
    m_inspected_material = material;
}

void Properties::ShowTransform(shared_ptr<Transform> transform) const
{
    if (component_begin("Transform", IconType::Component_Transform, transform, true, false))
    {
        //= REFLECT =====================================================
        Vector3 position = transform->GetPositionLocal();
        Vector3 rotation = transform->GetRotationLocal().ToEulerAngles();
        Vector3 scale    = transform->GetScaleLocal();
        //===============================================================

        ImGuiSp::vector3("Position", position);
        ImGui::SameLine();
        ImGuiSp::vector3("Rotation", rotation);
        ImGui::SameLine();
        ImGuiSp::vector3("Scale", scale);

        //= MAP ===========================================================
        transform->SetPositionLocal(position);
        transform->SetScaleLocal(scale);
        transform->SetRotationLocal(Quaternion::FromEulerAngles(rotation));
        //=================================================================
    }
    component_end();
}

void Properties::ShowLight(shared_ptr<Light> light) const
{
    if (!light)
        return;

    if (component_begin("Light", IconType::Component_Light, light))
    {
        //= REFLECT ======================================================================
        static vector<string> types = { "Directional", "Point", "Spot" };
        float intensity             = light->GetIntensityLumens();
        float temperature_kelvin    = light->GetTemperature();
        float angle                 = light->GetAngle() * Math::Helper::RAD_TO_DEG * 2.0f;
        bool shadows                = light->GetShadowsEnabled();
        bool shadows_transparent    = light->GetShadowsTransparentEnabled();
        bool volumetric             = light->GetVolumetricEnabled();
        float bias                  = light->GetBias();
        float normal_bias           = light->GetNormalBias();
        float range                 = light->GetRange();
        m_colorPicker_light->SetColor(light->GetColor());
        //================================================================================

        // type
        ImGui::Text("Type");
        ImGui::SameLine(column_pos_x);
        uint32_t selection_index = static_cast<uint32_t>(light->GetLightType());
        if (ImGuiSp::combo_box("##LightType", types, &selection_index))
        {
            light->SetLightType(static_cast<LightType>(selection_index));
        }

        // temperature
        {
            ImGui::Text("Temperature");

            // color
            ImGui::SameLine(column_pos_x);
            m_colorPicker_light->Update();

            // kelvin
            ImGui::SameLine();
            ImGuiSp::draw_float_wrap("K", &temperature_kelvin, 0.3f, 1000.0f, 40000.0f);
            ImGuiSp::tooltip("Temperature expressed in Kelvin");
        }
        // intensity
        {
            static vector<string> intensity_types =
            {
                "Sky Sunlight Noon",
                "Sky Sunlight Morning Evening",
                "Sky Overcast Day",
                "Sky Twilight",
                "Bulb Stadium",
                "Bulb 500 watt",
                "Bulb 150 watt",
                "Bulb 100 watt",
                "Bulb 60 watt",
                "Bulb 25 watt",
                "Bulb Flashlight",
                "Black Hole",
                "Custom"
            };

            ImGui::Text("Intensity");

            // light types
            ImGui::SameLine(column_pos_x);
            uint32_t intensity_type_index = static_cast<uint32_t>(light->GetIntensity());
            if (ImGuiSp::combo_box("##light_intensity_type", intensity_types, &intensity_type_index))
            {
                light->SetIntensity(static_cast<LightIntensity>(intensity_type_index));
                intensity = light->GetIntensityLumens();
            }
            ImGuiSp::tooltip("Common light types");

            // lumens
            ImGui::SameLine();
            ImGuiSp::draw_float_wrap("lm", &intensity, 1.0f, 5.0f, 120000.0f);
            ImGuiSp::tooltip("Intensity expressed in lumens");
        }

        // Shadows
        ImGui::Text("Shadows");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##light_shadows", &shadows);

        // Shadow supplements
        ImGui::BeginDisabled(!shadows);
        {
            // Transparent shadows
            ImGui::Text("Transparent Shadows");
            ImGui::SameLine(column_pos_x); ImGui::Checkbox("##light_shadows_transparent", &shadows_transparent);
            ImGuiSp::tooltip("Allows transparent objects to cast colored translucent shadows");

            // Volumetric
            ImGui::Text("Volumetric");
            ImGui::SameLine(column_pos_x); ImGui::Checkbox("##light_volumetric", &volumetric);
            ImGuiSp::tooltip("The shadow map is used to determine which parts of the \"air\" should be lit");

        }
        ImGui::EndDisabled();

        // Bias
        ImGui::Text("Bias");
        ImGui::SameLine(column_pos_x);
        ImGui::InputFloat("##lightBias", &bias, 1.0f, 1.0f, "%.0f");

        // Normal Bias
        ImGui::Text("Normal Bias");
        ImGui::SameLine(column_pos_x);
        ImGui::InputFloat("##lightNormalBias", &normal_bias, 1.0f, 1.0f, "%.0f");

        // Range
        if (light->GetLightType() != LightType::Directional)
        {
            ImGui::Text("Range");
            ImGui::SameLine(column_pos_x);
            ImGuiSp::draw_float_wrap("##lightRange", &range, 0.01f, 0.0f, 1000.0f);
        }

        // Angle
        if (light->GetLightType() == LightType::Spot)
        {
            ImGui::Text("Angle");
            ImGui::SameLine(column_pos_x);
            ImGuiSp::draw_float_wrap("##lightAngle", &angle, 0.01f, 1.0f, 179.0f);
        }

        //= MAP =====================================================================================================================
        if (intensity != light->GetIntensityLumens())                      light->SetIntensityLumens(intensity);
        if (shadows != light->GetShadowsEnabled())                         light->SetShadowsEnabled(shadows);
        if (shadows_transparent != light->GetShadowsTransparentEnabled())  light->SetShadowsTransparentEnabled(shadows_transparent);
        if (volumetric != light->GetVolumetricEnabled())                   light->SetVolumetricEnabled(volumetric);
        if (bias != light->GetBias())                                      light->SetBias(bias);
        if (normal_bias != light->GetNormalBias())                         light->SetNormalBias(normal_bias);
        if (angle != light->GetAngle() * Math::Helper::RAD_TO_DEG * 0.5f)  light->SetAngle(angle * Math::Helper::DEG_TO_RAD * 0.5f);
        if (range != light->GetRange())                                    light->SetRange(range);
        if (m_colorPicker_light->GetColor() != light->GetColor())          light->SetColor(m_colorPicker_light->GetColor());
        if (temperature_kelvin != light->GetTemperature())                 light->SetTemperature(temperature_kelvin);
        //===========================================================================================================================
    }
    component_end();
}

void Properties::ShowRenderable(shared_ptr<Renderable> renderable) const
{
    if (!renderable)
        return;

    if (component_begin("Renderable", IconType::Component_Renderable, renderable))
    {
        //= REFLECT ===========================================================
        Mesh* mesh              = renderable->GetMesh();
        Material* material      = renderable->GetMaterial();
        uint32_t instance_count = renderable->GetInstanceCount();
        string name_mesh        = mesh ? mesh->GetObjectName() : "N/A";
        string name_material    = material ? material->GetObjectName() : "N/A";
        bool cast_shadows       = renderable->GetCastShadows();
        //=====================================================================

        // mesh
        ImGui::Text("Mesh");
        ImGui::SameLine(column_pos_x);
        ImGui::InputText("##renderable_mesh", &name_mesh, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

        // instancing
        if (instance_count != 0)
        {
            ImGui::Text("Instances");
            ImGui::SameLine(column_pos_x);
            ImGui::LabelText("##renderable_mesh", to_string(instance_count).c_str(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
        }

        // material
        ImGui::Text("Material");
        ImGui::SameLine(column_pos_x);
        ImGui::InputText("##renderable_material", &name_material, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
        if (auto payload = ImGuiSp::receive_drag_drop_payload(ImGuiSp::DragPayloadType::Material))
        {
            renderable->SetMaterial(std::get<const char*>(payload->data));
        }

        // cast shadows
        ImGui::Text("Cast Shadows");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##RenderableCastShadows", &cast_shadows);

        //= MAP ===================================================================================
        if (cast_shadows != renderable->GetCastShadows()) renderable->SetCastShadows(cast_shadows);
        //=========================================================================================
    }
    component_end();
}

void Properties::ShowPhysicsBody(shared_ptr<PhysicsBody> body) const
{
    if (!body)
        return;

    const auto input_text_flags = ImGuiInputTextFlags_CharsDecimal;
    const float step            = 0.1f;
    const float step_fast       = 0.1f;
    const auto precision        = "%.3f";

    if (component_begin("PhysicsBody", IconType::Component_PhysicsBody, body))
    {
        //= REFLECT =============================================================
        float mass                = body->GetMass();
        float friction            = body->GetFriction();
        float friction_rolling    = body->GetFrictionRolling();
        float restitution         = body->GetRestitution();
        bool use_gravity          = body->GetUseGravity();
        bool is_kinematic         = body->GetIsKinematic();
        bool freeze_pos_x         = static_cast<bool>(body->GetPositionLock().x);
        bool freeze_pos_y         = static_cast<bool>(body->GetPositionLock().y);
        bool freeze_pos_z         = static_cast<bool>(body->GetPositionLock().z);
        bool freeze_rot_x         = static_cast<bool>(body->GetRotationLock().x);
        bool freeze_rot_y         = static_cast<bool>(body->GetRotationLock().y);
        bool freeze_rot_z         = static_cast<bool>(body->GetRotationLock().z);
        Vector3 center_of_mass    = body->GetCenterOfMass();
        Vector3 bounding_box      = body->GetBoundingBox();
        //=======================================================================

        // body type
        {
            static vector<string> shape_types =
            {
                "Rigid Body",
                "Vehicle"
            };

            ImGui::Text("Body Type");
            ImGui::SameLine(column_pos_x);
            uint32_t selection_index = static_cast<uint32_t>(body->GetBodyType());
            if (ImGuiSp::combo_box("##physics_body_type", shape_types, &selection_index))
            {
                body->SetBodyType(static_cast<PhysicsBodyType>(selection_index));
            }
        }

        // mass
        ImGui::Text("Mass");
        ImGui::SameLine(column_pos_x); ImGui::InputFloat("##physics_body_mass", &mass, step, step_fast, precision, input_text_flags);

        // friction
        ImGui::Text("Friction");
        ImGui::SameLine(column_pos_x); ImGui::InputFloat("##physics_body_friction", &friction, step, step_fast, precision, input_text_flags);

        // rolling friction
        ImGui::Text("Rolling Friction");
        ImGui::SameLine(column_pos_x); ImGui::InputFloat("##physics_body_rolling_friction", &friction_rolling, step, step_fast, precision, input_text_flags);

        // restitution
        ImGui::Text("Restitution");
        ImGui::SameLine(column_pos_x); ImGui::InputFloat("##physics_body_restitution", &restitution, step, step_fast, precision, input_text_flags);

        // use gravity
        ImGui::Text("Use Gravity");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##physics_body_use_gravity", &use_gravity);

        // is kinematic
        ImGui::Text("Is Kinematic");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##physics_body_is_kinematic", &is_kinematic);

        // freeze position
        ImGui::Text("Freeze Position");
        ImGui::SameLine(column_pos_x); ImGui::Text("X");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_pos_x", &freeze_pos_x);
        ImGui::SameLine(); ImGui::Text("Y");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_pos_y", &freeze_pos_y);
        ImGui::SameLine(); ImGui::Text("Z");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_pos_z", &freeze_pos_z);

        // freeze rotation
        ImGui::Text("Freeze Rotation");
        ImGui::SameLine(column_pos_x); ImGui::Text("X");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_rot_x", &freeze_rot_x);
        ImGui::SameLine(); ImGui::Text("Y");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_rot_y", &freeze_rot_y);
        ImGui::SameLine(); ImGui::Text("Z");
        ImGui::SameLine(); ImGui::Checkbox("##physics_body_rot_z", &freeze_rot_z);

        ImGui::Separator();

        // collision shape
        {
            static vector<string> shape_types =
            {
                "Box",
                "Sphere",
                "Static Plane",
                "Cylinder",
                "Capsule",
                "Cone",
                "Terrain",
                "Mesh Convex Hull (Cheap)",
                "Mesh (Expensive)"
            };

            ImGui::Text("Shape Type");
            ImGui::SameLine(column_pos_x);
            uint32_t selection_index = static_cast<uint32_t>(body->GetShapeType());
            if (ImGuiSp::combo_box("##physics_body_shape", shape_types, &selection_index))
            {
                body->SetShapeType(static_cast<PhysicsShape>(selection_index));
            }
        }
        
        // center
        ImGui::Text("Shape Center");
        ImGui::SameLine(column_pos_x); ImGui::PushID("physics_body_shape_center_x"); ImGui::InputFloat("X", &center_of_mass.x, step, step_fast, precision, input_text_flags); ImGui::PopID();
        ImGui::SameLine();             ImGui::PushID("physics_body_shape_center_y"); ImGui::InputFloat("Y", &center_of_mass.y, step, step_fast, precision, input_text_flags); ImGui::PopID();
        ImGui::SameLine();             ImGui::PushID("physics_body_shape_center_z"); ImGui::InputFloat("Z", &center_of_mass.z, step, step_fast, precision, input_text_flags); ImGui::PopID();
        
        // size
        ImGui::Text("Shape Size");
        ImGui::SameLine(column_pos_x); ImGui::PushID("physics_body_shape_size_x"); ImGui::InputFloat("X", &bounding_box.x, step, step_fast, precision, input_text_flags); ImGui::PopID();
        ImGui::SameLine();             ImGui::PushID("physics_body_shape_size_y"); ImGui::InputFloat("Y", &bounding_box.y, step, step_fast, precision, input_text_flags); ImGui::PopID();
        ImGui::SameLine();             ImGui::PushID("physics_body_shape_size_z"); ImGui::InputFloat("Z", &bounding_box.z, step, step_fast, precision, input_text_flags); ImGui::PopID();

        //= MAP ===============================================================================================================================================================================================
        if (mass != body->GetMass())                                      body->SetMass(mass);
        if (friction != body->GetFriction())                              body->SetFriction(friction);
        if (friction_rolling != body->GetFrictionRolling())               body->SetFrictionRolling(friction_rolling);
        if (restitution != body->GetRestitution())                        body->SetRestitution(restitution);
        if (use_gravity != body->GetUseGravity())                         body->SetUseGravity(use_gravity);
        if (is_kinematic != body->GetIsKinematic())                       body->SetIsKinematic(is_kinematic);
        if (freeze_pos_x != static_cast<bool>(body->GetPositionLock().x)) body->SetPositionLock(Vector3(static_cast<float>(freeze_pos_x), static_cast<float>(freeze_pos_y), static_cast<float>(freeze_pos_z)));
        if (freeze_pos_y != static_cast<bool>(body->GetPositionLock().y)) body->SetPositionLock(Vector3(static_cast<float>(freeze_pos_x), static_cast<float>(freeze_pos_y), static_cast<float>(freeze_pos_z)));
        if (freeze_pos_z != static_cast<bool>(body->GetPositionLock().z)) body->SetPositionLock(Vector3(static_cast<float>(freeze_pos_x), static_cast<float>(freeze_pos_y), static_cast<float>(freeze_pos_z)));
        if (freeze_rot_x != static_cast<bool>(body->GetRotationLock().x)) body->SetRotationLock(Vector3(static_cast<float>(freeze_rot_x), static_cast<float>(freeze_rot_y), static_cast<float>(freeze_rot_z)));
        if (freeze_rot_y != static_cast<bool>(body->GetRotationLock().y)) body->SetRotationLock(Vector3(static_cast<float>(freeze_rot_x), static_cast<float>(freeze_rot_y), static_cast<float>(freeze_rot_z)));
        if (freeze_rot_z != static_cast<bool>(body->GetRotationLock().z)) body->SetRotationLock(Vector3(static_cast<float>(freeze_rot_x), static_cast<float>(freeze_rot_y), static_cast<float>(freeze_rot_z)));
        if (center_of_mass != body->GetCenterOfMass())                    body->SetCenterOfMass(center_of_mass);
        if (bounding_box != body->GetBoundingBox())                       body->SetBoundingBox(bounding_box);
        //=====================================================================================================================================================================================================
    }
    component_end();
}

void Properties::ShowConstraint(shared_ptr<Constraint> constraint) const
{
    if (!constraint)
        return;

    if (component_begin("Constraint", IconType::Component_AudioSource, constraint))
    {
        //= REFLECT ========================================================================================
        vector<string> constraint_types = {"Point", "Hinge", "Slider", "ConeTwist" };
        auto other_body                 = constraint->GetBodyOther();
        bool other_body_dirty           = false;
        Vector3 position                = constraint->GetPosition();
        Vector3 rotation                = constraint->GetRotation().ToEulerAngles();
        Vector2 high_limit              = constraint->GetHighLimit();
        Vector2 low_limit               = constraint->GetLowLimit();
        string other_body_name          = other_body.expired() ? "N/A" : other_body.lock()->GetObjectName();
        //==================================================================================================

        const auto inputTextFlags   = ImGuiInputTextFlags_CharsDecimal;
        const float step            = 0.1f;
        const float step_fast       = 0.1f;
        const char* precision       = "%.3f";

        // Type
        ImGui::Text("Type");
        ImGui::SameLine(column_pos_x);
        uint32_t selection_index = static_cast<uint32_t>(constraint->GetConstraintType());
        if (ImGuiSp::combo_box("##constraintType", constraint_types, &selection_index))
        {
            constraint->SetConstraintType(static_cast<ConstraintType>(selection_index));
        }

        // Other body
        ImGui::Text("Other Body"); ImGui::SameLine(column_pos_x);
        ImGui::PushID("##OtherBodyName");
        ImGui::InputText("", &other_body_name, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
        if (auto payload = ImGuiSp::receive_drag_drop_payload(ImGuiSp::DragPayloadType::Entity))
        {
            const uint64_t entity_id = get<uint64_t>(payload->data);
            other_body               = World::GetEntityById(entity_id);
            other_body_dirty         = true;
        }
        ImGui::PopID();

        // Position
        ImGui::Text("Position");
        ImGui::SameLine(column_pos_x); ImGui::Text("X");
        ImGui::SameLine(); ImGui::InputFloat("##ConsPosX", &position.x, step, step_fast, precision, inputTextFlags);
        ImGui::SameLine(); ImGui::Text("Y");
        ImGui::SameLine(); ImGui::InputFloat("##ConsPosY", &position.y, step, step_fast, precision, inputTextFlags);
        ImGui::SameLine(); ImGui::Text("Z");
        ImGui::SameLine(); ImGui::InputFloat("##ConsPosZ", &position.z, step, step_fast, precision, inputTextFlags);

        // Rotation
        ImGui::Text("Rotation");
        ImGui::SameLine(column_pos_x); ImGui::Text("X");
        ImGui::SameLine(); ImGui::InputFloat("##ConsRotX", &rotation.x, step, step_fast, precision, inputTextFlags);
        ImGui::SameLine(); ImGui::Text("Y");
        ImGui::SameLine(); ImGui::InputFloat("##ConsRotY", &rotation.y, step, step_fast, precision, inputTextFlags);
        ImGui::SameLine(); ImGui::Text("Z");
        ImGui::SameLine(); ImGui::InputFloat("##ConsRotZ", &rotation.z, step, step_fast, precision, inputTextFlags);

        // High Limit
        ImGui::Text("High Limit");
        ImGui::SameLine(column_pos_x); ImGui::Text("X");
        ImGui::SameLine(); ImGui::InputFloat("##ConsHighLimX", &high_limit.x, step, step_fast, precision, inputTextFlags);
        if (constraint->GetConstraintType() == ConstraintType_Slider)
        {
            ImGui::SameLine(); ImGui::Text("Y");
            ImGui::SameLine(); ImGui::InputFloat("##ConsHighLimY", &high_limit.y, step, step_fast, precision, inputTextFlags);
        }

        // Low Limit
        ImGui::Text("Low Limit");
        ImGui::SameLine(column_pos_x); ImGui::Text("X");
        ImGui::SameLine(); ImGui::InputFloat("##ConsLowLimX", &low_limit.x, step, step_fast, precision, inputTextFlags);
        if (constraint->GetConstraintType() == ConstraintType_Slider)
        {
            ImGui::SameLine(); ImGui::Text("Y");
            ImGui::SameLine(); ImGui::InputFloat("##ConsLowLimY", &low_limit.y, step, step_fast, precision, inputTextFlags);
        }

        //= MAP =======================================================================================================================
        if (other_body_dirty)                                       { constraint->SetBodyOther(other_body); other_body_dirty = false; }
        if (position != constraint->GetPosition())                  constraint->SetPosition(position);
        if (rotation != constraint->GetRotation().ToEulerAngles())  constraint->SetRotation(Quaternion::FromEulerAngles(rotation));
        if (high_limit != constraint->GetHighLimit())               constraint->SetHighLimit(high_limit);
        if (low_limit != constraint->GetLowLimit())                 constraint->SetLowLimit(low_limit);
        //=============================================================================================================================
    }
    component_end();
}

void Properties::ShowMaterial(Material* material) const
{
    if (!material)
        return;

    if (component_begin("Material", IconType::Component_Material, nullptr, false))
    {
        //= REFLECT ===========================================
        Math::Vector2 tiling = Vector2(
            material->GetProperty(MaterialProperty::UvTilingX),
            material->GetProperty(MaterialProperty::UvTilingY)
        );

        Math::Vector2 offset = Vector2(
            material->GetProperty(MaterialProperty::UvOffsetX),
            material->GetProperty(MaterialProperty::UvOffsetY)
        );

        m_material_color_picker->SetColor(Color(
            material->GetProperty(MaterialProperty::ColorR),
            material->GetProperty(MaterialProperty::ColorG),
            material->GetProperty(MaterialProperty::ColorB),
            material->GetProperty(MaterialProperty::ColorA)
        ));
        //=====================================================

        // Name
        ImGui::Text("Name");
        ImGui::SameLine(column_pos_x); ImGui::Text(material->GetObjectName().c_str());

        if (material->GetProperty(MaterialProperty::CanBeEdited) == 1.0f)
        {
            // Texture slots
            {
                const auto show_property = [this, &material](const char* name, const char* tooltip, const MaterialTexture mat_tex, const MaterialProperty mat_property)
                {
                    bool show_texture  = mat_tex      != MaterialTexture::Undefined;
                    bool show_modifier = mat_property != MaterialProperty::Undefined;

                    // Name
                    if (name)
                    {
                        ImGui::Text(name);

                        if (tooltip)
                        {
                            ImGuiSp::tooltip(tooltip);
                        }

                        if (show_texture || show_modifier)
                        {
                            ImGui::SameLine(column_pos_x);
                        }
                    }

                    // Texture
                    if (show_texture)
                    {
                        auto setter = [&material, &mat_tex](const shared_ptr<RHI_Texture>& texture) { material->SetTexture(mat_tex, texture); };
                        ImGuiSp::image_slot(material->GetTexture_PtrShared(mat_tex), setter);

                        // 2nd textures, used for blending by the terrain
                        if (mat_tex == MaterialTexture::Color)
                        {
                            auto setter = [&material](const shared_ptr<RHI_Texture>& texture) { material->SetTexture(MaterialTexture::Color2, texture); };
                            ImGui::SameLine();
                            ImGuiSp::image_slot(material->GetTexture_PtrShared(MaterialTexture::Color2), setter);
                        }
                        else if (mat_tex == MaterialTexture::Normal)
                        {
                            auto setter = [&material](const shared_ptr<RHI_Texture>& texture) { material->SetTexture(MaterialTexture::Normal2, texture); };
                            ImGui::SameLine();
                            ImGuiSp::image_slot(material->GetTexture_PtrShared(MaterialTexture::Normal2), setter);
                        }

                        if (show_modifier)
                        {
                            ImGui::SameLine();
                        }
                    }

                    // Modifier
                    if (show_modifier)
                    {
                        if (mat_property == MaterialProperty::ColorTint)
                        {
                            m_material_color_picker->Update();
                        }
                        else
                        {
                            ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));
                            float value = material->GetProperty(mat_property);

                            if (mat_property != MaterialProperty::MetalnessMultiplier)
                            {
                                ImGuiSp::draw_float_wrap("", &value, 0.004f, 0.0f, 1.0f);
                            }
                            else
                            {
                                bool is_metallic = value != 0.0f;
                                ImGui::Checkbox("##metalness", &is_metallic);
                                value = is_metallic ? 1.0f : 0.0f;
                            }

                            material->SetProperty(mat_property, value);
                            ImGui::PopID();
                        }
                    }
                };

                show_property("Clearcoat",            "Extra white specular layer on top of others",                                       MaterialTexture::Undefined,  MaterialProperty::Clearcoat);
                show_property("Clearcoat roughness",  "Roughness of clearcoat specular",                                                   MaterialTexture::Undefined,  MaterialProperty::Clearcoat_Roughness);
                show_property("Anisotropic",          "Amount of anisotropy for specular reflection",                                      MaterialTexture::Undefined,  MaterialProperty::Anisotropic);
                show_property("Anisotropic rotation", "Rotates the direction of anisotropy, with 1.0 going full circle",                   MaterialTexture::Undefined,  MaterialProperty::AnisotropicRotation);
                show_property("Sheen",                "Amount of soft velvet like reflection near edges",                                  MaterialTexture::Undefined,  MaterialProperty::Sheen);
                show_property("Sheen tint",           "Mix between white and using base color for sheen reflection",                       MaterialTexture::Undefined,  MaterialProperty::SheenTint);
                show_property("Color",                "Surface color",                                                                     MaterialTexture::Color,      MaterialProperty::ColorTint);
                show_property("Roughness",            "Specifies microfacet roughness of the surface for diffuse and specular reflection", MaterialTexture::Roughness,  MaterialProperty::RoughnessMultiplier);
                show_property("Metalness",            "Blends between a non-metallic and metallic material model",                         MaterialTexture::Metalness,  MaterialProperty::MetalnessMultiplier);
                show_property("Normal",               "Controls the normals of the base layers",                                           MaterialTexture::Normal,     MaterialProperty::NormalMultiplier);
                show_property("Height",               "Perceived depth for parallax mapping",                                              MaterialTexture::Height,     MaterialProperty::HeightMultiplier);
                show_property("Occlusion",            "Amount of light loss, can be complementary to SSAO",                                MaterialTexture::Occlusion,  MaterialProperty::Undefined);
                show_property("Emission",             "Light emission from the surface, works nice with bloom",                            MaterialTexture::Emission,   MaterialProperty::Undefined);
                show_property("Alpha mask",           "Discards pixels",                                                                   MaterialTexture::AlphaMask,  MaterialProperty::Undefined);
            }

            // UV
            {
                // Tiling
                ImGui::Text("Tiling");
                ImGui::SameLine(column_pos_x); ImGui::Text("X");
                ImGui::SameLine(); ImGui::InputFloat("##matTilingX", &tiling.x, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
                ImGui::SameLine(); ImGui::Text("Y");
                ImGui::SameLine(); ImGui::InputFloat("##matTilingY", &tiling.y, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);

                // Offset
                ImGui::Text("Offset");
                ImGui::SameLine(column_pos_x); ImGui::Text("X");
                ImGui::SameLine(); ImGui::InputFloat("##matOffsetX", &offset.x, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
                ImGui::SameLine(); ImGui::Text("Y");
                ImGui::SameLine(); ImGui::InputFloat("##matOffsetY", &offset.y, 0.01f, 0.1f, "%.2f", ImGuiInputTextFlags_CharsDecimal);
            }
        }
        else
        {
            ImGui::Text("Can not be edited");
        }

        //= MAP ===============================================================================
        material->SetProperty(MaterialProperty::UvTilingX, tiling.x);
        material->SetProperty(MaterialProperty::UvTilingY, tiling.y);
        material->SetProperty(MaterialProperty::UvOffsetX, offset.x);
        material->SetProperty(MaterialProperty::UvOffsetY, offset.y);
        material->SetProperty(MaterialProperty::ColorR, m_material_color_picker->GetColor().r);
        material->SetProperty(MaterialProperty::ColorG, m_material_color_picker->GetColor().g);
        material->SetProperty(MaterialProperty::ColorB, m_material_color_picker->GetColor().b);
        material->SetProperty(MaterialProperty::ColorA, m_material_color_picker->GetColor().a);
        //=====================================================================================
    }

    component_end();
}

void Properties::ShowCamera(shared_ptr<Camera> camera) const
{
    if (!camera)
        return;

    if (component_begin("Camera", IconType::Component_Camera, camera))
    {
        //= REFLECT ====================================================================
        static vector<string> projection_types = { "Perspective", "Orthographic" };
        float aperture                         = camera->GetAperture();
        float shutter_speed                    = camera->GetShutterSpeed();
        float iso                              = camera->GetIso();
        float fov                              = camera->GetFovHorizontalDeg();
        float near_plane                       = camera->GetNearPlane();
        float far_plane                        = camera->GetFarPlane();
        bool first_person_control_enabled      = camera->GetIsControlEnabled();
        m_colorPicker_camera->SetColor(camera->GetClearColor());
        //==============================================================================

        const auto input_text_flags = ImGuiInputTextFlags_CharsDecimal;

        // Background
        ImGui::Text("Background");
        ImGui::SameLine(column_pos_x); m_colorPicker_camera->Update();

        // Projection
        ImGui::Text("Projection");
        ImGui::SameLine(column_pos_x);
        uint32_t selection_index = static_cast<uint32_t>(camera->GetProjectionType());
        if (ImGuiSp::combo_box("##cameraProjection", projection_types, &selection_index))
        {
            camera->SetProjection(static_cast<ProjectionType>(selection_index));
        }

        // Aperture
        ImGui::SetCursorPosX(column_pos_x);
        ImGuiSp::draw_float_wrap("Aperture (f-stop)", &aperture, 0.01f, 0.01f, 150.0f);
        ImGuiSp::tooltip("Aperture value in f-stop, controls the amount of light, depth of field and chromatic aberration");

        // Shutter speed
        ImGui::SetCursorPosX(column_pos_x);
        ImGuiSp::draw_float_wrap("Shutter Speed (sec)", &shutter_speed, 0.0001f, 0.0f, 1.0f, "%.4f");
        ImGuiSp::tooltip("Length of time for which the camera shutter is open, controls the amount of motion blur");

        // ISO
        ImGui::SetCursorPosX(column_pos_x);
        ImGuiSp::draw_float_wrap("ISO", &iso, 0.1f, 0.0f, 2000.0f);
        ImGuiSp::tooltip("Sensitivity to light, controls camera noise");

        // Field of View
        ImGui::SetCursorPosX(column_pos_x);
        ImGuiSp::draw_float_wrap("Field of View", &fov, 0.1f, 1.0f, 179.0f);

        // Clipping Planes
        ImGui::Text("Clipping Planes");
        ImGui::SameLine(column_pos_x);      ImGui::InputFloat("Near", &near_plane, 0.01f, 0.01f, "%.2f", input_text_flags);
        ImGui::SetCursorPosX(column_pos_x); ImGui::InputFloat("Far", &far_plane, 0.01f, 0.01f, "%.2f", input_text_flags);

        // FPS Control
        ImGui::Text("First Person Control");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##camera_first_person_control", &first_person_control_enabled);
        ImGuiSp::tooltip("Enables first person control while holding down the right mouse button (or when a controller is connected)");

        //= MAP =======================================================================================================================================
        if (aperture != camera->GetAperture())                                      camera->SetAperture(aperture);
        if (shutter_speed != camera->GetShutterSpeed())                             camera->SetShutterSpeed(shutter_speed);
        if (iso != camera->GetIso())                                                camera->SetIso(iso);
        if (fov != camera->GetFovHorizontalDeg())                                   camera->SetFovHorizontalDeg(fov);
        if (near_plane != camera->GetNearPlane())                                   camera->SetNearPlane(near_plane);
        if (far_plane != camera->GetFarPlane())                                     camera->SetFarPlane(far_plane);
        if (first_person_control_enabled != camera->GetIsControlEnabled()) camera->SetIsControlEnalbed(first_person_control_enabled);
        if (m_colorPicker_camera->GetColor() != camera->GetClearColor())            camera->SetClearColor(m_colorPicker_camera->GetColor());
        //=============================================================================================================================================
    }
    component_end();
}

void Properties::ShowEnvironment(shared_ptr<Environment> environment) const
{
    if (!environment)
        return;

    if (component_begin("Environment", IconType::Component_Environment, environment))
    {
        ImGui::Text("Sphere Map");

        ImGuiSp::image_slot(environment->GetTexture(), [&environment](const shared_ptr<RHI_Texture>& texture) { environment->SetTexture(texture); } );
    }
    component_end();
}

void Properties::ShowTerrain(shared_ptr<Terrain> terrain) const
{
    if (!terrain)
        return;

    if (component_begin("Terrain", IconType::Component_Terrain, terrain))
    {
        //= REFLECT =====================
        float min_y = terrain->GetMinY();
        float max_y = terrain->GetMaxY();
        //===============================

        const float cursor_y = ImGui::GetCursorPosY();

        ImGui::BeginGroup();
        {
            ImGui::Text("Height Map");

            ImGuiSp::image_slot(terrain->GetHeightMap(), [&terrain](const shared_ptr<RHI_Texture>& texture)
            {
                terrain->SetHeightMap(static_pointer_cast<RHI_Texture2D>(texture));
            });

            if (ImGuiSp::button("Generate", ImVec2(82.0f * Spartan::Window::GetDpiScale(), 0)))
            {
                terrain->GenerateAsync();
            }
        }
        ImGui::EndGroup();

        // Min, max
        ImGui::SameLine();
        ImGui::SetCursorPosY(cursor_y);
        ImGui::BeginGroup();
        {
            ImGui::InputFloat("Min Y", &min_y);
            ImGui::InputFloat("Max Y", &max_y);
        }
        ImGui::EndGroup();

        // Stats
        ImGui::BeginGroup();
        {
            ImGui::Text("Height samples: %d", terrain->GetHeightSampleCount());
            ImGui::Text("Vertices: %d",  terrain->GetVertexCount());
            ImGui::Text("Indices:  %d ", terrain->GetIndexCount());
            ImGui::Text("Trees:  %d ", terrain->GetTransformsTree().size());
            ImGui::Text("Plants 1:  %d ", terrain->GetTransformsPlant1().size());
            ImGui::Text("Plants 2:  %d ", terrain->GetTransformsPlant2().size());
        }
        ImGui::EndGroup();

        //= MAP =================================================
        if (min_y != terrain->GetMinY()) terrain->SetMinY(min_y);
        if (max_y != terrain->GetMaxY()) terrain->SetMaxY(max_y);
        //=======================================================
    }
    component_end();
}

void Properties::ShowAudioSource(shared_ptr<AudioSource> audio_source) const
{
    if (!audio_source)
        return;

    if (component_begin("Audio Source", IconType::Component_AudioSource, audio_source))
    {
        //= REFLECT ==============================================
        string audio_clip_name = audio_source->GetAudioClipName();
        bool mute              = audio_source->GetMute();
        bool play_on_start     = audio_source->GetPlayOnStart();
        bool loop              = audio_source->GetLoop();
        bool is_3d             = audio_source->Get3d();
        int priority           = audio_source->GetPriority();
        float volume           = audio_source->GetVolume();
        float pitch            = audio_source->GetPitch();
        float pan              = audio_source->GetPan();
        //========================================================

        // Audio clip
        ImGui::Text("Audio Clip");
        ImGui::SameLine(column_pos_x);
        ImGui::InputText("##audioSourceAudioClip", &audio_clip_name, ImGuiInputTextFlags_ReadOnly);
        if (auto payload = ImGuiSp::receive_drag_drop_payload(ImGuiSp::DragPayloadType::Audio))
        {
            audio_source->SetAudioClip(std::get<const char*>(payload->data));
        }

        // Play on start
        ImGui::Text("Play on Start");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##audioSourcePlayOnStart", &play_on_start);

        // Mute
        ImGui::Text("Mute");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##audioSourceMute", &mute);

        // Loop
        ImGui::Text("Loop");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##audioSourceLoop", &loop);

        // 3D
        ImGui::Text("3D");
        ImGui::SameLine(column_pos_x); ImGui::Checkbox("##audioSource3d", &is_3d);

        // Priority
        ImGui::Text("Priority");
        ImGui::SameLine(column_pos_x); ImGui::SliderInt("##audioSourcePriority", &priority, 0, 255);

        // Volume
        ImGui::Text("Volume");
        ImGui::SameLine(column_pos_x); ImGui::SliderFloat("##audioSourceVolume", &volume, 0.0f, 1.0f);

        // Pitch
        ImGui::Text("Pitch");
        ImGui::SameLine(column_pos_x); ImGui::SliderFloat("##audioSourcePitch", &pitch, 0.0f, 3.0f);

        // Pan
        ImGui::Text("Pan");
        ImGui::SameLine(column_pos_x); ImGui::SliderFloat("##audioSourcePan", &pan, -1.0f, 1.0f);

        //= MAP =========================================================================================
        if (mute != audio_source->GetMute())                 audio_source->SetMute(mute);
        if (play_on_start != audio_source->GetPlayOnStart()) audio_source->SetPlayOnStart(play_on_start);
        if (loop != audio_source->GetLoop())                 audio_source->SetLoop(loop);
        if (is_3d != audio_source->Get3d())                  audio_source->Set3d(is_3d);
        if (priority != audio_source->GetPriority())         audio_source->SetPriority(priority);
        if (volume != audio_source->GetVolume())             audio_source->SetVolume(volume);
        if (pitch != audio_source->GetPitch())               audio_source->SetPitch(pitch);
        if (pan != audio_source->GetPan())                   audio_source->SetPan(pan);
        //===============================================================================================
    }
    component_end();
}

void Properties::ShowAudioListener(shared_ptr<AudioListener> audio_listener) const
{
    if (!audio_listener)
        return;

    if (component_begin("Audio Listener", IconType::Component_AudioListener, audio_listener))
    {

    }
    component_end();
}

void Properties::ShowReflectionProbe(shared_ptr<ReflectionProbe> reflection_probe) const
{
    if (!reflection_probe)
        return;

    if (component_begin("Reflection Probe", IconType::Component_ReflectionProbe, reflection_probe))
    {
        //= REFLECT =============================================================
        int resolution             = reflection_probe->GetResolution();
        Vector3 extents            = reflection_probe->GetExtents();
        int update_interval_frames = reflection_probe->GetUpdateIntervalFrames();
        int update_face_count      = reflection_probe->GetUpdateFaceCount();
        float plane_near           = reflection_probe->GetNearPlane();
        float plane_far            = reflection_probe->GetFarPlane();
        //=======================================================================

        // Resolution
        ImGui::Text("Resolution");
        ImGui::SameLine(column_pos_x);
        ImGui::InputInt("##reflection_probe_resolution", &resolution);

        // Update interval frames
        ImGui::Text("Update interval frames");
        ImGui::SameLine(column_pos_x);
        ImGui::InputInt("##reflection_probe_update_interval_frames", &update_interval_frames);

        // Update face count
        ImGui::Text("Update face count");
        ImGui::SameLine(column_pos_x);
        ImGui::InputInt("##reflection_probe_update_face_count", &update_face_count);

        // Near plane
        ImGui::Text("Near plane");
        ImGui::SameLine(column_pos_x);
        ImGui::InputFloat("##reflection_probe_plane_near", &plane_near, 1.0f, 1.0f, "%.1f");

        // Far plane
        ImGui::Text("Far plane");
        ImGui::SameLine(column_pos_x);
        ImGui::InputFloat("##reflection_probe_plane_far", &plane_far, 1.0f, 1.0f, "%.1f");

        // Extents
        const ImGuiInputTextFlags_ input_text_flags = ImGuiInputTextFlags_CharsDecimal;
        const float step                            = 0.1f;
        const float step_fast                       = 0.1f;
        const char* precision                       = "%.3f";
        ImGui::Text("Extents");
        ImGui::SameLine(column_pos_x); ImGui::PushID("##reflection_probe_extents_x"); ImGui::InputFloat("X", &extents.x, step, step_fast, precision, input_text_flags); ImGui::PopID();
        ImGui::SameLine();             ImGui::PushID("##reflection_probe_extents_y"); ImGui::InputFloat("Y", &extents.y, step, step_fast, precision, input_text_flags); ImGui::PopID();
        ImGui::SameLine();             ImGui::PushID("##reflection_probe_extents_z"); ImGui::InputFloat("Z", &extents.z, step, step_fast, precision, input_text_flags); ImGui::PopID();

        //= MAP =====================================================================================================================================
        if (resolution             != reflection_probe->GetResolution())           reflection_probe->SetResolution(resolution);
        if (extents                != reflection_probe->GetExtents())              reflection_probe->SetExtents(extents);
        if (update_interval_frames != reflection_probe->GetUpdateIntervalFrames()) reflection_probe->SetUpdateIntervalFrames(update_interval_frames);
        if (update_face_count      != reflection_probe->GetUpdateFaceCount())      reflection_probe->SetUpdateFaceCount(update_face_count);
        if (plane_near             != reflection_probe->GetNearPlane())            reflection_probe->SetNearPlane(plane_near);
        if (plane_far              != reflection_probe->GetFarPlane())             reflection_probe->SetFarPlane(plane_far);
        //===========================================================================================================================================
    }
    component_end();
}

void Properties::ShowAddComponentButton() const
{
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - 50);
    if (ImGuiSp::button("Add Component"))
    {
        ImGui::OpenPopup("##ComponentContextMenu_Add");
    }
    ComponentContextMenu_Add();
}

void Properties::ComponentContextMenu_Add() const
{
    if (ImGui::BeginPopup("##ComponentContextMenu_Add"))
    {
        if (auto entity = m_inspected_entity.lock())
        {
            // CAMERA
            if (ImGui::MenuItem("Camera"))
            {
                entity->AddComponent<Camera>();
            }

            // LIGHT
            if (ImGui::BeginMenu("Light"))
            {
                if (ImGui::MenuItem("Directional"))
                {
                    entity->AddComponent<Light>()->SetLightType(LightType::Directional);
                }
                else if (ImGui::MenuItem("Point"))
                {
                    entity->AddComponent<Light>()->SetLightType(LightType::Point);
                }
                else if (ImGui::MenuItem("Spot"))
                {
                    entity->AddComponent<Light>()->SetLightType(LightType::Spot);
                }

                ImGui::EndMenu();
            }

            // PHYSICS
            if (ImGui::BeginMenu("Physics"))
            {
                if (ImGui::MenuItem("Physics Body"))
                {
                    entity->AddComponent<PhysicsBody>();
                }
                else if (ImGui::MenuItem("Constraint"))
                {
                    entity->AddComponent<Constraint>();
                }

                ImGui::EndMenu();
            }

            // AUDIO
            if (ImGui::BeginMenu("Audio"))
            {
                if (ImGui::MenuItem("Audio Source"))
                {
                    entity->AddComponent<AudioSource>();
                }
                else if (ImGui::MenuItem("Audio Listener"))
                {
                    entity->AddComponent<AudioListener>();
                }

                ImGui::EndMenu();
            }

            // ENVIRONMENT
            if (ImGui::BeginMenu("Environment"))
            {
                if (ImGui::MenuItem("Environment"))
                {
                    entity->AddComponent<Environment>();
                }

                ImGui::EndMenu();
            }

            // TERRAIN
            if (ImGui::MenuItem("Terrain"))
            {
                entity->AddComponent<Terrain>();
            }

            // PROBE
            if (ImGui::BeginMenu("Probe"))
            {
                if (ImGui::MenuItem("Reflection Probe"))
                {
                    entity->AddComponent<ReflectionProbe>();
                }

                ImGui::EndMenu();
            }
        }

        ImGui::EndPopup();
    }
}
