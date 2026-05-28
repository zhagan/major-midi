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

void MediaLibrary::ScanDirRecursive(const char* base_path,
                                    const char* relative_path,
                                    const char* ext,
                                    char        dest[][kPathMax],
                                    size_t&     count,
                                    size_t      max_count)
{
    if(count >= max_count)
        return;

    char full_path[kPathMax + 16]{};
    BuildFullPath(base_path, relative_path, full_path, sizeof(full_path));

    DIR     dir;
    FILINFO fno;
    if(f_opendir(&dir, full_path) != FR_OK)
        return;

    while(f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != '\0')
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

            ScanDirRecursive(base_path,
                             child_relative,
                             ext,
                             dest,
                             count,
                             max_count);
            if(count >= max_count)
                break;
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

        const size_t copy_len = std::strlen(relative_file) < (kPathMax - 1)
                                    ? std::strlen(relative_file)
                                    : (kPathMax - 1);
        std::memcpy(dest[count], relative_file, copy_len);
        dest[count][copy_len] = '\0';
        count++;
        if(count >= max_count)
            break;
    }

    f_closedir(&dir);
}

size_t MediaLibrary::FindFileIndex(const char* path,
                                   const char  file_paths[][kPathMax],
                                   size_t      file_count) const
{
    if(path == nullptr || path[0] == '\0')
        return file_count;

    for(size_t i = 0; i < file_count; i++)
    {
        if(std::strncmp(file_paths[i], path, kPathMax) == 0)
            return i;
    }

    return file_count;
}

void MediaLibrary::RefreshBrowser(const char* base_path,
                                  const char* ext,
                                  char*       current_dir,
                                  BrowserDirEntry entries[],
                                  size_t&     count,
                                  const char  file_paths[][kPathMax],
                                  size_t      file_count)
{
    count = 0;

    if(current_dir == nullptr || entries == nullptr)
        return;

    if(current_dir[0] != '\0' && count < kMaxBrowserEntries)
    {
        entries[count].is_up  = true;
        entries[count].is_dir = true;
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
        if(count >= kMaxBrowserEntries)
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
        if(count >= kMaxBrowserEntries)
            break;
        if(IsHiddenName(fno.fname) || (fno.fattrib & AM_DIR) != 0
           || !HasExtCaseInsensitive(fno.fname, ext))
            continue;

        char relative_file[kPathMax]{};
        if(current_dir[0] != '\0')
            std::snprintf(
                relative_file, sizeof(relative_file), "%s/%s", current_dir, fno.fname);
        else
            std::snprintf(relative_file, sizeof(relative_file), "%s", fno.fname);

        const size_t file_index = FindFileIndex(relative_file, file_paths, file_count);
        if(file_index >= file_count)
            continue;

        BrowserDirEntry& entry = entries[count];
        entry                  = BrowserDirEntry{};
        entry.index            = file_index;
        CopyTrunc(entry.name, sizeof(entry.name), fno.fname);
        count++;
    }

    f_closedir(&dir);
}

bool MediaLibrary::BrowserSelect(const char* base_path,
                                 const char* ext,
                                 char*       current_dir,
                                 BrowserDirEntry entries[],
                                 size_t&     count,
                                 size_t      cursor,
                                 size_t&     selected_index,
                                 const char  file_paths[][kPathMax],
                                 size_t      file_count)
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
        RefreshBrowser(base_path,
                       ext,
                       current_dir,
                       entries,
                       count,
                       file_paths,
                       file_count);
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
        RefreshBrowser(base_path,
                       ext,
                       current_dir,
                       entries,
                       count,
                       file_paths,
                       file_count);
        return false;
    }

    selected_index = entry.index;
    return true;
}

void MediaLibrary::Scan()
{
    midi_count_ = 0;
    sf2_count_  = 0;
    ScanDirRecursive("0:/midi", "", "mid", midi_files_, midi_count_, kMaxMidiFiles);
    ScanDirRecursive(
        "0:/soundfonts", "", "sf2", sf2_files_, sf2_count_, kMaxSoundFonts);
    ResetMidiBrowser();
    ResetSoundFontBrowser();
}

const char* MediaLibrary::MidiName(size_t index) const
{
    return index < midi_count_ ? midi_files_[index] : "";
}

const char* MediaLibrary::SoundFontName(size_t index) const
{
    return index < sf2_count_ ? sf2_files_[index] : "";
}

size_t MediaLibrary::FindMidiByName(const char* name) const
{
    return FindFileIndex(name, midi_files_, midi_count_);
}

size_t MediaLibrary::FindSoundFontByName(const char* name) const
{
    return FindFileIndex(name, sf2_files_, sf2_count_);
}

void MediaLibrary::BuildMidiPath(size_t index, char* out, size_t out_sz) const
{
    if(out_sz == 0)
        return;
    if(index >= midi_count_)
    {
        out[0] = '\0';
        return;
    }
    BuildFullPath("0:/midi", midi_files_[index], out, out_sz);
}

void MediaLibrary::BuildSoundFontPath(size_t index, char* out, size_t out_sz) const
{
    if(out_sz == 0)
        return;
    if(index >= sf2_count_)
    {
        out[0] = '\0';
        return;
    }
    BuildFullPath("0:/soundfonts", sf2_files_[index], out, out_sz);
}

void MediaLibrary::ResetMidiBrowser()
{
    midi_browser_dir_[0] = '\0';
    RefreshBrowser("0:/midi",
                   "mid",
                   midi_browser_dir_,
                   midi_browser_entries_,
                   midi_browser_count_,
                   midi_files_,
                   midi_count_);
}

void MediaLibrary::ResetSoundFontBrowser()
{
    sf2_browser_dir_[0] = '\0';
    RefreshBrowser("0:/soundfonts",
                   "sf2",
                   sf2_browser_dir_,
                   sf2_browser_entries_,
                   sf2_browser_count_,
                   sf2_files_,
                   sf2_count_);
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

bool MediaLibrary::MidiBrowserSelect(size_t cursor, size_t& selected_index)
{
    return BrowserSelect("0:/midi",
                         "mid",
                         midi_browser_dir_,
                         midi_browser_entries_,
                         midi_browser_count_,
                         cursor,
                         selected_index,
                         midi_files_,
                         midi_count_);
}

bool MediaLibrary::SoundFontBrowserSelect(size_t cursor, size_t& selected_index)
{
    return BrowserSelect("0:/soundfonts",
                         "sf2",
                         sf2_browser_dir_,
                         sf2_browser_entries_,
                         sf2_browser_count_,
                         cursor,
                         selected_index,
                         sf2_files_,
                         sf2_count_);
}

BrowserEntry MediaLibrary::BrowserEntryAt(size_t cursor) const
{
    BrowserEntry entry{};
    if(cursor == 0)
    {
        entry.kind = BrowserEntryKind::FxSettings;
        return entry;
    }

    if(cursor == 1)
    {
        entry.kind = BrowserEntryKind::SongSettings;
        return entry;
    }

    cursor -= 2;
    if(cursor < midi_count_)
    {
        entry.kind  = BrowserEntryKind::Midi;
        entry.index = cursor;
        return entry;
    }

    entry.kind  = BrowserEntryKind::SoundFont;
    entry.index = cursor >= midi_count_ ? (cursor - midi_count_) : 0;
    if(entry.index >= sf2_count_)
        entry.index = sf2_count_ > 0 ? (sf2_count_ - 1) : 0;
    return entry;
}

} // namespace major_midi
