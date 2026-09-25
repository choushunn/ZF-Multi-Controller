#include "core/dvp_camera.h"
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
                QImage img = frameToImage(
                    frame.iWidth, frame.iHeight, frame.format, buffer, frame.uBytes);
                if (!img.isNull()) {
                    lastFrame_ = img;
                    emit frameReady(img);
                }
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
    if (lastFrame_.isNull())
        return false;

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    return lastFrame_.save(filePath, "PNG");
}

QImage DvpCamera::frameToImage(int width, int height, int format, const void *buffer, int bytes)
{
    if (!buffer || width <= 0 || height <= 0)
        return {};

    const uchar *data = static_cast<const uchar *>(buffer);

    // format 对应 dvpImageFormat 枚举
    switch (format) {
    case 0:  // FORMAT_MONO
        return QImage(data, width, height, width, QImage::Format_Grayscale8).copy();
    case 15: // FORMAT_RGB32
        return QImage(data, width, height, width * 4, QImage::Format_RGB32).copy();
    case 8:  // FORMAT_RGB24
        return QImage(data, width, height, width * 3, QImage::Format_RGB888).copy();
    default:
        // 尝试按字节数推断
        if (bytes >= static_cast<int>(width * height * 4))
            return QImage(data, width, height, width * 4, QImage::Format_RGB32).copy();
        if (bytes >= static_cast<int>(width * height * 3))
            return QImage(data, width, height, width * 3, QImage::Format_RGB888).copy();
        if (bytes >= static_cast<int>(width * height))
            return QImage(data, width, height, width, QImage::Format_Grayscale8).copy();
        return {};
    }
}
