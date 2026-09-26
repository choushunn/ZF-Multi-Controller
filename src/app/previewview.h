#pragma once

#include <QGraphicsView>

class QGraphicsScene;
class QGraphicsPixmapItem;

// 摄像头预览视图：图像自适应居中显示，顶部/左侧叠加像素刻度尺。
// 无帧时显示默认灰色背景；有帧时按当前缩放比例自动选择整齐刻度步长。
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
    void paintEvent(QPaintEvent *event) override;

private:
    void fitImage();
    void drawRulers(QPainter &painter);

    QGraphicsScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_item = nullptr;
    QSize m_frameSize;
};