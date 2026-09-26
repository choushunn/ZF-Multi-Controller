#include "core/hardware/dvp_camera.h"
#include "core/logger.h"

#include "DVPCamera.h"

#include <QImage>
#include <QDir>
#include <QFileInfo>
#include <cstring>
#include <thread>
#include <chrono>

// ---- DvpCamera 实现 ----

DvpCamera::DvpCamera(QObject *parent)
    : ICamera(parent)
{
}

DvpCamera::~DvpCamera()
{
    stopCapture();
    disconnect();
}

QStringList DvpCamera::availableCameras()
{
    QStringList list;
    dvpUint32 count = 0;
    if (dvpRefresh(&count) != DVP_STATUS_OK)
        return list;

    for (dvpUint32 i = 0; i < count; ++i) {
        dvpCameraInfo info;
        memset(&info, 0, sizeof(info));
        if (dvpEnum(i, &info) == DVP_STATUS_OK) {
            // 优先用 UserID，其次序列号
            QString id = QString::fromUtf8(info.UserID);
            if (id.isEmpty())
                id = QString::fromUtf8(info.SerialNumber);
            list << id;
        }
    }
    return list;
}

bool DvpCamera::connectTo(const QString &id)
{
    if (connected_)
        return false;

    const QByteArray userId = id.toUtf8();
    dvpHandle h = 0;
    if (dvpOpenByUserId(const_cast<char *>(userId.constData()),
                        dvpOpenMode::OPEN_NORMAL, &h) != DVP_STATUS_OK) {
        emit logMessage(QStringLiteral("相机连接失败: %1").arg(id));
        return false;
    }

    handle_ = h;
    connected_ = true;

    // 获取相机信息
    dvpCameraInfo info;
    memset(&info, 0, sizeof(info));
    if (dvpGetCameraInfo(h, &info) == DVP_STATUS_OK) {
        description_ = QStringLiteral("%1 (%2)")
            .arg(QString::fromUtf8(info.FriendlyName))
            .arg(QString::fromUtf8(info.SerialNumber));
    } else {
        description_ = id;
    }

    emit logMessage(QStringLiteral("相机已连接: %1").arg(description_));
    emit connectedChanged(true);
    return true;
}

void DvpCamera::disconnect()
{
    if (!connected_)
        return;

    stopCapture();

    if (handle_) {
        dvpClose(handle_);
        handle_ = 0;
    }
    connected_ = false;
    emit connectedChanged(false);
    emit logMessage(QStringLiteral("相机已断开"));
}

bool DvpCamera::startCapture()
{
    if (!connected_ || capturing_)
        return false;

    if (dvpStart(handle_) != DVP_STATUS_OK) {
        emit logMessage(QStringLiteral("启动相机采集失败"));
        return false;
    }

    capturing_ = true;
    // 启动后台采集线程
    captureThread_ = std::thread([this]() {
        while (capturing_ && connected_) {
            dvpFrame frame;
            void *buffer = nullptr;
            // 阻塞获取帧，超时 1000ms
            dvpStatus status = dvpGetFrame(handle_, &frame, &buffer, 1000);
            if (status == DVP_STATUS_OK && buffer) {
                // 最新帧优先：UI 尚未消费上一帧则丢弃本帧
                if (framePending_.load(std::memory_order_acquire))
                    continue;
                QImage *dst = &frameBuf_[slot_];
                if (!renderFrameTo(*dst, frame.iWidth, frame.iHeight, frame.format, buffer, frame.uBytes))
                    continue;   // 无法识别的格式：保留上一帧显示
                {
                    QMutexLocker lock(&frameMutex_);  // 与 saveFrame 的读取互斥
                    lastFrame_ = *dst;
                }
                slot_ = 1 - slot_;
                framePending_.store(true, std::memory_order_release);
                emit frameReady(*dst);
            } else if (status != DVP_STATUS_TIME_OUT) {
                emit logMessage(QStringLiteral("采帧失败，状态: %1").arg(static_cast<int>(status)));
            }
        }
    });

    emit logMessage(QStringLiteral("相机采集已启动"));
    return true;
}

void DvpCamera::stopCapture()
{
    if (!capturing_)
        return;

    capturing_ = false;
    if (captureThread_.joinable())
        captureThread_.join();

    if (handle_ && connected_)
        dvpStop(handle_);

    emit logMessage(QStringLiteral("相机采集已停止"));
}

bool DvpCamera::saveFrame(const QString &filePath)
{
    QMutexLocker lock(&frameMutex_);  // 与采集线程的 lastFrame_ 写入互斥
    if (lastFrame_.isNull())
        return false;

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    return lastFrame_.save(filePath, "PNG");
}

bool DvpCamera::renderFrameTo(QImage &dst, int width, int height, int format,
                              const void *buffer, int bytes)
{
    if (!buffer || width <= 0 || height <= 0)
        return false;

    const uchar *data = static_cast<const uchar *>(buffer);

    // format 对应 dvpImageFormat 枚举；先确定目标 QImage 格式与行字节数
    QImage::Format qfmt;
    int stride;
    switch (format) {
    case 0:  // FORMAT_MONO
        qfmt = QImage::Format_Grayscale8; stride = width; break;
    case 15: // FORMAT_RGB32
        qfmt = QImage::Format_RGB32; stride = width * 4; break;
    case 8:  // FORMAT_RGB24
        qfmt = QImage::Format_RGB888; stride = width * 3; break;
    default:
        // 尝试按字节数推断
        if (bytes >= static_cast<int>(width * height * 4)) {
            qfmt = QImage::Format_RGB32; stride = width * 4;
        } else if (bytes >= static_cast<int>(width * height * 3)) {
            qfmt = QImage::Format_RGB888; stride = width * 3;
        } else if (bytes >= static_cast<int>(width * height)) {
            qfmt = QImage::Format_Grayscale8; stride = width;
        } else {
            return false;
        }
        break;
    }

    // 仅尺寸/格式变化时重建，否则复用缓冲（消除每帧 malloc/free 抖动）
    const QSize size(width, height);
    if (dst.size() != size || dst.format() != qfmt)
        dst = QImage(size, qfmt);
    memcpy(dst.bits(), data, size_t(stride) * static_cast<size_t>(height));
    return true;
}
