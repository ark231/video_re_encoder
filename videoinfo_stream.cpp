#include "videoinfo_stream.hpp"

#include <QDebug>

namespace concat {
inline namespace operators {
QDataStream& operator<<(QDataStream& stream, const VideoInfo& info) {
    stream << info.resolution;
    stream << info.framerate;
    stream << info.is_vfr;
    stream << info.audio_codec;
    stream << info.video_codec;
    stream << info.encoding_args;
    return stream;
}
QDataStream& operator>>(QDataStream& stream, VideoInfo& info) {
    stream >> info.resolution;
    stream >> info.framerate;
    stream >> info.is_vfr;
    stream >> info.audio_codec;
    stream >> info.video_codec;
    stream >> info.encoding_args;
    return stream;
}
}  // namespace operators
}  // namespace concat