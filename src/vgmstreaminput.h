/*
 * Fooyin
 * Copyright © 2026, Luke Taylor <luket@pm.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <core/coresettings.h>
#include <core/engine/audioinput.h>

extern "C"
{
#if __has_include(<vgmstream/libvgmstream.h>)
#include <vgmstream/libvgmstream.h>
#else
#include <libvgmstream.h>
#endif
}

namespace Fooyin::VGMStream {
struct VGMStreamDeleter
{
    void operator()(libvgmstream_t* stream) const
    {
        if(stream) {
            libvgmstream_free(stream);
        }
    }
};
using VGMStreamPtr = std::unique_ptr<libvgmstream_t, VGMStreamDeleter>;

class VGMStreamDecoder : public AudioDecoder
{
public:
    VGMStreamDecoder();

    [[nodiscard]] QStringList extensions() const override;
    [[nodiscard]] bool isSeekable() const override;
    [[nodiscard]] RepeatHandling repeatHandling() const override;
    [[nodiscard]] bool trackHasChanged() const override;
    [[nodiscard]] Track changedTrack() const override;

    std::optional<AudioFormat> init(const AudioSource& source, const Track& track, DecoderOptions options) override;
    void start() override;
    void stop() override;
    void seek(uint64_t pos) override;

    AudioBuffer readBuffer(size_t bytes) override;

protected:
    void playbackHintsChanged(PlaybackHints hints) override;

private:
    [[nodiscard]] bool openStream(int64_t position = 0);
    [[nodiscard]] libvgmstream_config_t config() const;

    FySettings m_settings;
    DecoderOptions m_options;
    AudioFormat m_format;
    AudioSource m_source;
    QString m_path;
    VGMStreamPtr m_stream;
    Track m_changedTrack;
    int m_subsong;
    bool m_isDecoding;
    bool m_repeatTrack;
    bool m_allowInfiniteRepeat;
    size_t m_bufferOffset;
    size_t m_bufferRemaining;
};

class VGMStreamReader : public AudioReader
{
public:
    VGMStreamReader();

    [[nodiscard]] QStringList extensions() const override;
    [[nodiscard]] bool canReadCover() const override;
    [[nodiscard]] bool canWriteMetaData() const override;
    [[nodiscard]] int subsongCount() const override;

    bool init(const AudioSource& source) override;
    bool readTrack(const AudioSource& source, Track& track) override;

private:
    [[nodiscard]] libvgmstream_config_t config() const;

    FySettings m_settings;
    QString m_path;
    int m_subsongCount;
};
} // namespace Fooyin::VGMStream
