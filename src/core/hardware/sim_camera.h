#pragma once

#include "core/icamera.h"

#include <QImage>
#include <QTimer>
#include <QPainter>
#include <QFont>
#include <atomic>

namespace mc {

// SimCamera：仿真相机，无硬件依赖。
// 生成渐变彩条测试图案，验证 ICamera 契约与 UI 采帧通路。
// 用于 M3 单元测试与无硬件环境的功能验证。
class SimCamera : public ICamera {
    Q_OBJECT

public:
    explicit SimCamera(QObject *parent = nullptr)
        : ICamera(parent)
    {
        connect(&timer_, &QTimer::timeout, this, &SimCamera::generateFrame);
    }

    ~SimCamera() override { stopCapture(); disconnect(); }

    QStringList availableCameras() override
    {
        return {QStringLiteral("SIM-CAM-001"), QStringLiteral("SIM-CAM-002")};
    }

    bool connectTo(const QString &id) override
    {
        if (connected_)
            return false;
        deviceId_ = id;
        connected_ = true;
        description_ = QStringLiteral("仿真相机 %1").arg(id);
        emit logMessage(QStringLiteral("仿真相机已连接: %1").arg(id));
        emit connectedChanged(true);
        return true;
    }

    void disconnect() override
    {
        if (!connected_)
            return;
        stopCapture();
        connected_ = false;
        emit connectedChanged(false);
        emit logMessage(QStringLiteral("仿真相机已断开"));
    }

    bool isConnected() const override { return connected_; }

    bool startCapture() override
    {
        if (!connected_ || capturing_)
            return false;
        capturing_ = true;
        timer_.start(33); // ~30 FPS
        emit logMessage(QStringLiteral("仿真相机采集已启动"));
        return true;
    }

    void stopCapture() override
    {
        if (!capturing_)
            return;
        capturing_ = false;
        timer_.stop();
        emit logMessage(QStringLiteral("仿真相机采集已停止"));
    }

    bool isCapturing() const override { return capturing_; }

    bool saveFrame(const QString &filePath) override
    {
        if (lastFrame_.isNull())
            return false;
        return lastFrame_.save(filePath, "PNG");
    }

    QString description() const override { return description_; }

private slots:
    void generateFrame()
    {
        // 生成 640x480 渐变彩条图案，帧号递增
        const int w = 640, h = 480;
        QImage img(w, h, QImage::Format_RGB32);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const int r = (x * 255) / w;
                const int g = (y * 255) / h;
                const int b = static_cast<int>((frameCounter_ * 7) & 0xFF);
                img.setPixel(x, y, qRgb(r, g, b));
            }
        }
        // 叠加帧号文字
        QPainter p(&img);
        p.setPen(QPen(Qt::white));
        p.setFont(QFont("Arial", 20));
        p.drawText(img.rect(), Qt::AlignCenter, QStringLiteral("Frame %1").arg(frameCounter_));
        p.end();

        lastFrame_ = img;
        ++frameCounter_;
        emit frameReady(img);
    }

private:
    QString deviceId_;
    QString description_;
    QTimer timer_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> capturing_{false};
    int frameCounter_ = 0;
    QImage lastFrame_;
};

} // namespace mc
