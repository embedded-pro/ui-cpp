// ui.shell is a pair of pure interfaces, so this translation unit defines nothing. It exists to
// give the static library an object file, and to fail the build if either header stops compiling
// on its own.
#include "ui/shell/AppShell.hpp"
#include "ui/shell/FormView.hpp"

namespace ui::shell
{
    static_assert(sizeof(ShellSpec) > 0);
    static_assert(sizeof(PageSpec) > 0);
}
