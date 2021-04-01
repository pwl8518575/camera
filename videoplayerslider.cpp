#include "videoplayerslider.h"

VideoPlayerSlider::VideoPlayerSlider(QWidget * parent) : QSlider(parent)
{
    m_bPressed = false;
}

void VideoPlayerSlider::mousePressEvent(QMouseEvent *e)
{
    m_bPressed = true;
    QSlider::mousePressEvent(e);//必须有这句，否则手动不能移动滑块
}

void VideoPlayerSlider::mouseMoveEvent(QMouseEvent *e)
{
    QSlider::mouseMoveEvent(e);//必须有这句，否则手动不能移动滑块
}

void VideoPlayerSlider::mouseReleaseEvent(QMouseEvent *e)
{
    m_bPressed = false;
    qint64 i64Pos = value();
    emit sigProgress(i64Pos);
    QSlider::mouseReleaseEvent(e);//必须有这句，否则手动不能移动滑块
}

void VideoPlayerSlider::setProgress(qint64 i64Progress)
{
    if(!m_bPressed)
        setValue((int)i64Progress);
}
