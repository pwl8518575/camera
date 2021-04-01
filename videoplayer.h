#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QtMultimedia>
#include <QVideoWidget>

namespace Ui {
class VideoPlayer;
}

class VideoPlayer : public QWidget
{
    Q_OBJECT
public:

    explicit VideoPlayer(QWidget *parent = 0);
    ~VideoPlayer();
    bool            m_bReLoad;
    QString         m_VideoDir;
public slots:
    void            OnSetMediaFile(void);
    void            OnSlider(qint64);
    void            OnDurationChanged(qint64);
    void            OnStateChanged(QMediaPlayer::State);
private:
    QVideoWidget    *       m_pPlayerWidget;
    QMediaPlayer    *       m_pPlayer;
    Ui::VideoPlayer *ui;
};

#endif // VIDEOPLAYER_H
