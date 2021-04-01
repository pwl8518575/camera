#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QImage>
#include <QMessageBox>
#include <QHBoxLayout>
#include "videoplayer.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_pVideoProcess = new VideoProcess();
    ui->cbVideoDevList->addItems(m_pVideoProcess->VideoGeDevices());
    connect(m_pVideoProcess,SIGNAL(sigVideoNewFrame(Mat)),this,SLOT(slotUpdateImage(Mat)));

    m_pRecoderState = new QLabel("STOP");
    ui->statusbar->addWidget(new QLabel("Recoder State:"));
    ui->statusbar->addWidget(m_pRecoderState);

    connect(m_pVideoProcess,SIGNAL(sigRecoderMsg(QString)),this,SLOT(slotRecoderStateMsg(QString)));

    ui->videoPlayerWidget->m_VideoDir = ui->leVideoDir->text();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slotRecoderStateMsg(QString text)
{
    m_pRecoderState->setText(text);
}

void MainWindow::slotUpdateImage(Mat srcImage)
{
    //显示方法二
    QImage image2 = QImage(srcImage.data, srcImage.cols, srcImage.rows, QImage::Format_RGB888);
    ui->imageLabel->setPixmap(QPixmap::fromImage(image2));
    ui->imageLabel->resize(image2.size());
    ui->imageLabel->show();
}

void MainWindow::on_btnStartStop_clicked()
{
    int ret = 0;
    if(ui->cbVideoDevList->count() <= 0 || ui->cbVideoDevList->currentIndex() < 0)
    {
        QMessageBox::information(this, "Device", "No available Camera");
        return ;
    }
    ui->videoPlayerWidget->m_VideoDir = ui->leVideoDir->text();
    if(ui->btnStartStop->text() == "START")
    {
        ret = m_pVideoProcess->VideoStart(ui->cbVideoDevList->currentIndex(),ui->lePeriod->text().toInt(),
                                    ui->leVideoDir->text(),ui->leRtmpUrl->text());
        if(ret == 0)
        {
            ui->btnStartStop->setText("STOP");
            ui->leVideoDir->setEnabled(false);
            ui->leRtmpUrl->setEnabled(false);
            ui->lePeriod->setEnabled(false);
        }
    }
    else
    {
        m_pVideoProcess->VideoStop();
        ui->btnStartStop->setText("START");
        ui->leVideoDir->setEnabled(true);
        ui->leRtmpUrl->setEnabled(true);
        ui->lePeriod->setEnabled(true);
    }
}
