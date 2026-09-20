#pragma once

#include <array>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace ui
{
    // A fixed-capacity std::function replacement. Deliberately not emil's infra::Function: Tier 1
    // carries no external dependency, and the four consumers pin three different emil revisions.
    template<class Signature, std::size_t Capacity = 4 * sizeof(void*)>
    class Callback;

    template<class Result, class... Arguments, std::size_t Capacity>
    class Callback<Result(Arguments...), Capacity>
    {
    public:
        Callback() = default;

        template<class Invocable>
        requires(!std::is_same_v<std::decay_t<Invocable>, Callback>)
        Callback(Invocable&& invocable)
        {
            Assign(std::forward<Invocable>(invocable));
        }

        Callback(const Callback& other)
        {
            if (other.operations != nullptr)
                other.operations->copy(other.storage.data(), storage.data());

            operations = other.operations;
        }

        Callback& operator=(const Callback& other)
        {
            if (this != &other)
            {
                Reset();

                if (other.operations != nullptr)
                    other.operations->copy(other.storage.data(), storage.data());

                operations = other.operations;
            }

            return *this;
        }

        ~Callback()
        {
            Reset();
        }

        template<class Invocable>
        requires(!std::is_same_v<std::decay_t<Invocable>, Callback>)
        Callback& operator=(Invocable&& invocable)
        {
            Reset();
            Assign(std::forward<Invocable>(invocable));
            return *this;
        }

        explicit operator bool() const
        {
            return operations != nullptr;
        }

        Result operator()(Arguments... arguments) const
        {
            return operations->invoke(storage.data(), std::forward<Arguments>(arguments)...);
        }

        void Reset()
        {
            if (operations != nullptr)
                operations->destroy(storage.data());

            operations = nullptr;
        }

    private:
        struct Operations
        {
            Result (*invoke)(const std::byte* storage, Arguments...);
            void (*copy)(const std::byte* from, std::byte* to);
            void (*destroy)(std::byte* storage);
        };

        template<class Invocable>
        void Assign(Invocable&& invocable)
        {
            using Stored = std::decay_t<Invocable>;
            static_assert(sizeof(Stored) <= Capacity, "Callback target exceeds inline capacity; capture less");
            static_assert(alignof(Stored) <= alignof(std::max_align_t), "Callback target over-aligned");
            static_assert(std::is_copy_constructible_v<Stored>, "Callback target must be copy constructible");

            new (storage.data()) Stored{ std::forward<Invocable>(invocable) };

            static constexpr Operations stored{
                [](const std::byte* from, Arguments... arguments) -> Result
                {
                    return (*std::launder(reinterpret_cast<const Stored*>(from)))(std::forward<Arguments>(arguments)...);
                },
                [](const std::byte* from, std::byte* to)
                {
                    new (to) Stored{ *std::launder(reinterpret_cast<const Stored*>(from)) };
                },
                [](std::byte* target)
                {
                    std::launder(reinterpret_cast<Stored*>(target))->~Stored();
                }
            };

            operations = &stored;
        }

        alignas(std::max_align_t) std::array<std::byte, Capacity> storage{};
        const Operations* operations{ nullptr };
    };
}
