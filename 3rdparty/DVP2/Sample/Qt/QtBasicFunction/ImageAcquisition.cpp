#include "ImageAcquisition.h"
#include "DVPCamera.h"

QImageAcquisition::QImageAcquisition(dvpHandle &handle,QObject *parent) :
    QThread(parent)
{
    // 初始化成员变量
    m_handle = handle;
    m_bAcquireImg = false;
    pBuffer = NULL;
}

QImageAcquisition::~QImageAcquisition()
{

}

bool QImageAcquisition::IsValidHandle(dvpHandle handle)
{
    bool bValidHandle = false;
    dvpIsValid(handle, &bValidHandle);

    return bValidHandle;
}

void QImageAcquisition::run()
{
    dvpStatus status;

    while(1)
    {
        if (m_bAcquireImg == false)
        {
            qDebug("quit thread!");
            break;
        }

        status = dvpGetFrame(m_handle, &m_pFrame, &pBuffer, m_uGrabTimeout);

        if (status == DVP_STATUS_OK)
        {
            //这里将采集图像、图像转换放置在工作线程中实现，解决主界面在高帧率显示时卡顿问题
            if(m_pFrame.format==FORMAT_BGR24)
            {
                m_threadMutex.lock();
                m_ShowImage = QPixmap::fromImage(QImage((uchar*)pBuffer,m_pFrame.iWidth, m_pFrame.iHeight,m_pFrame.iWidth*3, QImage::Format_BGR888,0,0));
                m_threadMutex.unlock();
                emit dispSignal();
            }
            /*
            else if(m_pFrame.format==FORMAT_MONO)
            {
                m_threadMutex.lock();
                m_ShowImage = QPixmap::fromImage(QImage((uchar*)pBuffer,m_pFrame.iWidth, m_pFrame.iHeight,m_pFrame.iWidth, QImage::Format_Mono,0,0));
                m_threadMutex.unlock();
                emit dispSignal();
            }*/
            else
            {
                //如果不是BGR24格式就显示一张自定义图片
                m_threadMutex.lock();
                QPixmap clearPix = QPixmap("../QtBasicFunction/warning.png");
                m_ShowImage = clearPix;
                m_threadMutex.unlock();
                emit dispSignal();
            }
            //【注：】案例暂不支持其他目标格式（MONO，RAW）的显示
        }
    }
}


























