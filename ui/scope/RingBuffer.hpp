#pragma once

#include <cstddef>
#include <vector>

namespace ui::scope
{
    // Sized once, then never allocates: the oscilloscope pushes a sample per acquisition tick and
    // must not reach the allocator on that path.
    class RingBuffer
    {
    public:
        void Resize(std::size_t capacity);
        void Push(float value);
        void Clear();

        // Index 0 is the oldest retained sample, Count() - 1 the newest, whether or not the
        // buffer has wrapped.
        [[nodiscard]] float At(std::size_t index) const;

        [[nodiscard]] std::size_t Count() const;
        [[nodiscard]] std::size_t Capacity() const;
        [[nodiscard]] bool IsFull() const;

    private:
        std::vector<float> data;
        std::size_t head{ 0 };
        std::size_t count{ 0 };
    };
}
