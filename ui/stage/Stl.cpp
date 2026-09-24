#include "ui/stage/Stl.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>

namespace ui::stage
{
    namespace
    {
        static_assert(std::numeric_limits<float>::is_iec559, "binary STL stores IEEE-754 floats");

        constexpr std::size_t headerSize{ 80 };
        constexpr std::size_t countSize{ 4 };
        constexpr std::size_t triangleSize{ 50 };
        constexpr std::size_t normalSize{ 12 };
        constexpr float weldTolerance{ 1e-6f };

        using Corners = std::vector<Vector3>;

        [[nodiscard]] std::uint32_t ReadUint32(std::span<const std::byte> data, std::size_t offset)
        {
            std::uint32_t value{ 0 };

            for (std::size_t i = 0; i < 4; ++i)
                value |= static_cast<std::uint32_t>(data[offset + i]) << (8 * i);

            return value;
        }

        [[nodiscard]] float ReadFloat(std::span<const std::byte> data, std::size_t offset)
        {
            return std::bit_cast<float>(ReadUint32(data, offset));
        }

        [[nodiscard]] bool IsFinite(Vector3 value)
        {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        }

        [[nodiscard]] std::optional<std::uint32_t> BinaryTriangleCount(std::span<const std::byte> data)
        {
            if (data.size() < headerSize + countSize)
                return std::nullopt;

            const auto count = ReadUint32(data, headerSize);

            if (headerSize + countSize + std::uint64_t{ count } * triangleSize != data.size())
                return std::nullopt;

            return count;
        }

        [[nodiscard]] std::optional<Corners> ReadBinary(std::span<const std::byte> data, std::uint32_t count, const StlOptions& options)
        {
            if (count > options.maximumTriangles)
                return std::nullopt;

            Corners corners;
            corners.reserve(std::size_t{ count } * 3);

            for (std::size_t t = 0; t < count; ++t)
                for (std::size_t c = 0; c < 3; ++c)
                {
                    const auto offset = headerSize + countSize + t * triangleSize + normalSize + c * 12;
                    const Vector3 corner{ ReadFloat(data, offset), ReadFloat(data, offset + 4), ReadFloat(data, offset + 8) };

                    if (!IsFinite(corner))
                        return std::nullopt;

                    corners.push_back(corner * options.scale);
                }

            return corners;
        }

        [[nodiscard]] bool IsDigit(char c)
        {
            return c >= '0' && c <= '9';
        }

        [[nodiscard]] std::size_t ReadDigits(std::string_view token, std::size_t& position, double& value)
        {
            std::size_t digits{ 0 };

            for (; position < token.size() && IsDigit(token[position]); ++position, ++digits)
                value = value * 10.0 + (token[position] - '0');

            return digits;
        }

        [[nodiscard]] std::optional<int> ReadExponent(std::string_view token, std::size_t& position)
        {
            if (position == token.size() || (token[position] != 'e' && token[position] != 'E'))
                return 0;

            ++position;
            const auto negative = position < token.size() && token[position] == '-';

            if (position < token.size() && (token[position] == '-' || token[position] == '+'))
                ++position;

            double exponent{ 0.0 };

            if (ReadDigits(token, position, exponent) == 0 || exponent > 400.0)
                return std::nullopt;

            return negative ? -static_cast<int>(exponent) : static_cast<int>(exponent);
        }

        // Hand-written because std::from_chars for float is missing from older libc++, which
        // would break the macOS build. Decimal only: STL has no hex floats, infinities or NaNs.
        [[nodiscard]] std::optional<float> ParseFloat(std::string_view token)
        {
            std::size_t position{ 0 };
            const auto negative = !token.empty() && token[0] == '-';

            if (!token.empty() && (token[0] == '-' || token[0] == '+'))
                ++position;

            double mantissa{ 0.0 };
            auto digits = ReadDigits(token, position, mantissa);
            std::size_t fractionDigits{ 0 };

            if (position < token.size() && token[position] == '.')
            {
                ++position;
                fractionDigits = ReadDigits(token, position, mantissa);
                digits += fractionDigits;
            }

            const auto exponent = ReadExponent(token, position);

            if (digits == 0 || !exponent.has_value() || position != token.size())
                return std::nullopt;

            const auto value = mantissa * std::pow(10.0, *exponent - static_cast<int>(fractionDigits));

            return static_cast<float>(negative ? -value : value);
        }

        class Tokens
        {
        public:
            explicit Tokens(std::string_view text)
                : text(text)
            {}

            [[nodiscard]] bool AtEnd()
            {
                while (position < text.size() && IsSpace(text[position]))
                    ++position;

                return position == text.size();
            }

            [[nodiscard]] std::string_view Next()
            {
                const auto start = AtEnd() ? text.size() : position;

                while (position < text.size() && !IsSpace(text[position]))
                    ++position;

                return text.substr(start, position - start);
            }

        private:
            [[nodiscard]] static bool IsSpace(char c)
            {
                return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
            }

            std::string_view text;
            std::size_t position{ 0 };
        };

        [[nodiscard]] std::optional<Vector3> ReadVertex(Tokens& tokens)
        {
            const auto x = ParseFloat(tokens.Next());
            const auto y = ParseFloat(tokens.Next());
            const auto z = ParseFloat(tokens.Next());

            if (!x.has_value() || !y.has_value() || !z.has_value())
                return std::nullopt;

            return Vector3{ *x, *y, *z };
        }

        // Only the vertex records matter: facet normals are recomputed, and the loop keywords
        // carry nothing a triple of vertices does not.
        [[nodiscard]] std::optional<Corners> ReadAscii(std::string_view text, const StlOptions& options)
        {
            Tokens tokens{ text };

            if (tokens.Next() != "solid")
                return std::nullopt;

            Corners corners;

            while (!tokens.AtEnd())
            {
                if (tokens.Next() != "vertex")
                    continue;

                const auto vertex = ReadVertex(tokens);

                if (!vertex || !IsFinite(*vertex) || corners.size() >= options.maximumTriangles * 3)
                    return std::nullopt;

                corners.push_back(*vertex * options.scale);
            }

            if (corners.empty() || corners.size() % 3 != 0)
                return std::nullopt;

            return corners;
        }

        [[nodiscard]] float WeldStep(const Corners& corners)
        {
            Vector3 low = corners.front();
            Vector3 high = corners.front();

            for (const auto& corner : corners)
            {
                low = Vector3{ std::min(low.x, corner.x), std::min(low.y, corner.y), std::min(low.z, corner.z) };
                high = Vector3{ std::max(high.x, corner.x), std::max(high.y, corner.y), std::max(high.z, corner.z) };
            }

            const auto diagonal = scene::Length(high - low);

            return diagonal > 0.0f ? diagonal * weldTolerance : 1.0f;
        }

        [[nodiscard]] std::vector<std::uint32_t> Weld(const Corners& corners, Mesh& mesh)
        {
            using Key = std::array<std::int64_t, 3>;

            const auto step = WeldStep(corners);
            const auto quantise = [step](float value)
            {
                return static_cast<std::int64_t>(std::llround(value / step));
            };

            std::vector<std::pair<Key, std::uint32_t>> keyed;
            keyed.reserve(corners.size());

            for (std::uint32_t i = 0; i < corners.size(); ++i)
                keyed.emplace_back(Key{ quantise(corners[i].x), quantise(corners[i].y), quantise(corners[i].z) }, i);

            std::ranges::sort(keyed);

            std::vector<std::uint32_t> remap(corners.size());

            for (std::size_t i = 0; i < keyed.size(); ++i)
            {
                if (i == 0 || keyed[i].first != keyed[i - 1].first)
                    mesh.AddVertex(corners[keyed[i].second]);

                remap[keyed[i].second] = static_cast<std::uint32_t>(mesh.vertices.size() - 1);
            }

            return remap;
        }

        [[nodiscard]] std::vector<std::uint32_t> KeepAll(const Corners& corners, Mesh& mesh)
        {
            std::vector<std::uint32_t> remap(corners.size());

            for (std::uint32_t i = 0; i < corners.size(); ++i)
                remap[i] = mesh.AddVertex(corners[i]);

            return remap;
        }

        [[nodiscard]] Mesh BuildMesh(const Corners& corners, const StlOptions& options)
        {
            Mesh mesh;
            mesh.vertices.reserve(corners.size());
            mesh.faces.reserve(corners.size() / 3);

            const auto remap = options.weld ? Weld(corners, mesh) : KeepAll(corners, mesh);

            for (std::size_t i = 0; i + 2 < remap.size(); i += 3)
            {
                const auto a = remap[i];
                const auto b = remap[i + 1];
                const auto c = remap[i + 2];

                if (a != b && b != c && a != c)
                    mesh.AddTriangle(a, b, c);
            }

            mesh.Finish(options.featureAngleDegrees);
            return mesh;
        }
    }

    std::optional<Mesh> ParseStl(std::span<const std::byte> data, const StlOptions& options)
    {
        const auto binaryCount = BinaryTriangleCount(data);

        const auto corners = binaryCount
                                 ? ReadBinary(data, *binaryCount, options)
                                 : ReadAscii(std::string_view{ reinterpret_cast<const char*>(data.data()), data.size() }, options);

        if (!corners || corners->empty())
            return std::nullopt;

        return BuildMesh(*corners, options);
    }

    std::optional<Mesh> LoadStlFile(std::string_view path, const StlOptions& options)
    {
        std::ifstream file{ std::string{ path }, std::ios::binary };

        if (!file)
            return std::nullopt;

        const std::vector<char> bytes{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };

        return ParseStl(std::as_bytes(std::span{ bytes }), options);
    }
}
