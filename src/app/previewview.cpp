#include "previewview.h"

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QWidget>
#include <cmath>

namespace {

// 取 1/2/5 × 10^n 的“整齐”步长，使刻度间距接近期望值
double niceStep(double raw)
{
    if (raw <= 0)
        return 1.0;
    const double mag = std::pow(10.0, std::floor(std::log10(raw)));
    const double norm = raw / mag;
    const double nice = (norm < 1.5) ? 1.0 : (norm < 3.5) ? 2.0 : (norm < 7.5) ? 5.0 : 10.0;
    return nice * mag;
}

constexpr int kTickLen = 6;         // 刻度线长度（屏幕像素）
constexpr double kTargetPx = 60.0;  // 期望的相邻刻度屏幕间距
constexpr int kPadX = 48;           // 左侧刻度区宽度（容纳刻度线+数字）
constexpr int kPadY = 22;           // 顶部刻度区高度（容纳刻度线+数字）

} // namespace

namespace {

// 刻度尺图层：独立透明控件，绘制与图像互不影响
class RulerLayer : public QWidget
{
public:
    explicit RulerLayer(PreviewView *owner, QWidget *parent)
        : QWidget(parent)
        , m_owner(owner)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);  // 鼠标穿透到下层预览
        setAttribute(Qt::WA_NoSystemBackground);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        m_owner->drawRulers(p);
    }

private:
    PreviewView *m_owner;
};

} // namespace

PreviewView::PreviewView(QWidget *parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    m_item = m_scene->addPixmap(QPixmap());
    m_item->setVisible(false);

    setBackgroundBrush(QColor(QStringLiteral("#ababab")));
    setRenderHint(QPainter::SmoothPixmapTransform);
    // 关闭滚动条：始终适配视口
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 刻度尺图层悬浮于预览之上
    m_ruler = new RulerLayer(this, this);
    m_ruler->setGeometry(rect());
    m_ruler->raise();
}

void PreviewView::showFrame(const QImage &frame)
{
    if (frame.isNull())
        return;
    const QPixmap pix = QPixmap::fromImage(frame);
    m_item->setPixmap(pix);
    m_item->setVisible(true);
    m_frameSize = frame.size();
    fitImage();
    m_ruler->update();
}

void PreviewView::clearFrame()
{
    m_item->setVisible(false);
    m_frameSize = QSize();
    m_ruler->update();
}

QSize PreviewView::frameSize() const
{
    return m_frameSize;
}

void PreviewView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    // 刻度尺图层与预览视口同步几何（图层坐标与视口坐标一致）
    m_ruler->setGeometry(rect());
    m_ruler->raise();
    fitImage();
}

void PreviewView::fitImage()
{
    if (!m_item->isVisible() || m_item->pixmap().isNull())
        return;
    const QSize vp = viewport()->size();
    const QSizeF img = m_item->pixmap().size();
    if (vp.width() <= 0 || vp.height() <= 0 || img.isNull())
        return;
    // 可用区域：四周留出对称刻度带（左右 kPadX、上下 kPadY）
    const qreal sx = (vp.width() - 2.0 * kPadX) / img.width();
    const qreal sy = (vp.height() - 2.0 * kPadY) / img.height();
    const qreal s = qMin(sx, sy);
    if (s <= 0)
        return;
    QTransform t;
    t.translate(kPadX, kPadY);  // 图像左上角位于刻度带内侧
    t.scale(s, s);
    setTransform(t);
    m_ruler->update();
}

void PreviewView::drawRulers(QPainter &painter)
{
    if (!m_item->isVisible() || m_item->pixmap().isNull())
        return;
    // 图像在屏幕上占据的矩形（位于刻度带内侧，与图像像素一一对应）
    const QRectF sceneRect = m_item->sceneBoundingRect();
    QRectF screen = QRectF(mapFromScene(sceneRect.topLeft()), mapFromScene(sceneRect.bottomRight()))
                        .normalized();
    const double scaleX = screen.width() / sceneRect.width();
    const double scaleY = screen.height() / sceneRect.height();
    if (scaleX <= 0 || scaleY <= 0)
        return;

    // 刻度步长（图像像素），使屏幕间距约 kTargetPx
    const double stepX = niceStep(kTargetPx / scaleX);
    const double stepY = niceStep(kTargetPx / scaleY);

    const QPen linePen(QColor(60, 60, 60), 1);
    painter.setFont(QFont(painter.font().family(), 8));
    const QFontMetrics fm(painter.font());

    // 顶部刻度（X 像素坐标，短线固定绘制在顶部带内，包含 0 刻度）
    for (double x = 0; x <= sceneRect.width(); x += stepX) {
        const double sx = screen.left() + x * scaleX;
        painter.setPen(linePen);
        painter.drawLine(QPointF(sx, kPadY - kTickLen), QPointF(sx, kPadY - 1));
        const QString label = QString::number(static_cast<int>(std::llround(x)));
        const QRectF textRect(sx - 30, 0, 60, kPadY - kTickLen - 3);
        painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, label);
    }

    // 左侧刻度（Y 像素坐标，短线固定绘制在左侧带内，包含 0 刻度）
    for (double y = 0; y <= sceneRect.height(); y += stepY) {
        const double sy = screen.top() + y * scaleY;
        painter.setPen(linePen);
        painter.drawLine(QPointF(kPadX - kTickLen, sy), QPointF(kPadX - 1, sy));
        const QString label = QString::number(static_cast<int>(std::llround(y)));
        const QRectF textRect(0, sy - fm.height() / 2.0,
                              kPadX - kTickLen - 3, fm.height());
        painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
    }
}