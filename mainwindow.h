#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "videoprocess.h"
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void slotUpdateImage(Mat srcImage);

    void slotRecoderStateMsg(QString text);
private slots:
    void on_btnStartStop_clicked();

private:
    Ui::MainWindow *ui;
    VideoProcess *m_pVideoProcess;
    bool m_Started;
    QLabel *m_pRecoderState;
};
#endif // MAINWINDOW_H
