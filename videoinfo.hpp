#ifndef VIDEO_CONCATENATER_VIDEOINFO
#define VIDEO_CONCATENATER_VIDEOINFO

#include <QSet>
#include <QSize>
#include <QString>
#include <QVector>
#include <variant>
namespace concat {
template <class T>
struct SameAsHighest {
    T value;
};
template <class T>
struct SameAsLowest {
    T value;
};
struct SameAsInput {
    QString value;
};
template <class T>
struct ValueRange {
    T highest;
    T lowest;
};
template <class T>
using RangedVariant = std::variant<SameAsHighest<T>, SameAsLowest<T>, T, ValueRange<T>>;
struct VideoInfo {
    RangedVariant<QSize> resolution;
    RangedVariant<double> framerate;
    bool is_vfr;
    std::variant<SameAsInput, QString, QSet<QString>> audio_codec;
    std::variant<SameAsInput, QString, QSet<QString>> video_codec;
    QVector<QString> encoding_args;
};
}  // namespace concat

#endif