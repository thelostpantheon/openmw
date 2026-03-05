#ifndef OPENMW_VITA_VITAIME_H
#define OPENMW_VITA_VITAIME_H

#ifdef __vita__

#include <string>

namespace Vita
{
    /// Load the SCE IME sysmodule. Call once at startup.
    void initIme();

    /// Open a modal Vita IME dialog for text input.
    /// Blocks until the user confirms or cancels.
    /// @param title     Dialog title (UTF-8).
    /// @param initial   Pre-filled text (UTF-8), may be empty.
    /// @param maxLen    Maximum character count (clamped to 2048).
    /// @return Entered text as UTF-8, or empty string if cancelled.
    std::string openTextDialog(const char* title, const char* initial = "", int maxLen = 128);
}

#endif // __vita__
#endif // OPENMW_VITA_VITAIME_H
