#pragma once

#include "app_state.h"
#include "daisy_patch_sm.h"
#include "dev/oled_ssd130x.h"
#include "media_library.h"

namespace major_midi
{

class UiRenderer
{
  public:
    using DisplayT = daisy::OledDisplay<daisy::SSD130xI2c128x64Driver>;

    void Init();
    void ShowSplash(bool chunked = false);
    void RenderScreenSaver(uint32_t now_ms, bool chunked = false);
    void SetDisplayXOffset(uint8_t offset);
    void Render(const AppState&    state,
                const MediaLibrary& library,
                uint32_t            now_ms,
                bool                chunked = false);
    void ServiceChunkedUpdate(uint8_t max_bytes = 16);
    bool IsChunkedUpdateActive() const;

  private:
    void Present(bool chunked);

    DisplayT display_;
    int      saver_x_              = 18;
    int      saver_y_              = 12;
    int      saver_dx_             = 1;
    int      saver_dy_             = 1;
    uint32_t saver_last_update_ms_ = 0;
};

} // namespace major_midi
