#ifndef IMAGEACQUISITION_H
#define IMAGEACQUISITION_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QMessageBox>
#include "DVPCamera.h"

#define m_uGrabTimeout   3000                // 图像获取的超时时间 ms

class QImageAcquisition : public QThread
{
    Q_OBJECT
public:
    explicit QImageAcquisition(dvpHandle &handle,QObject *parent = 0);
    ~QImageAcquisition();

    void  run();
    bool IsValidHandle(dvpHandle handle);    // 判断句柄是否有效
    dvpHandle    m_handle;
    QPixmap      m_ShowImage;                // 显示图像对象
    QMutex       m_threadMutex;              // 互斥量
    bool         m_bAcquireImg;              // 采集线程是否结束的标志：true 运行；false 退出
    bool         ThreadSoftTriggerFlag;      // 软触发标志

    dvpFrame     m_pFrame;                   // 采集到的图像的结构体指针
    void *       pBuffer;                    // 采集到的图像的内存首地址
private:


signals:
    void         dispSignal();               // 显示图像信号

};

#endif // IMAGEACQUISITION_H

