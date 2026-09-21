#include "ui/backend/recording/RecordingPaintedHost.hpp"

namespace ui::backend::recording
{
    RecordingPaintedHost::RecordingPaintedHost(PaintedView& hosted)
    {
        hosted.AttachHost(*this);
    }

    void RecordingPaintedHost::Invalidate()
    {
        ++invalidateCount;
    }

    void RecordingPaintedHost::OnViewDestroyed()
    {
        viewDestroyed = true;
    }

    std::size_t RecordingPaintedHost::InvalidateCount() const
    {
        return invalidateCount;
    }

    bool RecordingPaintedHost::ViewDestroyed() const
    {
        return viewDestroyed;
    }

    void RecordingPaintedHost::ClearInvalidateCount()
    {
        invalidateCount = 0;
    }
}
