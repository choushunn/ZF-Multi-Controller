#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , isConnected(false)
{
    ui->setupUi(this);
    
    // 初始化定时器，用于更新设备状态
    statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateDeviceStatus);
    
    // 初始化序列号数组
    memset(currentSerialNo, 0, sizeof(currentSerialNo));
    
    // 初始化设备管理库
    TLI_InitializeSimulations();
    
    // 设置位置输入范围（微米）
    double minPositionUm = MIN_POSITION_MM * 1000; // 转换为微米
    double maxPositionUm = MAX_POSITION_MM * 1000; // 转换为微米
    ui->positionSpinBox->setRange(minPositionUm, maxPositionUm);
    ui->positionSpinBox->setSuffix(" μm");
    
    // 设置点动步长输入范围（微米）
    ui->jogStepSpinBox->setRange(0.1, maxPositionUm); // 最大步长不超过最大位置
    ui->jogStepSpinBox->setSuffix(" μm");
    
    // 设置速度和加速度控件的默认值（设备单位）
    // 注意：实际单位将在设备连接后设置
    ui->velocitySpinBox->setValue(DEFAULT_MAX_VELOCITY);
    ui->accelerationSpinBox->setValue(DEFAULT_ACCELERATION);
    
    logMessage("应用程序已启动");
    
    // 自动刷新设备列表
    refreshDeviceList();
}

MainWindow::~MainWindow()
{
    // 如果设备已连接，断开连接
    if (isConnected) {
        CC_StopImmediate(currentSerialNo);
        CC_Close(currentSerialNo);
    }
    
    // 释放设备管理库
    TLI_UninitializeSimulations();
    
    delete ui;
}

void MainWindow::on_connectBtn_clicked()
{
    if (!isConnected) {
        // 连接设备
        QString serialNo = ui->serialNoComboBox->currentText().trimmed();
        if (serialNo.isEmpty()) {
            QMessageBox::warning(this, "警告", "请选择设备序列号");
            return;
        }
        
        // 将QString转换为char数组
        QByteArray ba = serialNo.toLocal8Bit();
        strncpy_s(currentSerialNo, ba.constData(), sizeof(currentSerialNo) - 1);
        
        // 尝试连接设备
        if (CC_Open(currentSerialNo) == 0) {
            // 连接成功
            isConnected = true;
            updateConnectionStatus(true);
            logMessage(QString("成功连接到设备: %1").arg(serialNo));
            
            // 加载设备设置
            if (CC_LoadSettings(currentSerialNo)) {
                logMessage("设备设置加载成功");
            } else {
                logMessage("警告: 设备设置加载失败");
            }
            
            // 启用通道
            if (CC_EnableChannel(currentSerialNo) == 0) {
                logMessage("设备通道已启用");
                
                // 设置软件限位范围：0.5mm到24.5mm
                // 将毫米转换为位置值
                int minPosition = static_cast<int>((MIN_POSITION_MM * 1000) / POSITION_TO_UM);
                int maxPosition = static_cast<int>((MAX_POSITION_MM * 1000) / POSITION_TO_UM);
                
                if (CC_SetStageAxisLimits(currentSerialNo, minPosition, maxPosition) == 0) {
                    logMessage(QString("软件限位已设置: %1mm - %2mm").arg(MIN_POSITION_MM).arg(MAX_POSITION_MM));
                } else {
                    logMessage("警告: 设置软件限位失败");
                }
                
                // 设置更平滑的速度和加速度参数
                if (CC_SetVelParams(currentSerialNo, DEFAULT_ACCELERATION, DEFAULT_MAX_VELOCITY) == 0) {
                    logMessage(QString("默认速度参数已设置: 最大速度=%1, 加速度=%2").arg(DEFAULT_MAX_VELOCITY).arg(DEFAULT_ACCELERATION));
                } else {
                    logMessage("警告: 设置默认速度参数失败");
                }
                
                // 设置点动速度参数，使用相同的平滑参数
                if (CC_SetJogVelParams(currentSerialNo, DEFAULT_ACCELERATION, DEFAULT_MAX_VELOCITY) == 0) {
                    logMessage("点动速度参数已设置");
                } else {
                    logMessage("警告: 设置点动速度参数失败");
                }
            } else {
                logMessage("警告: 设备通道启用失败");
            }
            
            // 启动状态更新定时器
            statusTimer->start(500); // 每500ms更新一次状态
            
            // 启动设备轮询以获取最新状态
            if (CC_StartPolling(currentSerialNo, 200)) { // 每200ms轮询一次
                logMessage("设备轮询已启动");
            } else {
                logMessage("警告: 设备轮询启动失败");
            }
            
            // 获取设备当前的速度和加速度参数，并转换为实际物理单位显示
            int currentVelocity, currentAcceleration;
            if (CC_GetVelParams(currentSerialNo, &currentAcceleration, &currentVelocity) == 0) {
                // 将设备单位转换为实际物理单位
                double realVelocity, realAcceleration;
                
                if (CC_GetRealValueFromDeviceUnit(currentSerialNo, currentVelocity, &realVelocity, UNIT_TYPE_VELOCITY) == 0) {
                    ui->velocitySpinBox->setValue(realVelocity);
                } else {
                    logMessage("警告: 速度单位转换失败，使用设备单位值");
                    ui->velocitySpinBox->setValue(currentVelocity);
                }
                
                if (CC_GetRealValueFromDeviceUnit(currentSerialNo, currentAcceleration, &realAcceleration, UNIT_TYPE_ACCELERATION) == 0) {
                    ui->accelerationSpinBox->setValue(realAcceleration);
                } else {
                    logMessage("警告: 加速度单位转换失败，使用设备单位值");
                    ui->accelerationSpinBox->setValue(currentAcceleration);
                }
                
                logMessage(QString("当前速度参数: %1 μm/s, 加速度参数: %2 μm/s²")
                          .arg(ui->velocitySpinBox->value(), 0, 'f', 2)
                          .arg(ui->accelerationSpinBox->value(), 0, 'f', 2));
            } else {
                logMessage("警告: 获取设备当前速度参数失败，使用默认值");
                
                // 使用默认值进行转换
                double realVelocity, realAcceleration;
                
                if (CC_GetRealValueFromDeviceUnit(currentSerialNo, DEFAULT_MAX_VELOCITY, &realVelocity, UNIT_TYPE_VELOCITY) == 0) {
                    ui->velocitySpinBox->setValue(realVelocity);
                } else {
                    ui->velocitySpinBox->setValue(DEFAULT_MAX_VELOCITY);
                }
                
                if (CC_GetRealValueFromDeviceUnit(currentSerialNo, DEFAULT_ACCELERATION, &realAcceleration, UNIT_TYPE_ACCELERATION) == 0) {
                    ui->accelerationSpinBox->setValue(realAcceleration);
                } else {
                    ui->accelerationSpinBox->setValue(DEFAULT_ACCELERATION);
                }
            }
            
            // 获取设备信息
            TLI_DeviceInfo deviceInfo;
            if (TLI_GetDeviceInfo(currentSerialNo, &deviceInfo) == 1) {
                QString info = QString("类型: %1, 描述: %2")
                    .arg(deviceInfo.typeID)
                    .arg(deviceInfo.description);
                logMessage(QString("设备信息已更新: %1").arg(info));
            } else {
                logMessage("警告: 获取设备信息失败");    
            }
            
            // 检查设备是否需要归位
            if (CC_NeedsHoming(currentSerialNo)) {
                logMessage("设备需要归位后才能移动");
            } else if (CC_CanMoveWithoutHomingFirst(currentSerialNo)) {
                logMessage("设备可以在不归位的情况下移动");
            } else {
                logMessage("设备需要归位后才能移动");
            }
        } else {
            // 连接失败
            QMessageBox::critical(this, "错误", "无法连接到设备，请检查序列号是否正确");
            logMessage(QString("连接设备失败: %1").arg(serialNo));
        }
    } else {
        // 断开设备连接
        // 停止电机
        if (CC_StopImmediate(currentSerialNo) != 0) {
            logMessage("警告: 停止电机失败");
        }
        
        // 禁用通道
        if (CC_DisableChannel(currentSerialNo) != 0) {
            logMessage("警告: 禁用通道失败");
        }
        
        // 停止设备轮询
        CC_StopPolling(currentSerialNo);
        
        // 关闭连接
        CC_Close(currentSerialNo);
        
        // 停止状态更新定时器
        statusTimer->stop();
        
        // 更新UI状态
        isConnected = false;
        updateConnectionStatus(false);
        logMessage("设备已断开连接");
    }
}

void MainWindow::on_refreshBtn_clicked()
{
    refreshDeviceList();
}

void MainWindow::on_homeBtn_clicked()
{
    if (!isConnected) return;
    
    // 检查设备是否支持归位
    if (!CC_CanHome(currentSerialNo)) {
        logMessage("设备不支持归位操作");
        return;
    }
    
    // 检查设备状态
    DWORD state = CC_GetStatusBits(currentSerialNo);
    if ((state & 0x00000010) || (state & 0x00000020) || (state & 0x00000040) || (state & 0x00000080) || (state & 0x00000200)) { 
        // 检查是否正在移动：顺时针移动、逆时针移动、顺时针点动、逆时针点动或正在归位
        logMessage("设备正在移动中，请稍后再试");
        return;
    }
    
    // 开始归位操作
    short result = CC_Home(currentSerialNo);
    if (result == 0) {
        logMessage("开始归位操作...");
        
        // 可以添加等待归位完成的逻辑
        // 这里只是启动归位操作，实际完成需要通过状态轮询或消息队列检测
    } else {
        logMessage(QString("归位操作失败，错误代码: %1").arg(result));
    }
}

void MainWindow::on_stopBtn_clicked()
{
    if (!isConnected) return;
    
    // 停止电机
    if (CC_StopImmediate(currentSerialNo) == 0) {
        logMessage("电机已停止");
    } else {
        logMessage("停止电机失败");
    }
}

void MainWindow::on_moveBtn_clicked()
{
    if (!isConnected) return;
    
    // 检查设备状态
    DWORD state = CC_GetStatusBits(currentSerialNo);
    if ((state & 0x00000010) || (state & 0x00000020) || (state & 0x00000040) || (state & 0x00000080) || (state & 0x00000200)) { 
        // 检查是否正在移动：顺时针移动、逆时针移动、顺时针点动、逆时针点动或正在归位
        logMessage("设备正在移动中，请稍后再试");
        return;
    }
    
    // 检查设备是否需要归位
    if (CC_NeedsHoming(currentSerialNo) && !(state & 0x00000400)) { // 检查是否已归位
        logMessage("设备需要先归位才能移动");
        return;
    }
    
    // 获取目标位置（微米）
    double targetUm = ui->positionSpinBox->value();
    int position = static_cast<int>(targetUm / POSITION_TO_UM); // 将微米转换为位置值
    
    // 检查位置是否在软件限位范围内
    int minPosition = static_cast<int>((MIN_POSITION_MM * 1000) / POSITION_TO_UM);
    int maxPosition = static_cast<int>((MAX_POSITION_MM * 1000) / POSITION_TO_UM);
    
    if (position < minPosition || position > maxPosition) {
        logMessage(QString("位置超出软件限位范围，有效范围: %1mm - %2mm").arg(MIN_POSITION_MM).arg(MAX_POSITION_MM));
        return;
    }
    
    // 移动到指定位置
    short result = CC_MoveToPosition(currentSerialNo, position);
    if (result == 0) {
        logMessage(QString("移动到位置: %1μm").arg(targetUm, 0, 'f', 1));
    } else {
        logMessage(QString("移动失败，错误代码: %1").arg(result));
    }
}

void MainWindow::on_setVelocityBtn_clicked()
{
    if (!isConnected) return;
    
    // 获取UI中的实际物理单位值
    double realVelocity = ui->velocitySpinBox->value();
    double realAcceleration = ui->accelerationSpinBox->value();
    
    // 将实际物理单位转换为设备单位
    int deviceVelocity, deviceAcceleration;
    
    if (CC_GetDeviceUnitFromRealValue(currentSerialNo, realVelocity, &deviceVelocity, UNIT_TYPE_VELOCITY) != 0) {
        logMessage("警告: 速度单位转换失败，使用默认值");
        deviceVelocity = DEFAULT_MAX_VELOCITY;
    }
    
    if (CC_GetDeviceUnitFromRealValue(currentSerialNo, realAcceleration, &deviceAcceleration, UNIT_TYPE_ACCELERATION) != 0) {
        logMessage("警告: 加速度单位转换失败，使用默认值");
        deviceAcceleration = DEFAULT_ACCELERATION;
    }
    
    // 设置速度参数（使用设备单位）
    if (CC_SetVelParams(currentSerialNo, deviceAcceleration, deviceVelocity) == 0) {
        logMessage(QString("速度参数已更新: 最大速度=%1 μm/s, 加速度=%3 μm/s²")
                  .arg(realVelocity, 0, 'f', 2).arg(deviceVelocity)
                  .arg(realAcceleration, 0, 'f', 2).arg(deviceAcceleration));
        
        // 同时更新点动速度参数，保持一致性
        if (CC_SetJogVelParams(currentSerialNo, deviceAcceleration, deviceVelocity) == 0) {
            logMessage("点动速度参数已同步更新");
        } else {
            logMessage("警告: 同步更新点动速度参数失败");
        }
    } else {
        logMessage("设置速度参数失败");
    }
}

void MainWindow::on_jogForwardBtn_clicked()
{
    if (!isConnected) return;
    
    // 获取点动步长（微米）
    double stepUm = ui->jogStepSpinBox->value();
    int stepSize = static_cast<int>(stepUm / POSITION_TO_UM); // 将微米转换为位置值
    
    // 获取当前位置
    int currentPosition = CC_GetPosition(currentSerialNo);
    
    // 计算移动后的位置
    int newPosition = currentPosition + stepSize;
    
    // 检查移动后的位置是否在软件限位范围内
    int maxPosition = static_cast<int>((MAX_POSITION_MM * 1000) / POSITION_TO_UM);
    
    if (newPosition > maxPosition) {
        logMessage(QString("正向点动会超出软件限位范围，最大位置: %1mm").arg(MAX_POSITION_MM));
        return;
    }
    
    // 正向点动
    if (CC_MoveRelative(currentSerialNo, stepSize) == 0) {
        logMessage(QString("正向点动: %1μm").arg(stepUm, 0, 'f', 1));
    } else {
        logMessage("正向点动失败");
    }
}

void MainWindow::on_jogBackwardBtn_clicked()
{
    if (!isConnected) return;
    
    // 获取点动步长（微米）
    double stepUm = ui->jogStepSpinBox->value();
    int stepSize = -static_cast<int>(stepUm / POSITION_TO_UM); // 将微米转换为位置值，取负值
    
    // 获取当前位置
    int currentPosition = CC_GetPosition(currentSerialNo);
    
    // 计算移动后的位置
    int newPosition = currentPosition + stepSize;
    
    // 检查移动后的位置是否在软件限位范围内
    int minPosition = static_cast<int>((MIN_POSITION_MM * 1000) / POSITION_TO_UM);
    
    if (newPosition < minPosition) {
        logMessage(QString("反向点动会超出软件限位范围，最小位置: %1mm").arg(MIN_POSITION_MM));
        return;
    }
    
    // 反向点动
    if (CC_MoveRelative(currentSerialNo, stepSize) == 0) {
        logMessage(QString("反向点动: %1μm").arg(stepUm, 0, 'f', 1));
    } else {
        logMessage("反向点动失败");
    }
}

void MainWindow::updateDeviceStatus()
{
    if (!isConnected) return;
    
    // 获取当前位置
    int position = CC_GetPosition(currentSerialNo);
    double distanceUm = position * POSITION_TO_UM; // 转换为微米
    ui->positionLabel->setText(QString("%1").arg(distanceUm, 0, 'f', 1));
    
    // 检查设备状态
    DWORD state = CC_GetStatusBits(currentSerialNo);
    
    // 解析状态位
    QString statusInfo = "状态: ";
    if (state & 0x00000010) {
        statusInfo += "顺时针移动 | ";
    }
    if (state & 0x00000020) {
        statusInfo += "逆时针移动 | ";
    }
    if (state & 0x00000040) {
        statusInfo += "顺时针点动 | ";
    }
    if (state & 0x00000080) {
        statusInfo += "逆时针点动 | ";
    }
    if (state & 0x00000200) {
        statusInfo += "正在归位 | ";
    }
    if (state & 0x00000400) {
        statusInfo += "已归位 | ";
    }
    if (state & 0x00000001) {
        statusInfo += "顺时针限位 | ";
    }
    if (state & 0x00000002) {
        statusInfo += "逆时针限位 | ";
    }
    if (state & 0x80000000) {
        statusInfo += "通道已启用 | ";
    } else {
        statusInfo += "通道已禁用 | ";
    }
    
    // 移除最后的分隔符
    if (statusInfo.endsWith(" | ")) {
        statusInfo = statusInfo.left(statusInfo.length() - 3);
    }
    
    // 更新状态标签（如果有）
    // ui->statusLabel->setText(statusInfo);
    
    // 将状态信息记录到日志
    static QString lastStatusInfo;
    if (statusInfo != lastStatusInfo) {
        logMessage(statusInfo);
        lastStatusInfo = statusInfo;
    }
}

void MainWindow::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->logTextEdit->appendPlainText(QString("[%1] %2").arg(timestamp).arg(message));
}

void MainWindow::updateConnectionStatus(bool connected)
{
    ui->connectionStatusLabel->setText(connected ? "已连接" : "未连接");
    ui->connectBtn->setText(connected ? "断开" : "连接");
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

void MainWindow::refreshDeviceList()
{
    // 刷新设备列表
    TLI_BuildDeviceList();
    
    // 获取设备数量
    short numDevices = TLI_GetDeviceListSize();
    
    // 清空下拉框
    ui->serialNoComboBox->clear();
    
    if (numDevices > 0) {
        logMessage(QString("发现 %1 个设备").arg(numDevices));
        
        // 获取设备列表
        char deviceList[1024]; // 假设缓冲区足够大
        if (TLI_GetDeviceListExt(deviceList, 1024) == 0) {
            // 设备列表是以逗号分隔的字符串
            QStringList devices = QString(deviceList).split(',');
            
            for (int i = 0; i < devices.size(); i++) {
                if (!devices[i].isEmpty()) {
                    // 获取设备信息
                    TLI_DeviceInfo deviceInfo;
                    if (TLI_GetDeviceInfo(devices[i].toStdString().c_str(), &deviceInfo) == 1) {
                        logMessage(QString("设备 %1: 序列号=%2, 类型=%3, 描述=%4")
                            .arg(i + 1)
                            .arg(devices[i])
                            .arg(deviceInfo.typeID)
                            .arg(deviceInfo.description));
                        
                        // 将设备序列号添加到下拉框中
                        ui->serialNoComboBox->addItem(devices[i]);
                    }
                }
            }
            
            // 如果有设备，默认选择第一个
            if (ui->serialNoComboBox->count() > 0) {
                ui->serialNoComboBox->setCurrentIndex(0);
            }
        }
    } else {
        logMessage("未发现任何设备");
        QMessageBox::information(this, "信息", "未发现任何KDC101设备");
    }
}
