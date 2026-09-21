#include "ui/scope/RingBuffer.hpp"
#include <algorithm>

namespace ui::scope
{
    void RingBuffer::Resize(std::size_t capacity)
    {
        data.assign(capacity, 0.0f);
        head = 0;
        count = 0;
    }

    void RingBuffer::Push(float value)
    {
        if (data.empty())
            return;

        data[head] = value;
        head = (head + 1) % data.size();

        if (count < data.size())
            ++count;
    }

    void RingBuffer::Clear()
    {
        std::ranges::fill(data, 0.0f);
        head = 0;
        count = 0;
    }

    float RingBuffer::At(std::size_t index) const
    {
        if (index >= count)
            return 0.0f;

        if (count < data.size())
            return data[index];

        return data[(head + index) % data.size()];
    }

    std::size_t RingBuffer::Count() const
    {
        return count;
    }

    std::size_t RingBuffer::Capacity() const
    {
        return data.size();
    }

    bool RingBuffer::IsFull() const
    {
        return !data.empty() && count == data.size();
    }
}
