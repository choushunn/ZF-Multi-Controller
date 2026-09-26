#include "previewview.h"

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QFont>
#include <QResizeEvent>
#include <QFontMetrics>
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

constexpr int kTickLen = 6;     // 刻度线长度（屏幕像素）
constexpr double kTargetPx = 60.0;  // 期望的相邻刻度屏幕间距

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
    viewport()->update();
}

void PreviewView::clearFrame()
{
    m_item->setVisible(false);
    m_frameSize = QSize();
    viewport()->update();
}

QSize PreviewView::frameSize() const
{
    return m_frameSize;
}

void PreviewView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    fitImage();
}

void PreviewView::fitImage()
{
    if (!m_item->isVisible() || m_item->pixmap().isNull())
        return;
    fitInView(m_item, Qt::KeepAspectRatio);
    viewport()->update();
}

void PreviewView::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent(event);
    if (!m_item->isVisible() || m_item->pixmap().isNull())
        return;
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);
    drawRulers(painter);
}

void PreviewView::drawRulers(QPainter &painter)
{
    // 图像在屏幕上占据的矩形（与图像像素一一对应）
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

    const QPen linePen(QColor(0, 0, 0, 160), 1);
    const QPen outlinePen(Qt::black);
    const QPen textPen(Qt::white);
    painter.setFont(QFont(painter.font().family(), 7));

    // 顶部刻度（X 像素坐标）
    for (double x = stepX; x < sceneRect.width(); x += stepX) {
        const double sx = screen.left() + x * scaleX;
        painter.setPen(linePen);
        painter.drawLine(QPointF(sx, screen.top() + 1), QPointF(sx, screen.top() + kTickLen));
        painter.drawLine(QPointF(sx, screen.top() + screen.height()),
                         QPointF(sx, screen.top() + screen.height() - kTickLen));
        const QString label = QString::number(static_cast<int>(std::llround(x)));
        // 文字描边两次（黑底白字）增强对比
        painter.setPen(outlinePen);
        painter.drawText(QPointF(sx + 3, screen.top() + kTickLen + 8), label);
        painter.setPen(textPen);
        painter.drawText(QPointF(sx + 2, screen.top() + kTickLen + 7), label);
    }

    // 左侧刻度（Y 像素坐标）
    for (double y = stepY; y < sceneRect.height(); y += stepY) {
        const double sy = screen.top() + y * scaleY;
        painter.setPen(linePen);
        painter.drawLine(QPointF(screen.left() + 1, sy), QPointF(screen.left() + kTickLen, sy));
        painter.drawLine(QPointF(screen.left() + screen.width(), sy),
                         QPointF(screen.left() + screen.width() - kTickLen, sy));
        const QString label = QString::number(static_cast<int>(std::llround(y)));
        painter.setPen(outlinePen);
        painter.drawText(QPointF(screen.left() + kTickLen + 8, sy), label);
        painter.setPen(textPen);
        painter.drawText(QPointF(screen.left() + kTickLen + 7, sy - 1), label);
    }
}