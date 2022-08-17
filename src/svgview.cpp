/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "svgview.h"

#include <QSvgRenderer>

#include <QWheelEvent>
#include <QMouseEvent>
#include <QGraphicsRectItem>
#include <QGraphicsSvgItem>
#include <QPaintEvent>
#include <qmath.h>

#ifdef USE_OPENGLWIDGETS
#include <QtOpenGLWidgets/qopenglwidget.h>
#endif

SvgView::SvgView(QWidget *parent) : QGraphicsView(parent),
    m_renderer(Native),
    m_svgItem(nullptr),
    m_backgroundItem(nullptr),
    m_outlineItem(nullptr)
{
    setScene(new QGraphicsScene(this));
    setTransformationAnchor(AnchorViewCenter);
    setResizeAnchor(AnchorViewCenter);
    setDragMode(ScrollHandDrag);
    setViewportUpdateMode(FullViewportUpdate);

    //setViewportMargins(-2, -2, -2, -2); I added this
    setViewportMargins(-1, -1, 0, 0);
    //setFrameStyle(QFrame::NoFrame);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Prepare background check-board pattern
    /**
    QPixmap tilePixmap(32, 32);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(220, 220, 220);
    tilePainter.fillRect(0, 0, 16, 16, color);
    tilePainter.fillRect(16, 16, 16, 16, color);
    tilePainter.end();

    setBackgroundBrush(tilePixmap);
    **/

}

void SvgView::drawBackground(QPainter *p, const QRectF &)
{
    p->save();
    p->resetTransform();
    //p->fillRect(viewport()->rect(), QBrush(Qt::gray, Qt::SolidPattern));
    //p->drawTiledPixmap(viewport()->rect(), backgroundBrush().texture());
    p->restore();
}

QSize SvgView::svgSize() const
{
    return m_svgItem ? m_svgItem->boundingRect().size().toSize() : QSize();
}


void SvgView::setup(double xSize, double ySize)
{
    mXSize = xSize;
    mYSize = ySize;
    scene()->clear();
    resetTransform();

    scene()->setSceneRect(0, 0, mXSize, mYSize);
    qreal scaleFactor = bordered_min_scale_factor();
    scale(scaleFactor, scaleFactor);
    centerOn(sceneRect().center()); // Center view

    add_print_bed_outline();
}

void SvgView::add_print_bed_outline()
{
    scene()->addRect(0,0, mXSize, mYSize, Qt::NoPen, QBrush(Qt::gray, Qt::SolidPattern));
    // add print bed area outlines
    scene()->addLine(0,0, mXSize,0, outlinePen);
    scene()->addLine(mXSize,0, mXSize,mYSize, outlinePen);
    scene()->addLine(mXSize,mYSize, 0,mYSize, outlinePen);
    scene()->addLine(0,mYSize, 0,0, outlinePen);
}

void SvgView::clear_lines()
{
    scene()->clear();
    add_print_bed_outline();
}

bool SvgView::openFile(const QString &fileName)
{
    QGraphicsScene *s = scene();

    const bool drawBackground = (m_backgroundItem ? m_backgroundItem->isVisible() : false);
    const bool drawOutline = (m_outlineItem ? m_outlineItem->isVisible() : true);

    QScopedPointer<QGraphicsSvgItem> svgItem(new QGraphicsSvgItem(fileName));
    if (!svgItem->renderer()->isValid()) return false;

    s->clear();
    resetTransform();

    m_svgItem = svgItem.take();
    m_svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
    m_svgItem->setCacheMode(QGraphicsItem::NoCache);
    m_svgItem->setZValue(0);

    m_backgroundItem = new QGraphicsRectItem(m_svgItem->boundingRect());
    m_backgroundItem->setBrush(Qt::white);
    m_backgroundItem->setPen(Qt::NoPen);
    m_backgroundItem->setVisible(drawBackground);
    m_backgroundItem->setZValue(-1);

    m_outlineItem = new QGraphicsRectItem(m_svgItem->boundingRect());
    QPen outline(Qt::black, 2, Qt::DashLine);
    outline.setCosmetic(true);
    m_outlineItem->setPen(outline);
    m_outlineItem->setBrush(Qt::NoBrush);
    m_outlineItem->setVisible(drawOutline);
    m_outlineItem->setZValue(1);

    s->addItem(m_backgroundItem);
    s->addItem(m_svgItem);
    s->addItem(m_outlineItem);

    s->setSceneRect(m_svgItem->boundingRect());

    this->centerOn(m_svgItem); // Center on SVG

    qreal scaleFactor = this->size().height() / m_svgItem->boundingRect().size().height();
    scale(scaleFactor, scaleFactor);

    return true;
}



void SvgView::setRenderer(RendererType type)
{
    m_renderer = type;

    if (m_renderer == OpenGL)
    {
#ifdef USE_OPENGLWIDGETS
        setViewport(new QOpenGLWidget);
#endif
    }
    else
    {
        setViewport(new QWidget);
    }
}

void SvgView::setAntialiasing(bool antialiasing)
{
    setRenderHint(QPainter::Antialiasing, antialiasing);
}

void SvgView::setViewBackground(bool enable)
{
    if (!m_backgroundItem) return;
    m_backgroundItem->setVisible(enable);
}

void SvgView::setViewOutline(bool enable)
{
    if (!m_outlineItem) return;
    m_outlineItem->setVisible(enable);
}

qreal SvgView::zoomFactor() const
{
    return transform().m11();
}

void SvgView::zoomIn()
{
    zoomBy(2);
}

void SvgView::zoomOut()
{
    zoomBy(0.5);
}

void SvgView::resetZoom()
{
    qreal scaleFactor = bordered_min_scale_factor();
    if (!qFuzzyCompare(zoomFactor(), scaleFactor)) // if not already the same
    {
        resetTransform();
        scale(scaleFactor, scaleFactor);
        emit zoomChanged();
        centerOn(sceneRect().center()); // Center view
    }
}

void SvgView::paintEvent(QPaintEvent *event)
{
    if (m_renderer == Image)
    {
        if (m_image.size() != viewport()->size())
        {
            m_image = QImage(viewport()->size(), QImage::Format_ARGB32_Premultiplied);
        }

        QPainter imagePainter(&m_image);
        QGraphicsView::render(&imagePainter);
        imagePainter.end();

        QPainter p(viewport());
        p.drawImage(0, 0, m_image);

    }
    else
    {
        QGraphicsView::paintEvent(event);
    }
}

void SvgView::wheelEvent(QWheelEvent *event)
{
    zoomBy(qPow(1.2, event->angleDelta().y() / 240.0));
}

void SvgView::zoomBy(qreal factor)
{
    qreal scaleFactor = min_scale_factor();
    const qreal currentZoom = zoomFactor();
    if ((factor < 1 && currentZoom < scaleFactor) || (factor > 1 && currentZoom > (scaleFactor * 10))) // Zoom Limits
    {
        //return; // DO NOTHING HERE?
    }
    scale(factor, factor);
    if (zoomFactor() < bordered_min_scale_factor()) // I added this
    {
     resetZoom();
    } // end add
    emit zoomChanged();
}

QSvgRenderer *SvgView::renderer() const
{
    if (m_svgItem) return m_svgItem->renderer();
    else return nullptr;
}

qreal SvgView::min_scale_factor()
{
    qreal scaleFactorX = size().width()  / sceneRect().width();
    qreal scaleFactorY = size().height() / sceneRect().height();
    return scaleFactorX < scaleFactorY ? scaleFactorX : scaleFactorY;
}

qreal SvgView::bordered_min_scale_factor()
{
    qreal scaleFactorX = size().width()  / (sceneRect().width() + 0.25);
    qreal scaleFactorY = size().height() / (sceneRect().height() + 0.25);
    return scaleFactorX < scaleFactorY ? scaleFactorX : scaleFactorY;
}
