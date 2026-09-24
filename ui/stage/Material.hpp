#pragma once

#include "ui/core/Color.hpp"
#include "ui/scene/Vector3.hpp"
#include "ui/theme/Theme.hpp"
#include <cstdint>
#include <optional>

namespace ui::stage
{
    // Flat-shaded per face: ambient plus Lambert diffuse plus a Blinn-Phong highlight. With a role
    // set the base colour follows the active theme and `color` is ignored.
    struct Material
    {
        std::optional<theme::ColorRole> role{ theme::ColorRole::SceneSurface };
        Color color{ Color::Rgb(0xB4BCC8) };

        float ambient{ 0.35f };
        float diffuse{ 0.65f };
        float specular{ 0.0f };
        float shininess{ 16.0f };
        std::uint8_t opacity{ 255 };

        // Translucent faces are never culled, so their back faces show through the front.
        bool doubleSided{ false };

        // Outlines only, no fill. Implies an outline in the fill colour when none is set.
        bool wireframe{ false };

        // Draws the mesh's hard edges: a box's twelve, a cylinder's rims.
        bool outline{ false };
        std::optional<theme::ColorRole> outlineRole{ theme::ColorRole::SceneEdge };
        Color outlineColor{ colors::black };
        float outlineWidth{ 1.0f };
    };

    namespace materials
    {
        [[nodiscard]] constexpr Material Themed(theme::ColorRole role)
        {
            Material material;
            material.role = role;
            return material;
        }

        [[nodiscard]] constexpr Material Matte(Color color)
        {
            Material material;
            material.role = std::nullopt;
            material.color = color;
            return material;
        }

        [[nodiscard]] constexpr Material Plastic(Color color)
        {
            auto material = Matte(color);
            material.specular = 0.25f;
            material.shininess = 24.0f;
            return material;
        }

        [[nodiscard]] constexpr Material Metal(Color color)
        {
            auto material = Matte(color);
            material.ambient = 0.3f;
            material.diffuse = 0.55f;
            material.specular = 0.6f;
            material.shininess = 48.0f;
            material.outline = true;
            return material;
        }

        [[nodiscard]] constexpr Material Aluminium()
        {
            return Metal(Color::Rgb(0xC8CCD2));
        }

        [[nodiscard]] constexpr Material Steel()
        {
            return Metal(Color::Rgb(0x8A9099));
        }

        [[nodiscard]] constexpr Material Translucent(Color color, std::uint8_t opacity = 96)
        {
            auto material = Matte(color);
            material.opacity = opacity;
            material.doubleSided = true;
            return material;
        }

        [[nodiscard]] constexpr Material Wireframe(Color color, float width = 1.0f)
        {
            auto material = Matte(color);
            material.wireframe = true;
            material.outline = true;
            material.outlineRole = std::nullopt;
            material.outlineColor = color;
            material.outlineWidth = width;
            return material;
        }
    }

    struct Lighting
    {
        // World direction pointing at the key light; need not be normalised.
        scene::Vector3 towardsKey{ 0.45f, 0.3f, 1.0f };

        // Puts the key at the eye instead, so the lit side always faces the viewer.
        bool headlight{ false };

        // A weak light from the eye so faces turned away from the key are shaded, not black.
        float fill{ 0.25f };
    };
}
