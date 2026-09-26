#pragma once

#include "core/icamera.h"

#include <QImage>
#include <QMutex>
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
    // 消费者（UI）处理完上一帧后调用，生产者据此判断是否允许写下一帧
    void frameConsumed() override { framePending_.store(false, std::memory_order_release); }

private:
    unsigned int handle_ = 0;  // dvpHandle
    std::atomic<bool> connected_{false};
    std::atomic<bool> capturing_{false};
    std::thread captureThread_;
    QString description_;
    QImage lastFrame_;      // 最近一次 emit 的帧（saveFrame 使用，QMutex 保证读写互斥）
    QMutex frameMutex_;     // 保护 lastFrame_ 的跨线程读写（采集线程写 / saveFrame 读）
    QImage frameBuf_[2];    // 双缓冲：采集线程写入非显示缓冲，UI 线程读显示缓冲
    int slot_ = 0;          // 下一帧写入的缓冲槽（写入后取反）
    std::atomic<bool> framePending_{false};  // 消费者尚未消费上一帧时为 true（丢帧门控）

    // 将 DVP 帧写入 dst（复用 dst 缓冲，仅在尺寸/格式变化时重建）
    static bool renderFrameTo(QImage &dst, int width, int height, int format,
                              const void *buffer, int bytes);
};
