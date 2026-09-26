#pragma once

#include <QGraphicsView>

class QGraphicsScene;
class QGraphicsPixmapItem;

// 摄像头预览容器：图像图层（本类，自适应显示图像）与刻度尺图层
// （内部 RulerLayer 透明控件）完全分离，互不影响。
// 图像 fit 到四周留出刻度带的内区；有帧时按当前缩放比例自动选择整齐刻度步长。
class PreviewView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit PreviewView(QWidget *parent = nullptr);

    // 更新预览帧（内部缓存，图像尺寸可随后续帧变化）
    void showFrame(const QImage &frame);
    // 清空预览，恢复灰色背景
    void clearFrame();
    // 当前帧尺寸（像素），无帧返回空
    QSize frameSize() const;

protected:
    void resizeEvent(QResizeEvent *event) override;

public:
    // 在刻度尺图层上绘制外围刻度（图层坐标 = 视口坐标）；供内部刻度图层调用
    void drawRulers(QPainter &painter);

private:
    void fitImage();

    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_item = nullptr;
    QSize m_frameSize;
    QWidget *m_ruler = nullptr;  // 刻度尺图层（透明、穿透鼠标事件）
};