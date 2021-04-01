#include "videoplayer.h"
#include "ui_videoplayer.h"

#include <QFileDialog>

VideoPlayer::VideoPlayer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VideoPlayer)
{
    ui->setupUi(this);

    m_pPlayer = new QMediaPlayer;
    m_pPlayerWidget = new QVideoWidget;
    m_pPlayer->setVideoOutput(m_pPlayerWidget);
    m_pPlayerWidget->setAutoFillBackground(true);
    ui->verticalLayout->addWidget(m_pPlayerWidget);

    QPalette qplte;
    qplte.setColor(QPalette::Window, QColor(0,0,0));
    m_pPlayerWidget->setPalette(qplte);

    //载入
    connect(ui->BtnLoad, SIGNAL(clicked()), this, SLOT(OnSetMediaFile()));
    //播放
    connect(ui->BtnPlay, SIGNAL(clicked()), m_pPlayer, SLOT(play()));
    //暂停
    connect(ui->BtnPause, SIGNAL(clicked()), m_pPlayer, SLOT(pause()));
    //停止
    connect(ui->BtnStop, SIGNAL(clicked()), m_pPlayer, SLOT(stop()));
    connect(m_pPlayer, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(OnStateChanged(QMediaPlayer::State)));

    ui->BtnStop->setEnabled(false);
    //设置滑块行为
    m_bReLoad = true;
    ui->slider->setEnabled(false);
    connect(m_pPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(OnSlider(qint64)));
    connect(m_pPlayer, SIGNAL(durationChanged(qint64)), this, SLOT(OnDurationChanged(qint64)));
    connect(ui->slider, SIGNAL(sigProgress(qint64)), m_pPlayer, SLOT(setPosition(qint64)));
}

VideoPlayer::~VideoPlayer()
{
    delete m_pPlayer;
    delete m_pPlayerWidget;
    delete ui;
}

void VideoPlayer::OnSetMediaFile(void)
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setDirectory(m_VideoDir);
    QStringList fileNames;
    if (dialog.exec())
        fileNames = dialog.selectedFiles();

    if(!fileNames.empty())
    {
        m_pPlayer->setMedia(QUrl::fromLocalFile(fileNames[0]));
        m_bReLoad = true;
        ui->slider->setValue(0);
    }
}

void VideoPlayer::OnSlider(qint64 i64Pos)
{
    ui->slider->setProgress(i64Pos);
}

void VideoPlayer::OnDurationChanged(qint64 i64Duration)
{
    if(i64Duration > 0 && m_bReLoad)
    {
        ui->slider->setRange(0, i64Duration);
        m_bReLoad = false;
    }
}

void VideoPlayer::OnStateChanged(QMediaPlayer::State enumState)
{
    if(QMediaPlayer::StoppedState == enumState)
    {
        ui->BtnPlay->setEnabled(true);
        ui->BtnStop->setEnabled(false);
        ui->slider->setEnabled(false);
    }
    else if(QMediaPlayer::PlayingState == enumState)
    {
        ui->BtnPlay->setEnabled(false);
        ui->BtnStop->setEnabled(true);
        ui->slider->setEnabled(true);
    }
}
