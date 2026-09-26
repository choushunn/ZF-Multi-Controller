#pragma once

#include "core/icamera.h"

#include <QLibrary>
#include <QImage>

#include <atomic>
#include <memory>
#include <QMutex>

// ToupCamera 通过运行时加载 toupcam.dll 接入 ToupTek/同源相机。
// 不在源码树复制厂商 SDK；优先使用应用目录，再使用 TCC 的已安装 SDK。
class ToupCamera : public mc::ICamera {
    Q_OBJECT

public:
    explicit ToupCamera(QObject *parent = nullptr);
    ~ToupCamera() override;

    QStringList availableCameras() override;
    QStringList availableCameraNames() override;
    bool connectTo(const QString &id) override;
    void disconnect() override;
    bool isConnected() const override { return connected_; }

    // 相机参数（曝光/增益/分辨率）
    bool exposureRange(double *minMs, double *maxMs) override;
    bool setExposure(double ms) override;
    double exposure() const override;
    bool autoExposureEnabled() const override;
    bool setAutoExposure(bool on) override;
    bool gainRange(double *minPct, double *maxPct) override;
    bool setGain(double pct) override;
    double gain() const override;
    QStringList resolutions() const override;
    int currentResolution() const override;
    bool setResolution(int index) override;

    bool startCapture() override;
    void stopCapture() override;
    bool isCapturing() const override { return capturing_; }
    bool saveFrame(const QString &filePath) override;
    QString description() const override { return description_; }
    // 消费者（UI）处理完上一帧后调用，生产者据此判断是否允许写下一帧
    void frameConsumed() override { framePending_.store(false, std::memory_order_release); }

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
    std::atomic<bool> capturing_{false};   // SDK 回调线程跨线程读取
    QString description_;
    QImage lastFrame_;       // 最近一次 emit 的帧（saveFrame 使用，QMutex 保证读写互斥）
    QMutex frameMutex_;      // 保护 lastFrame_ 的跨线程读写（回调线程写 / saveFrame 读）
    QImage frameBuf_[2];     // 双缓冲：回调线程写入非显示缓冲，UI 线程读显示缓冲
    int slot_ = 0;           // 下一帧写入的缓冲槽（写入后取反）
    std::atomic<bool> framePending_{false};  // 消费者尚未消费上一帧时为 true（丢帧门控）
    QStringList idList_;    // 与 nameList_ 逐项对应的设备标识（序列号/UserID）
    QStringList nameList_;  // 设备型号名（displayName）
};
