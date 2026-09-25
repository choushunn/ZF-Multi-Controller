#pragma once

#include "core/icamera.h"

#include <QLibrary>
#include <QImage>

#include <memory>

// ToupCamera 通过运行时加载 toupcam.dll 接入 ToupTek/同源相机。
// 不在源码树复制厂商 SDK；优先使用应用目录，再使用 TCC 的已安装 SDK。
class ToupCamera : public mc::ICamera {
    Q_OBJECT

public:
    explicit ToupCamera(QObject *parent = nullptr);
    ~ToupCamera() override;

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
    struct Api;
    struct Device;
    struct FrameInfo;

    static void __stdcall onFrame(const void *data, const FrameInfo *info,
                                  int snap, void *context);
    bool ensureApi();

    std::unique_ptr<QLibrary> library_;
    std::unique_ptr<Api> api_;
    void *handle_ = nullptr;
    bool connected_ = false;
    bool capturing_ = false;
    QString description_;
    QImage lastFrame_;
};
