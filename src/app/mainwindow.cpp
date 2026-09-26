#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QPixmap>
#include <QImage>
#include <QSplitter>
#include <QTimer>
#include <QKeyEvent>
#include <QDir>
#include <QStandardPaths>

#include "core/hardware/kcubemotor.h"
#include "core/appconfig.h"
#include "core/logger.h"
#include "core/icamera.h"

#if MC_HAS_HARDWARE
#include "core/hardware/toup_camera.h"
#endif
#if MC_HAS_SIM_CAMERA
#include "core/hardware/sim_camera.h"
#endif

using mc::AppConfig;
using mc::Logger;
using mc::LogLevel;
using mc::MotorConfig;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , statusTimer(new QTimer(this))
    , motor(new KCubeMotor(this))
    , camera(nullptr)
{
    ui->setupUi(this);

    // 从配置注入电机参数
    applyConfig();

    // 初始化相机：优先 DVP2，无硬件时用仿真相机
#if MC_HAS_HARDWARE
    // 现场相机为 ToupTek（VID_0547），不能用 DVP2 SDK 枚举。
    camera = new ToupCamera(this);
#elif MC_HAS_SIM_CAMERA
    camera = new mc::SimCamera(this);
#else
    camera = nullptr;
#endif

    // 连接电机信号
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateDeviceStatus);
    connect(motor, &KCubeMotor::logMessage, this, &MainWindow::onMotorLog);
    connect(motor, &KCubeMotor::connectedChanged, this, &MainWindow::onConnectedChanged);

    // 连接 Logger 信号（分级日志输出到 UI）
    connect(&Logger::instance(), &Logger::message, this,
        [this](LogLevel level, const QString &formatted) {
            // 分级日志着色
            QString color = QStringLiteral("#1A2233");
            switch (level) {
            case LogLevel::Debug: color = QStringLiteral("#8B96A8"); break;
            case LogLevel::Info:  color = QStringLiteral("#1A2233"); break;
            case LogLevel::Warn:  color = QStringLiteral("#B26A00"); break;
            case LogLevel::Error: color = QStringLiteral("#D64045"); break;
            }
            const QString html = QStringLiteral("<span style='color:%1'>%2</span>")
                .arg(color).arg(formatted.toHtmlEscaped());
            ui->logTextEdit->appendHtml(html);
        });

    // 连接相机信号
    if (camera) {
        connect(ui->cameraConnectBtn, &QPushButton::clicked,
                this, &MainWindow::onCameraConnectBtnClicked);
        connect(ui->captureBtn, &QPushButton::clicked,
                this, &MainWindow::onCaptureBtnClicked);
        connect(ui->saveImageBtn, &QPushButton::clicked,
                this, &MainWindow::onSaveImageBtnClicked);

        connect(camera, &mc::ICamera::frameReady,
                this, &MainWindow::onCameraFrameReady);
        connect(camera, &mc::ICamera::connectedChanged,
                this, &MainWindow::onCameraConnectedChanged);
        connect(camera, &mc::ICamera::logMessage,
                this, &MainWindow::onCameraLog);
    }

    // 设置位置输入范围（微米），从配置读取
    const auto &cfg = AppConfig::instance().data();
    const double minPositionUm = cfg.minPositionMm * 1000;
    const double maxPositionUm = cfg.maxPositionMm * 1000;
    ui->positionSpinBox->setRange(minPositionUm, maxPositionUm);
    ui->positionSpinBox->setSuffix(" μm");
    ui->jogStepSpinBox->setRange(0.1, maxPositionUm);
    ui->jogStepSpinBox->setSuffix(" μm");
    // 速度/加速度上限与默认值均取自配置，避免硬编码漂移导致 setValue 被 clamp
    ui->velocitySpinBox->setMaximum(cfg.defaultMaxVelocity);
    ui->accelerationSpinBox->setMaximum(cfg.defaultAcceleration);
    ui->velocitySpinBox->setValue(cfg.defaultMaxVelocity);
    ui->accelerationSpinBox->setValue(cfg.defaultAcceleration);

    onMotorLog(QStringLiteral("应用程序已启动"));

    // 帮助层：为关键控件解释专业术语
    setupToolTips();

    // 左(控制)/右(显示) 可拖拽分割：右侧优先吃窗口增长，分隔手柄可见可调
    ui->mainSplitter->setHandleWidth(7);
    ui->mainSplitter->setChildrenCollapsible(false);
    ui->mainSplitter->widget(0)->setMinimumWidth(400);  // 左侧容纳等宽排列的输入列
    ui->mainSplitter->widget(1)->setMinimumWidth(440);
    ui->mainSplitter->setStretchFactor(0, 0);
    ui->mainSplitter->setStretchFactor(1, 1);
    ui->mainSplitter->setSizes({430, 730});  // 初始分配，防止折叠
    // 分隔手柄使用 Qt 原生样式，保留可拖拽与最小宽度设置

    // "视图 → 全屏模式"：启动即全屏，可经菜单/快捷键退出
    QMenu *viewMenu = menuBar()->addMenu(QStringLiteral("视图(&V)"));
    m_fullscreenAct = viewMenu->addAction(QStringLiteral("全屏模式(&F)"));
    m_fullscreenAct->setCheckable(true);
    m_fullscreenAct->setChecked(true);   // 与 main() 的 showFullScreen() 保持一致
    m_fullscreenAct->setShortcut(QKeySequence::FullScreen);  // F11
    connect(m_fullscreenAct, &QAction::toggled, this, [this](bool fs) {
        if (fs) showFullScreen();
        else    showNormal();
    });

    // 首次设备/相机列表刷新延迟到事件循环启动、主窗口已显示之后再执行：
    // 避免"未发现设备"弹窗先于主窗口出现。手动"刷新列表"仍同步刷新并正常弹窗。
    QTimer::singleShot(0, this, [this]() {
        refreshDeviceList();
        refreshCameraList();
    });
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    // 全屏模式下按 Esc 退出全屏（进入/退出也可用 F11，经菜单 QAction 切换）
    if (e->key() == Qt::Key_Escape && isFullScreen()) {
        m_fullscreenAct->setChecked(false);  // 触发 toggled → showNormal()
        return;
    }
    QMainWindow::keyPressEvent(e);
}

MainWindow::~MainWindow()
{
    saveConfig();
    delete ui;
}

void MainWindow::applyConfig()
{
    const auto &cfg = AppConfig::instance().data();
    motor->setConfig(MotorConfig(cfg.positionToUm, cfg.minPositionMm,
                                 cfg.maxPositionMm, cfg.defaultMaxVelocity,
                                 cfg.defaultAcceleration));
}

void MainWindow::saveConfig()
{
    auto &appCfg = AppConfig::instance();
    // 如果已连接设备，记录其序列号作为默认
    if (motor->isConnected())
        appCfg.setDefaultSerial(motor->serialNumber());
    appCfg.save();
}

void MainWindow::on_connectBtn_clicked()
{
    if (!motor->isConnected()) {
        const QString serialNo = ui->serialNoComboBox->currentText().trimmed();
        if (serialNo.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("请选择设备序列号"));
            return;
        }

        if (motor->connectTo(serialNo)) {
            onMotorLog(QStringLiteral("成功连接到设备: %1").arg(serialNo));

            // 读取当前速度/加速度，并转换为实际物理单位显示
            double currentVelocity = motor->velocityUmps();
            double currentAcceleration = motor->accelerationUmps2();
            if (currentVelocity > 0)
                ui->velocitySpinBox->setValue(currentVelocity);
            if (currentAcceleration > 0)
                ui->accelerationSpinBox->setValue(currentAcceleration);

            statusTimer->start(500); // 每 500ms 更新一次状态
        } else {
            QMessageBox::critical(this, QStringLiteral("错误"),
                                  QStringLiteral("无法连接到设备，请检查序列号是否正确"));
            onMotorLog(QStringLiteral("连接设备失败: %1").arg(serialNo));
        }
    } else {
        motor->disconnect();
        statusTimer->stop();
    }
}

void MainWindow::on_refreshBtn_clicked()
{
    refreshDeviceList();
}

void MainWindow::on_homeBtn_clicked()
{
    showMotionResult(QStringLiteral("归位操作已启动"),
                     QStringLiteral("归位失败，详情见日志"), motor->home());
}

void MainWindow::on_stopBtn_clicked()
{
    showMotionResult(QStringLiteral("电机已停止"),
                     QStringLiteral("停止电机失败，详情见日志"), motor->stop());
}

void MainWindow::on_moveBtn_clicked()
{
    showMotionResult(QStringLiteral("已移动到 %1 μm")
                         .arg(QString::number(ui->positionSpinBox->value(), 'f', 1)),
                     QStringLiteral("移动失败，详情见日志"), motor->moveToUm(ui->positionSpinBox->value()));
}

void MainWindow::on_setVelocityBtn_clicked()
{
    showMotionResult(QStringLiteral("速度参数已更新"),
                     QStringLiteral("设置速度失败，详情见日志"),
                     motor->setVelocity(ui->velocitySpinBox->value(), ui->accelerationSpinBox->value()));
}

void MainWindow::on_jogForwardBtn_clicked()
{
    showMotionResult(QStringLiteral("正向点动 %1 μm")
                         .arg(QString::number(ui->jogStepSpinBox->value(), 'f', 1)),
                     QStringLiteral("点动失败，详情见日志"), motor->moveRelativeUm(+ui->jogStepSpinBox->value()));
}

void MainWindow::on_jogBackwardBtn_clicked()
{
    showMotionResult(QStringLiteral("反向点动 %1 μm")
                         .arg(QString::number(ui->jogStepSpinBox->value(), 'f', 1)),
                     QStringLiteral("点动失败，详情见日志"), motor->moveRelativeUm(-ui->jogStepSpinBox->value()));
}

void MainWindow::updateDeviceStatus()
{
    if (!motor->isConnected())
        return;

    ui->positionLabel->setText(QStringLiteral("%1").arg(motor->positionUm(), 0, 'f', 1));

    static QString lastStatusInfo;
    const QString info = motor->status().describe();
    if (info != lastStatusInfo) {
        onMotorLog(info);
        lastStatusInfo = info;
    }
}

void MainWindow::onMotorLog(const QString &message)
{
    const QString timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    const QString formatted = QStringLiteral("[%1] %2").arg(timestamp, message);
    ui->logTextEdit->appendPlainText(formatted);
    // 同时写入 Logger（会分级落盘）
    Logger::instance().info(message);
}

void MainWindow::showMotionResult(const QString &okMsg, const QString &failMsg, bool ok)
{
    // 失败用红色突出，成功用默认色；信息只在状态栏停留数秒，完整原因仍写日志
    statusBar()->setStyleSheet(ok ? QString()
                                  : QStringLiteral("QStatusBar { color: #D64045; }"));
    statusBar()->showMessage(ok ? okMsg : failMsg, ok ? 5000 : 10000);
}

void MainWindow::setupToolTips()
{
    ui->connectBtn->setToolTip(QStringLiteral("连接/断开选定的 KDC101 电机控制器。连接后电机控件才可用"));
    ui->homeBtn->setToolTip(QStringLiteral("归位：让电机回到原点。首次移动前必须执行一次"));
    ui->moveBtn->setToolTip(QStringLiteral("移动到目标位置（绝对位置，单位 μm）"));
    ui->setVelocityBtn->setToolTip(QStringLiteral("把左侧的速度/加速度值应用到电机"));
    ui->jogForwardBtn->setToolTip(QStringLiteral("正向点动：按步长微调"));
    ui->jogBackwardBtn->setToolTip(QStringLiteral("反向点动：按步长微调"));
    ui->positionLabel->setToolTip(QStringLiteral("当前电机位置（μm），归位后数值才有意义"));
    ui->cameraConnectBtn->setToolTip(QStringLiteral("连接/断开相机"));
    ui->captureBtn->setToolTip(QStringLiteral("冻结/恢复当前画面"));
    ui->saveImageBtn->setToolTip(QStringLiteral("把当前画面保存为图片文件"));
}

void MainWindow::onConnectedChanged(bool connected)
{
    ui->connectionStatusLabel->setText(connected ? QStringLiteral("已连接") : QStringLiteral("未连接"));
    // 状态配色：已连接=绿，未连接=灰
    ui->connectionStatusLabel->setStyleSheet(connected
        ? QStringLiteral("color: #22A55A;")
        : QStringLiteral("color: #9AA3B0;"));
    ui->connectBtn->setText(connected ? QStringLiteral("断开") : QStringLiteral("连接"));
    enableControls(connected);
}

void MainWindow::enableControls(bool enabled)
{
    ui->homeBtn->setEnabled(enabled);
    ui->stopBtn->setEnabled(enabled);
    ui->moveBtn->setEnabled(enabled);
    ui->positionSpinBox->setEnabled(enabled);
    ui->velocitySpinBox->setEnabled(enabled);
    ui->accelerationSpinBox->setEnabled(enabled);
    ui->setVelocityBtn->setEnabled(enabled);
    ui->jogForwardBtn->setEnabled(enabled);
    ui->jogBackwardBtn->setEnabled(enabled);
    ui->jogStepSpinBox->setEnabled(enabled);
    ui->serialNoComboBox->setEnabled(!enabled);
}

void MainWindow::enableCameraControls(bool enabled)
{
    ui->captureBtn->setEnabled(enabled);
    ui->saveImageBtn->setEnabled(enabled);
    ui->cameraConnectBtn->setText(enabled ? QStringLiteral("断开相机") : QStringLiteral("连接相机"));
    // 断开相机时把「捕获」按钮复位为「捕获图像」状态
    ui->captureBtn->setText(QStringLiteral("捕获图像"));
}

void MainWindow::refreshDeviceList()
{
    const QStringList devices = motor->availableDevices();

    const QString current = ui->serialNoComboBox->currentText();
    ui->serialNoComboBox->clear();

    if (devices.isEmpty()) {
        onMotorLog(QStringLiteral("未发现任何设备"));
        QMessageBox::information(this, QStringLiteral("信息"), QStringLiteral("未发现任何KDC101设备"));
        return;
    }

    onMotorLog(QStringLiteral("发现 %1 个设备").arg(devices.size()));
    for (int i = 0; i < devices.size(); ++i)
        ui->serialNoComboBox->addItem(devices[i]);

    // 优先选择配置中的默认设备，其次保留原有选择，否则选第一个
    const QString &defaultSerial = AppConfig::instance().defaultSerial();
    int idx = ui->serialNoComboBox->findText(defaultSerial);
    if (idx < 0) idx = ui->serialNoComboBox->findText(current);
    ui->serialNoComboBox->setCurrentIndex(idx >= 0 ? idx : 0);
}

void MainWindow::refreshCameraList()
{
    if (!camera)
        return;

    const QStringList cams = camera->availableCameras();
    if (cams.isEmpty())
        onMotorLog(QStringLiteral("未发现相机设备"));
    else
        onMotorLog(QStringLiteral("发现 %1 个相机").arg(cams.size()));
}

void MainWindow::onCameraConnectBtnClicked()
{
    if (!camera)
        return;

    if (!camera->isConnected()) {
        // 获取可用相机列表并连接第一个
        const QStringList cams = camera->availableCameras();
        if (cams.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("未发现相机设备"));
            return;
        }

        // 优先用配置中的 UserID
        const QString &cfgUserId = AppConfig::instance().data().cameraUserId;
        QString id = cams.contains(cfgUserId) ? cfgUserId : cams.first();

        if (!camera->connectTo(id)) {
            QMessageBox::critical(this, QStringLiteral("错误"),
                                  QStringLiteral("无法连接到相机: %1").arg(id));
            return;
        }

        // 连接成功后启动采集
        camera->startCapture();
    } else {
        camera->disconnect();
    }
}

void MainWindow::onCaptureBtnClicked()
{
    if (!camera)
        return;

    if (camera->isCapturing()) {
        // 正在实时预览：冻结当前帧
        camera->stopCapture();
        ui->captureBtn->setText(QStringLiteral("恢复预览"));
        onMotorLog(QStringLiteral("画面已冻结，可点击保存图像"));
    } else {
        // 已冻结：恢复实时预览
        camera->startCapture();
        ui->captureBtn->setText(QStringLiteral("捕获图像"));
        onMotorLog(QStringLiteral("已恢复实时预览"));
    }
}

void MainWindow::onSaveImageBtnClicked()
{
    if (!camera)
        return;

    const QString dir = AppConfig::instance().data().imageSaveDir;
    const QString defaultPath = QDir(dir).absoluteFilePath(
        QStringLiteral("capture_%1.png")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))));

    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存图像"), defaultPath,
        QStringLiteral("PNG 图像 (*.png);;JPEG 图像 (*.jpg)"));

    if (path.isEmpty())
        return;

    if (camera->saveFrame(path))
        onMotorLog(QStringLiteral("图像已保存: %1").arg(path));
    else
        QMessageBox::warning(this, QStringLiteral("警告"), QStringLiteral("保存图像失败"));
}

void MainWindow::onCameraFrameReady(const QImage &frame)
{
    // 在 label 中显示帧
    const QPixmap pix = QPixmap::fromImage(frame).scaled(
        ui->cameraDisplayLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->cameraDisplayLabel->setPixmap(pix);
    ui->imageInfoLabel->setText(QStringLiteral("图像信息: %1x%2").arg(frame.width()).arg(frame.height()));
}

void MainWindow::onCameraConnectedChanged(bool connected)
{
    ui->cameraStatusLabel->setText(connected ? QStringLiteral("相机状态: 已连接") : QStringLiteral("相机状态: 未连接"));
    // 状态配色：已连接=绿，未连接=灰
    ui->cameraStatusLabel->setStyleSheet(connected
        ? QStringLiteral("color: #22A55A;")
        : QStringLiteral("color: #9AA3B0;"));
    enableCameraControls(connected);
}

void MainWindow::onCameraLog(const QString &message)
{
    onMotorLog(message);
}
