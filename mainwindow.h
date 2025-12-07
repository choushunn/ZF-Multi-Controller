#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>


#include "Thorlabs.MotionControl.KCube.DCServo.h"
#include "DVPCamera.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    // 常量定义
    static constexpr double POSITION_TO_UM = 0.029; // 1个位置对应0.029微米（10个位置=0.29um）
    static constexpr double MIN_POSITION_MM = 0.005;  // 最小位置0.005mm
    static constexpr double MAX_POSITION_MM = 24.5; // 最大位置24.5mm
    static constexpr int DEFAULT_MAX_VELOCITY = 2600; // 默认最大速度，更平滑的运动
    static constexpr int DEFAULT_ACCELERATION = 4000; // 默认加速度，更平滑的运动
    
    // 单位类型常量
    static constexpr int UNIT_TYPE_DISTANCE = 0;    // 距离单位
    static constexpr int UNIT_TYPE_VELOCITY = 1;    // 速度单位
    static constexpr int UNIT_TYPE_ACCELERATION = 2; // 加速度单位

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

private:
    // KDC101 控制相关函数
    void logMessage(const QString &message);
    void updateConnectionStatus(bool connected);
    void enableControls(bool enabled);
    void refreshDeviceList();
    
    Ui::MainWindow *ui;
    QTimer *statusTimer;
    bool isConnected;
    char currentSerialNo[16];
};
#endif // MAINWINDOW_H
