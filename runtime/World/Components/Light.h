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

//= INCLUDES =========================
#include <array>
#include <memory>
#include "Component.h"
#include "Renderable.h"
#include "../../Math/Vector4.h"
#include "../../Math/Vector3.h"
#include "../../Math/Matrix.h"
#include "../../RHI/RHI_Definitions.h"
#include "../../Math/Frustum.h"
#include "../../Rendering/Color.h"
//====================================

namespace Spartan
{
    //= FWD DECLARATIONS =
    class Camera;
    //====================

    enum class LightType
    {
        Directional,
        Point,
        Spot
    };

    enum class LightIntensity
    {
        sky_sunlight_noon,            // Direct sunlight at noon, the brightest light
        sky_sunlight_morning_evening, // Direct sunlight at morning or evening, less intense than noon light
        sky_overcast_day,             // Light on an overcast day, considerably less intense than direct sunlight
        sky_twilight,                 // Light just after sunset, a soft and less intense light
        bulb_stadium,                 // Intense light used in stadiums for sports events, comparable to sunlight
        bulb_500_watt,                // A very bright domestic bulb or small industrial light
        bulb_150_watt,                // A bright domestic bulb, equivalent to an old-school incandescent bulb
        bulb_100_watt,                // A typical bright domestic bulb
        bulb_60_watt,                 // A medium intensity domestic bulb
        bulb_25_watt,                 // A low intensity domestic bulb, used for mood lighting or as a night light
        bulb_flashlight,              // Light emitted by an average flashlight, portable and less intense
        black_hole,                   // No light emitted
        custom                        // Custom intensity
    };

    struct ShadowSlice
    {
        Math::Vector3 min    = Math::Vector3::Zero;
        Math::Vector3 max    = Math::Vector3::Zero;
        Math::Vector3 center = Math::Vector3::Zero;
        Math::Frustum frustum;
    };

    struct ShadowMap
    {
        std::shared_ptr<RHI_Texture> texture_color;
        std::shared_ptr<RHI_Texture> texture_depth;
        std::vector<ShadowSlice> slices;
    };

    class SP_CLASS Light : public Component
    {
    public:
        Light(std::weak_ptr<Entity> entity);
        ~Light() = default;

        //= COMPONENT ================================
        void OnInitialize() override;
        void OnTick() override;
        void Serialize(FileStream* stream) override;
        void Deserialize(FileStream* stream) override;
        //============================================

        const LightType GetLightType() const { return m_light_type; }
        void SetLightType(LightType type);

        // Color
        void SetTemperature(const float temperature_kelvin);
        float GetTemperature() const { return m_temperature_kelvin; }
        void SetColor(const Color& rgb);
        const Color& GetColor() const { return m_color_rgb; }

        // Intensity
        void SetIntensityLumens(const float lumens);
        void SetIntensity(const LightIntensity lumens);
        float GetIntensityLumens() const    { return m_intensity_lumens; }
        LightIntensity GetIntensity() const { return m_intensity; }
        float GetIntensityWatt(Camera* camera) const;

        bool GetShadowsEnabled() const { return m_shadows_enabled; }
        void SetShadowsEnabled(bool cast_shadows);

        bool GetShadowsTransparentEnabled() const { return m_shadows_transparent_enabled; }
        void SetShadowsTransparentEnabled(bool cast_transparent_shadows);

        bool GetVolumetricEnabled() const             { return m_volumetric_enabled; }
        void SetVolumetricEnabled(bool is_volumetric) { m_volumetric_enabled = is_volumetric; }

        void SetRange(float range);
        auto GetRange() const { return m_range; }

        void SetAngle(float angle_rad);
        auto GetAngle() const { return m_angle_rad; }

        void SetBias(float value) { m_bias = value; }
        float GetBias() const     { return m_bias; }

        void SetNormalBias(float value) { m_normal_bias = value; }
        auto GetNormalBias() const { return m_normal_bias; }

        const Math::Matrix& GetViewMatrix(uint32_t index = 0) const;
        const Math::Matrix& GetProjectionMatrix(uint32_t index = 0) const;
        float GetCascadeEnd(uint32_t index = 0) const;

        RHI_Texture* GetDepthTexture() const { return m_shadow_map.texture_depth.get(); }
        RHI_Texture* GetColorTexture() const { return m_shadow_map.texture_color.get(); }
        uint32_t GetShadowArraySize() const;
        void CreateShadowMap();

        bool IsInViewFrustum(std::shared_ptr<Renderable> renderable, uint32_t index) const;

    private:
        void ComputeViewMatrix();
        void ComputeProjectionMatrix(uint32_t index = 0);
        void ComputeCascadeSplits();

        // Intensity
        LightIntensity m_intensity = LightIntensity::bulb_500_watt;
        float m_intensity_lumens   = 2600.0f;

        // Shadows
        bool m_shadows_enabled             = true;
        bool m_shadows_transparent_enabled = true;
        uint32_t m_cascade_count           = 3;
        ShadowMap m_shadow_map;

        // Bias
        float m_bias        = 0.0f;
        float m_normal_bias = 5.0f;

        // Misc
        LightType m_light_type     = LightType::Directional;
        Color m_color_rgb          = Color::standard_black;;
        float m_temperature_kelvin = 0.0f;
        bool m_volumetric_enabled  = true;
        float m_range              = 10.0f;
        float m_angle_rad          = 0.5f; // about 30 degrees
        bool m_initialized         = false;
        std::array<Math::Matrix, 6> m_matrix_view;
        std::array<Math::Matrix, 6> m_matrix_projection;
        std::array<float, 3> m_cascade_ends;

        // Dirty checks
        bool m_is_dirty                     = true;
        Math::Matrix m_previous_camera_view = Math::Matrix::Identity;
    };
}
