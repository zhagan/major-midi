#include "media_library.h"

#include <cctype>
#include <cstdio>
#include <cstring>

extern "C"
{
#include "ff.h"
}

namespace major_midi
{

namespace
{
void CopyTrunc(char* dst, size_t dst_sz, const char* src)
{
    if(dst == nullptr || dst_sz == 0)
        return;

    dst[0] = '\0';
    if(src == nullptr)
        return;

    const size_t len = std::strlen(src);
    const size_t copy_len = len < (dst_sz - 1) ? len : (dst_sz - 1);
    std::memcpy(dst, src, copy_len);
    dst[copy_len] = '\0';
}
} // namespace

bool MediaLibrary::HasExtCaseInsensitive(const char* name, const char* ext) const
{
    if(!name || !ext)
        return false;

    const char* dot = std::strrchr(name, '.');
    if(!dot || dot[1] == '\0')
        return false;

    dot++;
    while(*dot && *ext)
    {
        if(std::tolower(static_cast<unsigned char>(*dot))
           != std::tolower(static_cast<unsigned char>(*ext)))
            return false;
        dot++;
        ext++;
    }

    return *dot == '\0' && *ext == '\0';
}

bool MediaLibrary::IsHiddenName(const char* name) const
{
    if(name == nullptr || name[0] == '\0')
        return true;

    return name[0] == '_' || name[0] == '.'
           || (name[0] == '.' && name[1] == '_');
}

const char* MediaLibrary::LeafName(const char* path) const
{
    if(path == nullptr)
        return "";

    const char* slash = std::strrchr(path, '/');
    return slash != nullptr ? (slash + 1) : path;
}

void MediaLibrary::BuildFullPath(const char* base_path,
                                 const char* relative_path,
                                 char*       out,
                                 size_t      out_sz) const
{
    if(out == nullptr || out_sz == 0)
        return;

    out[0] = '\0';
    if(base_path == nullptr || base_path[0] == '\0')
        return;

    if(relative_path == nullptr || relative_path[0] == '\0')
        std::snprintf(out, out_sz, "%s", base_path);
    else
        std::snprintf(out, out_sz, "%s/%s", base_path, relative_path);
}

bool MediaLibrary::FindFirstRecursive(const char* base_path,
                                      const char* relative_path,
                                      const char* ext,
                                      char*       out_relative_path,
                                      size_t      out_sz) const
{
    char full_path[kPathMax + 16]{};
    BuildFullPath(base_path, relative_path, full_path, sizeof(full_path));

    DIR     dir;
    FILINFO fno;
    if(f_opendir(&dir, full_path) != FR_OK)
        return false;

    bool found = false;
    while(!found && f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != '\0')
    {
        if(IsHiddenName(fno.fname))
            continue;

        if(fno.fattrib & AM_DIR)
        {
            char child_relative[kPathMax]{};
            if(relative_path != nullptr && relative_path[0] != '\0')
                std::snprintf(
                    child_relative, sizeof(child_relative), "%s/%s", relative_path, fno.fname);
            else
                std::snprintf(child_relative, sizeof(child_relative), "%s", fno.fname);

            if(FindFirstRecursive(base_path, child_relative, ext, out_relative_path, out_sz))
                found = true;
            continue;
        }

        if(!HasExtCaseInsensitive(fno.fname, ext))
            continue;

        char relative_file[kPathMax]{};
        if(relative_path != nullptr && relative_path[0] != '\0')
            std::snprintf(
                relative_file, sizeof(relative_file), "%s/%s", relative_path, fno.fname);
        else
            std::snprintf(relative_file, sizeof(relative_file), "%s", fno.fname);

        CopyTrunc(out_relative_path, out_sz, relative_file);
        found = true;
    }

    f_closedir(&dir);
    return found;
}

size_t MediaLibrary::DirEntryCount(const char* base_path,
                                   const char* ext,
                                   const char* dir_relative_path) const
{
    char full_path[kPathMax + 16]{};
    BuildFullPath(base_path, dir_relative_path, full_path, sizeof(full_path));

    DIR     dir;
    FILINFO fno;
    if(f_opendir(&dir, full_path) != FR_OK)
        return 0;

    size_t count = 0;
    while(f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != '\0')
    {
        if(IsHiddenName(fno.fname))
            continue;
        if(fno.fattrib & AM_DIR)
            count++;
        else if(HasExtCaseInsensitive(fno.fname, ext))
            count++;
    }

    f_closedir(&dir);
    return count;
}

bool MediaLibrary::DirEntryAt(const char* base_path,
                              const char* ext,
                              const char* dir_relative_path,
                              size_t      index,
                              bool&       out_is_dir,
                              char*       out_name,
                              size_t      out_name_sz) const
{
    char full_path[kPathMax + 16]{};
    BuildFullPath(base_path, dir_relative_path, full_path, sizeof(full_path));

    DIR     dir;
    FILINFO fno;
    if(f_opendir(&dir, full_path) != FR_OK)
        return false;

    bool   found = false;
    size_t seen  = 0;

    // Directories first, then extension-matched files -- same order the
    // front-panel browser uses (RefreshBrowser), so index N means the same
    // entry in both places.
    while(f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != '\0')
    {
        if(IsHiddenName(fno.fname) || (fno.fattrib & AM_DIR) == 0)
            continue;
        if(seen == index)
        {
            out_is_dir = true;
            CopyTrunc(out_name, out_name_sz, fno.fname);
            found = true;
            break;
        }
        seen++;
    }

    if(!found)
    {
        f_rewinddir(&dir);
        while(f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != '\0')
        {
            if(IsHiddenName(fno.fname) || (fno.fattrib & AM_DIR) != 0
               || !HasExtCaseInsensitive(fno.fname, ext))
                continue;
            if(seen == index)
            {
                out_is_dir = false;
                CopyTrunc(out_name, out_name_sz, fno.fname);
                found = true;
                break;
            }
            seen++;
        }
    }

    f_closedir(&dir);
    return found;
}

void MediaLibrary::RefreshBrowser(const char*     base_path,
                                  const char*     ext,
                                  char*           current_dir,
                                  BrowserDirEntry entries[],
                                  size_t&         count,
                                  size_t          max_count)
{
    count = 0;

    if(current_dir == nullptr || entries == nullptr)
        return;

    if(current_dir[0] != '\0' && count < max_count)
    {
        entries[count]         = BrowserDirEntry{};
        entries[count].is_up   = true;
        entries[count].is_dir  = true;
        std::snprintf(entries[count].name, sizeof(entries[count].name), "[..]");
        count++;
    }

    char full_path[kPathMax + 16]{};
    BuildFullPath(base_path, current_dir, full_path, sizeof(full_path));

    DIR     dir;
    FILINFO fno;
    if(f_opendir(&dir, full_path) != FR_OK)
        return;

    while(f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != '\0')
    {
        if(count >= max_count)
            break;
        if(IsHiddenName(fno.fname) || (fno.fattrib & AM_DIR) == 0)
            continue;

        BrowserDirEntry& entry = entries[count];
        entry                  = BrowserDirEntry{};
        entry.is_dir           = true;
        CopyTrunc(entry.name, sizeof(entry.name), fno.fname);
        count++;
    }

    f_rewinddir(&dir);
    while(f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != '\0')
    {
        if(count >= max_count)
            break;
        if(IsHiddenName(fno.fname) || (fno.fattrib & AM_DIR) != 0
           || !HasExtCaseInsensitive(fno.fname, ext))
            continue;

        BrowserDirEntry& entry = entries[count];
        entry                  = BrowserDirEntry{};
        CopyTrunc(entry.name, sizeof(entry.name), fno.fname);
        if(current_dir[0] != '\0')
            std::snprintf(entry.relative_path,
                          sizeof(entry.relative_path),
                          "%s/%s",
                          current_dir,
                          fno.fname);
        else
            std::snprintf(
                entry.relative_path, sizeof(entry.relative_path), "%s", fno.fname);
        count++;
    }

    f_closedir(&dir);
}

bool MediaLibrary::BrowserSelect(const char*     base_path,
                                 const char*     ext,
                                 char*           current_dir,
                                 BrowserDirEntry entries[],
                                 size_t&         count,
                                 size_t          max_count,
                                 size_t          cursor,
                                 char*           out_relative_path,
                                 size_t          out_sz)
{
    if(cursor >= count)
        return false;

    const BrowserDirEntry& entry = entries[cursor];
    if(entry.is_up)
    {
        char* slash = std::strrchr(current_dir, '/');
        if(slash != nullptr)
            *slash = '\0';
        else
            current_dir[0] = '\0';
        RefreshBrowser(base_path, ext, current_dir, entries, count, max_count);
        return false;
    }

    if(entry.is_dir)
    {
        if(current_dir[0] != '\0')
        {
            char next_dir[kPathMax]{};
            std::snprintf(next_dir, sizeof(next_dir), "%s/%s", current_dir, entry.name);
            std::snprintf(current_dir, kPathMax, "%s", next_dir);
        }
        else
        {
            std::snprintf(current_dir, kPathMax, "%s", entry.name);
        }
        RefreshBrowser(base_path, ext, current_dir, entries, count, max_count);
        return false;
    }

    if(out_relative_path != nullptr && out_sz > 0)
        CopyTrunc(out_relative_path, out_sz, entry.relative_path);
    return true;
}

void MediaLibrary::Scan()
{
    ResetMidiBrowser();
    ResetSoundFontBrowser();
}

bool MediaLibrary::MidiFileExists(const char* relative_path) const
{
    if(relative_path == nullptr || relative_path[0] == '\0')
        return false;
    char full_path[kPathMax + 16]{};
    BuildFullPath("0:/midi", relative_path, full_path, sizeof(full_path));
    FILINFO fno;
    return f_stat(full_path, &fno) == FR_OK && (fno.fattrib & AM_DIR) == 0;
}

bool MediaLibrary::SoundFontFileExists(const char* relative_path) const
{
    if(relative_path == nullptr || relative_path[0] == '\0')
        return false;
    char full_path[kPathMax + 16]{};
    BuildFullPath("0:/soundfonts", relative_path, full_path, sizeof(full_path));
    FILINFO fno;
    return f_stat(full_path, &fno) == FR_OK && (fno.fattrib & AM_DIR) == 0;
}

bool MediaLibrary::FindFirstMidiFile(char* out_relative_path, size_t out_sz) const
{
    if(out_relative_path == nullptr || out_sz == 0)
        return false;
    out_relative_path[0] = '\0';
    return FindFirstRecursive("0:/midi", "", "mid", out_relative_path, out_sz);
}

bool MediaLibrary::FindFirstSoundFont(char* out_relative_path, size_t out_sz) const
{
    if(out_relative_path == nullptr || out_sz == 0)
        return false;
    out_relative_path[0] = '\0';
    return FindFirstRecursive("0:/soundfonts", "", "sf2", out_relative_path, out_sz);
}

void MediaLibrary::BuildMidiPath(const char* relative_path, char* out, size_t out_sz) const
{
    if(out_sz == 0)
        return;
    if(relative_path == nullptr || relative_path[0] == '\0')
    {
        out[0] = '\0';
        return;
    }
    BuildFullPath("0:/midi", relative_path, out, out_sz);
}

void MediaLibrary::BuildSoundFontPath(const char* relative_path, char* out, size_t out_sz) const
{
    if(out_sz == 0)
        return;
    if(relative_path == nullptr || relative_path[0] == '\0')
    {
        out[0] = '\0';
        return;
    }
    BuildFullPath("0:/soundfonts", relative_path, out, out_sz);
}

void MediaLibrary::ResetMidiBrowser()
{
    midi_browser_dir_[0] = '\0';
    RefreshBrowser("0:/midi",
                   "mid",
                   midi_browser_dir_,
                   midi_browser_entries_,
                   midi_browser_count_,
                   kMaxMidiBrowserEntries);
}

void MediaLibrary::ResetSoundFontBrowser()
{
    sf2_browser_dir_[0] = '\0';
    RefreshBrowser("0:/soundfonts",
                   "sf2",
                   sf2_browser_dir_,
                   sf2_browser_entries_,
                   sf2_browser_count_,
                   kMaxSf2BrowserEntries);
}

size_t MediaLibrary::MidiBrowserCount() const
{
    return midi_browser_count_;
}

size_t MediaLibrary::SoundFontBrowserCount() const
{
    return sf2_browser_count_;
}

const char* MediaLibrary::MidiBrowserName(size_t index) const
{
    return index < midi_browser_count_ ? midi_browser_entries_[index].name : "";
}

const char* MediaLibrary::SoundFontBrowserName(size_t index) const
{
    return index < sf2_browser_count_ ? sf2_browser_entries_[index].name : "";
}

bool MediaLibrary::MidiBrowserIsDirectory(size_t index) const
{
    return index < midi_browser_count_ && midi_browser_entries_[index].is_dir;
}

bool MediaLibrary::SoundFontBrowserIsDirectory(size_t index) const
{
    return index < sf2_browser_count_ && sf2_browser_entries_[index].is_dir;
}

bool MediaLibrary::MidiBrowserSelect(size_t cursor, char* out_relative_path, size_t out_sz)
{
    return BrowserSelect("0:/midi",
                         "mid",
                         midi_browser_dir_,
                         midi_browser_entries_,
                         midi_browser_count_,
                         kMaxMidiBrowserEntries,
                         cursor,
                         out_relative_path,
                         out_sz);
}

bool MediaLibrary::SoundFontBrowserSelect(size_t cursor, char* out_relative_path, size_t out_sz)
{
    return BrowserSelect("0:/soundfonts",
                         "sf2",
                         sf2_browser_dir_,
                         sf2_browser_entries_,
                         sf2_browser_count_,
                         kMaxSf2BrowserEntries,
                         cursor,
                         out_relative_path,
                         out_sz);
}

size_t MediaLibrary::MidiDirEntryCount(const char* dir_relative_path) const
{
    return DirEntryCount("0:/midi", "mid", dir_relative_path);
}

bool MediaLibrary::MidiDirEntryAt(const char* dir_relative_path,
                                  size_t      index,
                                  bool&       out_is_dir,
                                  char*       out_name,
                                  size_t      out_name_sz) const
{
    return DirEntryAt(
        "0:/midi", "mid", dir_relative_path, index, out_is_dir, out_name, out_name_sz);
}

size_t MediaLibrary::SoundFontDirEntryCount(const char* dir_relative_path) const
{
    return DirEntryCount("0:/soundfonts", "sf2", dir_relative_path);
}

bool MediaLibrary::SoundFontDirEntryAt(const char* dir_relative_path,
                                      size_t      index,
                                      bool&       out_is_dir,
                                      char*       out_name,
                                      size_t      out_name_sz) const
{
    return DirEntryAt(
        "0:/soundfonts", "sf2", dir_relative_path, index, out_is_dir, out_name, out_name_sz);
}

} // namespace major_midi
