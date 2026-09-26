#include "core/hardware/toup_camera.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace {
constexpr unsigned kMaxCameras = 16;
constexpr unsigned kSdkOk = 0;
using EventCallback = void (__stdcall *)(unsigned, void *);
using DataCallback = void (__stdcall *)(const void *, const void *, int, void *);
// ToupTek SDK 错误码为 HRESULT，成功即 ==0；此处不引入 Windows.h
inline bool sdkOk(int hr) { return hr == 0; }
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
    using SimpleInt = int (__stdcall *)(void *);                       // HRESULT 型单句柄
    using GetResNumber = unsigned (__stdcall *)(void *);               // Toupcam_get_ResolutionNumber
    using GetESize = int (__stdcall *)(void *, unsigned *);            // Toupcam_get_eSize
    using GetExpTime = int (__stdcall *)(void *, unsigned *);          // 曝光(μs)
    using PutExpTime = int (__stdcall *)(void *, unsigned);
    using GetExpRange = int (__stdcall *)(void *, unsigned *, unsigned *, unsigned *);
    using GetGain = int (__stdcall *)(void *, unsigned short *);       // 增益(%)
    using PutGain = int (__stdcall *)(void *, unsigned short);
    using GetGainRange = int (__stdcall *)(void *, unsigned short *, unsigned short *, unsigned short *);
    using GetResInfo = int (__stdcall *)(void *, unsigned, unsigned *, unsigned *);  // Toupcam_get_Resolution
    using PutESize = int (__stdcall *)(void *, unsigned);                         // Toupcam_put_eSize

    EnumV2 enumerate = nullptr;
    Open open = nullptr;
    Close close = nullptr;
    StartPushModeV3 start = nullptr;
    Stop stop = nullptr;
    PutESize putESize = nullptr;         // Toupcam_put_eSize
    GetResNumber getResNumber = nullptr; // Toupcam_get_ResolutionNumber
    GetESize getESize = nullptr;         // Toupcam_get_eSize
    GetResInfo getResolution = nullptr;  // Toupcam_get_Resolution(宽高)
    GetExpTime getExpTime = nullptr;
    PutExpTime putExpTime = nullptr;
    GetExpRange getExpRange = nullptr;
    GetGain getGain = nullptr;
    PutGain putGain = nullptr;
    GetGainRange getGainRange = nullptr;
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
    api->getResNumber = reinterpret_cast<Api::GetResNumber>(library_->resolve("Toupcam_get_ResolutionNumber"));
    api->getESize = reinterpret_cast<Api::GetESize>(library_->resolve("Toupcam_get_eSize"));
    api->getResolution = reinterpret_cast<Api::GetResInfo>(library_->resolve("Toupcam_get_Resolution"));
    api->putESize = reinterpret_cast<Api::PutESize>(library_->resolve("Toupcam_put_eSize"));
    api->getExpTime = reinterpret_cast<Api::GetExpTime>(library_->resolve("Toupcam_get_ExpoTime"));
    api->putExpTime = reinterpret_cast<Api::PutExpTime>(library_->resolve("Toupcam_put_ExpoTime"));
    api->getExpRange = reinterpret_cast<Api::GetExpRange>(library_->resolve("Toupcam_get_ExpTimeRange"));
    api->getGain = reinterpret_cast<Api::GetGain>(library_->resolve("Toupcam_get_ExpoAGain"));
    api->putGain = reinterpret_cast<Api::PutGain>(library_->resolve("Toupcam_put_ExpoAGain"));
    api->getGainRange = reinterpret_cast<Api::GetGainRange>(library_->resolve("Toupcam_get_ExpoAGainRange"));
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
    idList_.clear();
    nameList_.clear();
    if (!ensureApi())
        return QStringList();

    Device devices[kMaxCameras] = {};
    const unsigned count = qMin(api_->enumerate(devices), kMaxCameras);
    for (unsigned i = 0; i < count; ++i) {
        const QString id = QString::fromWCharArray(devices[i].id);
        const QString name = QString::fromWCharArray(devices[i].displayName);
        if (!id.isEmpty()) {
            idList_ << id;
            nameList_ << name;
            if (description_.isEmpty())
                description_ = name;
        }
    }
    return idList_;
}

QStringList ToupCamera::availableCameraNames()
{
    // 保证名称列表与标识列表同时就绪（枚举成本集中在 availableCameras()）
    if (nameList_.isEmpty() && idList_.isEmpty())
        availableCameras();
    return nameList_;
}

// —— 曝光/增益/分辨率参数（单位换算：μs→ms、unsigned short %→double）——
static bool hasHandle(const void *h) { return h != nullptr; }

bool ToupCamera::exposureRange(double *minMs, double *maxMs)
{
    if (!ensureApi() || !api_->getExpRange || !hasHandle(handle_) || !minMs || !maxMs)
        return false;
    unsigned mn = 0, mx = 0, def = 0;
    if (sdkOk(api_->getExpRange(handle_, &mn, &mx, &def))) {
        *minMs = mx > 0 ? double(mn) / 1000.0 : 0.001;
        *maxMs = double(mx) / 1000.0;
        return mx > 0;
    }
    return false;
}

bool ToupCamera::setExposure(double ms)
{
    if (!ensureApi() || !api_->putExpTime || !hasHandle(handle_) || ms < 0)
        return false;
    return sdkOk(api_->putExpTime(handle_, unsigned(ms * 1000.0)));
}

double ToupCamera::exposure() const
{
    if (!api_ || !api_->getExpTime || !hasHandle(handle_))
        return -1.0;
    unsigned us = 0;
    return sdkOk(api_->getExpTime(handle_, &us)) ? double(us) / 1000.0 : -1.0;
}

bool ToupCamera::gainRange(double *minPct, double *maxPct)
{
    if (!ensureApi() || !api_->getGainRange || !hasHandle(handle_) || !minPct || !maxPct)
        return false;
    unsigned short mn = 0, mx = 0, def = 0;
    if (sdkOk(api_->getGainRange(handle_, &mn, &mx, &def))) {
        *minPct = double(mn);
        *maxPct = double(mx);
        return mx > 0;
    }
    return false;
}

bool ToupCamera::setGain(double pct)
{
    if (!ensureApi() || !api_->putGain || !hasHandle(handle_) || pct < 0)
        return false;
    return sdkOk(api_->putGain(handle_, static_cast<unsigned short>(qRound(pct))));
}

double ToupCamera::gain() const
{
    if (!api_ || !api_->getGain || !hasHandle(handle_))
        return -1.0;
    unsigned short v = 0;
    return sdkOk(api_->getGain(handle_, &v)) ? double(v) : -1.0;
}

QStringList ToupCamera::resolutions() const
{
    QStringList list;
    if (!handle_ || !api_ || !api_->getResNumber || !api_->getResolution)
        return list;
    const int n = static_cast<int>(api_->getResNumber(handle_));
    for (int i = 0; i < n; ++i) {
        unsigned w = 0, h = 0;
        if (sdkOk(api_->getResolution(handle_, static_cast<unsigned>(i), &w, &h)) && w > 0 && h > 0)
            list << QStringLiteral("%1x%2").arg(w).arg(h);
    }
    return list;
}

int ToupCamera::currentResolution() const
{
    if (!handle_ || !api_ || !api_->getESize)
        return -1;
    unsigned idx = 0;
    return sdkOk(api_->getESize(handle_, &idx)) ? static_cast<int>(idx) : -1;
}

bool ToupCamera::setResolution(int index)
{
    if (!ensureApi() || !api_->putESize || !hasHandle(handle_) || index < 0)
        return false;
    // 切换分辨率需先停流再应用，最后恢复采集
    const bool wasCapturing = capturing_;
    if (wasCapturing)
        stopCapture();
    const bool ok = sdkOk(api_->putESize(handle_, static_cast<unsigned>(index)));
    if (wasCapturing)
        startCapture();
    return ok;
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
