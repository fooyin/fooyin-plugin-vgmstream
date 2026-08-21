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

#include "vgmstreaminput.h"

#include "vgmstreamdefs.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>

#include <cstring>
#include <limits>

Q_LOGGING_CATEGORY(VGMSTREAM, "fy.vgmstream")

using namespace Qt::StringLiterals;

namespace Fooyin::VGMStream {
namespace {
constexpr qsizetype StreamBufferSize = 32 * 1024LL;

struct StreamFileSource
{
    QIODevice* device{nullptr};
    ArchiveReader* archiveReader{nullptr};
    QString name;
};

struct StreamFileData
{
    std::shared_ptr<StreamFileSource> source;
    std::unique_ptr<QIODevice> ownedDevice;
    QIODevice* device{nullptr};
    QByteArray name;
    QByteArray buffer;
    qint64 bufferOffset{0};
    qint64 validBufferSize{0};
};

libstreamfile_t* makeStreamFile(const std::shared_ptr<StreamFileSource>& source, const QString& name);

QString normalisedStreamName(QString name)
{
    name.replace(u'\\', u'/');
    return QDir::cleanPath(name);
}

int streamFileRead(void* userData, uint8_t* destination, int64_t offset, int length)
{
    auto* data = static_cast<StreamFileData*>(userData);
    if(!data || !data->device || !destination || offset < 0 || length <= 0) {
        return 0;
    }

    qint64 currentOffset{offset};
    int bytesRead{0};

    if(currentOffset >= data->bufferOffset && currentOffset < data->bufferOffset + data->validBufferSize) {
        const qint64 bufferPosition = currentOffset - data->bufferOffset;
        const auto amount
            = static_cast<int>(std::min<qint64>(data->validBufferSize - bufferPosition, length - bytesRead));
        std::memcpy(destination, data->buffer.constData() + bufferPosition, amount);
        currentOffset += amount;
        bytesRead += amount;
    }

    while(bytesRead < length) {
        if(currentOffset >= data->device->size()
           || (data->device->pos() != currentOffset && !data->device->seek(currentOffset))) {
            break;
        }

        data->bufferOffset    = currentOffset;
        data->validBufferSize = data->device->read(data->buffer.data(), data->buffer.size());
        if(data->validBufferSize <= 0) {
            break;
        }

        const auto amount = static_cast<int>(std::min<qint64>(data->validBufferSize, length - bytesRead));
        std::memcpy(destination + bytesRead, data->buffer.constData(), amount);
        currentOffset += amount;
        bytesRead += amount;
    }

    return bytesRead;
}

QString streamTitle(libvgmstream_t* stream, const QString& filename)
{
    if(!stream) {
        return {};
    }

    const QByteArray encodedFilename = filename.toUtf8();
    QByteArray title(1024, '\0');

    libvgmstream_title_t config{};
    config.remove_extension = true;
    config.remove_archive   = true;
    config.filename         = encodedFilename.constData();

    if(libvgmstream_get_title(stream, &config, title.data(), title.size()) < 0) {
        return {};
    }

    return QString::fromUtf8(title.constData());
}

int64_t streamFileSize(void* userData)
{
    const auto* data = static_cast<StreamFileData*>(userData);
    return data && data->device ? std::max<int64_t>(data->device->size(), 0) : 0;
}

const char* streamFileName(void* userData)
{
    const auto* data = static_cast<StreamFileData*>(userData);
    return data ? data->name.constData() : "";
}

libstreamfile_t* streamFileOpen(void* userData, const char* filename)
{
    const auto* data = static_cast<StreamFileData*>(userData);
    if(!data || !data->source) {
        return nullptr;
    }
    return makeStreamFile(data->source, QString::fromUtf8(filename));
}

void streamFileClose(libstreamfile_t* streamFile)
{
    if(streamFile) {
        delete static_cast<StreamFileData*>(streamFile->user_data);
        delete streamFile;
    }
}

libstreamfile_t* makeStreamFile(const std::shared_ptr<StreamFileSource>& source, const QString& name)
{
    if(!source) {
        return nullptr;
    }

    const QString normalisedName = normalisedStreamName(name);

    auto data    = std::make_unique<StreamFileData>();
    data->source = source;
    data->name   = normalisedName.toUtf8();
    data->buffer.resize(StreamBufferSize);

    if(normalisedName == source->name) {
        data->device = source->device;
    }
    else if(source->archiveReader) {
        ArchiveEntryData entry = source->archiveReader->entry(normalisedName);
        data->ownedDevice      = std::move(entry.device);
        data->device           = data->ownedDevice.get();
    }
    else {
        auto file = std::make_unique<QFile>(normalisedName);
        if(file->open(QIODevice::ReadOnly)) {
            data->ownedDevice = std::move(file);
            data->device      = data->ownedDevice.get();
        }
    }

    if(!data->device || !data->device->isOpen() || !data->device->isReadable() || data->device->isSequential()) {
        return nullptr;
    }

    auto streamFile       = std::make_unique<libstreamfile_t>();
    streamFile->user_data = data.release();
    streamFile->read      = streamFileRead;
    streamFile->get_size  = streamFileSize;
    streamFile->get_name  = streamFileName;
    streamFile->open      = streamFileOpen;
    streamFile->close     = streamFileClose;

    return streamFile.release();
}

QStringList supportedExtensions()
{
    int count{0};
    const char* const* values = libvgmstream_get_extensions(&count);

    QStringList extensions;
    extensions.reserve(count);

    for(int i{0}; i < count; ++i) {
        extensions.emplace_back(QString::fromLatin1(values[i]));
    }

    return extensions;
}

libstreamfile_t* openStreamFile(const AudioSource& source, const QString& path = {})
{
    if(!source.device) {
        const QString filename = path.isEmpty() ? source.filepath : path;
        return libstreamfile_open_from_stdio(filename.toUtf8().constData());
    }

    auto streamSource           = std::make_shared<StreamFileSource>();
    streamSource->device        = source.device;
    streamSource->archiveReader = source.archiveReader;
    streamSource->name          = normalisedStreamName(source.filepath);
    return makeStreamFile(streamSource, path.isEmpty() ? streamSource->name : path);
}

uint64_t durationForStream(const libvgmstream_format_t* format)
{
    if(!format || format->play_samples <= 0 || format->sample_rate <= 0) {
        return 0;
    }

    const auto samples    = static_cast<uint64_t>(format->play_samples);
    const auto sampleRate = static_cast<uint64_t>(format->sample_rate);
    return ((samples / sampleRate) * 1000) + (((samples % sampleRate) * 1000) / sampleRate);
}

int bitDepth(libvgmstream_sfmt_t format)
{
    switch(format) {
        case LIBVGMSTREAM_SFMT_PCM16:
            return 16;
        case LIBVGMSTREAM_SFMT_PCM24:
            return 24;
        case LIBVGMSTREAM_SFMT_PCM32:
        case LIBVGMSTREAM_SFMT_FLOAT:
            return 32;
    }
    return 0;
}

void applyTag(Track& track, const QString& tag, const QString& value)
{
    if(tag.compare("TITLE"_L1, Qt::CaseInsensitive) == 0) {
        track.setTitle(value);
    }
    else if(tag.compare("ARTIST"_L1, Qt::CaseInsensitive) == 0) {
        track.setArtists({value});
    }
    else if(tag.compare("ALBUMARTIST"_L1, Qt::CaseInsensitive) == 0
            || tag.compare("ALBUM ARTIST"_L1, Qt::CaseInsensitive) == 0) {
        track.setAlbumArtists({value});
    }
    else if(tag.compare("ALBUM"_L1, Qt::CaseInsensitive) == 0) {
        track.setAlbum(value);
    }
    else if(tag.compare("TRACK"_L1, Qt::CaseInsensitive) == 0
            || tag.compare("TRACKNUMBER"_L1, Qt::CaseInsensitive) == 0) {
        track.setTrackNumber(value);
    }
    else if(tag.compare("DISC"_L1, Qt::CaseInsensitive) == 0
            || tag.compare("DISCNUMBER"_L1, Qt::CaseInsensitive) == 0) {
        track.setDiscNumber(value);
    }
    else if(tag.compare("DATE"_L1, Qt::CaseInsensitive) == 0) {
        track.setDate(value);
    }
    else if(tag.compare("GENRE"_L1, Qt::CaseInsensitive) == 0) {
        track.setGenres({value});
    }
    else if(tag.compare("COMMENT"_L1, Qt::CaseInsensitive) == 0) {
        track.setComment(value);
    }
    else if(tag.compare("REPLAYGAIN_TRACK_GAIN"_L1, Qt::CaseInsensitive) == 0) {
        track.setRGTrackGain(value.toFloat());
    }
    else if(tag.compare("REPLAYGAIN_TRACK_PEAK"_L1, Qt::CaseInsensitive) == 0) {
        track.setRGTrackPeak(value.toFloat());
    }
    else if(tag.compare("REPLAYGAIN_ALBUM_GAIN"_L1, Qt::CaseInsensitive) == 0) {
        track.setRGAlbumGain(value.toFloat());
    }
    else if(tag.compare("REPLAYGAIN_ALBUM_PEAK"_L1, Qt::CaseInsensitive) == 0) {
        track.setRGAlbumPeak(value.toFloat());
    }
    else {
        track.addExtraTag(tag, value);
    }
}

void readExternalTags(const AudioSource& source, const QString& path, Track& track)
{
    const QFileInfo fileInfo{path};
    const QString tagPath = fileInfo.dir().filePath(u"!tags.m3u"_s);

    libstreamfile_t* tagFile = openStreamFile(source, tagPath);
    if(!tagFile) {
        return;
    }

    libvgmstream_tags_t* tags = libvgmstream_tags_init(tagFile);
    if(tags) {
        libvgmstream_tags_find(tags, fileInfo.fileName().toUtf8().constData());
        while(libvgmstream_tags_next_tag(tags)) {
            applyTag(track, QString::fromUtf8(tags->key), QString::fromUtf8(tags->val));
        }
        libvgmstream_tags_free(tags);
    }

    libstreamfile_close(tagFile);
}
} // namespace

VGMStreamDecoder::VGMStreamDecoder()
    : m_subsong{0}
    , m_isDecoding{false}
    , m_repeatTrack{false}
    , m_allowInfiniteRepeat{true}
    , m_bufferOffset{0}
    , m_bufferRemaining{0}
{ }

QStringList VGMStreamDecoder::extensions() const
{
    return supportedExtensions();
}

bool VGMStreamDecoder::isSeekable() const
{
    return true;
}

AudioDecoder::RepeatHandling VGMStreamDecoder::repeatHandling() const
{
    return RepeatHandling::DecoderLoop;
}

bool VGMStreamDecoder::trackHasChanged() const
{
    return m_changedTrack.isValid();
}

Track VGMStreamDecoder::changedTrack() const
{
    return m_changedTrack;
}

libvgmstream_config_t VGMStreamDecoder::config() const
{
    const int configuredLoops = std::clamp(m_settings.value(LoopCount, DefaultLoopCount).toInt(), 1, 10);
    const int loopCount       = (m_options & NoLooping) ? 1 : configuredLoops;
    const int fadeLength      = std::max(m_settings.value(FadeLength, DefaultFadeLength).toInt(), 0);

    libvgmstream_config_t result{};
    result.allow_play_forever    = true;
    result.play_forever          = m_repeatTrack;
    result.ignore_loop           = static_cast<bool>(m_options & NoLooping);
    result.loop_count            = loopCount;
    result.fade_time             = static_cast<double>(fadeLength) / 1000.0;
    result.auto_downmix_channels = AudioFormat::MaxChannels;
    result.force_sfmt            = LIBVGMSTREAM_SFMT_FLOAT;
    return result;
}

bool VGMStreamDecoder::openStream(int64_t position)
{
    libstreamfile_t* streamFile = openStreamFile(m_source);
    if(!streamFile) {
        qCWarning(VGMSTREAM) << "Unable to open" << m_path;
        return false;
    }

    auto streamConfig{config()};
    VGMStreamPtr stream{libvgmstream_create(streamFile, m_subsong, &streamConfig)};
    libstreamfile_close(streamFile);
    if(!stream) {
        qCWarning(VGMSTREAM) << "Unsupported or invalid VGMStream file" << m_path;
        return false;
    }

    const auto* streamFormat = stream->format;
    if(!streamFormat) {
        return false;
    }

    const AudioFormat format{SampleFormat::F32, streamFormat->sample_rate, streamFormat->channels};
    if(!format.isValid()) {
        return false;
    }

    libvgmstream_seek(stream.get(), std::max<int64_t>(position, 0));
    m_stream          = std::move(stream);
    m_format          = format;
    m_bufferOffset    = 0;
    m_bufferRemaining = 0;

    return true;
}

std::optional<AudioFormat> VGMStreamDecoder::init(const AudioSource& source, const Track& track, DecoderOptions options)
{
    stop();

    m_changedTrack        = {};
    m_options             = options;
    m_allowInfiniteRepeat = !(options & (NoLooping | NoInfiniteLooping));
    m_repeatTrack         = m_allowInfiniteRepeat && isRepeatingTrack();

    if(source.device && source.device->isSequential()) {
        return {};
    }

    m_source  = source;
    m_path    = source.filepath;
    m_subsong = track.subsong() + 1;

    if(!openStream()) {
        return {};
    }

    if(options & UpdateTracks) {
        const uint64_t duration = durationForStream(m_stream->format);
        if(duration > 0 && track.duration() != duration) {
            m_changedTrack = track;
            m_changedTrack.setDuration(duration);
        }
    }

    return m_format;
}

void VGMStreamDecoder::start()
{
    m_isDecoding = true;
}

void VGMStreamDecoder::stop()
{
    m_stream.reset();
    m_source          = {};
    m_changedTrack    = {};
    m_isDecoding      = false;
    m_bufferOffset    = 0;
    m_bufferRemaining = 0;
}

void VGMStreamDecoder::seek(uint64_t pos)
{
    if(!m_stream) {
        return;
    }
    libvgmstream_seek(m_stream.get(), m_format.framesForDuration(pos));
    m_bufferOffset    = 0;
    m_bufferRemaining = 0;
}

void VGMStreamDecoder::playbackHintsChanged(PlaybackHints /*hints*/)
{
    const bool repeatTrack = m_allowInfiniteRepeat && isRepeatingTrack();
    if(repeatTrack == m_repeatTrack || !m_stream) {
        return;
    }

    const int bytesPerFrame = m_format.bytesPerFrame();
    const int64_t pendingFrames
        = bytesPerFrame > 0 ? static_cast<int64_t>(m_bufferRemaining / static_cast<size_t>(bytesPerFrame)) : 0;
    const int64_t position = std::max<int64_t>(libvgmstream_get_play_position(m_stream.get()) - pendingFrames, 0);

    const bool oldRepeat = m_repeatTrack;
    m_repeatTrack        = repeatTrack;
    if(!openStream(position)) {
        m_repeatTrack = oldRepeat;
    }
}

AudioBuffer VGMStreamDecoder::readBuffer(size_t bytes)
{
    if(!m_isDecoding || !m_stream) {
        return {};
    }

    const int requestedFrames = m_format.framesForBytes(bytes);
    if(requestedFrames <= 0) {
        return {};
    }

    const int64_t decodedPosition = libvgmstream_get_play_position(m_stream.get());
    const auto pendingFrames = static_cast<int64_t>(m_bufferRemaining / static_cast<size_t>(m_format.bytesPerFrame()));
    const int64_t position   = std::max<int64_t>(decodedPosition - pendingFrames, 0);
    const auto startTime
        = position > 0
            ? m_format.durationForFrames(static_cast<int>(std::min<int64_t>(position, std::numeric_limits<int>::max())))
            : 0;

    AudioBuffer buffer{m_format, startTime};
    buffer.resize(bytes);

    size_t bytesDone{0};
    while(bytesDone < bytes) {
        if(m_bufferRemaining == 0) {
            if(m_stream->decoder->done) {
                break;
            }

            if(libvgmstream_render(m_stream.get()) < 0) {
                qCWarning(VGMSTREAM) << "Error decoding" << m_path;
                break;
            }

            m_bufferOffset    = 0;
            m_bufferRemaining = static_cast<size_t>(std::max(m_stream->decoder->buf_bytes, 0));
            if(m_bufferRemaining == 0) {
                continue;
            }
        }

        const size_t amount = std::min(m_bufferRemaining, bytes - bytesDone);
        const auto* input   = static_cast<const std::byte*>(m_stream->decoder->buf);
        std::memcpy(buffer.data() + bytesDone, input + m_bufferOffset, amount);
        bytesDone += amount;
        m_bufferOffset += amount;
        m_bufferRemaining -= amount;
    }

    if(bytesDone == 0) {
        return {};
    }

    buffer.resize(bytesDone);
    return buffer;
}

VGMStreamReader::VGMStreamReader()
    : m_subsongCount{1}
{ }

QStringList VGMStreamReader::extensions() const
{
    return supportedExtensions();
}

bool VGMStreamReader::canReadCover() const
{
    return false;
}

bool VGMStreamReader::canWriteMetaData() const
{
    return false;
}

int VGMStreamReader::subsongCount() const
{
    return m_subsongCount;
}

libvgmstream_config_t VGMStreamReader::config() const
{
    libvgmstream_config_t result{};
    result.allow_play_forever = true;
    result.loop_count         = std::clamp(m_settings.value(LoopCount, DefaultLoopCount).toInt(), 1, 10);
    result.fade_time
        = static_cast<double>(std::max(m_settings.value(FadeLength, DefaultFadeLength).toInt(), 0)) / 1000.0;
    result.auto_downmix_channels = AudioFormat::MaxChannels;
    return result;
}

bool VGMStreamReader::init(const AudioSource& source)
{
    m_subsongCount = 1;

    if(source.device && source.device->isSequential()) {
        return false;
    }

    m_path                      = source.filepath;
    libstreamfile_t* streamFile = openStreamFile(source);
    if(!streamFile) {
        return false;
    }

    auto streamConfig{config()};
    VGMStreamPtr stream{libvgmstream_create(streamFile, 0, &streamConfig)};
    libstreamfile_close(streamFile);
    if(!stream) {
        return false;
    }

    m_subsongCount = std::max(stream->format->subsong_count, 1);
    if(stream->format->subsong_index > 0) {
        m_subsongCount = 1;
    }

    return true;
}

bool VGMStreamReader::readTrack(const AudioSource& source, Track& track)
{
    AudioSource streamSource{source};
    streamSource.filepath = m_path;

    libstreamfile_t* streamFile = openStreamFile(streamSource);
    if(!streamFile) {
        return false;
    }

    auto streamConfig{config()};
    VGMStreamPtr stream{libvgmstream_create(streamFile, track.subsong() + 1, &streamConfig)};
    libstreamfile_close(streamFile);
    if(!stream || !stream->format) {
        return false;
    }

    const auto* format = stream->format;
    const int depth    = bitDepth(format->sample_format);
    if(format->sample_rate <= 0 || format->channels <= 0 || depth == 0) {
        return false;
    }

    const qint64 sourceSize = source.size > 0 ? static_cast<qint64>(source.size)
                            : source.device   ? source.device->size()
                                              : QFileInfo{m_path}.size();
    track.setFileSize(std::max<qint64>(sourceSize, 0));

    track.setDuration(durationForStream(format));
    track.setSampleRate(format->sample_rate);
    track.setChannels(format->channels);
    track.setBitDepth(depth);
    track.setCodec(QString::fromUtf8(format->codec_name));
    track.setCodecProfile(QString::fromUtf8(format->meta_name));

    if(format->stream_bitrate > 0) {
        track.setBitrate(format->stream_bitrate / 1000);
    }
    if(m_settings.value(GenerateTitles, DefaultGenerateTitles).toBool()) {
        const QString title = streamTitle(stream.get(), m_path);
        if(!title.isEmpty()) {
            track.setTitle(title);
        }
    }
    if(format->stream_name[0] != '\0') {
        track.addExtraTag(u"STREAM_NAME"_s, QString::fromUtf8(format->stream_name));
    }
    if(format->subsong_count > 1) {
        track.addExtraTag(u"STREAM_COUNT"_s, QString::number(format->subsong_count));
        track.addExtraTag(u"STREAM_INDEX"_s,
                          QString::number(format->subsong_index > 0 ? format->subsong_index : track.subsong() + 1));
    }

    readExternalTags(streamSource, m_path, track);
    return true;
}
} // namespace Fooyin::VGMStream
