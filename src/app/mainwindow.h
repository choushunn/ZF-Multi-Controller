#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QAction>
#include <QElapsedTimer>

#include "core/motorconfig.h"

class QThread;

class KCubeMotor;
namespace mc {
class ICamera;  // 相机统一接口（icamera.h）
}

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    // Esc 在全屏模式下退出全屏
    void keyPressEvent(QKeyEvent *event) override;
    // 捕获摄像头显示区的双击(放大全屏)与 Esc(退出)事件
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // KDC101 控制相关槽函数
    void on_connectBtn_clicked();
    void on_refreshBtn_clicked();
    void on_homeBtn_clicked();
    void on_stopBtn_clicked();
    void on_moveBtn_clicked();
    void on_setVelocityBtn_clicked();
    void on_jogForwardBtn_clicked();
    void on_jogBackwardBtn_clicked();
    void updateDeviceStatus();
    // 电机对象的信号转发
    void onMotorLog(const QString &message);
    void onConnectedChanged(bool connected);

    // 相机控制
    void onCameraConnectBtnClicked();
    void onCaptureBtnClicked();
    void onSaveImageBtnClicked();
    void onCameraFrameReady(const QImage &frame);
    void onCameraConnectedChanged(bool connected);
    void onCameraLog(const QString &message);

private:
    void enableControls(bool enabled);
    void enableCameraControls(bool enabled);
    // 相机连接后初始化曝光/增益/分辨率控件
    void setupCameraControls();
    bool cameraControlsValid() const;
    // 自动曝光时按相机实际曝光值刷新滑块与数值标签
    void syncExposureUi();
    // 分段映射：前段(0..kSegmentASteps)精细覆盖 min..350ms，后段 350ms..max 粗略
    int msToSliderPos(double ms) const;
    double sliderPosToMs(int pos) const;
    bool syncingExposure_ = false;
    QTimer *exposureRefreshTimer = nullptr;
    struct ExposureCtx { double minMs = 0.0; double maxMs = 0.0; bool valid = false; } exposure_;
    void refreshDeviceList();
    void refreshCameraList();
    // 枚举结果回填：在工作线程完成后于 UI 线程执行
    void applyDeviceList(const QStringList &devices);
    void applyCameraList(const QStringList &ids, const QStringList &names);
    void applyConfig();
    void saveConfig();
    // 集中为关键控件设置 tooltip，解释专业术语（帮助层）
    void setupToolTips();
    // 在状态栏即时反馈运动操作结果（完整原因仍写日志）
    void showMotionResult(const QString &okMsg, const QString &failMsg, bool ok);
    // 摄像头显示区：键盘双击放大/退出全屏
    void toggleCameraFullscreen();
    void updateFullscreenButtonText();
    void restoreCameraToPanel();

    Ui::MainWindow *ui;
    QTimer *statusTimer;
    KCubeMotor *motor;
    mc::ICamera *camera;
    QAction *m_fullscreenAct = nullptr;  // "视图 → 全屏模式"（checkable，F11 切换）
    bool m_cameraFullscreen = false;     // 摄像头显示区当前是否处于全屏
    QWidget *m_cameraParent = nullptr;   // 全屏前的父容器，用于恢复
    QLayout *m_cameraLayout = nullptr;   // 全屏前的父布局，用于恢复
    QElapsedTimer m_fpsClock;            // 相机帧率统计时钟
    qint64 m_lastFpsMs = -1;             // 上一帧时间戳(ms)
    double m_frameFps = 0.0;             // 平滑后的帧率(EWMA)
    bool deviceEnumInFlight_ = false;   // KDC101 设备枚举线程是否在跑（防止并发枚举）
    bool cameraEnumInFlight_ = false;   // 相机枚举线程是否在跑（防止并发枚举）
    QThread *deviceEnumThread_ = nullptr;  // 设备枚举工作线程（析构时等待结束，防悬垂）
    QThread *cameraEnumThread_ = nullptr;  // 相机枚举工作线程（析构时等待结束，防悬垂）
};

#endif // MAINWINDOW_H
