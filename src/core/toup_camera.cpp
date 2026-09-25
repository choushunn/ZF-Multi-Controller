#include "core/toup_camera.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace {
constexpr unsigned kMaxCameras = 16;
constexpr unsigned kSdkOk = 0;
using EventCallback = void (__stdcall *)(unsigned, void *);
using DataCallback = void (__stdcall *)(const void *, const void *, int, void *);
}

struct ToupCamera::Device {
    wchar_t displayName[64];
    wchar_t id[64];
    const void *model;
};

// The first fields are ABI-compatible with ToupcamFrameInfoV2.  The SDK owns
// the pointer only for the callback duration, so frames are copied immediately.
struct ToupCamera::FrameInfo {
    unsigned width;
    unsigned height;
    unsigned flag;
    unsigned seq;
    unsigned long long timestamp;
};

struct ToupCamera::Api {
    using EnumV2 = unsigned (__stdcall *)(Device *);
    using Open = void * (__stdcall *)(const wchar_t *);
    using Close = void (__stdcall *)(void *);
    using StartPushModeV3 = long (__stdcall *)(void *, DataCallback, void *, EventCallback, void *);
    using Stop = long (__stdcall *)(void *);

    EnumV2 enumerate = nullptr;
    Open open = nullptr;
    Close close = nullptr;
    StartPushModeV3 start = nullptr;
    Stop stop = nullptr;
};

ToupCamera::ToupCamera(QObject *parent)
    : ICamera(parent)
{
}

ToupCamera::~ToupCamera()
{
    disconnect();
}

bool ToupCamera::ensureApi()
{
    if (api_)
        return true;

    const QString appDll = QDir(QCoreApplication::applicationDirPath()).filePath("toupcam.dll");
    const QString installedDll = QStringLiteral("C:/Program Files/TCC/toupcam.dll");
    const QString dllPath = QFileInfo::exists(appDll) ? appDll : installedDll;
    if (!QFileInfo::exists(dllPath)) {
        emit logMessage(QStringLiteral("未找到 ToupTek SDK（toupcam.dll）"));
        return false;
    }

    library_ = std::make_unique<QLibrary>(dllPath);
    if (!library_->load()) {
        emit logMessage(QStringLiteral("加载 ToupTek SDK 失败: %1").arg(library_->errorString()));
        library_.reset();
        return false;
    }

    auto api = std::make_unique<Api>();
    api->enumerate = reinterpret_cast<Api::EnumV2>(library_->resolve("Toupcam_EnumV2"));
    api->open = reinterpret_cast<Api::Open>(library_->resolve("Toupcam_Open"));
    api->close = reinterpret_cast<Api::Close>(library_->resolve("Toupcam_Close"));
    api->start = reinterpret_cast<Api::StartPushModeV3>(library_->resolve("Toupcam_StartPushModeV3"));
    api->stop = reinterpret_cast<Api::Stop>(library_->resolve("Toupcam_Stop"));
    if (!api->enumerate || !api->open || !api->close || !api->start || !api->stop) {
        emit logMessage(QStringLiteral("ToupTek SDK 缺少所需接口"));
        library_->unload();
        library_.reset();
        return false;
    }
    api_ = std::move(api);
    return true;
}

QStringList ToupCamera::availableCameras()
{
    QStringList cameras;
    if (!ensureApi())
        return cameras;

    Device devices[kMaxCameras] = {};
    const unsigned count = qMin(api_->enumerate(devices), kMaxCameras);
    for (unsigned i = 0; i < count; ++i) {
        const QString id = QString::fromWCharArray(devices[i].id);
        const QString name = QString::fromWCharArray(devices[i].displayName);
        if (!id.isEmpty()) {
            cameras << id;
            if (description_.isEmpty())
                description_ = name;
        }
    }
    return cameras;
}

bool ToupCamera::connectTo(const QString &id)
{
    if (connected_ || !ensureApi())
        return false;

    handle_ = api_->open(reinterpret_cast<const wchar_t *>(id.utf16()));
    if (!handle_) {
        emit logMessage(QStringLiteral("ToupTek 相机连接失败: %1").arg(id));
        return false;
    }
    connected_ = true;
    emit connectedChanged(true);
    emit logMessage(QStringLiteral("ToupTek 相机已连接: %1").arg(description_.isEmpty() ? id : description_));
    return true;
}

void ToupCamera::disconnect()
{
    if (!connected_)
        return;
    stopCapture();
    api_->close(handle_);
    handle_ = nullptr;
    connected_ = false;
    emit connectedChanged(false);
    emit logMessage(QStringLiteral("相机已断开"));
}

bool ToupCamera::startCapture()
{
    if (!connected_ || capturing_)
        return false;
    if (api_->start(handle_, reinterpret_cast<DataCallback>(&ToupCamera::onFrame), this, nullptr, nullptr) != kSdkOk) {
        emit logMessage(QStringLiteral("启动 ToupTek 相机采集失败"));
        return false;
    }
    capturing_ = true;
    emit logMessage(QStringLiteral("相机采集已启动"));
    return true;
}

void ToupCamera::stopCapture()
{
    if (!capturing_)
        return;
    api_->stop(handle_);
    capturing_ = false;
    emit logMessage(QStringLiteral("相机采集已停止"));
}

bool ToupCamera::saveFrame(const QString &filePath)
{
    if (lastFrame_.isNull())
        return false;
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    return lastFrame_.save(filePath, "PNG");
}

void __stdcall ToupCamera::onFrame(const void *data, const FrameInfo *info, int, void *context)
{
    auto *self = static_cast<ToupCamera *>(context);
    if (!self || !data || !info || !self->capturing_)
        return;
    const QImage image(static_cast<const uchar *>(data), static_cast<int>(info->width),
                       static_cast<int>(info->height), static_cast<int>(info->width) * 3,
                       QImage::Format_RGB888);
    if (!image.isNull()) {
        self->lastFrame_ = image.copy();
        emit self->frameReady(self->lastFrame_);
    }
}
