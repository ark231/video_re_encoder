#include "videoinfo.hpp"
namespace concat {
void VideoInfo::resolve_reference() {
    if (std::holds_alternative<SameAsHighest<QSize>>(this->resolution)) {
        this->resolution = std::get<SameAsHighest<QSize>>(this->resolution).value;
    } else if (std::holds_alternative<SameAsLowest<QSize>>(this->resolution)) {
        this->resolution = std::get<SameAsLowest<QSize>>(resolution).value;
    }
    if (std::holds_alternative<SameAsHighest<double>>(this->framerate)) {
        this->framerate = std::get<SameAsHighest<double>>(this->framerate).value;
    } else if (std::holds_alternative<SameAsLowest<double>>(this->framerate)) {
        this->framerate = std::get<SameAsLowest<double>>(this->framerate).value;
    }
    if (std::holds_alternative<SameAsInput<QString>>(this->audio_codec)) {
        this->audio_codec = std::get<SameAsInput<QString>>(this->audio_codec).value;
    }
    if (std::holds_alternative<SameAsInput<QString>>(this->video_codec)) {
        this->video_codec = std::get<SameAsInput<QString>>(this->video_codec).value;
    }
}
}  // namespace concat