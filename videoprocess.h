#ifndef VIDEOWRITER_H
#define VIDEOWRITER_H

#include <QObject>
#include <QThread>
#include </usr/local/Cellar/opencv/4.5.0/include/opencv4/opencv2/opencv.hpp>
#include <iostream>
#include <stdio.h>
//#include <linux/videodev2.h>

#include<QCamera>
#include<QCameraViewfinder>
#include<QCameraImageCapture>

#include<QCameraCaptureBufferFormatControl>
#include <QDateTime>

using namespace cv;
using namespace std;

//#ifdef __cplusplus
extern "C"
{
//#endif
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/mem.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/eval.h>
    #include <libavutil/parseutils.h>
    #include <libavfilter/avfilter.h>
    #include <libavutil/opt.h>
//#ifdef __cplusplus
};
//#endif

class VideoProcess : public QThread
{
    Q_OBJECT
public:
    explicit VideoProcess(QObject *parent = 0);
    ~VideoProcess();
    QList<QString> VideoGeDevices(void);
    int VideoStart(int index,int silencePeriod,QString videoDir,QString rtmp);
    int VideoStop();

private:
    int VideoDeviceList();
    int VideoInit(int deviceIndex);
    int VideoDeInit();
    int VideoFileOpen(string filename,double fps,Mat &frame);
    int VideoFileWrite(Mat &frame);
    int VideoFileClose();
    bool ContentChangeDetection(Mat &frame);
    bool VideoChangeDetection(Mat &frame);
    int VideoRtmpInit();
    int VideoRtmpRun(Mat &frame);
protected:
    //QThread的虚函数
    //线程处理函数
    //不能直接调用，通过start()间接调用
    void run();

signals:
    void sigVideoNewFrame(Mat frame);
    void sigRecoderMsg(QString test);

public slots:

private:
    VideoCapture *m_VideoCapture = nullptr;
    VideoWriter *m_pVideoWriter = nullptr;
    bool m_RunFlag = false;
    bool m_RecoderRun = false;
    int m_silencePeriod = 10;
    bool m_rtmpReady = false;
    QString m_VideoDir;
    QString m_rtmpUrl;
    Mat m_lastVideoframe;
    int       m_NoChangeTicks = 0;
    QDateTime m_lastDetectionTime;
    QDateTime m_lastVideoChangeTime;
    QList<struct v4l2_capability> m_VideoDevices;
    QCamera *camera;//系统摄像头
    QCameraViewfinder *cameraViewFinder;//摄像头取景器部件
    QCameraImageCapture *cameraImageCapture;//截图部件

    //rtmp
    //像素格式转换上下文
    SwsContext *m_pVsc = nullptr;
    //输出的数据结构
    AVFrame *m_pYuv = nullptr;
    //编码器上下文
    AVCodecContext *m_pVc = nullptr;
    //rtmp flv 封装器
    AVFormatContext *m_pIc = nullptr;
    //视频流
    AVStream *m_pVs = nullptr;

    AVPacket m_Pack;
    int m_Vpts = 0;
    int aHash(Mat matSrc1, Mat matSrc2);
    int pHash(Mat matSrc1, Mat matSrc2);
    double getMSSIM(const Mat &i1, const Mat &i2);
    double diffentpixelHash(Mat &matSrc1, Mat &matSrc2);
};

#endif // VIDEOWRITER_H
