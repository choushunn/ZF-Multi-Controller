#pragma once

#include "core/icamera.h"

#include <QImage>
#include <atomic>
#include <thread>

// DvpCamera：DVP2 SDK 的 ICamera 实现。
// 封装 dvpRefresh/dvpEnum/dvpOpenByUserId/dvpStart/dvpGetFrame/dvpStop/dvpClose，
// 在后台 std::thread 中调用 dvpGetFrame 获取帧并转换为 QImage，
// 通过 frameReady 信号回传 UI。
// 注：dvpHandle 的实际类型定义来自 DVPCamera.h（unsigned int），
// 此处用 unsigned int 作为前向声明，避免头文件直接依赖厂商 SDK。
class DvpCamera : public mc::ICamera {
    Q_OBJECT

public:
    explicit DvpCamera(QObject *parent = nullptr);
    ~DvpCamera() override;

    QStringList availableCameras() override;
    bool connectTo(const QString &id) override;
    void disconnect() override;
    bool isConnected() const override { return connected_; }

    bool startCapture() override;
    void stopCapture() override;
    bool isCapturing() const override { return capturing_; }

    bool saveFrame(const QString &filePath) override;
    QString description() const override { return description_; }

private:
    unsigned int handle_ = 0;  // dvpHandle
    std::atomic<bool> connected_{false};
    std::atomic<bool> capturing_{false};
    std::thread captureThread_;
    QString description_;
    QImage lastFrame_;  // 用于 saveFrame

    // 将 DVP 帧转为 QImage
    static QImage frameToImage(int width, int height, int format, const void *buffer, int bytes);
};
