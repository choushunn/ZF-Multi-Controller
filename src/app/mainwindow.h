#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QAction>

#include "core/motorconfig.h"

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
    void refreshDeviceList();
    void refreshCameraList();
    void applyConfig();
    void saveConfig();
    // 集中为关键控件设置 tooltip，解释专业术语（帮助层）
    void setupToolTips();
    // 在状态栏即时反馈运动操作结果（完整原因仍写日志）
    void showMotionResult(const QString &okMsg, const QString &failMsg, bool ok);

    Ui::MainWindow *ui;
    QTimer *statusTimer;
    KCubeMotor *motor;
    mc::ICamera *camera;
    QAction *m_fullscreenAct = nullptr;  // "视图 → 全屏模式"（checkable，F11 切换）
};

#endif // MAINWINDOW_H
