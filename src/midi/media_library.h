#pragma once

#include <cstddef>
#include <cstdint>

namespace major_midi
{

enum class BrowserEntryKind : uint8_t
{
    FxSettings,
    SongSettings,
    Midi,
    SoundFont,
};

struct BrowserEntry
{
    BrowserEntryKind kind  = BrowserEntryKind::Midi;
    size_t           index = 0;
};

class MediaLibrary
{
  public:
    static constexpr size_t kMaxMidiFiles  = 64;
    static constexpr size_t kMaxSoundFonts = 32;
    static constexpr size_t kNameMax       = 32;
    static constexpr size_t kPathMax       = 96;
    static constexpr size_t kMaxBrowserEntries = 64;

    void Scan();

    size_t MidiCount() const { return midi_count_; }
    size_t SoundFontCount() const { return sf2_count_; }
    size_t BrowserEntryCount() const { return 2 + midi_count_ + sf2_count_; }

    const char* MidiName(size_t index) const;
    const char* SoundFontName(size_t index) const;
    size_t      FindMidiByName(const char* name) const;
    size_t      FindSoundFontByName(const char* name) const;
    void        BuildMidiPath(size_t index, char* out, size_t out_sz) const;
    void        BuildSoundFontPath(size_t index, char* out, size_t out_sz) const;
    void        ResetMidiBrowser();
    void        ResetSoundFontBrowser();
    size_t      MidiBrowserCount() const;
    size_t      SoundFontBrowserCount() const;
    const char* MidiBrowserName(size_t index) const;
    const char* SoundFontBrowserName(size_t index) const;
    bool        MidiBrowserIsDirectory(size_t index) const;
    bool        SoundFontBrowserIsDirectory(size_t index) const;
    bool        MidiBrowserSelect(size_t cursor, size_t& selected_index);
    bool        SoundFontBrowserSelect(size_t cursor, size_t& selected_index);

    BrowserEntry BrowserEntryAt(size_t cursor) const;

  private:
    struct BrowserDirEntry
    {
        bool   is_up   = false;
        bool   is_dir  = false;
        size_t index   = 0;
        char   name[kNameMax]{};
    };

    bool HasExtCaseInsensitive(const char* name, const char* ext) const;
    bool IsHiddenName(const char* name) const;
    void ScanDirRecursive(const char* base_path,
                          const char* relative_path,
                          const char* ext,
                          char        dest[][kPathMax],
                          size_t&     count,
                          size_t      max_count);
    void RefreshBrowser(const char* base_path,
                        const char* ext,
                        char*       current_dir,
                        BrowserDirEntry entries[],
                        size_t&     count,
                        const char  file_paths[][kPathMax],
                        size_t      file_count);
    size_t FindFileIndex(const char* path,
                         const char  file_paths[][kPathMax],
                         size_t      file_count) const;
    void BuildFullPath(const char* base_path,
                       const char* relative_path,
                       char*       out,
                       size_t      out_sz) const;
    const char* LeafName(const char* path) const;
    bool BrowserSelect(const char* base_path,
                       const char* ext,
                       char*       current_dir,
                       BrowserDirEntry entries[],
                       size_t&     count,
                       size_t      cursor,
                       size_t&     selected_index,
                       const char  file_paths[][kPathMax],
                       size_t      file_count);

    char   midi_files_[kMaxMidiFiles][kPathMax]{};
    char   sf2_files_[kMaxSoundFonts][kPathMax]{};
    size_t midi_count_ = 0;
    size_t sf2_count_  = 0;
    char   midi_browser_dir_[kPathMax]{};
    char   sf2_browser_dir_[kPathMax]{};
    BrowserDirEntry midi_browser_entries_[kMaxBrowserEntries]{};
    BrowserDirEntry sf2_browser_entries_[kMaxBrowserEntries]{};
    size_t midi_browser_count_ = 0;
    size_t sf2_browser_count_  = 0;
};

} // namespace major_midi
