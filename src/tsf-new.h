/* TinySoundFont - v0.9 - SoundFont2 synthesizer - https://github.com/schellingb/TinySoundFont
									 no warranty implied; use at your own risk
   Do this:
	  #define TSF_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.
   // i.e. it should look like this:
   #include ...
   #include ...
   #define TSF_IMPLEMENTATION
   #include "tsf.h"

   [OPTIONAL] #define TSF_NO_STDIO to remove stdio dependency
   [OPTIONAL] #define TSF_MALLOC, TSF_REALLOC, and TSF_FREE to avoid stdlib.h
   [OPTIONAL] #define TSF_MEMCPY, TSF_MEMSET to avoid string.h
   [OPTIONAL] #define TSF_POW, TSF_POWF, TSF_EXPF, TSF_LOG, TSF_TAN, TSF_LOG10, TSF_SQRT to avoid math.h

   NOT YET IMPLEMENTED
	 - Support for ChorusEffectsSend and ReverbEffectsSend generators
	 - Better low-pass filter without lowering performance too much

   LICENSE (MIT)

   Copyright (C) 2017-2025 Bernhard Schelling
   Based on SFZero, Copyright (C) 2012 Steve Folta (https://github.com/stevefolta/SFZero)

   Permission is hereby granted, free of charge, to any person obtaining a copy of this
   software and associated documentation files (the "Software"), to deal in the Software
   without restriction, including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
   to whom the Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef TSF_INCLUDE_TSF_INL
#define TSF_INCLUDE_TSF_INL

#ifdef __cplusplus
extern "C"
{
#define CPP_DEFAULT0 = 0
#else
#define CPP_DEFAULT0
#endif

//define this if you want the API functions to be static
#ifdef TSF_STATIC
#define TSFDEF static
#else
#define TSFDEF extern
#endif

    // The load functions will return a pointer to a struct tsf which all functions
    // thereafter take as the first parameter.
    // On error the tsf_load* functions will return NULL most likely due to invalid
    // data (or if the file did not exist in tsf_load_filename).
    typedef struct tsf tsf;

#ifndef TSF_NO_STDIO
    // Directly load a SoundFont from a .sf2 file path
    TSFDEF tsf* tsf_load_filename(const char* filename);
#endif

    // Load a SoundFont from a block of memory
    TSFDEF tsf* tsf_load_memory(const void* buffer, int size);

    // Debug
    TSFDEF void tsf_debug_wantlearn();
    TSFDEF void tsf_init_lut();

    // Stream structure for the generic loading
    struct tsf_stream
    {
        // Custom data given to the functions as the first parameter
        void* data;

        // Function pointer will be called to read 'size' bytes into ptr (returns number of read bytes)
        int (*read)(void* data, void* ptr, unsigned int size);

        // Function pointer will be called to skip ahead over 'count' bytes (returns 1 on success, 0 on error)
        int (*skip)(void* data, unsigned int count);
    };

    // Generic SoundFont loading method using the stream structure above
    TSFDEF tsf* tsf_load(struct tsf_stream* stream);

    // Copy a tsf instance from an existing one, use tsf_close to close it as well.
    // All copied tsf instances and their original instance are linked, and share the underlying soundfont.
    // This allows loading a soundfont only once, but using it for multiple independent playbacks.
    // (This function isn't thread-safe without locking.)
    TSFDEF tsf* tsf_copy(tsf* f);

    // Free the memory related to this tsf instance
    TSFDEF void tsf_close(tsf* f);

    // Stop all playing notes immediately and reset all channel parameters
    TSFDEF void tsf_reset(tsf* f);

    // Returns the preset index from a bank and preset number, or -1 if it does not exist in the loaded SoundFont
    TSFDEF int tsf_get_presetindex(const tsf* f, int bank, int preset_number);

    // Returns the number of presets in the loaded SoundFont
    TSFDEF int tsf_get_presetcount(const tsf* f);

    // Returns the name of a preset index >= 0 and < tsf_get_presetcount()
    TSFDEF const char* tsf_get_presetname(const tsf* f, int preset_index);

    // Returns the name of a preset by bank and preset number
    TSFDEF const char*
    tsf_bank_get_presetname(const tsf* f, int bank, int preset_number);

    // Supported output modes by the render methods:
    // TSF_STEREO_INTERLEAVED, TSF_STEREO_UNWEAVED, TSF_MONO.
    enum TSFOutputMode
    {
        // Two channels with single left/right samples one after another
        TSF_STEREO_INTERLEAVED,
        // Two channels with all samples for the left channel first then right
        TSF_STEREO_UNWEAVED,
        // A single channel (stereo instruments are mixed into center)
        TSF_MONO
    };

    // Thread safety:
    //
    // 1. Rendering / voices:
    //
    // Your audio output which calls the tsf_render* functions will most likely
    // run on a different thread than where the playback tsf_note* functions
    // are called. In which case some sort of concurrency control like a
    // mutex needs to be used so they are not called at the same time.
    // Alternatively, you can pre-allocate a maximum number of voices that can
    // play simultaneously by calling tsf_set_max_voices after loading.
    // That way memory re-allocation will not happen during tsf_note_on and
    // TSF should become mostly thread safe.
    // There is a theoretical chance that ending notes would negatively influence
    // a voice that is rendering at the time but it is hard to say.
    // Also be aware, this has not been tested much.
    //
    // 2. Channels:
    //
    // Calls to tsf_channel_set_... functions may allocate new channels
    // if no channel with that number was previously used. Make sure to
    // create all channels at the beginning as required if you call tsf_render*
    // from a different thread.

    // Setup the parameters for the voice render methods
    //   outputmode: if mono or stereo and how stereo channel data is ordered
    //   samplerate: the number of samples per second (output frequency)
    //   global_gain_db: volume gain in decibels (>0 means higher, <0 means lower)
    TSFDEF void tsf_set_output(tsf*                 f,
                               enum TSFOutputMode   outputmode,
                               int                  samplerate,
                               float global_gain_db CPP_DEFAULT0);

    // Set the global gain as a volume factor
    //   global_gain: the desired volume where 1.0 is 100%
    TSFDEF void tsf_set_volume(tsf* f, float global_gain);

    // Set the maximum number of voices to play simultaneously
    // Depending on the soundfond, one note can cause many new voices to be started,
    // so don't keep this number too low or otherwise sounds may not play.
    //   max_voices: maximum number to pre-allocate and set the limit to
    //   (tsf_set_max_voices returns 0 if allocation failed, otherwise 1)
    TSFDEF int tsf_set_max_voices(tsf* f, int max_voices);

    // Start playing a note
    //   preset_index: preset index >= 0 and < tsf_get_presetcount()
    //   key: note value between 0 and 127 (60 being middle C)
    //   vel: velocity as a float between 0.0 (equal to note off) and 1.0 (full)
    //   bank: instrument bank number (alternative to preset_index)
    //   preset_number: preset number (alternative to preset_index)
    //   (tsf_note_on returns 0 if the allocation of a new voice failed, otherwise 1)
    //   (tsf_bank_note_on returns 0 if preset does not exist or allocation failed, otherwise 1)
    TSFDEF int tsf_note_on(tsf* f, int preset_index, int key, float vel);
    TSFDEF int
    tsf_bank_note_on(tsf* f, int bank, int preset_number, int key, float vel);

    // Stop playing a note
    //   (bank_note_off returns 0 if preset does not exist, otherwise 1)
    TSFDEF void tsf_note_off(tsf* f, int preset_index, int key);
    TSFDEF int  tsf_bank_note_off(tsf* f, int bank, int preset_number, int key);

    // Stop playing all notes (end with sustain and release)
    TSFDEF void tsf_note_off_all(tsf* f);

    // Returns the number of active voices
    TSFDEF int tsf_active_voice_count(tsf* f);

    // Render output samples into a buffer
    // You can either render as signed 16-bit values (tsf_render_short) or
    // as 32-bit float values (tsf_render_float)
    //   buffer: target buffer of size samples * output_channels * sizeof(type)
    //   samples: number of samples to render
    //   flag_mixing: if 0 clear the buffer first, otherwise mix into existing data
    TSFDEF void tsf_render_short(tsf*            f,
                                 short*          buffer,
                                 int             samples,
                                 int flag_mixing CPP_DEFAULT0);
    TSFDEF void tsf_render_float(tsf*            f,
                                 float*          buffer,
                                 int             samples,
                                 int flag_mixing CPP_DEFAULT0);

    // Higher level channel based functions, set up channel parameters
    //   channel: channel number
    //   preset_index: preset index >= 0 and < tsf_get_presetcount()
    //   preset_number: preset number (alternative to preset_index)
    //   flag_mididrums: 0 for normal channels, otherwise apply MIDI drum channel rules
    //   bank: instrument bank number (alternative to preset_index)
    //   pan: stereo panning value from 0.0 (left) to 1.0 (right) (default 0.5 center)
    //   volume: linear volume scale factor (default 1.0 full)
    //   pitch_wheel: pitch wheel position 0 to 16383 (default 8192 unpitched)
    //   pitch_range: range of the pitch wheel in semitones (default 2.0, total +/- 2 semitones)
    //   tuning: tuning of all playing voices in semitones (default 0.0, standard (A440) tuning)
    //   flag_sustain: 0 to end notes that were held sustained and disable holding sustain otherwise enable it
    //   (tsf_set_preset_number and set_bank_preset return 0 if preset does not exist, otherwise 1)
    //   (tsf_channel_set_... return 0 if a new channel needed allocation and that failed, otherwise 1)
    TSFDEF int
    tsf_channel_set_presetindex(tsf* f, int channel, int preset_index);
    TSFDEF int tsf_channel_set_presetnumber(tsf*               f,
                                            int                channel,
                                            int                preset_number,
                                            int flag_mididrums CPP_DEFAULT0);
    TSFDEF int tsf_channel_set_bank(tsf* f, int channel, int bank);
    TSFDEF int tsf_channel_set_bank_preset(tsf* f,
                                           int  channel,
                                           int  bank,
                                           int  preset_number);
    TSFDEF int tsf_channel_set_pan(tsf* f, int channel, float pan);
    TSFDEF int tsf_channel_set_midipan(tsf* f, int channel, int pan);
    TSFDEF int tsf_channel_set_volume(tsf* f, int channel, float volume);
    TSFDEF int tsf_channel_set_midivolume(tsf* f, int channel, int volume);
    TSFDEF int tsf_channel_set_pitchwheel(tsf* f, int channel, int pitch_wheel);
    TSFDEF int
    tsf_channel_set_pitchrange(tsf* f, int channel, float pitch_range);
    TSFDEF int tsf_channel_set_tuning(tsf* f, int channel, float tuning);
    TSFDEF int
    tsf_channel_set_midifilter(tsf* f, int channel, short fc, short q);

    // Start or stop playing notes on a channel (needs channel preset to be set)
    //   channel: channel number
    //   key: note value between 0 and 127 (60 being middle C)
    //   vel: velocity as a float between 0.0 (equal to note off) and 1.0 (full)
    //   (tsf_channel_note_on returns 0 on allocation failure of new voice, otherwise 1)
    TSFDEF int  tsf_channel_note_on(tsf* f, int channel, int key, float vel);
    TSFDEF void tsf_channel_note_off(tsf* f, int channel, int key);
    TSFDEF void
                tsf_channel_note_off_all(tsf* f,
                                         int  channel); //end with sustain and release
    TSFDEF void tsf_channel_sounds_off_all(tsf* f,
                                           int  channel); //end immediately

    // Apply a MIDI control change to the channel (not all controllers are supported!)
    //    (tsf_channel_midi_control returns 0 on allocation failure of new channel, otherwise 1)
    TSFDEF int tsf_channel_midi_control(tsf* f,
                                        int  channel,
                                        int  controller,
                                        int  control_value);

    // Get current values set on the channels
    TSFDEF int tsf_channel_get_preset_index(tsf* f, int channel);
    TSFDEF int
    tsf_channel_get_preset_bank(tsf* f, int channel, int* flag CPP_DEFAULT0);
    TSFDEF int   tsf_channel_get_preset_number(tsf* f, int channel);
    TSFDEF float tsf_channel_get_pan(tsf* f, int channel);
    TSFDEF float tsf_channel_get_volume(tsf* f, int channel);
    TSFDEF int   tsf_channel_get_pitchwheel(tsf* f, int channel);
    TSFDEF float tsf_channel_get_pitchrange(tsf* f, int channel);
    TSFDEF float tsf_channel_get_tuning(tsf* f, int channel);

#ifdef __cplusplus
#undef CPP_DEFAULT0
}
#endif

// end header
// ---------------------------------------------------------------------------------------------------------
#endif //TSF_INCLUDE_TSF_INL

#ifdef TSF_IMPLEMENTATION
#undef TSF_IMPLEMENTATION

// The lower this block size is the more accurate the effects are.
// Increasing the value significantly lowers the CPU usage of the voice rendering.
// If LFO affects the low-pass filter it can be hearable even as low as 8.
#ifndef TSF_RENDER_EFFECTSAMPLEBLOCK
#define TSF_RENDER_EFFECTSAMPLEBLOCK 64
#endif

// When using tsf_render_short, to do the conversion a buffer of a fixed size is
// allocated on the stack. On low memory platforms this could be made smaller.
// Increasing this above 512 should not have a significant impact on performance.
// The value should be a multiple of TSF_RENDER_EFFECTSAMPLEBLOCK.
#ifndef TSF_RENDER_SHORTBUFFERBLOCK
#define TSF_RENDER_SHORTBUFFERBLOCK 512
#endif

// Grace release time for quick voice off (avoid clicking noise)
#define TSF_FASTRELEASETIME 0.01f

#if !defined(TSF_MALLOC) || !defined(TSF_FREE) || !defined(TSF_REALLOC)
#include <stdlib.h>
#define TSF_MALLOC malloc
#define TSF_FREE free
#define TSF_REALLOC realloc
#endif

#if !defined(TSF_MEMCPY) || !defined(TSF_MEMSET)
#include <string.h>
#define TSF_MEMCPY memcpy
#define TSF_MEMSET memset
#endif

#if !defined(TSF_POW) || !defined(TSF_POWF) || !defined(TSF_EXPF)    \
    || !defined(TSF_LOG) || !defined(TSF_TAN) || !defined(TSF_LOG10) \
    || !defined(TSF_SQRT)
#include <math.h>
#if !defined(__cplusplus) && !defined(NAN) && !defined(powf) && !defined(expf) \
    && !defined(sqrtf)
#define powf (float)pow   // deal with old math.h
#define expf (float)exp   // files that come without
#define sqrtf (float)sqrt // powf, expf and sqrtf
#endif
#define TSF_POW pow
#define TSF_POWF powf
#define TSF_EXPF expf
#define TSF_LOG log
#define TSF_TAN tan
#define TSF_LOG10 log10
#define TSF_SQRTF sqrtf
#endif

#ifndef TSF_NO_STDIO
#include <stdio.h>
#endif

#define TSF_TRUE 1
#define TSF_FALSE 0
#define TSF_BOOL unsigned char
#define TSF_PI 3.14159265358979323846264338327950288
#define TSF_NULL 0

#define TSF_CC74_AMOUNT 9600
#define TSF_CC71_AMOUNT 960

#ifdef DEBUG
#ifdef TSF_NO_STDIO
////crash on errors and warnings to find broken midi files while debugging
#define TSF_ERROR(msg, ...) *(int*)0 = 0xbad;
#define TSF_WARN(msg, ...) *(int*)0 = 0xf00d;
#else
////print errors and warnings
#define TSF_ERROR(msg, ...) fprintf(stderr, ("ERROR: " msg), ##__VA_ARGS__);
#define TSF_WARN(msg, ...) fprintf(stderr, ("WARNING: " msg), ##__VA_ARGS__);
#endif //TSF_NO_STDIO
#endif //NDEBUG

#ifndef TSF_ERROR
#define TSF_ERROR(msg, ...)
#endif

#ifndef TSF_WARN
#define TSF_WARN(msg, ...)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    typedef char           tsf_fourcc[4];
    typedef signed char    tsf_s8;
    typedef unsigned char  tsf_u8;
    typedef unsigned short tsf_u16;
    typedef signed short   tsf_s16;
    typedef unsigned int   tsf_u32;
    typedef char           tsf_char20[20];

#define TSF_FourCCEquals(value1, value2)              \
    (value1[0] == value2[0] && value1[1] == value2[1] \
     && value1[2] == value2[2] && value1[3] == value2[3])

    struct tsf
    {
        struct tsf_preset*   presets;
        float*               fontSamples;
        struct tsf_voice*    voices;
        struct tsf_channels* channels;

        int          presetNum;
        int          voiceNum;
        int          maxVoiceNum;
        unsigned int voicePlayIndex;

        enum TSFOutputMode outputmode;
        float              outSampleRate;
        float              globalGainDB;
        int*               refCount;

        unsigned int       fontSampleCount;
        struct tsf_sample* samples;
        int                sampleNum;
    };

//----------------------- loader
#ifndef TSF_NO_STDIO
    static int tsf_stream_stdio_read(FILE* f, void* ptr, unsigned int size)
    {
        return (int)fread(ptr, 1, size, f);
    }
    static int tsf_stream_stdio_skip(FILE* f, unsigned int count)
    {
        return !fseek(f, count, SEEK_CUR);
    }
    TSFDEF tsf* tsf_load_filename(const char* filename)
    {
        tsf*              res;
        struct tsf_stream stream
            = {TSF_NULL,
               (int (*)(void*, void*, unsigned int))&tsf_stream_stdio_read,
               (int (*)(void*, unsigned int))&tsf_stream_stdio_skip};
#if __STDC_WANT_SECURE_LIB__
        FILE* f = TSF_NULL;
        fopen_s(&f, filename, "rb");
#else
        FILE* f = fopen(filename, "rb");
#endif
        if(!f)
        {
            //if (e) *e = TSF_FILENOTFOUND;
            return TSF_NULL;
        }
        stream.data = f;
        res         = tsf_load(&stream);
        fclose(f);
        return res;
    }
#endif

    struct tsf_stream_memory
    {
        const char*  buffer;
        unsigned int total, pos;
    };
    static int tsf_stream_memory_read(struct tsf_stream_memory* m,
                                      void*                     ptr,
                                      unsigned int              size)
    {
        if(size > m->total - m->pos)
            size = m->total - m->pos;
        TSF_MEMCPY(ptr, m->buffer + m->pos, size);
        m->pos += size;
        return size;
    }
    static int tsf_stream_memory_skip(struct tsf_stream_memory* m,
                                      unsigned int              count)
    {
        if(m->pos + count > m->total)
            return 0;
        m->pos += count;
        return 1;
    }
    TSFDEF tsf* tsf_load_memory(const void* buffer, int size)
    {
        struct tsf_stream stream
            = {TSF_NULL,
               (int (*)(void*, void*, unsigned int))&tsf_stream_memory_read,
               (int (*)(void*, unsigned int))&tsf_stream_memory_skip};
        struct tsf_stream_memory f = {0, 0, 0};
        f.buffer                   = (const char*)buffer;
        f.total                    = size;
        stream.data                = &f;
        return tsf_load(&stream);
    }

    enum
    {
        TSF_LOOPMODE_NONE,
        TSF_LOOPMODE_CONTINUOUS,
        TSF_LOOPMODE_SUSTAIN
    };

    enum
    {
        TSF_SEGMENT_NONE,
        TSF_SEGMENT_DELAY,
        TSF_SEGMENT_ATTACK,
        TSF_SEGMENT_HOLD,
        TSF_SEGMENT_DECAY,
        TSF_SEGMENT_SUSTAIN,
        TSF_SEGMENT_RELEASE,
        TSF_SEGMENT_DONE
    };

    struct tsf_hydra
    {
        struct tsf_hydra_phdr* phdrs;
        struct tsf_hydra_pbag* pbags;
        struct tsf_hydra_pmod* pmods;
        struct tsf_hydra_pgen* pgens;
        struct tsf_hydra_inst* insts;
        struct tsf_hydra_ibag* ibags;
        struct tsf_hydra_imod* imods;
        struct tsf_hydra_igen* igens;
        struct tsf_hydra_shdr* shdrs;
        int phdrNum, pbagNum, pmodNum, pgenNum, instNum, ibagNum, imodNum,
            igenNum, shdrNum;
    };

    union tsf_hydra_genamount
    {
        struct
        {
            tsf_u8 lo, hi;
        } range;
        tsf_s16 shortAmount;
        tsf_u16 wordAmount;
    };
    struct tsf_hydra_phdr
    {
        tsf_char20 presetName;
        tsf_u16    preset, bank, presetBagNdx;
        tsf_u32    library, genre, morphology;
    };
    struct tsf_hydra_pbag
    {
        tsf_u16 genNdx, modNdx;
    };
    struct tsf_hydra_pmod
    {
        tsf_u16 modSrcOper, modDestOper;
        tsf_s16 modAmount;
        tsf_u16 modAmtSrcOper, modTransOper;
    };
    struct tsf_hydra_pgen
    {
        tsf_u16                   genOper;
        union tsf_hydra_genamount genAmount;
    };
    struct tsf_hydra_inst
    {
        tsf_char20 instName;
        tsf_u16    instBagNdx;
    };
    struct tsf_hydra_ibag
    {
        tsf_u16 instGenNdx, instModNdx;
    };
    struct tsf_hydra_imod
    {
        tsf_u16 modSrcOper, modDestOper;
        tsf_s16 modAmount;
        tsf_u16 modAmtSrcOper, modTransOper;
    };
    struct tsf_hydra_igen
    {
        tsf_u16                   genOper;
        union tsf_hydra_genamount genAmount;
    };
    struct tsf_hydra_shdr
    {
        tsf_char20 sampleName;
        tsf_u32    start, end, startLoop, endLoop, sampleRate;
        tsf_u8     originalPitch;
        tsf_s8     pitchCorrection;
        tsf_u16    sampleLink, sampleType;
    };

#define TSFR(FIELD) stream->read(stream->data, &i->FIELD, sizeof(i->FIELD));
    static void tsf_hydra_read_phdr(struct tsf_hydra_phdr* i,
                                    struct tsf_stream*     stream)
    {
        TSFR(presetName)
        TSFR(preset) TSFR(bank) TSFR(presetBagNdx) TSFR(library) TSFR(genre)
            TSFR(morphology)
    }
    static void tsf_hydra_read_pbag(struct tsf_hydra_pbag* i,
                                    struct tsf_stream*     stream)
    {
        TSFR(genNdx) TSFR(modNdx)
    }
    static void tsf_hydra_read_pmod(struct tsf_hydra_pmod* i,
                                    struct tsf_stream*     stream)
    {
        TSFR(modSrcOper)
        TSFR(modDestOper) TSFR(modAmount) TSFR(modAmtSrcOper) TSFR(modTransOper)
    }
    static void tsf_hydra_read_pgen(struct tsf_hydra_pgen* i,
                                    struct tsf_stream*     stream)
    {
        TSFR(genOper) TSFR(genAmount)
    }
    static void tsf_hydra_read_inst(struct tsf_hydra_inst* i,
                                    struct tsf_stream*     stream)
    {
        TSFR(instName) TSFR(instBagNdx)
    }
    static void tsf_hydra_read_ibag(struct tsf_hydra_ibag* i,
                                    struct tsf_stream*     stream)
    {
        TSFR(instGenNdx) TSFR(instModNdx)
    }
    static void tsf_hydra_read_imod(struct tsf_hydra_imod* i,
                                    struct tsf_stream*     stream)
    {
        TSFR(modSrcOper)
        TSFR(modDestOper) TSFR(modAmount) TSFR(modAmtSrcOper) TSFR(modTransOper)
    }
    static void tsf_hydra_read_igen(struct tsf_hydra_igen* i,
                                    struct tsf_stream*     stream)
    {
        TSFR(genOper) TSFR(genAmount)
    }
    static void tsf_hydra_read_shdr(struct tsf_hydra_shdr* i,
                                    struct tsf_stream*     stream)
    {
        TSFR(sampleName)
        TSFR(start) TSFR(end) TSFR(startLoop) TSFR(endLoop) TSFR(sampleRate)
            TSFR(originalPitch) TSFR(pitchCorrection) TSFR(sampleLink)
                TSFR(sampleType)
    }
#undef TSFR

    struct tsf_riffchunk
    {
        tsf_fourcc id;
        tsf_u32    size;
    };
    struct tsf_envelope
    {
        float delay, attack, hold, decay, sustain, release, keynumToHold,
            keynumToDecay;
    };
    struct tsf_voice_envelope
    {
        unsigned char       segment, segmentIsExponential : 1, isAmpEnv : 1;
        short               midiVelocity;
        float               level, slope;
        int                 samplesUntilNextSegment;
        struct tsf_envelope parameters;
    };
    struct tsf_voice_lowpass
    {
        double   QInv, a0, a1, b1, b2, z1, z2;
        TSF_BOOL active;
    };
    struct tsf_voice_lfo
    {
        int   samplesUntil;
        float level, delta;
    };

    struct tsf_modoper
    {
        unsigned char index : 7;
        unsigned char cc
            : 1; // cc=0=index is general control; cc=1=index is midi control
        unsigned char d : 1; // d=0=positive (0 -> 127); d=1=negative (127 -> 0)
        unsigned char p : 1; // p=0=unipolar(0 -> 1); p=1=bipolar(-1 -> +1)
        unsigned char type : 6; // 0=linear, 1=concave, 2=convex, 4=switch
    };

    struct tsf_modulator
    {
        union
        {
            unsigned int       modSrcOper;
            struct tsf_modoper modSrcOperDetails;
        };
        unsigned int modDestOper;
        int          modAmount;
        union
        {
            unsigned int       modAmtSrcOper;
            struct tsf_modoper modAmtSrcOperDetails;
        };
        unsigned int modTransOper;
    };

    struct tsf_region
    {
        int                 loop_mode;
        unsigned int        sample_rate;
        unsigned char       lokey, hikey, lovel, hivel;
        unsigned int        group, offset, end, loop_start, loop_end;
        int                 transpose, tune, pitch_keycenter, pitch_keytrack;
        float               attenuation, pan;
        struct tsf_envelope ampenv, modenv;
        int                 initialFilterQ, initialFilterFc;
        int   modEnvToPitch, modEnvToFilterFc, modLfoToFilterFc, modLfoToVolume;
        float delayModLFO;
        int   freqModLFO, modLfoToPitch;
        float delayVibLFO;
        int   freqVibLFO, vibLfoToPitch;
        float reverbSend, chorusSend;
        int   sampleID, instrumentID;
        int   modulatorNum;
        struct tsf_modulator* modulators;
    };

    struct tsf_preset
    {
        char               presetName[21];
        tsf_u16            preset, bank;
        struct tsf_region* regions;
        int                regionNum;
    };

    struct tsf_sample
    {
        char    sampleName[21];
        tsf_u32 start, end, startLoop, endLoop, sampleRate;
        tsf_u8  originalPitch;
        tsf_s8  pitchCorrection;
        tsf_u16 sampleLink, sampleType;
    };

    struct tsf_voice
    {
        int   playingPreset, playingKey, playingChannel, heldSustain;
        short playingVelocity;
        struct tsf_region*        region;
        double                    pitchInputTimecents, pitchOutputFactor;
        double                    sourceSamplePosition;
        float                     noteGainDB, panFactorLeft, panFactorRight;
        unsigned int              playIndex, loopStart, loopEnd;
        int                       pan;
        int                       initialFilterFc, initialFilterQ;
        int                       vibLfoToPitch;
        struct tsf_voice_envelope ampenv, modenv;
        struct tsf_voice_lowpass  lowpass;
        struct tsf_voice_lfo      modlfo, viblfo;
    };

    struct tsf_channel
    {
        unsigned short presetIndex, bank, pitchWheel, midiPan, midiVolume,
            midiExpression, midiRPN, midiData : 14, sustain : 1;
        unsigned short modWheel, midiQ, midiFc;
        float          pitchRange, tuning;
    };

    struct tsf_channels
    {
        void (*setupVoice)(tsf* f, struct tsf_voice* voice);
        int                channelNum, activeChannel;
        struct tsf_channel channels[1];
    };

    TSFDEF double tsf_timecents2Secsd(double timecents)
    {
        return TSF_POW(2.0, timecents / 1200.0);
    }
    TSFDEF float tsf_timecents2Secsf(float timecents)
    {
        return TSF_POWF(2.0f, timecents / 1200.0f);
    }
    TSFDEF float tsf_cents2Hertz(float cents)
    {
        return 8.176f * TSF_POWF(2.0f, cents / 1200.0f);
    }
    TSFDEF float tsf_decibelsToGain(float db)
    {
        return db > -100.f ? TSF_POWF(10.0f, db * 1 / 20.0f) : 0;
    }
    TSFDEF float tsf_gainToDecibels(float gain)
    {
        return gain <= .00001f ? -100.f : (float)(20.0 * TSF_LOG10(gain));
    }

    static TSF_BOOL tsf_riffchunk_read(struct tsf_riffchunk* parent,
                                       struct tsf_riffchunk* chunk,
                                       struct tsf_stream*    stream)
    {
        TSF_BOOL IsRiff, IsList;
        if(parent && sizeof(tsf_fourcc) + sizeof(tsf_u32) > parent->size)
            return TSF_FALSE;
        if(!stream->read(stream->data, &chunk->id, sizeof(tsf_fourcc))
           || *chunk->id <= ' ' || *chunk->id >= 'z')
            return TSF_FALSE;
        if(!stream->read(stream->data, &chunk->size, sizeof(tsf_u32)))
            return TSF_FALSE;
        if(parent
           && sizeof(tsf_fourcc) + sizeof(tsf_u32) + chunk->size > parent->size)
            return TSF_FALSE;
        if(parent)
            parent->size -= sizeof(tsf_fourcc) + sizeof(tsf_u32) + chunk->size;
        IsRiff = TSF_FourCCEquals(chunk->id, "RIFF"),
        IsList = TSF_FourCCEquals(chunk->id, "LIST");
        if(IsRiff && parent)
            return TSF_FALSE; //not allowed
        if(!IsRiff && !IsList)
            return TSF_TRUE; //custom type without sub type
        if(!stream->read(stream->data, &chunk->id, sizeof(tsf_fourcc))
           || *chunk->id <= ' ' || *chunk->id >= 'z')
            return TSF_FALSE;
        chunk->size -= sizeof(tsf_fourcc);
        return TSF_TRUE;
    }

    static TSF_BOOL debug_wantlearn = TSF_FALSE;
    TSFDEF void     tsf_debug_wantlearn()
    {
        debug_wantlearn = TSF_TRUE;
    }

    static void tsf_region_clear(struct tsf_region* i, TSF_BOOL for_relative)
    {
        TSF_MEMSET(i, 0, sizeof(struct tsf_region));
        i->hikey = i->hivel = 127;
        i->pitch_keycenter  = 60; // C4
        if(for_relative)
            return;

        i->pitch_keytrack = 100;

        i->pitch_keycenter = -1;

        // SF2 defaults in timecents.
        i->ampenv.delay = i->ampenv.attack = i->ampenv.hold = i->ampenv.decay
            = i->ampenv.release                             = -12000.0f;
        i->modenv.delay = i->modenv.attack = i->modenv.hold = i->modenv.decay
            = i->modenv.release                             = -12000.0f;

        i->initialFilterFc = 13500;
        i->initialFilterQ  = 0;

        i->delayModLFO = -12000.0f;
        i->delayVibLFO = -12000.0f;

        i->modulators   = NULL;
        i->modulatorNum = 0;
        i->sampleID     = -1;
        i->instrumentID = -1;
    }

    static const char* tsf_smurf_gen_name(unsigned int i)
    {
        static const char* gen_names[] = {
            "StartAddrOfs",
            "EndAddrOfs",
            "StartLoopAddrOfs",
            "EndLoopAddrOfs",
            "StartAddrCoarseOfs",
            "ModLFO2Pitch",
            "VibLFO2Pitch",
            "ModEnv2Pitch",
            "FilterFc",
            "FilterQ",
            "ModLFO2FilterFc",
            "ModEnv2FilterFc",
            "EndAddrCoarseOfs",
            "ModLFO2Vol",
            "Unused1",
            "ChorusSend",
            "ReverbSend",
            "Pan",
            "Unused2",
            "Unused3",
            "Unused4",
            "ModLFODelay",
            "ModLFOFreq",
            "VibLFODelay",
            "VibLFOFreq",
            "ModEnvDelay",
            "ModEnvAttack",
            "ModEnvHold",
            "ModEnvDecay",
            "ModEnvSustain",
            "ModEnvRelease",
            "Key2ModEnvHold",
            "Key2ModEnvDecay",
            "VolEnvDelay",
            "VolEnvAttack",
            "VolEnvHold",
            "VolEnvDecay",
            "VolEnvSustain",
            "VolEnvRelease",
            "Key2VolEnvHold",
            "Key2VolEnvDecay",
            "Instrument",
            "Reserved1",
            "KeyRange",
            "VelRange",
            "StartLoopAddrCoarseOfs",
            "Keynum",
            "Velocity",
            "Attenuation",
            "Reserved2",
            "EndLoopAddrCoarseOfs",
            "CoarseTune",
            "FineTune",
            "SampleId",
            "SampleModes",
            "Reserved3",
            "ScaleTune",
            "ExclusiveClass",
            "OverrideRootKey",
            "Dummy",
        };
        return i >= (sizeof(gen_names) / sizeof(gen_names[0])) ? TSF_NULL
                                                               : gen_names[i];
    }

    static void tsf_region_operator(struct tsf_region*         region,
                                    tsf_u16                    genOper,
                                    union tsf_hydra_genamount* amount,
                                    struct tsf_region*         merge_region)
    {
        enum
        {
            _GEN_TYPE_MASK = 0x0F,
            GEN_FLOAT      = 0x01,
            GEN_INT        = 0x02,
            GEN_UINT_ADD   = 0x03,
            GEN_UINT_ADD15 = 0x04,
            GEN_KEYRANGE   = 0x05,
            GEN_VELRANGE   = 0x06,
            GEN_LOOPMODE   = 0x07,
            GEN_GROUP      = 0x08,
            GEN_KEYCENTER  = 0x09,

            _GEN_LIMIT_MASK      = 0xF0,
            GEN_INT_LIMIT12K     = 0x10, //min -12000, max 12000
            GEN_INT_LIMITFC      = 0x20, //min 1500, max 13500
            GEN_INT_LIMITQ       = 0x30, //min 0, max 960
            GEN_INT_LIMIT960     = 0x40, //min -960, max 960
            GEN_INT_LIMIT16K4500 = 0x50, //min -16000, max 4500
            GEN_FLOAT_LIMIT12K5K = 0x60, //min -12000, max 5000
            GEN_FLOAT_LIMIT12K8K = 0x70, //min -12000, max 8000
            GEN_FLOAT_LIMIT1200  = 0x80, //min -1200, max 1200
            GEN_FLOAT_LIMITPAN   = 0x90, //* .001f, min -.5f, max .5f,
            GEN_FLOAT_LIMITATTN  = 0xA0, //* .1f, min 0, max 144.0
            GEN_FLOAT_LIMITFX    = 0xD0, //* .001f, min 0.0, max 1.0f (1000)

            _GEN_MAX = 59
        };
#define _TSFREGIONOFFSET(TYPE, FIELD) \
    (unsigned char)(((TYPE*)&((struct tsf_region*)0)->FIELD) - (TYPE*)0)
#define _TSFREGIONENVOFFSET(TYPE, ENV, FIELD)                            \
    (unsigned char)(((TYPE*)&((&(((struct tsf_region*)0)->ENV))->FIELD)) \
                    - (TYPE*)0)
        static const struct
        {
            unsigned char mode, offset;
        } genMetas[_GEN_MAX] = {
            {GEN_UINT_ADD,
             _TSFREGIONOFFSET(
                 unsigned int,
                 offset)}, // 0 StartAddrsOffset           +  (smpls)
            {GEN_UINT_ADD,
             _TSFREGIONOFFSET(unsigned int,
                              end)}, // 1 EndAddrsOffset             +  (smpls)
            {GEN_UINT_ADD,
             _TSFREGIONOFFSET(
                 unsigned int,
                 loop_start)}, // 2 StartloopAddrsOffset       +  (smpls)
            {GEN_UINT_ADD,
             _TSFREGIONOFFSET(
                 unsigned int,
                 loop_end)}, // 3 EndloopAddrsOffset         +  (smpls)
            {GEN_UINT_ADD15,
             _TSFREGIONOFFSET(
                 unsigned int,
                 offset)}, // 4 StartAddrsCoarseOffset     +  (32k smpls)
            {GEN_INT | GEN_INT_LIMIT12K,
             _TSFREGIONOFFSET(
                 int,
                 modLfoToPitch)}, // 5 ModLfoToPitch                 (cent fs)
            {GEN_INT | GEN_INT_LIMIT12K,
             _TSFREGIONOFFSET(
                 int,
                 vibLfoToPitch)}, // 6 VibLfoToPitch                 (cent fs)
            {GEN_INT | GEN_INT_LIMIT12K,
             _TSFREGIONOFFSET(
                 int,
                 modEnvToPitch)}, // 7 ModEnvToPitch                 (cent fs)
            {GEN_INT | GEN_INT_LIMITFC,
             _TSFREGIONOFFSET(
                 int,
                 initialFilterFc)}, // 8 InitialFilterFc               (cent)
            {GEN_INT | GEN_INT_LIMITQ,
             _TSFREGIONOFFSET(
                 int, initialFilterQ)}, // 9 InitialFilterQ                (cB)
            {GEN_INT | GEN_INT_LIMIT12K,
             _TSFREGIONOFFSET(
                 int,
                 modLfoToFilterFc)}, //10 ModLfoToFilterFc              (cent fs)
            {GEN_INT | GEN_INT_LIMIT12K,
             _TSFREGIONOFFSET(
                 int,
                 modEnvToFilterFc)}, //11 ModEnvToFilterFc              (cent fs)
            {GEN_UINT_ADD15,
             _TSFREGIONOFFSET(
                 unsigned int,
                 end)}, //12 EndAddrsCoarseOffset       +  (32k smpls)
            {GEN_INT | GEN_INT_LIMIT960,
             _TSFREGIONOFFSET(
                 int,
                 modLfoToVolume)}, //13 ModLfoToVolume                (cB fs )
            {0, (0)},              //   Unused
            {GEN_FLOAT | GEN_FLOAT_LIMITFX,
             _TSFREGIONOFFSET(
                 float, chorusSend)}, //15 ChorusEffectsSend           ? (0.1%)
            {GEN_FLOAT | GEN_FLOAT_LIMITFX,
             _TSFREGIONOFFSET(
                 float, reverbSend)}, //16 ReverbEffectsSend           ? (0.1%)
            {GEN_FLOAT | GEN_FLOAT_LIMITPAN,
             _TSFREGIONOFFSET(float,
                              pan)}, //17 Pan                           (0.1%)
            {0, (0)},                //   Unused
            {0, (0)},                //   Unused
            {0, (0)},                //   Unused
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K5K,
             _TSFREGIONOFFSET(
                 float,
                 delayModLFO)}, //21 DelayModLFO                   (timecent)
            {GEN_INT | GEN_INT_LIMIT16K4500,
             _TSFREGIONOFFSET(
                 int, freqModLFO)}, //22 FreqModLFO                    (cent)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K5K,
             _TSFREGIONOFFSET(
                 float,
                 delayVibLFO)}, //23 DelayVibLFO                   (timecent)
            {GEN_INT | GEN_INT_LIMIT16K4500,
             _TSFREGIONOFFSET(
                 int, freqVibLFO)}, //24 FreqVibLFO                    (cent)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K5K,
             _TSFREGIONENVOFFSET(
                 float,
                 modenv,
                 delay)}, //25 DelayModEnv                   (timecent)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K8K,
             _TSFREGIONENVOFFSET(
                 float,
                 modenv,
                 attack)}, //26 AttackModEnv                  (timecent)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K5K,
             _TSFREGIONENVOFFSET(
                 float,
                 modenv,
                 hold)}, //27 HoldModEnv                    (timecent)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K8K,
             _TSFREGIONENVOFFSET(
                 float,
                 modenv,
                 decay)}, //28 DecayModEnv                   (timecent)
            {GEN_FLOAT | GEN_FLOAT_LIMITFX,
             _TSFREGIONENVOFFSET(
                 float,
                 modenv,
                 sustain)}, //29 SustainModEnv (inversed)      (-0.1%)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K8K,
             _TSFREGIONENVOFFSET(
                 float,
                 modenv,
                 release)}, //30 ReleaseModEnv                 (timecent)
            {GEN_FLOAT | GEN_FLOAT_LIMIT1200,
             _TSFREGIONENVOFFSET(
                 float,
                 modenv,
                 keynumToHold)}, //31 KeynumToModEnvHold            (tcent/key)
            {GEN_FLOAT | GEN_FLOAT_LIMIT1200,
             _TSFREGIONENVOFFSET(
                 float,
                 modenv,
                 keynumToDecay)}, //32 KeynumToModEnvDecay           (tcent/key)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K5K,
             _TSFREGIONENVOFFSET(
                 float,
                 ampenv,
                 delay)}, //33 DelayVolEnv                   (timecent)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K8K,
             _TSFREGIONENVOFFSET(
                 float,
                 ampenv,
                 attack)}, //34 AttackVolEnv                  (timecent)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K5K,
             _TSFREGIONENVOFFSET(
                 float,
                 ampenv,
                 hold)}, //35 HoldVolEnv                    (timecent)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K8K,
             _TSFREGIONENVOFFSET(
                 float,
                 ampenv,
                 decay)}, //36 DecayVolEnv                   (timecent)
            {GEN_FLOAT | GEN_FLOAT_LIMITATTN,
             _TSFREGIONENVOFFSET(
                 float,
                 ampenv,
                 sustain)}, //37 SustainVolEnv                 (cB attn)
            {GEN_FLOAT | GEN_FLOAT_LIMIT12K8K,
             _TSFREGIONENVOFFSET(
                 float,
                 ampenv,
                 release)}, //38 ReleaseVolEnv                 (timecent)
            {GEN_FLOAT | GEN_FLOAT_LIMIT1200,
             _TSFREGIONENVOFFSET(
                 float,
                 ampenv,
                 keynumToHold)}, //39 KeynumToVolEnvHold            (tcent/key)
            {GEN_FLOAT | GEN_FLOAT_LIMIT1200,
             _TSFREGIONENVOFFSET(
                 float,
                 ampenv,
                 keynumToDecay)}, //40 KeynumToVolEnvDecay           (tcent/key)
            {0, (0)},             //41 Instrument (special/pgen)
            {0, (0)},             //   Reserved
            {GEN_KEYRANGE, (0)},  //43 KeyRange (pgen)             @ (MIDI ky#)
            {GEN_VELRANGE, (0)},  //44 VelRange (pgen)             @ (MIDI vel)
            {GEN_UINT_ADD15,
             _TSFREGIONOFFSET(
                 unsigned int,
                 loop_start)}, //45 StartloopAddrsCoarseOffset +  (smpls)
            {0, (0)},          //46 Keynum (special)           +@ (MIDI ky#)
            {0, (0)},          //47 Velocity (special)         +@ (MIDI vel)
            {GEN_FLOAT | GEN_FLOAT_LIMITATTN,
             _TSFREGIONOFFSET(
                 float, attenuation)}, //48 InitialAttenuation            (cB)
            {0, (0)},                  //   Reserved
            {GEN_UINT_ADD15,
             _TSFREGIONOFFSET(
                 unsigned int,
                 loop_end)}, //50 EndloopAddrsCoarseOffset   +  (smpls)
            {GEN_INT,
             _TSFREGIONOFFSET(
                 int, transpose)}, //51 CoarseTune                    (semitone)
            {GEN_INT,
             _TSFREGIONOFFSET(int,
                              tune)}, //52 FineTune                      (cent)
            {0, (0)},                 //53 SampleID (special/igen)
            {GEN_LOOPMODE,
             _TSFREGIONOFFSET(
                 int,
                 loop_mode)}, //54 SampleModes                +@ (Bit Flags)
            {0, (0)},         //   Reserved
            {GEN_INT,
             _TSFREGIONOFFSET(
                 int,
                 pitch_keytrack)}, //56 ScaleTuning                 @ (cent/key)
            {GEN_GROUP,
             _TSFREGIONOFFSET(
                 unsigned int,
                 group)}, //57 ExclusiveClass             +@ (arbitrary#)
            {GEN_KEYCENTER,
             _TSFREGIONOFFSET(
                 int,
                 pitch_keycenter)}, //58 OverridingRootKey          +@ (MIDI ky#)
        };
#undef _TSFREGIONOFFSET
#undef _TSFREGIONENVOFFSET
        if(amount)
        {
            int offset;
            if(genOper >= _GEN_MAX)
                return;
            offset = genMetas[genOper].offset;
            switch(genMetas[genOper].mode & _GEN_TYPE_MASK)
            {
                case GEN_FLOAT:
                    ((float*)region)[offset] = amount->shortAmount;
                    return;
                case GEN_INT:
                    ((int*)region)[offset] = amount->shortAmount;
                    return;
                case GEN_UINT_ADD:
                    ((unsigned int*)region)[offset] += amount->shortAmount;
                    return;
                case GEN_UINT_ADD15:
                    ((unsigned int*)region)[offset] += amount->shortAmount
                                                       << 15;
                    return;
                case GEN_KEYRANGE:
                    region->lokey = amount->range.lo;
                    region->hikey = amount->range.hi;
                    return;
                case GEN_VELRANGE:
                    region->lovel = amount->range.lo;
                    region->hivel = amount->range.hi;
                    return;
                case GEN_LOOPMODE:
                    region->loop_mode = ((amount->wordAmount & 3) == 3
                                             ? TSF_LOOPMODE_SUSTAIN
                                             : ((amount->wordAmount & 3) == 1
                                                    ? TSF_LOOPMODE_CONTINUOUS
                                                    : TSF_LOOPMODE_NONE));
                    return;
                case GEN_GROUP: region->group = amount->wordAmount; return;
                case GEN_KEYCENTER:
                    region->pitch_keycenter = amount->shortAmount;
                    return;
                default: TSF_WARN("Skip region gen: %d\n", genOper);
            }
        }
        else //merge regions and clamp values
        {
            for(genOper = 0; genOper != _GEN_MAX; genOper++)
            {
                int offset = genMetas[genOper].offset;
                switch(genMetas[genOper].mode & _GEN_TYPE_MASK)
                {
                    case GEN_FLOAT:
                    {
                        float *val = &((float*)region)[offset], vfactor, vmin,
                              vmax;
                        *val += ((float*)merge_region)[offset];
                        switch(genMetas[genOper].mode & _GEN_LIMIT_MASK)
                        {
                            case GEN_FLOAT_LIMIT12K5K:
                                vfactor = 1.0f;
                                vmin    = -12000.0f;
                                vmax    = 5000.0f;
                                break;
                            case GEN_FLOAT_LIMIT12K8K:
                                vfactor = 1.0f;
                                vmin    = -12000.0f;
                                vmax    = 8000.0f;
                                break;
                            case GEN_FLOAT_LIMIT1200:
                                vfactor = 1.0f;
                                vmin    = -1200.0f;
                                vmax    = 1200.0f;
                                break;
                            case GEN_FLOAT_LIMITPAN:
                                vfactor = 0.001f;
                                vmin    = -0.5f;
                                vmax    = 0.5f;
                                break;
                            case GEN_FLOAT_LIMITATTN:
                                vfactor = 0.01f;
                                vmin    = 0.0f;
                                vmax    = 14.4f;
                                break;
                            case GEN_FLOAT_LIMITFX:
                                vfactor = 0.001f;
                                vmin    = 0.0f;
                                vmax    = 1.0f;
                                break;
                            default: continue;
                        }
                        *val *= vfactor;
                        if(*val < vmin)
                            *val = vmin;
                        else if(*val > vmax)
                            *val = vmax;
                        continue;
                    }
                    case GEN_INT:
                    {
                        int *val = &((int*)region)[offset], vmin, vmax;
                        *val += ((int*)merge_region)[offset];
                        switch(genMetas[genOper].mode & _GEN_LIMIT_MASK)
                        {
                            case GEN_INT_LIMIT12K:
                                vmin = -12000;
                                vmax = 12000;
                                break;
                            case GEN_INT_LIMITFC:
                                vmin = 1500;
                                vmax = 13500;
                                break;
                            case GEN_INT_LIMITQ:
                                vmin = 0;
                                vmax = 960;
                                break;
                            case GEN_INT_LIMIT960:
                                vmin = -960;
                                vmax = 960;
                                break;
                            case GEN_INT_LIMIT16K4500:
                                vmin = -16000;
                                vmax = 4500;
                                break;
                            default: continue;
                        }
                        if(*val < vmin)
                            *val = vmin;
                        else if(*val > vmax)
                            *val = vmax;
                        continue;
                    }
                    case GEN_UINT_ADD:
                    {
                        ((unsigned int*)region)[offset]
                            += ((unsigned int*)merge_region)[offset];
                        continue;
                    }
                }
            }
        }
    }

    static void tsf_region_envtosecs(struct tsf_envelope* p,
                                     TSF_BOOL             sustainIsGain)
    {
        // EG times need to be converted from timecents to seconds.
        // Pin very short EG segments.  Timecents don't get to zero, and our EG is
        // happier with zero values.
        p->delay
            = (p->delay < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->delay));
        p->attack
            = (p->attack < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->attack));
        p->release
            = (p->release < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->release));

        // If we have dynamic hold or decay times depending on key number we need
        // to keep the values in timecents so we can calculate it during startNote
        if(!p->keynumToHold)
            p->hold
                = (p->hold < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->hold));
        if(!p->keynumToDecay)
            p->decay
                = (p->decay < -11950.0f ? 0.0f : tsf_timecents2Secsf(p->decay));

        if(p->sustain < 0.0f)
            p->sustain = 0.0f;
        else if(sustainIsGain)
            p->sustain = tsf_decibelsToGain(-p->sustain);
        else
            p->sustain = 1.0f - p->sustain;
    }

    static void tsf_region_copy(struct tsf_region*       dst,
                                const struct tsf_region* src)
    {
        if(dst->modulators)
            TSF_FREE(dst->modulators);
        *dst = *src;
        if(dst->modulatorNum)
        {
            dst->modulators = (struct tsf_modulator*)TSF_MALLOC(
                dst->modulatorNum * sizeof(struct tsf_modulator));
            TSF_MEMCPY(dst->modulators,
                       src->modulators,
                       src->modulatorNum * sizeof(struct tsf_modulator));
        }
        else
            dst->modulators = NULL;
    }

    static int tsf_load_presets(tsf*              res,
                                struct tsf_hydra* hydra,
                                unsigned int      fontSampleCount)
    {
        static const struct tsf_modulator default_modulators[] = {
            // default modulators by SF2 specs
            {0x0502,
             48,
             960,
             0x0,
             0}, // vel to attenuation, MIDI Note-On Velocity to Initial Attenuation (section 8.4.1)
            {0x0102, 8, -2400, 0x0, 0},     // vel to filterFc
            {0x000D, 6, 50, 0x0, 0},        // channel pressure to vibrato
            {0x0081, 6, 50, 0x0, 0},        // cc 1/mod-wheel to vibrato
            {0x0587, 48, 960, 0x0, 0},      // cc 7/vol to attenuation
            {0x028A, 17, 1000, 0x0, 0},     // cc 10/pan to pan
            {0x058B, 48, 960, 0x0, 0},      // cc 11/expression to attenuation
            {0x00DB, 16, 200, 0x0, 0},      // cc 91/reverb to reverb send
            {0x00DD, 15, 200, 0x0, 0},      // cc 93/chorus to chorus send
            {0x020E, 52, 12700, 0x0010, 0}, // pitch wheel to tuning
            // custom modulators heck yeah, Spessa
            {0x000A, 6, 50, 0x0, 0},  // poly pressure to vibrato
            {0x00DC, 13, 24, 0x0, 0}, // cc 92 (tremolo) to modLFO volume
            {0x02C8,
             38,
             1200,
             0x0,
             0},                       // cc 72 (release time) to volEnv release
            {0x02CA, 8, 6000, 0x0, 0}, // cc 74 (brightness) to filterFc
            {0x02C7, 9, 250, 0x0, 0},  // cc 71 (filter q) to filterq
        };

        enum
        {
            GenInstrument = 41,
            GenKeyRange   = 43,
            GenVelRange   = 44,
            GenSampleID   = 53
        };
        // Read each preset.
        struct tsf_hydra_phdr *pphdr, *pphdrMax;
        res->presetNum = hydra->phdrNum - 1;
        res->presets   = (struct tsf_preset*)TSF_MALLOC(
            res->presetNum * sizeof(struct tsf_preset));
        if(!res->presets)
            return 0;
        else
        {
            int i;
            for(i = 0; i != res->presetNum; i++)
                res->presets[i].regions = TSF_NULL;
        }
        for(pphdr = hydra->phdrs, pphdrMax = pphdr + hydra->phdrNum - 1;
            pphdr != pphdrMax;
            pphdr++)
        {
            int                    sortedIndex = 0, region_index = 0;
            struct tsf_hydra_phdr* otherphdr;
            struct tsf_preset*     preset;
            struct tsf_hydra_pbag *ppbag, *ppbagEnd;
            struct tsf_region      globalRegion;
            for(otherphdr = hydra->phdrs; otherphdr != pphdrMax; otherphdr++)
            {
                if(otherphdr == pphdr || otherphdr->bank > pphdr->bank)
                    continue;
                else if(otherphdr->bank < pphdr->bank)
                    sortedIndex++;
                else if(otherphdr->preset > pphdr->preset)
                    continue;
                else if(otherphdr->preset < pphdr->preset)
                    sortedIndex++;
                else if(otherphdr < pphdr)
                    sortedIndex++;
            }

            preset = &res->presets[sortedIndex];
            TSF_MEMCPY(preset->presetName,
                       pphdr->presetName,
                       sizeof(pphdr->presetName));
            preset->presetName[sizeof(preset->presetName) - 1]
                = '\0'; //should be zero terminated in source file but make sure
            preset->bank      = pphdr->bank;
            preset->preset    = pphdr->preset;
            preset->regionNum = 0;

            if(debug_wantlearn)
                printf("> presets[%d] %s (bags:%d)\n",
                       sortedIndex,
                       preset->presetName,
                       pphdr[1].presetBagNdx - pphdr->presetBagNdx);

            // Pass-1: Count regions covered by this preset
            for(ppbag = hydra->pbags + pphdr->presetBagNdx,
            ppbagEnd  = hydra->pbags + pphdr[1].presetBagNdx;
                ppbag != ppbagEnd;
                ppbag++)
            {
                unsigned char plokey = 0, phikey = 127, plovel = 0,
                              phivel = 127;
                struct tsf_hydra_pgen *ppgen, *ppgenEnd;
                struct tsf_hydra_inst* pinst;
                struct tsf_hydra_ibag *pibag, *pibagEnd;
                struct tsf_hydra_igen *pigen, *pigenEnd;
                for(ppgen = hydra->pgens + ppbag->genNdx,
                ppgenEnd  = hydra->pgens + ppbag[1].genNdx;
                    ppgen != ppgenEnd;
                    ppgen++)
                {
                    if(ppgen->genOper == GenKeyRange)
                    {
                        plokey = ppgen->genAmount.range.lo;
                        phikey = ppgen->genAmount.range.hi;
                        continue;
                    }
                    if(ppgen->genOper == GenVelRange)
                    {
                        plovel = ppgen->genAmount.range.lo;
                        phivel = ppgen->genAmount.range.hi;
                        continue;
                    }
                    if(ppgen->genOper != GenInstrument)
                        continue;
                    if(ppgen->genAmount.wordAmount >= hydra->instNum)
                        continue;
                    pinst = hydra->insts + ppgen->genAmount.wordAmount;

                    for(pibag = hydra->ibags + pinst->instBagNdx,
                    pibagEnd  = hydra->ibags + pinst[1].instBagNdx;
                        pibag != pibagEnd;
                        pibag++)
                    {
                        unsigned char ilokey = 0, ihikey = 127, ilovel = 0,
                                      ihivel = 127;
                        for(pigen = hydra->igens + pibag->instGenNdx,
                        pigenEnd  = hydra->igens + pibag[1].instGenNdx;
                            pigen != pigenEnd;
                            pigen++)
                        {
                            if(pigen->genOper == GenKeyRange)
                            {
                                ilokey = pigen->genAmount.range.lo;
                                ihikey = pigen->genAmount.range.hi;
                                continue;
                            }
                            if(pigen->genOper == GenVelRange)
                            {
                                ilovel = pigen->genAmount.range.lo;
                                ihivel = pigen->genAmount.range.hi;
                                continue;
                            }
                            if(pigen->genOper == GenSampleID && ihikey >= plokey
                               && ilokey <= phikey && ihivel >= plovel
                               && ilovel <= phivel)
                                preset->regionNum++;
                        }
                    }
                }
            }

            preset->regions = (struct tsf_region*)TSF_MALLOC(
                preset->regionNum * sizeof(struct tsf_region));
            if(!preset->regions)
            {
                int i;
                for(i = 0; i != res->presetNum; i++)
                    TSF_FREE(res->presets[i].regions);
                TSF_FREE(res->presets);
                return 0;
            }
            tsf_region_clear(&globalRegion, TSF_TRUE);

            // Pass-2: Zones.
            for(ppbag = hydra->pbags + pphdr->presetBagNdx,
            ppbagEnd  = hydra->pbags + pphdr[1].presetBagNdx;
                ppbag != ppbagEnd;
                ppbag++)
            {
                if(debug_wantlearn)
                    printf(">\tpbag [%lu]\n",
                           (uintptr_t)(ppbag
                                       - (hydra->pbags + pphdr->presetBagNdx)));

                struct tsf_hydra_pgen *ppgen, *ppgenEnd;
                struct tsf_hydra_inst* pinst;
                struct tsf_hydra_ibag *pibag, *pibagEnd;
                struct tsf_hydra_igen *pigen, *pigenEnd;
                struct tsf_region      presetRegion;
                TSF_BOOL               hadGenInstrument = TSF_FALSE;
                tsf_region_clear(&presetRegion, TSF_FALSE);
                tsf_region_copy(&presetRegion, &globalRegion);

                // Generators.
                for(ppgen = hydra->pgens + ppbag->genNdx,
                ppgenEnd  = hydra->pgens + ppbag[1].genNdx;
                    ppgen != ppgenEnd;
                    ppgen++)
                {
                    if(debug_wantlearn)
                        printf(
                            ">\t\tpgen [%lu] %d(%s)=%x\n",
                            (uintptr_t)(ppgen - (hydra->pgens + ppbag->genNdx)),
                            ppgen->genOper,
                            tsf_smurf_gen_name(ppgen->genOper),
                            ppgen->genAmount.wordAmount);

                    // Instrument.
                    if(ppgen->genOper == GenInstrument)
                    {
                        struct tsf_region
                            instRegion; // temporarilyy as a base for other region.
                        tsf_u16 whichInst = ppgen->genAmount.wordAmount;
                        if(whichInst >= hydra->instNum)
                            continue;


                        tsf_region_clear(&instRegion, TSF_FALSE);

                        // innitially defauly modulators
                        instRegion.modulatorNum
                            = sizeof(default_modulators)
                              / sizeof(default_modulators[0]);
                        instRegion.modulators
                            = (struct tsf_modulator*)TSF_REALLOC(
                                instRegion.modulators,
                                instRegion.modulatorNum
                                    * sizeof(struct tsf_modulator));
                        TSF_MEMCPY(instRegion.modulators,
                                   default_modulators,
                                   instRegion.modulatorNum
                                       * sizeof(struct tsf_modulator));

                        pinst = &hydra->insts[whichInst];
                        if(debug_wantlearn)
                            printf(">\t\t= inst %s (bags:%d)\n",
                                   pinst->instName,
                                   pinst[1].instBagNdx - pinst->instBagNdx);

                        for(pibag = hydra->ibags + pinst->instBagNdx,
                        pibagEnd  = hydra->ibags + pinst[1].instBagNdx;
                            pibag != pibagEnd;
                            pibag++)
                        {
                            if(debug_wantlearn)
                                printf(">\t\t\tibag [%lu]\n",
                                       (uintptr_t)(pibag
                                                   - (hydra->ibags
                                                      + pinst->instBagNdx)));

                            // Generators.
                            struct tsf_region zoneRegion;
                            tsf_region_clear(&zoneRegion, TSF_FALSE);
                            tsf_region_copy(&zoneRegion, &instRegion);

                            // Modulators
                            // Modulators in the IMOD sub-chunk are absolute. This means that an IMOD modulator replaces, rather than adds to, a
                            // default modulator. However the effect of a modulator on a generator is additive, IE the output of a modulator adds to a
                            // generator value.
                            struct tsf_hydra_imod *pimod, *pimodEnd;
                            int                    modulatorNum
                                = pibag[1].instModNdx - pibag->instModNdx;
                            if(modulatorNum)
                            {
                                zoneRegion.modulators
                                    = (struct tsf_modulator*)TSF_REALLOC(
                                        zoneRegion.modulators,
                                        (zoneRegion.modulatorNum + modulatorNum)
                                            * sizeof(struct tsf_modulator));
                                struct tsf_modulator* modulator
                                    = zoneRegion.modulators
                                      + zoneRegion.modulatorNum;

                                for(pimod = hydra->imods + pibag->instModNdx,
                                pimodEnd  = hydra->imods + pibag[1].instModNdx;
                                    pimod != pimodEnd;
                                    pimod++)
                                {
                                    if(debug_wantlearn)
                                    {
                                        printf(
                                            ">\t\t\t\timod [%lu] src:%x "
                                            "src2:%x\n",
                                            (uintptr_t)(pimod
                                                        - (hydra->imods
                                                           + pibag
                                                                 ->instModNdx)),
                                            pimod->modSrcOper,
                                            pimod->modAmtSrcOper);

                                        int idx = pimod->modSrcOper & 0x7F;
                                        int cc  = (pimod->modSrcOper & 0x80)
                                                 == 0x80;
                                        int d = (pimod->modSrcOper & 0x100)
                                                == 0x100;
                                        int p = (pimod->modSrcOper & 0x200)
                                                == 0x200;
                                        int typ = (pimod->modSrcOper & 0xFC00)
                                                  >> 10;

                                        printf(
                                            ">\t\t\t\t\tsrc idx = %d, cc = %d "
                                            "(0=general, 1=midi ctrl), d = %d, "
                                            "p = %d, typ = %d "
                                            "(0=linear,1=concave,2=convex,3="
                                            "switch)\n",
                                            idx,
                                            cc,
                                            d,
                                            p,
                                            typ);

                                        if(pimod->modAmtSrcOper)
                                        {
                                            int idx2
                                                = pimod->modAmtSrcOper & 0x7F;
                                            int cc2
                                                = (pimod->modAmtSrcOper & 0x80)
                                                  == 0x80;
                                            int d2
                                                = (pimod->modAmtSrcOper & 0x100)
                                                  == 0x100;
                                            int p2
                                                = (pimod->modAmtSrcOper & 0x200)
                                                  == 0x200;
                                            int typ2 = (pimod->modAmtSrcOper
                                                        & 0xFC00)
                                                       >> 10;
                                            printf(
                                                ">\t\t\t\t\tsrc2 idx = %d, cc "
                                                "= %d (0=general, 1=midi "
                                                "ctrl), d = %d, p = %d, typ = "
                                                "%d "
                                                "(0=linear,1=concave,2=convex,"
                                                "3=switch)\n",
                                                idx2,
                                                cc2,
                                                d2,
                                                p2,
                                                typ2);
                                        }
                                        printf(">\t\t\t\t\tdest gen = %d(%s), ",
                                               pimod->modDestOper,
                                               tsf_smurf_gen_name(
                                                   pimod->modDestOper));
                                        printf("amount = %d, ",
                                               pimod->modAmount);
                                        printf("trans = %d (0=linear, 2=abs)\n",
                                               pimod->modTransOper);
                                    } // debug_wantlearn

                                    struct tsf_modulator* exist_modulator
                                        = zoneRegion.modulators;
                                    for(; exist_modulator != modulator;
                                        exist_modulator++)
                                    {
                                        if(exist_modulator->modSrcOper
                                               == pimod->modSrcOper
                                           && exist_modulator->modDestOper
                                                  == pimod->modDestOper
                                           && exist_modulator->modAmtSrcOper
                                                  == pimod->modAmtSrcOper)
                                        {
                                            modulator->modAmount
                                                = pimod->modAmount;
                                            modulator->modTransOper
                                                = pimod->modTransOper;
                                            break;
                                        }
                                    }
                                    if(exist_modulator == modulator)
                                    {
                                        modulator->modSrcOper
                                            = pimod->modSrcOper;
                                        modulator->modDestOper
                                            = pimod->modDestOper;
                                        modulator->modAmtSrcOper
                                            = pimod->modAmtSrcOper;
                                        modulator->modAmount = pimod->modAmount;
                                        modulator->modTransOper
                                            = pimod->modTransOper;

                                        modulator++;
                                        zoneRegion.modulatorNum++;
                                    }
                                }
                            }

                            int hadSampleID = 0;
                            for(pigen = hydra->igens + pibag->instGenNdx,
                            pigenEnd  = hydra->igens + pibag[1].instGenNdx;
                                pigen != pigenEnd;
                                pigen++)
                            {
                                if(debug_wantlearn)
                                    printf(
                                        ">\t\t\t\tigen [%lu] %d(%s)=0x%x "
                                        "(%d)\n",
                                        (uintptr_t)(pigen
                                                    - (hydra->igens
                                                       + pibag->instGenNdx)),
                                        pigen->genOper,
                                        tsf_smurf_gen_name(pigen->genOper),
                                        pigen->genAmount.wordAmount,
                                        pigen->genAmount.shortAmount);

                                if(pigen->genOper == GenSampleID)
                                {
                                    struct tsf_hydra_shdr* pshdr;

                                    //preset region key and vel ranges are a filter for the zone regions
                                    if(zoneRegion.hikey < presetRegion.lokey
                                       || zoneRegion.lokey > presetRegion.hikey)
                                        continue;
                                    if(zoneRegion.hivel < presetRegion.lovel
                                       || zoneRegion.lovel > presetRegion.hivel)
                                        continue;
                                    if(presetRegion.lokey > zoneRegion.lokey)
                                        zoneRegion.lokey = presetRegion.lokey;
                                    if(presetRegion.hikey < zoneRegion.hikey)
                                        zoneRegion.hikey = presetRegion.hikey;
                                    if(presetRegion.lovel > zoneRegion.lovel)
                                        zoneRegion.lovel = presetRegion.lovel;
                                    if(presetRegion.hivel < zoneRegion.hivel)
                                        zoneRegion.hivel = presetRegion.hivel;

                                    //sum regions
                                    tsf_region_operator(&zoneRegion,
                                                        0,
                                                        TSF_NULL,
                                                        &presetRegion);

                                    // EG times need to be converted from timecents to seconds.
                                    tsf_region_envtosecs(&zoneRegion.ampenv,
                                                         TSF_TRUE);
                                    tsf_region_envtosecs(&zoneRegion.modenv,
                                                         TSF_FALSE);

                                    // LFO times need to be converted from timecents to seconds.
                                    zoneRegion.delayModLFO
                                        = (zoneRegion.delayModLFO < -11950.0f
                                               ? 0.0f
                                               : tsf_timecents2Secsf(
                                                     zoneRegion.delayModLFO));
                                    zoneRegion.delayVibLFO
                                        = (zoneRegion.delayVibLFO < -11950.0f
                                               ? 0.0f
                                               : tsf_timecents2Secsf(
                                                     zoneRegion.delayVibLFO));

                                    // Fixup sample positions
                                    pshdr = &hydra->shdrs[pigen->genAmount
                                                              .wordAmount];
                                    zoneRegion.instrumentID = whichInst;
                                    zoneRegion.sampleID
                                        = pigen->genAmount.wordAmount;
                                    zoneRegion.offset += pshdr->start;
                                    zoneRegion.end += pshdr->end;
                                    zoneRegion.loop_start += pshdr->startLoop;
                                    zoneRegion.loop_end += pshdr->endLoop;
                                    if(pshdr->endLoop > 0)
                                        zoneRegion.loop_end -= 1;
                                    if(zoneRegion.loop_end > fontSampleCount)
                                        zoneRegion.loop_end = fontSampleCount;
                                    if(zoneRegion.pitch_keycenter == -1)
                                        zoneRegion.pitch_keycenter
                                            = pshdr->originalPitch;
                                    zoneRegion.tune += pshdr->pitchCorrection;
                                    zoneRegion.sample_rate = pshdr->sampleRate;
                                    if(zoneRegion.end
                                       && zoneRegion.end < fontSampleCount)
                                        zoneRegion.end++;
                                    else
                                        zoneRegion.end = fontSampleCount;
                                    tsf_region_clear(
                                        &preset->regions[region_index],
                                        TSF_FALSE);
                                    tsf_region_copy(
                                        &preset->regions[region_index],
                                        &zoneRegion);
                                    region_index++;
                                    hadSampleID++;
                                }
                                else
                                    tsf_region_operator(&zoneRegion,
                                                        pigen->genOper,
                                                        &pigen->genAmount,
                                                        TSF_NULL);
                            }

                            // Handle instrument's global zone. only once.
                            if(pibag == hydra->ibags + pinst->instBagNdx
                               && hadSampleID == 0)
                            {
                                tsf_region_copy(&instRegion, &zoneRegion);
                            }

                            if(zoneRegion.modulators)
                                TSF_FREE(zoneRegion.modulators);
                        }

                        if(instRegion.modulators)
                            TSF_FREE(instRegion.modulators);
                        hadGenInstrument = TSF_TRUE;
                    }
                    else
                        tsf_region_operator(&presetRegion,
                                            ppgen->genOper,
                                            &ppgen->genAmount,
                                            TSF_NULL);
                }

                // In SoundFont 2.00, no modulators have yet been defined, and the PMOD sub-chunk will always consist
                // of ten zero valued bytes.
                // NOTE: SoundFont 2.01 Technical Specification - Page 23 - Printed 12/10/1996 5:57 PM
                struct tsf_hydra_pmod *ppmod, *ppmodEnd;
                // Modulators in the PMOD sub-chunk act as additively relative modulators with respect to those in the IMOD sub-chunk. In
                // other words, a PMOD modulator can increase or decrease the amount of an IMOD modulator.
                for(ppmod = hydra->pmods + ppbag->modNdx,
                ppmodEnd  = hydra->pmods + ppbag[1].modNdx;
                    ppmod != ppmodEnd;
                    ppmod++)
                {
                    if(debug_wantlearn)
                        printf(
                            ">\t\tpmod [%lu] src:%x dest gen:%d(%s)\n",
                            (uintptr_t)(ppmod - (hydra->pmods + ppbag->modNdx)),
                            ppmod->modSrcOper,
                            ppmod->modDestOper,
                            tsf_smurf_gen_name(ppmod->modDestOper));
                }

                // Handle preset's global zone.
                if(ppbag == hydra->pbags + pphdr->presetBagNdx
                   && !hadGenInstrument)
                {
                    tsf_region_copy(&globalRegion, &presetRegion);
                }

                if(presetRegion.modulators)
                    TSF_FREE(presetRegion.modulators);
            }
        }
        return 1;
    }

#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
    static int tsf_decode_ogg(const tsf_u8* pSmpl,
                              const tsf_u8* pSmplEnd,
                              float**       pRes,
                              tsf_u32*      pResNum,
                              tsf_u32*      pResMax,
                              tsf_u32       resInitial)
    {
        float *     res    = *pRes, *oldres;
        tsf_u32     resNum = *pResNum;
        tsf_u32     resMax = *pResMax;
        stb_vorbis* v;

// Use whatever stb_vorbis API that is available (either pull or push)
#if !defined(STB_VORBIS_NO_PULLDATA_API) && !defined(STB_VORBIS_NO_FROMMEMORY)
        v = stb_vorbis_open_memory(
            pSmpl, (int)(pSmplEnd - pSmpl), TSF_NULL, TSF_NULL);
#else
        {
            int use, err;
            v = stb_vorbis_open_pushdata(
                pSmpl, (int)(pSmplEnd - pSmpl), &use, &err, TSF_NULL);
            pSmpl += use;
        }
#endif
        if(v == TSF_NULL)
            return 0;

        for(;;)
        {
            float** outputs;
            int     n_samples;

// Decode one frame of vorbis samples with whatever stb_vorbis API that is available
#if !defined(STB_VORBIS_NO_PULLDATA_API) && !defined(STB_VORBIS_NO_FROMMEMORY)
            n_samples = stb_vorbis_get_frame_float(v, TSF_NULL, &outputs);
            if(!n_samples)
                break;
#else
            if(pSmpl >= pSmplEnd)
                break;
            {
                int use
                    = stb_vorbis_decode_frame_pushdata(v,
                                                       pSmpl,
                                                       (int)(pSmplEnd - pSmpl),
                                                       TSF_NULL,
                                                       &outputs,
                                                       &n_samples);
                pSmpl += use;
            }
            if(!n_samples)
                continue;
#endif

            // Expand our output buffer if necessary then copy over the decoded frame samples
            resNum += n_samples;
            if(resNum > resMax)
            {
                do
                {
                    resMax += (resMax ? (resMax < 1048576 ? resMax : 1048576)
                                      : resInitial);
                } while(resNum > resMax);
                res = (float*)TSF_REALLOC((oldres = res),
                                          resMax * sizeof(float));
                if(!res)
                {
                    TSF_FREE(oldres);
                    stb_vorbis_close(v);
                    return 0;
                }
            }
            TSF_MEMCPY(res + resNum - n_samples,
                       outputs[0],
                       n_samples * sizeof(float));
        }
        stb_vorbis_close(v);
        *pRes    = res;
        *pResNum = resNum;
        *pResMax = resMax;
        return 1;
    }

    static int tsf_decode_sf3_samples(const void*       rawBuffer,
                                      float**           pFloatBuffer,
                                      unsigned int*     pSmplCount,
                                      struct tsf_hydra* hydra)
    {
        const tsf_u8* smplBuffer = (const tsf_u8*)rawBuffer;
        tsf_u32       smplLength = *pSmplCount, resNum = 0, resMax = 0,
                resInitial
                = (smplLength > 0x100000 ? (smplLength & ~0xFFFFF) : 65536);
        float *res = TSF_NULL, *oldres;
        int    i, shdrLast = hydra->shdrNum - 1, is_sf3 = 0;
        for(i = 0; i <= shdrLast; i++)
        {
            struct tsf_hydra_shdr* shdr = &hydra->shdrs[i];
            if(shdr->sampleType
               & 0x30) // compression flags (sometimes Vorbis flag)
            {
                const tsf_u8 *pSmpl    = smplBuffer + shdr->start,
                             *pSmplEnd = smplBuffer + shdr->end;
                if(pSmpl + 4 > pSmplEnd || !TSF_FourCCEquals(pSmpl, "OggS"))
                {
                    shdr->start = shdr->end = shdr->startLoop = shdr->endLoop
                        = 0;
                    continue;
                }

                // Fix up sample indices in shdr (end index is set after decoding)
                shdr->start = resNum;
                shdr->startLoop += resNum;
                shdr->endLoop += resNum;
                if(!tsf_decode_ogg(
                       pSmpl, pSmplEnd, &res, &resNum, &resMax, resInitial))
                {
                    TSF_FREE(res);
                    return 0;
                }
                shdr->end = resNum;
                is_sf3    = 1;
            }
            else // raw PCM sample
            {
                float*  out;
                short * in        = (short*)smplBuffer + resNum, *inEnd;
                tsf_u32 oldResNum = resNum;
                if(is_sf3) // Fix up sample indices in shdr
                {
                    tsf_u32 fix_offset = resNum - shdr->start;
                    in -= fix_offset;
                    shdr->start = resNum;
                    shdr->end += fix_offset;
                    shdr->startLoop += fix_offset;
                    shdr->endLoop += fix_offset;
                }
                inEnd = in
                        + ((shdr->end >= shdr->endLoop ? shdr->end
                                                       : shdr->endLoop)
                           - resNum);
                if(i == shdrLast || (tsf_u8*)inEnd > (smplBuffer + smplLength))
                    inEnd = (short*)(smplBuffer + smplLength);
                if(inEnd <= in)
                    continue;

                // expand our output buffer if necessary then convert the PCM data from short to float
                resNum += (tsf_u32)(inEnd - in);
                if(resNum > resMax)
                {
                    do
                    {
                        resMax
                            += (resMax ? (resMax < 1048576 ? resMax : 1048576)
                                       : resInitial);
                    } while(resNum > resMax);
                    res = (float*)TSF_REALLOC((oldres = res),
                                              resMax * sizeof(float));
                    if(!res)
                    {
                        TSF_FREE(oldres);
                        return 0;
                    }
                }

                // Convert the samples from short to float
                for(out = res + oldResNum; in < inEnd;)
                    *(out++) = (float)(*(in++) / 32767.0);
            }
        }

        // Trim the sample buffer down then return success (unless out of memory)
        if(!(*pFloatBuffer = (float*)TSF_REALLOC(res, resNum * sizeof(float))))
            *pFloatBuffer = res;
        *pSmplCount = resNum;
        return (res ? 1 : 0);
    }
#endif

    static int tsf_load_samples(void**                pRawBuffer,
                                float**               pFloatBuffer,
                                unsigned int*         pSmplCount,
                                struct tsf_riffchunk* chunkSmpl,
                                struct tsf_stream*    stream)
    {
#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
        // With OGG Vorbis support we cannot pre-allocate the memory for tsf_decode_sf3_samples
        tsf_u32 resNum, resMax;
        float*  oldres;
        *pSmplCount = chunkSmpl->size;
        *pRawBuffer = (void*)TSF_MALLOC(*pSmplCount);
        if(!*pRawBuffer
           || !stream->read(stream->data, *pRawBuffer, chunkSmpl->size))
            return 0;
        if(chunkSmpl->id[3] != 'o')
            return 1;

        // Decode custom .sfo 'smpo' format where all samples are in a single ogg stream
        resNum = resMax = 0;
        if(!tsf_decode_ogg((tsf_u8*)*pRawBuffer,
                           (tsf_u8*)*pRawBuffer + chunkSmpl->size,
                           pFloatBuffer,
                           &resNum,
                           &resMax,
                           65536))
            return 0;
        if(!(*pFloatBuffer = (float*)TSF_REALLOC((oldres = *pFloatBuffer),
                                                 resNum * sizeof(float))))
            *pFloatBuffer = oldres;
        *pSmplCount = resNum;
        return (*pFloatBuffer ? 1 : 0);
#else
    // Inline convert the samples from short to float
    float *      res, *out;
    const short* in;
    (void)pRawBuffer;
    *pSmplCount   = chunkSmpl->size / (unsigned int)sizeof(short);
    *pFloatBuffer = (float*)TSF_MALLOC(*pSmplCount * sizeof(float));
    if(!*pFloatBuffer
       || !stream->read(stream->data, *pFloatBuffer, chunkSmpl->size))
        return 0;
    for(res = *pFloatBuffer,
    out     = res + *pSmplCount,
    in      = (short*)res + *pSmplCount;
        out != res;)
        *(--out) = (float)(*(--in) / 32767.0);
    return 1;
#endif
    }

    static int tsf_register_samples(tsf* res, struct tsf_hydra* hydra)
    {
        res->sampleNum = hydra->shdrNum - 1;
        res->samples   = (struct tsf_sample*)TSF_MALLOC(
            res->sampleNum * sizeof(struct tsf_sample));
        if(!res->samples)
            return 0;

        for(int i = 0; i < res->sampleNum; i++)
        {
            struct tsf_hydra_shdr* shdr   = &hydra->shdrs[i];
            struct tsf_sample*     sample = &res->samples[i];
            TSF_MEMCPY(
                sample->sampleName, shdr->sampleName, sizeof(shdr->sampleName));
            sample->sampleName[sizeof(sample->sampleName) - 1] = '\0';
            sample->start                                      = shdr->start;
            sample->end                                        = shdr->end;
            sample->startLoop       = shdr->startLoop;
            sample->endLoop         = shdr->endLoop;
            sample->sampleRate      = shdr->sampleRate;
            sample->originalPitch   = shdr->originalPitch;
            sample->pitchCorrection = shdr->pitchCorrection;
            sample->sampleLink      = shdr->sampleLink;
            sample->sampleType      = shdr->sampleType;
        }
        return 1;
    }

    //----------------------- synthesizer
    static double tanFromCents[12000];

    static float tsf_curve_convex(float value)
    {
        return value < 0.001 ? 0.0
               : value > 0.999
                   ? 1.0
                   : 1 - (-200.0L * 2 / 960.0f) * logl(value) / M_LN10;
    }

    static float tsf_curve_concave(float value)
    {
        return 1 - tsf_curve_convex(1 - value);
    }

    static float
    tsf_modulator_source_value(tsf* f, struct tsf_voice* v, int modSrc)
    {
        int idx = modSrc & 0x7F;
        int cc  = (modSrc & 0x80) == 0x80;
        int d   = (modSrc & 0x100) == 0x100;
        int p   = (modSrc & 0x200) == 0x200;
        int typ = (modSrc & 0xFC00) >> 10;

        int                       rawSourceValue = 0;
        const struct tsf_channel* c
            = v->playingChannel != -1
                  ? &f->channels->channels[v->playingChannel]
                  : TSF_NULL;

        if(cc)
        {
            switch(idx)
            {
                case 1: rawSourceValue = c ? c->modWheel : 0; break;
                case 7: rawSourceValue = c ? c->midiVolume : 16383; break;
                case 11: rawSourceValue = c ? c->midiExpression : 16383; break;
                case 10: rawSourceValue = c ? c->midiPan : 8192; break;
                case 74: rawSourceValue = c ? c->midiFc << 7 : 8192; break;
                case 71:
                    rawSourceValue = c ? c->midiQ << 7 : 8192;
                    break;
                    //default:
                    //	printf("missing cc#%d\n", idx);
            }
        }
        else
        {
            switch(idx)
            {
                case 0: // No-Controller
                    rawSourceValue = (127 << 7) + 127;
                    break;
                case 2: // Note-On Velocity
                    rawSourceValue = (v->playingVelocity << 7) + 127;
                    break;
                case 3: // Note-O Number
                    rawSourceValue = v->playingKey << 7;
                    break;
                case 127: // Link
                    break;
                default:
                    if(!c)
                        return 1.0;

                    switch(idx)
                    {
                        case 10: // TODO: Poly Pressure
                            rawSourceValue = 0;
                            break;
                        case 13: // TODO: Channel Pressure
                            rawSourceValue = 0;
                            break;
                        case 14: // Pitch Wheel
                            rawSourceValue = c->pitchWheel;
                            break;
                        case 16: // Pitch Wheel Sensitivity
                            rawSourceValue = c->pitchRange * 127;
                            break;
                    }
            }
        }

        if(rawSourceValue > 16383)
            rawSourceValue = 16383;

        float value = rawSourceValue / 16383.0;

        if(d)
        {
            value = 1 - value;
        }

        switch(typ)
        {
            case 0: // linear
                if(p)
                { // bipolar
                    return value * 2 - 1;
                }
                break;
            case 1: // concave
                if(p)
                {
                    if(value < 0.5)
                    {
                        value = 2 * value;
                        return -tsf_curve_concave(1 - value);
                    }
                    value = 2 * value - 1;
                }
                value = tsf_curve_concave(value);
                break;
            case 2: // convex
                if(p)
                {
                    if(value < 0.5)
                    {
                        value = 2 * value;
                        return -tsf_curve_convex(1 - value);
                    }
                    value = 2 * value - 1;
                }
                value = tsf_curve_convex(value);
                break;
            case 3: // switch
                value = value > 0.5 ? 1 : 0;
                if(p)
                { // multiply
                    return value * 2 - 1;
                }
                break;
        }

        return value;
    }

    static void tsf_voice_apply_modulator(tsf*                  f,
                                          struct tsf_voice*     v,
                                          struct tsf_modulator* modulator)
    {
        if(modulator->modAmount == 0)
            return;

        float value = tsf_modulator_source_value(f, v, modulator->modSrcOper);
        float value2
            = tsf_modulator_source_value(f, v, modulator->modAmtSrcOper);
        float computed = value * value2;

        if(modulator->modTransOper == 2)
        {
            if(computed < 0)
                computed = -computed;
        }

        switch(modulator->modDestOper)
        {
            case 6: // VibLfoToPitch
                v->vibLfoToPitch += computed * modulator->modAmount;
                break;
            case 8: // InitialFilterFc
                v->initialFilterFc += computed * modulator->modAmount;
                break;
            case 9: // InitialFilterQ
                v->initialFilterQ += computed * modulator->modAmount;
                break;
            case 17: // Pan
                v->pan += computed
                          * (modulator->modAmount
                             * 0.001f /* GEN_FLOAT_LIMITPAN->vfactor */
                             * 0.5f /* GEN_FLOAT_LIMITPAN */);
                break;
            case 48: // InitialAttenuation
                v->noteGainDB -= computed
                                 * (modulator->modAmount
                                    * 0.1f /* GEN_FLOAT_LIMITATTN->vfactor */);
                break;
        }

        //printf("applying modulator: %x %x -> %d = %.2f\n", modulator->modSrcOper, modulator->modAmtSrcOper, modulator->modDestOper, computed);
    }

    static int tsf_voice_envelope_release_samples(struct tsf_voice_envelope* e,
                                                  float outSampleRate)
    {
        return (int)((e->parameters.release <= 0 ? TSF_FASTRELEASETIME
                                                 : e->parameters.release)
                     * outSampleRate);
    }

    static void tsf_voice_envelope_nextsegment(struct tsf_voice_envelope* e,
                                               short active_segment,
                                               float outSampleRate)
    {
        switch(active_segment)
        {
            case TSF_SEGMENT_NONE:
                e->samplesUntilNextSegment
                    = (int)(e->parameters.delay * outSampleRate);
                if(e->samplesUntilNextSegment > 0)
                {
                    e->segment              = TSF_SEGMENT_DELAY;
                    e->segmentIsExponential = TSF_FALSE;
                    e->level                = 0.0;
                    e->slope                = 0.0;
                    return;
                }
                /* fall through */
            case TSF_SEGMENT_DELAY:
                e->samplesUntilNextSegment
                    = (int)(e->parameters.attack * outSampleRate);
                if(e->samplesUntilNextSegment > 0)
                {
                    if(!e->isAmpEnv)
                    {
                        //mod env attack duration scales with velocity (velocity of 1 is full duration, max velocity is 0.125 times duration)
                        e->samplesUntilNextSegment
                            = (int)(e->parameters.attack
                                    * ((145 - e->midiVelocity) / 144.0f)
                                    * outSampleRate);
                    }
                    e->segment              = TSF_SEGMENT_ATTACK;
                    e->segmentIsExponential = TSF_FALSE;
                    e->level                = 0.0f;
                    e->slope                = 1.0f / e->samplesUntilNextSegment;
                    return;
                }
                /* fall through */
            case TSF_SEGMENT_ATTACK:
                e->samplesUntilNextSegment
                    = (int)(e->parameters.hold * outSampleRate);
                if(e->samplesUntilNextSegment > 0)
                {
                    e->segment              = TSF_SEGMENT_HOLD;
                    e->segmentIsExponential = TSF_FALSE;
                    e->level                = 1.0f;
                    e->slope                = 0.0f;
                    return;
                }
                /* fall through */
            case TSF_SEGMENT_HOLD:
                e->samplesUntilNextSegment
                    = (int)(e->parameters.decay * outSampleRate);
                if(e->samplesUntilNextSegment > 0)
                {
                    e->segment = TSF_SEGMENT_DECAY;
                    e->level   = 1.0f;
                    if(e->isAmpEnv)
                    {
                        // I don't truly understand this; just following what LinuxSampler does.
                        float mysterySlope
                            = -9.226f / e->samplesUntilNextSegment;
                        e->slope                = TSF_EXPF(mysterySlope);
                        e->segmentIsExponential = TSF_TRUE;
                        if(e->parameters.sustain > 0.0f)
                        {
                            // Again, this is following LinuxSampler's example, which is similar to
                            // SF2-style decay, where "decay" specifies the time it would take to
                            // get to zero, not to the sustain level.  The SFZ spec is not that
                            // specific about what "decay" means, so perhaps it's really supposed
                            // to specify the time to reach the sustain level.
                            e->samplesUntilNextSegment
                                = (int)(TSF_LOG(e->parameters.sustain)
                                        / mysterySlope);
                        }
                    }
                    else
                    {
                        e->slope = -1.0f / e->samplesUntilNextSegment;
                        e->samplesUntilNextSegment
                            = (int)(e->parameters.decay
                                    * (1.0f - e->parameters.sustain)
                                    * outSampleRate);
                        e->segmentIsExponential = TSF_FALSE;
                    }
                    return;
                }
                /* fall through */
            case TSF_SEGMENT_DECAY:
                e->segment                 = TSF_SEGMENT_SUSTAIN;
                e->level                   = e->parameters.sustain;
                e->slope                   = 0.0f;
                e->samplesUntilNextSegment = 0x7FFFFFFF;
                e->segmentIsExponential    = TSF_FALSE;
                return;
            case TSF_SEGMENT_SUSTAIN:
                e->segment = TSF_SEGMENT_RELEASE;
                e->samplesUntilNextSegment
                    = tsf_voice_envelope_release_samples(e, outSampleRate);
                if(e->isAmpEnv)
                {
                    // I don't truly understand this; just following what LinuxSampler does.
                    float mysterySlope = -9.226f / e->samplesUntilNextSegment;
                    e->slope           = TSF_EXPF(mysterySlope);
                    e->segmentIsExponential = TSF_TRUE;
                }
                else
                {
                    e->slope = -e->level / e->samplesUntilNextSegment;
                    e->segmentIsExponential = TSF_FALSE;
                }
                return;
            case TSF_SEGMENT_RELEASE:
            default:
                e->segment              = TSF_SEGMENT_DONE;
                e->segmentIsExponential = TSF_FALSE;
                e->level = e->slope        = 0.0f;
                e->samplesUntilNextSegment = 0x7FFFFFF;
        }
    }

    static void tsf_voice_envelope_setup(struct tsf_voice_envelope* e,
                                         struct tsf_envelope* new_parameters,
                                         int                  midiNoteNumber,
                                         short                midiVelocity,
                                         TSF_BOOL             isAmpEnv,
                                         float                outSampleRate)
    {
        e->parameters = *new_parameters;
        if(e->parameters.keynumToHold)
        {
            e->parameters.hold
                += e->parameters.keynumToHold * (60.0f - midiNoteNumber);
            e->parameters.hold
                = (e->parameters.hold < -10000.0f
                       ? 0.0f
                       : tsf_timecents2Secsf(e->parameters.hold));
        }
        if(e->parameters.keynumToDecay)
        {
            e->parameters.decay
                += e->parameters.keynumToDecay * (60.0f - midiNoteNumber);
            e->parameters.decay
                = (e->parameters.decay < -10000.0f
                       ? 0.0f
                       : tsf_timecents2Secsf(e->parameters.decay));
        }
        e->midiVelocity = midiVelocity;
        e->isAmpEnv     = isAmpEnv;
        tsf_voice_envelope_nextsegment(e, TSF_SEGMENT_NONE, outSampleRate);
    }

    static void tsf_voice_envelope_process(struct tsf_voice_envelope* e,
                                           int   numSamples,
                                           float outSampleRate)
    {
        if(e->slope)
        {
            if(e->segmentIsExponential)
                e->level *= TSF_POWF(e->slope, (float)numSamples);
            else
                e->level += (e->slope * numSamples);
        }
        if((e->samplesUntilNextSegment -= numSamples) <= 0)
            tsf_voice_envelope_nextsegment(e, e->segment, outSampleRate);
    }

    static void tsf_voice_lowpass_setup(struct tsf_voice_lowpass* e,
                                        float                     sampleRate,
                                        float                     fres,
                                        float                     qdB)
    {
        //float Fc = (fres < 13500 ? tsf_cents2Hertz(fres) / sampleRate : 0.5f);
        e->active = fres < 13500; // (Fc < 0.499f);
        if(!e->active)
            return;

        e->QInv = 1.0 / TSF_POW(10.0, (qdB / 20.0));

        // Lowpass filter from http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/

        //double K = TSF_TAN(TSF_PI * Fc);
        double K = tanFromCents[(short)fres - 1500];

        double KK   = K * K;
        double norm = 1 / (1 + K * e->QInv + KK);
        e->a0       = KK * norm;
        e->a1       = 2 * e->a0;
        e->b1       = 2 * (KK - 1) * norm;
        e->b2       = (1 - K * e->QInv + KK) * norm;
    }

    static float tsf_voice_lowpass_process(struct tsf_voice_lowpass* e,
                                           double                    In)
    {
        double Out = In * e->a0 + e->z1;
        e->z1      = In * e->a1 + e->z2 - e->b1 * Out;
        e->z2      = In * e->a0 - e->b2 * Out;
        return (float)Out;
    }

    static void tsf_voice_lfo_setup(struct tsf_voice_lfo* e,
                                    float                 delay,
                                    int                   freqCents,
                                    float                 outSampleRate)
    {
        e->samplesUntil = (int)(delay * outSampleRate);
        e->delta = (4.0f * tsf_cents2Hertz((float)freqCents) / outSampleRate);
        e->level = 0;
    }

    static void tsf_voice_lfo_process(struct tsf_voice_lfo* e, int blockSamples)
    {
        if(e->samplesUntil > blockSamples)
        {
            e->samplesUntil -= blockSamples;
            return;
        }
        e->level += e->delta * blockSamples;
        if(e->level > 1.0f)
        {
            e->delta = -e->delta;
            e->level = 2.0f - e->level;
        }
        else if(e->level < -1.0f)
        {
            e->delta = -e->delta;
            e->level = -2.0f - e->level;
        }
    }

    static void tsf_voice_kill(struct tsf_voice* v)
    {
        v->playingPreset = -1;
    }

    static void tsf_voice_end(tsf* f, struct tsf_voice* v)
    {
        // if maxVoiceNum is set, assume that voice rendering and note queuing are on separate threads
        // so to minimize the chance that voice rendering would advance the segment at the same time
        // we just do it twice here and hope that it sticks
        int repeats = (f->maxVoiceNum ? 2 : 1);
        while(repeats--)
        {
            tsf_voice_envelope_nextsegment(
                &v->ampenv, TSF_SEGMENT_SUSTAIN, f->outSampleRate);
            tsf_voice_envelope_nextsegment(
                &v->modenv, TSF_SEGMENT_SUSTAIN, f->outSampleRate);
            if(v->region->loop_mode == TSF_LOOPMODE_SUSTAIN)
            {
                // Continue playing, but stop looping.
                v->loopEnd = v->loopStart;
            }
        }
    }

    static void tsf_voice_endquick(tsf* f, struct tsf_voice* v)
    {
        // if maxVoiceNum is set, assume that voice rendering and note queuing are on separate threads
        // so to minimize the chance that voice rendering would advance the segment at the same time
        // we just do it twice here and hope that it sticks
        int repeats = (f->maxVoiceNum ? 2 : 1);
        while(repeats--)
        {
            v->ampenv.parameters.release = 0.0f;
            tsf_voice_envelope_nextsegment(
                &v->ampenv, TSF_SEGMENT_SUSTAIN, f->outSampleRate);
            v->modenv.parameters.release = 0.0f;
            tsf_voice_envelope_nextsegment(
                &v->modenv, TSF_SEGMENT_SUSTAIN, f->outSampleRate);
        }
    }

    static void tsf_voice_calcpitchratio(struct tsf_voice* v,
                                         float             pitchShift,
                                         float             outSampleRate)
    {
        double note
            = v->playingKey + v->region->transpose + v->region->tune / 100.0;
        double adjustedPitch = v->region->pitch_keycenter
                               + (note - v->region->pitch_keycenter)
                                     * (v->region->pitch_keytrack / 100.0);
        if(pitchShift)
            adjustedPitch += pitchShift;
        v->pitchInputTimecents = adjustedPitch * 100.0;
        v->pitchOutputFactor
            = v->region->sample_rate
              / (tsf_timecents2Secsd(v->region->pitch_keycenter * 100.0)
                 * outSampleRate);
    }

    static void tsf_voice_render(tsf*              f,
                                 struct tsf_voice* v,
                                 float*            outputBuffer,
                                 int               numSamples)
    {
        struct tsf_region* region = v->region;
        float*             input  = f->fontSamples;
        float*             outL   = outputBuffer;
        float* outR = (f->outputmode == TSF_STEREO_UNWEAVED ? outL + numSamples
                                                            : TSF_NULL);
        float  lowpassFilterQDB, lowpassFc;
        //static const float SB_EMU_DB = 0.4f;
        v->noteGainDB      = f->globalGainDB - (region->attenuation * 0.1f);
        v->pan             = region->pan;
        v->initialFilterFc = region->initialFilterFc;
        v->initialFilterQ  = region->initialFilterQ;
        v->vibLfoToPitch   = region->vibLfoToPitch;

        // Applying Modulators
        if(region->modulators && region->modulatorNum)
            for(struct tsf_modulator* modulator = region->modulators;
                modulator != region->modulators + region->modulatorNum;
                modulator++)
            {
                tsf_voice_apply_modulator(f, v, modulator);
            }

        // Attenuation
        if(v->noteGainDB > 0.0f)
            v->noteGainDB = 0.0;
        else if(v->noteGainDB < -144.0f)
            v->noteGainDB = -144.0f;

        // Pan
        if(v->pan <= -0.5f)
        {
            v->panFactorLeft  = 1.0f;
            v->panFactorRight = 0.0f;
        }
        else if(v->pan >= 0.5f)
        {
            v->panFactorLeft  = 0.0f;
            v->panFactorRight = 1.0f;
        }
        else
        {
            v->panFactorLeft  = TSF_SQRTF(0.5f - v->pan);
            v->panFactorRight = TSF_SQRTF(0.5f + v->pan);
        }

        // Cache some values, to give them at least some chance of ending up in registers.
        TSF_BOOL updateModEnv
            = (region->modEnvToPitch || region->modEnvToFilterFc);
        TSF_BOOL updateModLFO
            = (v->modlfo.delta
               && (region->modLfoToPitch || region->modLfoToFilterFc
                   || region->modLfoToVolume));
        TSF_BOOL     updateVibLFO = (v->viblfo.delta && (v->vibLfoToPitch));
        TSF_BOOL     isLooping    = (v->loopStart < v->loopEnd);
        unsigned int tmpLoopStart = v->loopStart, tmpLoopEnd = v->loopEnd,
                     tmpSampleEnd           = region->end;
        double tmpSampleEndDbl              = (double)tmpSampleEnd,
               tmpLoopEndDbl                = (double)tmpLoopEnd + 1.0;
        double tmpSourceSamplePosition      = v->sourceSamplePosition;
        struct tsf_voice_lowpass tmpLowpass = v->lowpass;

        TSF_BOOL dynamicLowpass
            = (region->modLfoToFilterFc || region->modEnvToFilterFc
               || (v->initialFilterFc != 13500
                   || v->initialFilterQ
                          != 0)); // v->filterFc != 0.0 || v->filterQ != 0.0;
        float tmpSampleRate = f->outSampleRate, tmpInitialFilterFc,
              tmpInitialFilterQ, tmpModLfoToFilterFc, tmpModEnvToFilterFc;

        TSF_BOOL dynamicPitchRatio
            = (region->modLfoToPitch || region->modEnvToPitch
               || v->vibLfoToPitch);
        double pitchRatio;
        float  tmpModLfoToPitch, tmpVibLfoToPitch, tmpModEnvToPitch;

        TSF_BOOL dynamicGain = (region->modLfoToVolume != 0);
        float    noteGain    = 0, tmpModLfoToVolume;

        if(dynamicLowpass)
        {
            tmpModLfoToFilterFc = (float)region->modLfoToFilterFc;
            tmpModEnvToFilterFc = (float)region->modEnvToFilterFc;

            tmpInitialFilterFc
                = v->initialFilterFc; // + TSF_CC74_AMOUNT * v->filterFc;
            if(tmpInitialFilterFc < 1500)
                tmpInitialFilterFc = 1500;
            else if(tmpInitialFilterFc > 13500)
                tmpInitialFilterFc = 13500;

            tmpInitialFilterQ
                = v->initialFilterQ; // + TSF_CC71_AMOUNT * v->filterQ;
            if(tmpInitialFilterQ < 0)
                tmpInitialFilterQ = 0;
            else if(tmpInitialFilterQ > 960)
                tmpInitialFilterQ = 960;
        }
        else
            tmpInitialFilterFc = 0, tmpModLfoToFilterFc = 0,
            tmpModEnvToFilterFc = 0;

        if(dynamicPitchRatio)
            pitchRatio = 0, tmpModLfoToPitch = (float)region->modLfoToPitch,
            tmpVibLfoToPitch = (float)v->vibLfoToPitch,
            tmpModEnvToPitch = (float)region->modEnvToPitch;
        else
            pitchRatio = tsf_timecents2Secsd(v->pitchInputTimecents)
                         * v->pitchOutputFactor,
            tmpModLfoToPitch = 0, tmpVibLfoToPitch = 0, tmpModEnvToPitch = 0;

        if(dynamicGain)
            tmpModLfoToVolume = (float)region->modLfoToVolume * 0.1f;
        else
            noteGain = tsf_decibelsToGain(v->noteGainDB), tmpModLfoToVolume = 0;

        while(numSamples)
        {
            float gainMono, gainLeft, gainRight;
            int   blockSamples = (numSamples > TSF_RENDER_EFFECTSAMPLEBLOCK
                                      ? TSF_RENDER_EFFECTSAMPLEBLOCK
                                      : numSamples);
            numSamples -= blockSamples;

            if(dynamicLowpass)
            {
                float fres = tmpInitialFilterFc
                             + v->modlfo.level * tmpModLfoToFilterFc
                             + v->modenv.level * tmpModEnvToFilterFc;
                tsf_voice_lowpass_setup(
                    &tmpLowpass, tmpSampleRate, fres, tmpInitialFilterQ * 0.1);
            }

            if(dynamicPitchRatio)
                pitchRatio = tsf_timecents2Secsd(
                                 v->pitchInputTimecents
                                 + (v->modlfo.level * tmpModLfoToPitch
                                    + v->viblfo.level * tmpVibLfoToPitch
                                    + v->modenv.level * tmpModEnvToPitch))
                             * v->pitchOutputFactor;

            if(dynamicGain)
                noteGain = tsf_decibelsToGain(
                    v->noteGainDB + (v->modlfo.level * tmpModLfoToVolume));

            gainMono = noteGain * v->ampenv.level;

            // Update EG.
            tsf_voice_envelope_process(&v->ampenv, blockSamples, tmpSampleRate);
            if(updateModEnv)
                tsf_voice_envelope_process(
                    &v->modenv, blockSamples, tmpSampleRate);

            // Update LFOs.
            if(updateModLFO)
                tsf_voice_lfo_process(&v->modlfo, blockSamples);
            if(updateVibLFO)
                tsf_voice_lfo_process(&v->viblfo, blockSamples);

            switch(f->outputmode)
            {
                case TSF_STEREO_INTERLEAVED:
                    gainLeft  = gainMono * v->panFactorLeft,
                    gainRight = gainMono * v->panFactorRight;
                    while(blockSamples--
                          && tmpSourceSamplePosition < tmpSampleEndDbl)
                    {
                        unsigned int pos
                            = (unsigned int)tmpSourceSamplePosition;
                        unsigned int nextPos1
                            = (pos >= tmpLoopEnd && isLooping ? tmpLoopStart
                                                              : pos + 1);

                        // TODO: for cubic sampler
                        //unsigned int nextPos2 = (nextPos1 >= tmpLoopEnd && isLooping ? tmpLoopStart : nextPos1 + 1);
                        //unsigned int nextPos3 = (nextPos2 >= tmpLoopEnd && isLooping ? tmpLoopStart : nextPos2 + 1);

                        // Simple linear interpolation.
                        float alpha = (float)(tmpSourceSamplePosition - pos);
                        float val   = (input[pos] * (1.0f - alpha)
                                     + input[nextPos1] * alpha);

                        // Low-pass filter.
                        if(tmpLowpass.active)
                            val = tsf_voice_lowpass_process(&tmpLowpass, val);

                        *outL++ += val * gainLeft;
                        *outL++ += val * gainRight;

                        // Next sample.
                        tmpSourceSamplePosition += pitchRatio;
                        if(tmpSourceSamplePosition >= tmpLoopEndDbl
                           && isLooping)
                            tmpSourceSamplePosition
                                -= (tmpLoopEnd - tmpLoopStart + 1.0);
                    }
                    break;

                case TSF_STEREO_UNWEAVED:
                    gainLeft  = gainMono * v->panFactorLeft,
                    gainRight = gainMono * v->panFactorRight;
                    while(blockSamples--
                          && tmpSourceSamplePosition < tmpSampleEndDbl)
                    {
                        unsigned int pos
                            = (unsigned int)tmpSourceSamplePosition,
                            nextPos
                            = (pos >= tmpLoopEnd && isLooping ? tmpLoopStart
                                                              : pos + 1);

                        // Simple linear interpolation.
                        float alpha = (float)(tmpSourceSamplePosition - pos),
                              val   = (input[pos] * (1.0f - alpha)
                                     + input[nextPos] * alpha);

                        // Low-pass filter.
                        if(tmpLowpass.active)
                            val = tsf_voice_lowpass_process(&tmpLowpass, val);

                        *outL++ += val * gainLeft;
                        *outR++ += val * gainRight;

                        // Next sample.
                        tmpSourceSamplePosition += pitchRatio;
                        if(tmpSourceSamplePosition >= tmpLoopEndDbl
                           && isLooping)
                            tmpSourceSamplePosition
                                -= (tmpLoopEnd - tmpLoopStart + 1.0);
                    }
                    break;

                case TSF_MONO:
                    while(blockSamples--
                          && tmpSourceSamplePosition < tmpSampleEndDbl)
                    {
                        unsigned int pos
                            = (unsigned int)tmpSourceSamplePosition,
                            nextPos
                            = (pos >= tmpLoopEnd && isLooping ? tmpLoopStart
                                                              : pos + 1);

                        // Simple linear interpolation.
                        float alpha = (float)(tmpSourceSamplePosition - pos),
                              val   = (input[pos] * (1.0f - alpha)
                                     + input[nextPos] * alpha);

                        // Low-pass filter.
                        if(tmpLowpass.active)
                            val = tsf_voice_lowpass_process(&tmpLowpass, val);

                        *outL++ += val * gainMono;

                        // Next sample.
                        tmpSourceSamplePosition += pitchRatio;
                        if(tmpSourceSamplePosition >= tmpLoopEndDbl
                           && isLooping)
                            tmpSourceSamplePosition
                                -= (tmpLoopEnd - tmpLoopStart + 1.0);
                    }
                    break;
            }

            if(tmpSourceSamplePosition >= tmpSampleEndDbl
               || v->ampenv.segment == TSF_SEGMENT_DONE)
            {
                tsf_voice_kill(v);
                return;
            }
        }

        v->sourceSamplePosition = tmpSourceSamplePosition;
        if(tmpLowpass.active || dynamicLowpass)
            v->lowpass = tmpLowpass;
    }

    TSFDEF tsf* tsf_load(struct tsf_stream* stream)
    {
        tsf*                 res = TSF_NULL;
        struct tsf_riffchunk chunkHead;
        struct tsf_riffchunk chunkList;
        struct tsf_hydra     hydra;
        void*                rawBuffer   = TSF_NULL;
        float*               floatBuffer = TSF_NULL;
        tsf_u32              smplCount   = 0;

#define SkipChunk                                                \
    {                                                            \
        char fourcc[5] = {chunkList.id[0],                       \
                          chunkList.id[1],                       \
                          chunkList.id[2],                       \
                          chunkList.id[3],                       \
                          0};                                    \
        TSF_WARN("Skipping '%s' (%d)\n", fourcc, chunkList.size) \
        stream->skip(stream->data, chunkList.size);              \
    }

        if(!tsf_riffchunk_read(TSF_NULL, &chunkHead, stream)
           || !TSF_FourCCEquals(chunkHead.id, "sfbk"))
        {
            //if (e) *e = TSF_INVALID_NOSF2HEADER;
            TSF_ERROR("TSF_INVALID_NOSF2HEADER\n");
            return res;
        }

        // Read hydra and locate sample data.
        TSF_MEMSET(&hydra, 0, sizeof(hydra));
        while(tsf_riffchunk_read(&chunkHead, &chunkList, stream))
        {
            struct tsf_riffchunk chunk;
            if(TSF_FourCCEquals(chunkList.id, "pdta"))
            {
                while(tsf_riffchunk_read(&chunkList, &chunk, stream))
                {
#define HandleChunk(chunkName)                                            \
    (TSF_FourCCEquals(chunk.id, #chunkName)                               \
     && !(chunk.size % chunkName##SizeInFile))                            \
    {                                                                     \
        int num              = chunk.size / chunkName##SizeInFile, i;     \
        hydra.chunkName##Num = num;                                       \
        hydra.chunkName##s   = (struct tsf_hydra_##chunkName*)TSF_MALLOC( \
            num * sizeof(struct tsf_hydra_##chunkName));                \
        if(!hydra.chunkName##s)                                           \
            goto out_of_memory;                                           \
        for(i = 0; i < num; ++i)                                          \
            tsf_hydra_read_##chunkName(&hydra.chunkName##s[i], stream);   \
    }
                    enum
                    {
                        phdrSizeInFile = 38,
                        pbagSizeInFile = 4,
                        pmodSizeInFile = 10,
                        pgenSizeInFile = 4,
                        instSizeInFile = 22,
                        ibagSizeInFile = 4,
                        imodSizeInFile = 10,
                        igenSizeInFile = 4,
                        shdrSizeInFile = 46
                    };
                    if HandleChunk(phdr)
                        else if HandleChunk(pbag) else if HandleChunk(pmod) else if HandleChunk(pgen) else if HandleChunk(inst) else if HandleChunk(
                            ibag) else if HandleChunk(imod) else if HandleChunk(igen) else if HandleChunk(shdr) else SkipChunk
#undef HandleChunk
                }
            }
            else if(TSF_FourCCEquals(chunkList.id, "sdta"))
            {
                while(tsf_riffchunk_read(&chunkList, &chunk, stream))
                {
                    if((TSF_FourCCEquals(chunk.id, "smpl")
#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
                        || TSF_FourCCEquals(chunk.id, "smpo")
#endif
                            )
                       && !rawBuffer && !floatBuffer
                       && chunk.size >= sizeof(short))
                    {
                        if(!tsf_load_samples(&rawBuffer,
                                             &floatBuffer,
                                             &smplCount,
                                             &chunk,
                                             stream))
                            goto out_of_memory;
                    }
                    else
                        SkipChunk
                }
            }
            else
                SkipChunk
        }
        if(!hydra.phdrs || !hydra.pbags || !hydra.pmods || !hydra.pgens
           || !hydra.insts || !hydra.ibags || !hydra.imods || !hydra.igens
           || !hydra.shdrs)
        {
            //if (e) *e = TSF_INVALID_INCOMPLETE;
            TSF_ERROR("TSF_INVALID_INCOMPLETE\n");
        }
        else if(!rawBuffer && !floatBuffer)
        {
            //if (e) *e = TSF_INVALID_NOSAMPLEDATA;
            TSF_ERROR("TSF_INVALID_NOSAMPLEDATA\n");
        }
        else
        {
#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H
            if(!floatBuffer
               && !tsf_decode_sf3_samples(
                   rawBuffer, &floatBuffer, &smplCount, &hydra))
                goto out_of_memory;
#endif
            res = (tsf*)TSF_MALLOC(sizeof(tsf));
            if(res)
                TSF_MEMSET(res, 0, sizeof(tsf));
            if(!res || !tsf_load_presets(res, &hydra, smplCount))
                goto out_of_memory;
            if(!res || !tsf_register_samples(res, &hydra))
                goto out_of_memory;
            res->outSampleRate   = 44100.0f;
            res->fontSamples     = floatBuffer;
            res->fontSampleCount = smplCount;
            floatBuffer          = TSF_NULL; // don't free below
        }
        if(0)
        {
        out_of_memory:
            TSF_FREE(res);
            res = TSF_NULL;
            //if (e) *e = TSF_OUT_OF_MEMORY;
            TSF_ERROR("TSF_OUT_OF_MEMORY\n");
        }
        TSF_FREE(hydra.phdrs);
        TSF_FREE(hydra.pbags);
        TSF_FREE(hydra.pmods);
        TSF_FREE(hydra.pgens);
        TSF_FREE(hydra.insts);
        TSF_FREE(hydra.ibags);
        TSF_FREE(hydra.imods);
        TSF_FREE(hydra.igens);
        TSF_FREE(hydra.shdrs);
        TSF_FREE(rawBuffer);
        TSF_FREE(floatBuffer);
        return res;
    }

    TSFDEF tsf* tsf_copy(tsf* f)
    {
        tsf* res;
        if(!f)
            return TSF_NULL;
        if(!f->refCount)
        {
            f->refCount = (int*)TSF_MALLOC(sizeof(int));
            if(!f->refCount)
                return TSF_NULL;
            *f->refCount = 1;
        }
        res = (tsf*)TSF_MALLOC(sizeof(tsf));
        if(!res)
            return TSF_NULL;
        TSF_MEMCPY(res, f, sizeof(tsf));
        res->voices   = TSF_NULL;
        res->voiceNum = 0;
        res->channels = TSF_NULL;
        (*res->refCount)++;
        return res;
    }

    TSFDEF void tsf_close(tsf* f)
    {
        if(!f)
            return;
        if(!f->refCount || !--(*f->refCount))
        {
            TSF_FREE(f->samples);
            struct tsf_preset *preset    = f->presets,
                              *presetEnd = preset + f->presetNum;
            for(; preset != presetEnd; preset++)
            {
                for(int r = 0; r != preset->regionNum; r++)
                    if(preset->regions[r].modulators)
                        TSF_FREE(preset->regions[r].modulators);
                TSF_FREE(preset->regions);
            }
            TSF_FREE(f->presets);
            TSF_FREE(f->fontSamples);
            TSF_FREE(f->refCount);
        }
        TSF_FREE(f->channels);
        TSF_FREE(f->voices);
        TSF_FREE(f);
    }

    TSFDEF void tsf_reset(tsf* f)
    {
        struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
        for(; v != vEnd; v++)
            if(v->playingPreset != -1
               && (v->ampenv.segment < TSF_SEGMENT_RELEASE
                   || v->ampenv.parameters.release))
                tsf_voice_endquick(f, v);
        if(f->channels)
        {
            TSF_FREE(f->channels);
            f->channels = TSF_NULL;
        }
    }

    TSFDEF int tsf_get_presetindex(const tsf* f, int bank, int preset_number)
    {
        const struct tsf_preset* presets;
        int                      i, iMax;
        for(presets = f->presets, i = 0, iMax = f->presetNum; i < iMax; i++)
            if(presets[i].preset == preset_number && presets[i].bank == bank)
                return i;
        return -1;
    }

    TSFDEF int tsf_get_presetcount(const tsf* f)
    {
        return f->presetNum;
    }

    TSFDEF const char* tsf_get_presetname(const tsf* f, int preset)
    {
        return (preset < 0 || preset >= f->presetNum
                    ? TSF_NULL
                    : f->presets[preset].presetName);
    }

    TSFDEF const char*
    tsf_bank_get_presetname(const tsf* f, int bank, int preset_number)
    {
        return tsf_get_presetname(f,
                                  tsf_get_presetindex(f, bank, preset_number));
    }

    TSFDEF void tsf_set_output(tsf*               f,
                               enum TSFOutputMode outputmode,
                               int                samplerate,
                               float              global_gain_db)
    {
        f->outputmode    = outputmode;
        f->outSampleRate = (float)(samplerate >= 1 ? samplerate : 44100.0f);
        f->globalGainDB  = global_gain_db;
    }

    TSFDEF void tsf_set_volume(tsf* f, float global_volume)
    {
        f->globalGainDB = (global_volume == 1.0f
                               ? 0
                               : -tsf_gainToDecibels(1.0f / global_volume));
    }

    TSFDEF int tsf_set_max_voices(tsf* f, int max_voices)
    {
        int i           = f->voiceNum;
        int newVoiceNum = (f->voiceNum > max_voices ? f->voiceNum : max_voices);
        struct tsf_voice* newVoices = (struct tsf_voice*)TSF_REALLOC(
            f->voices, newVoiceNum * sizeof(struct tsf_voice));
        if(!newVoices)
            return 0;
        f->voices   = newVoices;
        f->voiceNum = f->maxVoiceNum = newVoiceNum;
        for(; i < max_voices; i++)
            f->voices[i].playingPreset = -1;
        return 1;
    }

    TSFDEF int tsf_note_on(tsf* f, int preset_index, int key, float vel)
    {
        short              midiVelocity = (short)(vel * 127);
        unsigned int       voicePlayIndex;
        struct tsf_region *region, *regionEnd;

        if(preset_index < 0 || preset_index >= f->presetNum)
            return 1;
        if(vel <= 0.0f)
        {
            tsf_note_off(f, preset_index, key);
            return 1;
        }

        // Play all matching regions.
        voicePlayIndex = f->voicePlayIndex++;
        for(region = f->presets[preset_index].regions,
        regionEnd  = region + f->presets[preset_index].regionNum;
            region != regionEnd;
            region++)
        {
            struct tsf_voice *voice, *v, *vEnd;
            TSF_BOOL          doLoop;
            float             lowpassFilterQDB, lowpassFc;
            if(key < region->lokey || key > region->hikey
               || midiVelocity < region->lovel || midiVelocity > region->hivel)
                continue;

            voice = TSF_NULL, v = f->voices, vEnd = v + f->voiceNum;
            if(region->group)
            {
                for(; v != vEnd; v++)
                    if(v->playingPreset == preset_index
                       && v->region->group == region->group)
                        tsf_voice_endquick(f, v);
                    else if(v->playingPreset == -1 && !voice)
                        voice = v;
            }
            else
                for(; v != vEnd; v++)
                    if(v->playingPreset == -1)
                    {
                        voice = v;
                        break;
                    }

            if(!voice)
            {
                if(f->maxVoiceNum)
                {
                    // Voices have been pre-allocated and limited to a maximum, try to kill a voice off in its release envelope
                    int bestKillReleaseSamplePos = -999999999;
                    for(v = f->voices; v != vEnd; v++)
                    {
                        if(v->ampenv.segment == TSF_SEGMENT_RELEASE)
                        {
                            // We're looking for the voice furthest into its release
                            int releaseSamplesDone
                                = tsf_voice_envelope_release_samples(
                                      &v->ampenv, f->outSampleRate)
                                  - v->ampenv.samplesUntilNextSegment;
                            if(releaseSamplesDone > bestKillReleaseSamplePos)
                            {
                                bestKillReleaseSamplePos = releaseSamplesDone;
                                voice                    = v;
                            }
                        }
                    }
                    if(!voice)
                        continue;
                    tsf_voice_kill(voice);
                }
                else
                {
                    // Allocate more voices so we don't need to kill one off.
                    struct tsf_voice* newVoices;
                    f->voiceNum += 4;
                    newVoices = (struct tsf_voice*)TSF_REALLOC(
                        f->voices, f->voiceNum * sizeof(struct tsf_voice));
                    if(!newVoices)
                        return 0;
                    f->voices                    = newVoices;
                    voice                        = &f->voices[f->voiceNum - 4];
                    voice[1].playingPreset       = voice[2].playingPreset
                        = voice[3].playingPreset = -1;
                }
            }

            voice->region          = region;
            voice->playingChannel  = -1;
            voice->playingPreset   = preset_index;
            voice->playingKey      = key;
            voice->playingVelocity = midiVelocity;
            voice->playIndex       = voicePlayIndex;
            voice->heldSustain     = 0;

#if 0
		voice->noteGainDB = (f->globalGainDB - (region->attenuation / 10.0));

		// Apply default modulator: MIDI Note-On Velocity to Initial Attenuation (section 8.4.1)
		voice->noteGainDB -= tsf_gainToDecibels(1.0f / vel);
#endif

            if(f->channels)
            {
                f->channels->setupVoice(
                    f, voice); // call to: tsf_channel_setup_voice
            }
            else
            {
                tsf_voice_calcpitchratio(voice, 0, f->outSampleRate);
                // The SFZ spec is silent about the pan curve, but a 3dB pan law seems common. This sqrt() curve matches what Dimension LE does; Alchemy Free seems closer to sin(adjustedPan * pi/2).

                // TODO: rename into initialPan
                voice->pan            = region->pan;
                voice->panFactorLeft  = TSF_SQRTF(0.5f - region->pan);
                voice->panFactorRight = TSF_SQRTF(0.5f + region->pan);
            }

            // Offset/end.
            voice->sourceSamplePosition = region->offset;

            // Loop.
            doLoop           = (region->loop_mode != TSF_LOOPMODE_NONE
                      && region->loop_start < region->loop_end);
            voice->loopStart = (doLoop ? region->loop_start : 0);
            voice->loopEnd   = (doLoop ? region->loop_end : 0);

            // Setup envelopes.
            tsf_voice_envelope_setup(&voice->ampenv,
                                     &region->ampenv,
                                     key,
                                     midiVelocity,
                                     TSF_TRUE,
                                     f->outSampleRate);
            tsf_voice_envelope_setup(&voice->modenv,
                                     &region->modenv,
                                     key,
                                     midiVelocity,
                                     TSF_FALSE,
                                     f->outSampleRate);

            // Setup LFO filters.
            tsf_voice_lfo_setup(&voice->modlfo,
                                region->delayModLFO,
                                region->freqModLFO,
                                f->outSampleRate);
            tsf_voice_lfo_setup(&voice->viblfo,
                                region->delayVibLFO,
                                region->freqVibLFO,
                                f->outSampleRate);
        }
        return 1;
    }

    TSFDEF int
    tsf_bank_note_on(tsf* f, int bank, int preset_number, int key, float vel)
    {
        int preset_index = tsf_get_presetindex(f, bank, preset_number);
        if(preset_index == -1)
            return 0;
        return tsf_note_on(f, preset_index, key, vel);
    }

    TSFDEF void tsf_note_off(tsf* f, int preset_index, int key)
    {
        struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum,
                         *vMatchFirst = TSF_NULL, *vMatchLast = TSF_NULL;
        for(; v != vEnd; v++)
        {
            //Find the first and last entry in the voices list with matching preset, key and look up the smallest play index
            if(v->playingPreset != preset_index || v->playingKey != key
               || v->ampenv.segment >= TSF_SEGMENT_RELEASE)
                continue;
            else if(!vMatchFirst || v->playIndex < vMatchFirst->playIndex)
                vMatchFirst = vMatchLast = v;
            else if(v->playIndex == vMatchFirst->playIndex)
                vMatchLast = v;
        }
        if(!vMatchFirst)
            return;
        for(v = vMatchFirst; v <= vMatchLast; v++)
        {
            //Stop all voices with matching preset, key and the smallest play index which was enumerated above
            if(v != vMatchFirst && v != vMatchLast
               && (v->playIndex != vMatchFirst->playIndex
                   || v->playingPreset != preset_index || v->playingKey != key
                   || v->ampenv.segment >= TSF_SEGMENT_RELEASE))
                continue;
            tsf_voice_end(f, v);
        }
    }

    TSFDEF int tsf_bank_note_off(tsf* f, int bank, int preset_number, int key)
    {
        int preset_index = tsf_get_presetindex(f, bank, preset_number);
        if(preset_index == -1)
            return 0;
        tsf_note_off(f, preset_index, key);
        return 1;
    }

    TSFDEF void tsf_note_off_all(tsf* f)
    {
        struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
        for(; v != vEnd; v++)
            if(v->playingPreset != -1
               && v->ampenv.segment < TSF_SEGMENT_RELEASE)
                tsf_voice_end(f, v);
    }

    TSFDEF int tsf_active_voice_count(tsf* f)
    {
        int               count = 0;
        struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
        for(; v != vEnd; v++)
            if(v->playingPreset != -1)
                count++;
        return count;
    }

    TSFDEF void
    tsf_render_short(tsf* f, short* buffer, int samples, int flag_mixing)
    {
        float outputSamples[TSF_RENDER_SHORTBUFFERBLOCK];
        int   channels        = (f->outputmode == TSF_MONO ? 1 : 2),
            maxChannelSamples = TSF_RENDER_SHORTBUFFERBLOCK / channels;
        while(samples > 0)
        {
            int channelSamples
                = (samples > maxChannelSamples ? maxChannelSamples : samples);
            short* bufferEnd    = buffer + channelSamples * channels;
            float* floatSamples = outputSamples;
            tsf_render_float(f, floatSamples, channelSamples, TSF_FALSE);
            samples -= channelSamples;

            if(flag_mixing)
                while(buffer != bufferEnd)
                {
                    float v  = *floatSamples++;
                    int   vi = *buffer
                             + (v < -1.00004566f
                                    ? (int)-32768
                                    : (v > 1.00001514f ? (int)32767
                                                       : (int)(v * 32767.5f)));
                    *buffer++ = (vi < -32768
                                     ? (short)-32768
                                     : (vi > 32767 ? (short)32767 : (short)vi));
                }
            else
                while(buffer != bufferEnd)
                {
                    float v = *floatSamples++;
                    *buffer++
                        = (v < -1.00004566f
                               ? (short)-32768
                               : (v > 1.00001514f ? (short)32767
                                                  : (short)(v * 32767.5f)));
                }
        }
    }

    TSFDEF void
    tsf_render_float(tsf* f, float* buffer, int samples, int flag_mixing)
    {
        struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
        if(!flag_mixing)
            TSF_MEMSET(buffer,
                       0,
                       (f->outputmode == TSF_MONO ? 1 : 2) * sizeof(float)
                           * samples);
        for(; v != vEnd; v++)
            if(v->playingPreset != -1)
                tsf_voice_render(f, v, buffer, samples);
    }

    static void tsf_channel_setup_voice(tsf* f, struct tsf_voice* v)
    {
        struct tsf_channel* c
            = &f->channels->channels[f->channels->activeChannel];
        v->pan            = v->region->pan;
        v->playingChannel = f->channels->activeChannel;
        tsf_voice_calcpitchratio(
            v,
            (c->pitchWheel == 8192
                 ? c->tuning
                 : ((c->pitchWheel / 16383.0f * c->pitchRange * 2.0f)
                    - c->pitchRange + c->tuning)),
            f->outSampleRate);
        v->lowpass.z1 = 0;
        v->lowpass.z2 = 0;
    }

    static struct tsf_channel* tsf_channel_init(tsf* f, int channel)
    {
        int i;
        if(f->channels && channel < f->channels->channelNum)
            return &f->channels->channels[channel];
        if(!f->channels)
        {
            f->channels = (struct tsf_channels*)TSF_MALLOC(
                sizeof(struct tsf_channels)
                + sizeof(struct tsf_channel) * channel);
            if(!f->channels)
                return TSF_NULL;
            f->channels->setupVoice    = &tsf_channel_setup_voice;
            f->channels->channelNum    = 0;
            f->channels->activeChannel = 0;
        }
        else
        {
            struct tsf_channels* newChannels
                = (struct tsf_channels*)TSF_REALLOC(
                    f->channels,
                    sizeof(struct tsf_channels)
                        + sizeof(struct tsf_channel) * channel);
            if(!newChannels)
                return TSF_NULL;
            f->channels = newChannels;
        }
        i                       = f->channels->channelNum;
        f->channels->channelNum = channel + 1;
        for(; i <= channel; i++)
        {
            struct tsf_channel* c = &f->channels->channels[i];
            c->presetIndex = c->bank = c->sustain = 0;
            c->pitchWheel = c->midiPan = 8192;
            c->modWheel                = 0;
            c->midiVolume = c->midiExpression = 16383;
            c->midiRPN                        = 0xFFFF;
            c->midiData                       = 0;
            c->pitchRange                     = 2.0f;
            c->tuning                         = 0.0f;
            c->midiFc                         = 0x40;
            c->midiQ                          = 0x40;
        }
        return &f->channels->channels[channel];
    }

    static void
    tsf_channel_applypitch(tsf* f, int channel, struct tsf_channel* c)
    {
        struct tsf_voice *v, *vEnd;
        float             pitchShift
            = (c->pitchWheel == 8192
                   ? c->tuning
                   : ((c->pitchWheel / 16383.0f * c->pitchRange * 2.0f)
                      - c->pitchRange + c->tuning));
        for(v = f->voices, vEnd = v + f->voiceNum; v != vEnd; v++)
            if(v->playingPreset != -1 && v->playingChannel == channel)
                tsf_voice_calcpitchratio(v, pitchShift, f->outSampleRate);
    }

    TSFDEF int
    tsf_channel_set_presetindex(tsf* f, int channel, int preset_index)
    {
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        c->presetIndex = (unsigned short)preset_index;
        return 1;
    }

    TSFDEF int tsf_channel_set_presetnumber(tsf* f,
                                            int  channel,
                                            int  preset_number,
                                            int  flag_mididrums)
    {
        int                 preset_index;
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        if(flag_mididrums)
        {
            preset_index = tsf_get_presetindex(
                f, 128 | (c->bank & 0x7FFF), preset_number);
            if(preset_index == -1)
                preset_index = tsf_get_presetindex(f, 128, preset_number);
            if(preset_index == -1)
                preset_index = tsf_get_presetindex(f, 128, 0);
            if(preset_index == -1)
                preset_index
                    = tsf_get_presetindex(f, (c->bank & 0x7FFF), preset_number);
        }
        else
            preset_index
                = tsf_get_presetindex(f, (c->bank & 0x7FFF), preset_number);
        if(preset_index == -1)
            preset_index = tsf_get_presetindex(f, 0, preset_number);
        if(preset_index != -1)
        {
            c->presetIndex = (unsigned short)preset_index;
            return 1;
        }
        return 0;
    }

    TSFDEF int tsf_channel_set_bank(tsf* f, int channel, int bank)
    {
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        c->bank = (unsigned short)bank;
        return 1;
    }

    TSFDEF int tsf_channel_set_bank_preset(tsf* f,
                                           int  channel,
                                           int  bank,
                                           int  preset_number)
    {
        int                 preset_index;
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        preset_index = tsf_get_presetindex(f, bank, preset_number);
        if(preset_index == -1)
            return 0;
        c->presetIndex = (unsigned short)preset_index;
        c->bank        = (unsigned short)bank;
        return 1;
    }

    TSFDEF int tsf_channel_set_pan(tsf* f, int channel, float pan)
    {
        struct tsf_voice *  v, *vEnd;
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        for(v = f->voices, vEnd = v + f->voiceNum; v != vEnd; v++)
            if(v->playingPreset != -1 && v->playingChannel == channel)
            {
                float newpan = v->region->pan + pan - 0.5f;
                if(newpan <= -0.5f)
                {
                    v->panFactorLeft  = 1.0f;
                    v->panFactorRight = 0.0f;
                }
                else if(newpan >= 0.5f)
                {
                    v->panFactorLeft  = 0.0f;
                    v->panFactorRight = 1.0f;
                }
                else
                {
                    v->panFactorLeft  = TSF_SQRTF(0.5f - newpan);
                    v->panFactorRight = TSF_SQRTF(0.5f + newpan);
                }
            }
        //c->panOffset = pan - 0.5f;
        return 1;
    }

    TSFDEF int tsf_channel_set_midipan(tsf* f, int channel, int pan)
    {
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        c->midiPan = (unsigned short)pan;
        return 1;
    }

    TSFDEF int tsf_channel_set_midivolume(tsf* f, int channel, int volume)
    {
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        c->midiVolume = (unsigned short)volume;
        return 1;
    }

    TSFDEF int tsf_channel_set_volume(tsf* f, int channel, float volume)
    {
        float               gainDB = tsf_gainToDecibels(volume), gainDBChange;
        struct tsf_voice *  v, *vEnd;
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
#if 0
	if (gainDB == c->gainDB) return 1;
	for (v = f->voices, vEnd = v + f->voiceNum, gainDBChange = gainDB - c->gainDB; v != vEnd; v++)
		if (v->playingPreset != -1 && v->playingChannel == channel)
			v->noteGainDB += gainDBChange;
	c->gainDB = gainDB;
#endif
        return 1;
    }

    TSFDEF int tsf_channel_set_pitchwheel(tsf* f, int channel, int pitch_wheel)
    {
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        if(c->pitchWheel == pitch_wheel)
            return 1;
        c->pitchWheel = (unsigned short)pitch_wheel;
        tsf_channel_applypitch(f, channel, c);
        return 1;
    }

    TSFDEF int
    tsf_channel_set_pitchrange(tsf* f, int channel, float pitch_range)
    {
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        if(c->pitchRange == pitch_range)
            return 1;
        c->pitchRange = pitch_range;
        if(c->pitchWheel != 8192)
            tsf_channel_applypitch(f, channel, c);
        return 1;
    }

    TSFDEF int tsf_channel_set_tuning(tsf* f, int channel, float tuning)
    {
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        if(c->tuning == tuning)
            return 1;
        c->tuning = tuning;
        tsf_channel_applypitch(f, channel, c);
        return 1;
    }

    TSFDEF int tsf_channel_set_sustain(tsf* f, int channel, int flag_sustain)
    {
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        if(!c->sustain == !flag_sustain)
            return 1;
        c->sustain = (unsigned short)(flag_sustain != 0);
        // Turning on sustain does no action now, just starts note_off behaving differently
        if(flag_sustain)
            return 1;
        // Turning off sustain, actually end voices that got a note_off and were set to heldSustain status
        struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
        for(; v != vEnd; v++)
            if(v->playingPreset != -1 && v->playingChannel == channel
               && v->ampenv.segment < TSF_SEGMENT_RELEASE && v->heldSustain)
                tsf_voice_end(f, v);
        return 1;
    }

    TSFDEF int tsf_channel_set_filter(tsf* f, int channel, float fc, float q)
    {
        struct tsf_voice *  v, *vEnd;
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
#if 0
	for (v = f->voices, vEnd = v + f->voiceNum; v != vEnd; v++)
		if (v->playingChannel == channel && v->playingPreset != -1)
		{
			v->filterFc = fc;
			v->filterQ = q;
		}
	c->filterFc = fc;
	c->filterQ = q;
#endif
        return 1;
    }

    TSFDEF int
    tsf_channel_set_midifilter(tsf* f, int channel, short fc, short q)
    {
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        c->midiFc = fc;
        c->midiQ  = q;
        return 1;
    }

    TSFDEF int tsf_channel_note_on(tsf* f, int channel, int key, float vel)
    {
        if(!f->channels || channel >= f->channels->channelNum)
            return 1;
        f->channels->activeChannel = channel;
        if(!vel)
        {
            tsf_channel_note_off(f, channel, key);
            return 1;
        }
        return tsf_note_on(
            f, f->channels->channels[channel].presetIndex, key, vel);
    }

    TSFDEF void tsf_channel_note_off(tsf* f, int channel, int key)
    {
        int               sustain = f->channels->channels[channel].sustain;
        struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum,
                         *vMatchFirst = TSF_NULL, *vMatchLast = TSF_NULL;
        for(; v != vEnd; v++)
        {
            //Find the first and last entry in the voices list with matching channel, key and look up the smallest play index
            if(v->playingPreset == -1 || v->playingChannel != channel
               || v->playingKey != key
               || v->ampenv.segment >= TSF_SEGMENT_RELEASE || v->heldSustain)
                continue;
            else if(!vMatchFirst || v->playIndex < vMatchFirst->playIndex)
                vMatchFirst = vMatchLast = v;
            else if(v->playIndex == vMatchFirst->playIndex)
                vMatchLast = v;
        }
        if(!vMatchFirst)
            return;
        for(sustain = f->channels->channels[channel].sustain, v = vMatchFirst;
            v <= vMatchLast;
            v++)
        {
            //Stop all voices with matching channel, key and the smallest play index which was enumerated above
            if(v != vMatchFirst && v != vMatchLast
               && (v->playIndex != vMatchFirst->playIndex
                   || v->playingPreset == -1 || v->playingChannel != channel
                   || v->playingKey != key
                   || v->ampenv.segment >= TSF_SEGMENT_RELEASE))
                continue;
            // Don't turn off if sustain is active, just mark as held by sustain so we don't forget it
            if(sustain)
                v->heldSustain = 1;
            else
                tsf_voice_end(f, v);
        }
    }

    TSFDEF void tsf_channel_note_off_all(tsf* f, int channel)
    {
        // Ignore sustain channel settings, note_off_all overrides
        struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
        for(; v != vEnd; v++)
            if(v->playingPreset != -1 && v->playingChannel == channel
               && v->ampenv.segment < TSF_SEGMENT_RELEASE)
                tsf_voice_end(f, v);
    }

    TSFDEF void tsf_channel_sounds_off_all(tsf* f, int channel)
    {
        struct tsf_voice *v = f->voices, *vEnd = v + f->voiceNum;
        for(; v != vEnd; v++)
            if(v->playingPreset != -1 && v->playingChannel == channel
               && (v->ampenv.segment < TSF_SEGMENT_RELEASE
                   || v->ampenv.parameters.release))
                tsf_voice_endquick(f, v);
    }

    TSFDEF int tsf_channel_midi_control(tsf* f,
                                        int  channel,
                                        int  controller,
                                        int  control_value)
    {
        struct tsf_channel* c = tsf_channel_init(f, channel);
        if(!c)
            return 0;
        switch(controller)
        {
            case 1 /*MODWHEEL_MSB */:
                c->modWheel = (unsigned short)((c->modWheel & 0x7F)
                                               | (control_value << 7));
                break;
            case 33 /*MODWHEEL_LSB */:
                c->modWheel
                    = (unsigned short)((c->modWheel & 0x3F80) | control_value);
                break;
            case 7 /*VOLUME_MSB*/:
                c->midiVolume = (unsigned short)((c->midiVolume & 0x7F)
                                                 | (control_value << 7));
                break;
            case 39 /*VOLUME_LSB*/:
                c->midiVolume = (unsigned short)((c->midiVolume & 0x3F80)
                                                 | control_value);
                break;
            case 11 /*EXPRESSION_MSB*/:
                c->midiExpression = (unsigned short)((c->midiExpression & 0x7F)
                                                     | (control_value << 7));
                break;
            case 43 /*EXPRESSION_LSB*/:
                c->midiExpression
                    = (unsigned short)((c->midiExpression & 0x3F80)
                                       | control_value);
                break;
            case 10 /*PAN_MSB*/:
                c->midiPan = (unsigned short)((c->midiPan & 0x7F)
                                              | (control_value << 7));
                break;
            case 42 /*PAN_LSB*/:
                c->midiPan
                    = (unsigned short)((c->midiPan & 0x3F80) | control_value);
                break;
            case 6 /*DATA_ENTRY_MSB*/:
                c->midiData = (unsigned short)((c->midiData & 0x7F)
                                               | (control_value << 7));
                goto TCMC_SET_DATA;
            case 38 /*DATA_ENTRY_LSB*/:
                c->midiData
                    = (unsigned short)((c->midiData & 0x3F80) | control_value);
                goto TCMC_SET_DATA;
            case 0 /*BANK_SELECT_MSB*/:
                c->bank = (unsigned short)(0x8000 | control_value);
                return 1; //bank select MSB alone acts like LSB
            case 32 /*BANK_SELECT_LSB*/:
                c->bank = (unsigned short)((c->bank & 0x8000
                                                ? ((c->bank & 0x7F) << 7)
                                                : 0)
                                           | control_value);
                return 1;
            case 101 /*RPN_MSB*/:
                c->midiRPN
                    = (unsigned short)(((c->midiRPN == 0xFFFF ? 0 : c->midiRPN)
                                        & 0x7F)
                                       | (control_value << 7));
                return 1;
            case 100 /*RPN_LSB*/:
                c->midiRPN
                    = (unsigned short)(((c->midiRPN == 0xFFFF ? 0 : c->midiRPN)
                                        & 0x3F80)
                                       | control_value);
                return 1;
            case 98 /*NRPN_LSB*/: c->midiRPN = 0xFFFF; return 1;
            case 99 /*NRPN_MSB*/: c->midiRPN = 0xFFFF; return 1;
            case 64 /*SUSTAIN*/:
                tsf_channel_set_sustain(f, channel, (int)(control_value >= 64));
                return 1;
            case 71 /*FILTER RESONANCE*/: c->midiQ = control_value; break;
            case 74 /*FILTER CUTOFF */: c->midiFc = control_value; break;
            case 120 /*ALL_SOUND_OFF*/:
                tsf_channel_sounds_off_all(f, channel);
                return 1;
            case 123 /*ALL_NOTES_OFF*/:
                tsf_channel_note_off_all(f, channel);
                return 1;
            case 121 /*ALL_CTRL_OFF*/:
                c->midiVolume = c->midiExpression = 16383;
                c->midiPan                        = 8192;
                c->bank                           = 0;
                c->midiRPN                        = 0xFFFF;
                c->midiData                       = 0;
                tsf_channel_set_volume(f, channel, 1.0f);
                tsf_channel_set_pan(f, channel, 0.5f);
                tsf_channel_set_pitchrange(f, channel, 2.0f);
                tsf_channel_set_tuning(f, channel, 0);
                tsf_channel_set_filter(f, channel, 0, 0);
                return 1;
        }
        return 1;

    TCMC_SET_DATA:
        if(c->midiRPN == 0)
            tsf_channel_set_pitchrange(
                f, channel, (c->midiData >> 7) + 0.01f * (c->midiData & 0x7F));
        else if(c->midiRPN == 1)
            tsf_channel_set_tuning(f,
                                   channel,
                                   (int)c->tuning
                                       + ((float)c->midiData - 8192.0f)
                                             / 8192.0f); //fine tune
        else if(c->midiRPN == 2 && controller == 6)
            tsf_channel_set_tuning(
                f,
                channel,
                ((float)control_value - 64.0f)
                    + (c->tuning - (int)c->tuning)); //coarse tune
        return 1;
    }

    TSFDEF int tsf_channel_get_preset_index(tsf* f, int channel)
    {
        return (f->channels && channel < f->channels->channelNum
                    ? f->channels->channels[channel].presetIndex
                    : 0);
    }

    TSFDEF int tsf_channel_get_preset_bank(tsf* f, int channel, int* flag)
    {
        int bank = (f->channels && channel < f->channels->channelNum
                        ? f->channels->channels[channel].bank
                        : 0);
        if(flag)
            *flag = bank & 0x8000;
        return bank & 0x7FFF;
    }

    TSFDEF int tsf_channel_get_preset_number(tsf* f, int channel)
    {
        return (
            f->channels && channel < f->channels->channelNum
                ? f->presets[f->channels->channels[channel].presetIndex].preset
                : 0);
    }

    TSFDEF float tsf_channel_get_pan(tsf* f, int channel)
    {
        return (f->channels && channel < f->channels->channelNum
                    ? (f->channels->channels[channel].midiPan / 16383.0f)
                    : 0.5f);
    }

    TSFDEF float tsf_channel_get_volume(tsf* f, int channel)
    {
        return (f->channels && channel < f->channels->channelNum
                    ? (f->channels->channels[channel].midiVolume / 16383.0f)
                    : 1.0f); // tsf_decibelsToGain
    }

    TSFDEF int tsf_channel_get_pitchwheel(tsf* f, int channel)
    {
        return (f->channels && channel < f->channels->channelNum
                    ? f->channels->channels[channel].pitchWheel
                    : 8192);
    }

    TSFDEF float tsf_channel_get_pitchrange(tsf* f, int channel)
    {
        return (f->channels && channel < f->channels->channelNum
                    ? f->channels->channels[channel].pitchRange
                    : 2.0f);
    }

    TSFDEF float tsf_channel_get_tuning(tsf* f, int channel)
    {
        return (f->channels && channel < f->channels->channelNum
                    ? f->channels->channels[channel].tuning
                    : 0.0f);
    }


    TSFDEF void tsf_init_lut()
    {
        for(int cents = 1500; cents < 13500; cents++)
        {
            float  Fc                  = tsf_cents2Hertz(cents) / 44100;
            double K                   = TSF_TAN(TSF_PI * Fc);
            tanFromCents[cents - 1500] = K;
        }
    }

#ifdef __cplusplus
}
#endif

#endif //TSF_IMPLEMENTATION