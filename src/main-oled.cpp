#include "daisy_patch_sm.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;
using namespace patch_sm;

DaisyPatchSM hw;
using Disp = OledDisplay<SSD130x4WireSoftSpi128x64Driver>;
Disp display;

int main(void)
{
    hw.Init();

    // --- MANUAL OLED RESET PULSE ---
    const Pin rst_pin = DaisyPatchSM::A2; // <-- your wired RESET pin
    GPIO      rst;
    rst.Init(rst_pin, GPIO::Mode::OUTPUT, GPIO::Pull::NOPULL, GPIO::Speed::LOW);

    rst.Write(false);
    System::Delay(50);
    rst.Write(true);
    System::Delay(100);
    // -------------------------------

    Disp::Config cfg;
    auto&        tcfg = cfg.driver_config.transport_config;

    tcfg.pin_config.sclk  = DaisyPatchSM::D10;
    tcfg.pin_config.mosi  = DaisyPatchSM::D9;
    tcfg.pin_config.dc    = DaisyPatchSM::D1; // keep DC on a solid digital pin
    tcfg.pin_config.reset = rst_pin;          // same pin as above

    display.Init(cfg);

    while(1)
    {
        display.Fill(false);
        display.SetCursor(0, 0);
        display.WriteString("SoftSPI OK", Font_6x8, true);
        display.Update();
        System::Delay(50);
    }
}