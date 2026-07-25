#pragma once

#include <cstddef>
#include <cstdint>

namespace major_midi
{

class MediaLibrary
{
  public:
    static constexpr size_t kNameMax              = 32;
    static constexpr size_t kPathMax              = 96;
    static constexpr size_t kMaxMidiBrowserEntries = 128;
    static constexpr size_t kMaxSf2BrowserEntries  = 32;

    void Scan();

    bool MidiFileExists(const char* relative_path) const;
    bool SoundFontFileExists(const char* relative_path) const;
    bool FindFirstMidiFile(char* out_relative_path, size_t out_sz) const;
    bool FindFirstSoundFont(char* out_relative_path, size_t out_sz) const;

    void BuildMidiPath(const char* relative_path, char* out, size_t out_sz) const;
    void BuildSoundFontPath(const char* relative_path, char* out, size_t out_sz) const;

    void        ResetMidiBrowser();
    void        ResetSoundFontBrowser();
    size_t      MidiBrowserCount() const;
    size_t      SoundFontBrowserCount() const;
    const char* MidiBrowserName(size_t index) const;
    const char* SoundFontBrowserName(size_t index) const;
    bool        MidiBrowserIsDirectory(size_t index) const;
    bool        SoundFontBrowserIsDirectory(size_t index) const;
    bool        MidiBrowserSelect(size_t cursor, char* out_relative_path, size_t out_sz);
    bool        SoundFontBrowserSelect(size_t cursor, char* out_relative_path, size_t out_sz);

    // Live, on-demand directory browsing for the SysEx remote control. Independent
    // of the front-panel browser's own cursor/cache above, since a connected web
    // remote may be looking at a different folder than what's on the OLED.
    size_t MidiDirEntryCount(const char* dir_relative_path) const;
    bool   MidiDirEntryAt(const char* dir_relative_path,
                          size_t      index,
                          bool&       out_is_dir,
                          char*       out_name,
                          size_t      out_name_sz) const;
    size_t SoundFontDirEntryCount(const char* dir_relative_path) const;
    bool   SoundFontDirEntryAt(const char* dir_relative_path,
                              size_t      index,
                              bool&       out_is_dir,
                              char*       out_name,
                              size_t      out_name_sz) const;

  private:
    struct BrowserDirEntry
    {
        bool   is_up   = false;
        bool   is_dir  = false;
        char   name[kNameMax]{};
        char   relative_path[kPathMax]{}; // only meaningful when !is_dir && !is_up
    };

    bool HasExtCaseInsensitive(const char* name, const char* ext) const;
    bool IsHiddenName(const char* name) const;
    bool FindFirstRecursive(const char* base_path,
                            const char* relative_path,
                            const char* ext,
                            char*       out_relative_path,
                            size_t      out_sz) const;
    void RefreshBrowser(const char*      base_path,
                        const char*      ext,
                        char*            current_dir,
                        BrowserDirEntry  entries[],
                        size_t&          count,
                        size_t           max_count);
    void BuildFullPath(const char* base_path,
                       const char* relative_path,
                       char*       out,
                       size_t      out_sz) const;
    const char* LeafName(const char* path) const;
    bool BrowserSelect(const char*     base_path,
                       const char*     ext,
                       char*           current_dir,
                       BrowserDirEntry entries[],
                       size_t&         count,
                       size_t          max_count,
                       size_t          cursor,
                       char*           out_relative_path,
                       size_t          out_sz);
    size_t DirEntryCount(const char* base_path,
                        const char* ext,
                        const char* dir_relative_path) const;
    bool   DirEntryAt(const char* base_path,
                      const char* ext,
                      const char* dir_relative_path,
                      size_t      index,
                      bool&       out_is_dir,
                      char*       out_name,
                      size_t      out_name_sz) const;

    char   midi_browser_dir_[kPathMax]{};
    char   sf2_browser_dir_[kPathMax]{};
    BrowserDirEntry midi_browser_entries_[kMaxMidiBrowserEntries]{};
    BrowserDirEntry sf2_browser_entries_[kMaxSf2BrowserEntries]{};
    size_t midi_browser_count_ = 0;
    size_t sf2_browser_count_  = 0;
};

} // namespace major_midi
