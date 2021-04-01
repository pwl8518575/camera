#include "videoprocess.h"
#include <QDebug>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <QDateTime>
#include <sys/ioctl.h>
//#include <linux/videodev2.h>
#include <stddef.h>
#include <chrono>
#include <stdint.h>

VideoProcess::VideoProcess(QObject *parent):QThread(parent)
{
    VideoDeviceList();
    qRegisterMetaType<Mat>("Mat");
}                    

VideoProcess::~VideoProcess()
{
    VideoDeInit();
    VideoFileClose();
}

QList<QString> VideoProcess::VideoGeDevices()
{
    QList<QString> devices;
    for(int i = 0; i < m_VideoDevices.count();i++)
    {
        devices.append(QString::fromUtf8((char *)m_VideoDevices[i].card) + QString::fromUtf8((char *)m_VideoDevices[i].driver));
    }
    return devices;
}

int VideoProcess::VideoDeviceList(void)
{
    int fd,i;
    char deviceName[256];
    //struct v4l2_capability video_cap;

    m_VideoDevices.clear();
    for(i=0;;i++)
    {
        sprintf(deviceName,"/dev/video%d",i);
        if((fd = open(deviceName, O_RDONLY)) == -1){
            break;
        }
//        if(ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == -1)
//        {
//            qDebug() << "cam_info: Can't get capabilities";
//        }
//        m_VideoDevices.append(video_cap);
    }
    close(fd);
    return 0;
}

int VideoProcess::VideoInit(int deviceIndex)
{
    // use default camera as video source
    m_VideoCapture = new VideoCapture(deviceIndex,CAP_V4L);

    // check if we succeeded 检测  是否能打开摄像头和 外接设备
    if (!m_VideoCapture->isOpened()) {
        qDebug() << "ERROR! Unable to open camera\n";
        return -1;
    }
    //设置摄像头的参数
    //m_VideoCapture->set(CAP_PROP_FPS,30.0);
    return 0;
}

int VideoProcess::VideoDeInit(void)
{
    if(m_VideoCapture != nullptr)
    {
        m_VideoCapture->release();
        delete m_VideoCapture;
    }
    m_VideoCapture = nullptr;
    return 0;
}

int VideoProcess::VideoFileOpen(string filename,double fps,Mat &frame)
{
    //--- EINITIALIZ VIDEOWRITER
    m_pVideoWriter = new VideoWriter();
    bool isColor = (frame.type() == CV_8UC3);
    int codec = VideoWriter::fourcc('M', 'P', '4', '2');  // select desired codec (must be available at runtime)
    //double fps = 25.0;                          // framerate of the created video stream
    //string filename = "./live.avi";             // name of the output video file
    m_pVideoWriter->open(filename, codec, fps, frame.size(), isColor);
    // check if we succeeded
    if (!m_pVideoWriter->isOpened()) {
        qDebug() << "Could not open the output video file for write\n";
        return -1;
    }
    m_RecoderRun = true;
    return 0;
}

int VideoProcess::VideoFileWrite(Mat &frame)
{
    // encode the frame into the videofile stream
    m_pVideoWriter->write(frame);
    return 0;
}

int VideoProcess::VideoFileClose(void)
{
    if(m_pVideoWriter != nullptr)
    {
        // 新加  并行判断
        emit sigRecoderMsg("STOP");

        m_pVideoWriter->release();
        delete m_pVideoWriter;
    }
    m_pVideoWriter = nullptr;
    m_RecoderRun = false;
    return 0;
}

int VideoProcess::VideoStart(int videoindex,int silencePeriod,QString videoDir,QString rtmp)
{
    int ret = 0;
    m_RunFlag = true;
    ret = VideoInit(videoindex);
    if(ret)
        return ret;

    m_rtmpUrl = rtmp;
    m_rtmpReady = true;
    ret = VideoRtmpInit();
    if(ret){
        m_rtmpReady = false;
        qDebug() << "Error VideoRtmpInit";
    }
    m_silencePeriod = silencePeriod;
    m_VideoDir = videoDir;
    m_VideoCapture->read(m_lastVideoframe);
    this->start();
    return 0;
}

int VideoProcess::VideoStop(void)
{
    m_RunFlag = false;
    //this->terminate();
    //msleep(10);
    return 0;
}

bool VideoProcess::VideoChangeDetection(Mat &frame)
{
    if(m_RecoderRun)
    {
        bool changeFlags;
        //If the video recorder is in operation, it is detected every other silent period
        if(m_lastDetectionTime.secsTo(QDateTime::currentDateTime()) >= 1)
        {
            changeFlags = ContentChangeDetection(frame);
            if(changeFlags == false)
                m_NoChangeTicks ++;
            else
                m_NoChangeTicks = 0;
        }

        //If the video recorder is in operation, it is detected every other silent period
        if(m_NoChangeTicks > m_silencePeriod)
        {
            m_NoChangeTicks = 0;
            return false;
        }
        return m_RecoderRun;
    }
    else
    {
        //If the video recorder has stopped, detect it every period
        return ContentChangeDetection(frame);
    }
}

bool VideoProcess::ContentChangeDetection(Mat &frame)
{
    double diffent_pixel_sum = 0;

    diffent_pixel_sum = diffentpixelHash(m_lastVideoframe,frame);

    m_lastDetectionTime = QDateTime::currentDateTime();

    //根据  帧差 判断视频变化情况
    if (diffent_pixel_sum > 20000)
    {
        m_lastVideoframe = frame.clone();
        return true;
    }else
        return false;
}


int VideoProcess::VideoRtmpInit(void)
{
    //注册所有的编解码器
    avcodec_register_all();
    //注册所有的封装器
    av_register_all();
    //注册所有网络协议
    avformat_network_init();

    // 1 使用opencv打开rtsp相机
    int inWidth = (int)m_VideoCapture->get(CAP_PROP_FRAME_WIDTH);
    int inHeight = (int)m_VideoCapture->get(CAP_PROP_FRAME_HEIGHT);
    int fps = (int)m_VideoCapture->get(CAP_PROP_FPS);
    //fps = 25;

    //2 初始化格式转换上下文
    m_pVsc = sws_getCachedContext(m_pVsc,
        inWidth, inHeight, AV_PIX_FMT_BGR24,     //源宽、高、像素格式
        inWidth, inHeight, AV_PIX_FMT_YUV420P,//目标宽、高、像素格式
        SWS_BICUBIC,  // 尺寸变化使用算法
        nullptr, nullptr, nullptr
    );
    if (!m_pVsc){
        qDebug() << "sws_getCachedContext failed!";
        return -1;
    }
    //3 初始化输出的数据结构
    m_pYuv = av_frame_alloc();
    m_pYuv->format = AV_PIX_FMT_YUV420P;
    m_pYuv->width = inWidth;
    m_pYuv->height = inHeight;
    m_pYuv->pts = 0;
    //分配yuv空间
    int ret = av_frame_get_buffer(m_pYuv, 32);
    if (ret != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        qDebug() << QString("%1").arg(buf);
    }
    //4 初始化编码上下文
    //a 找到编码器
    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        qDebug() << "Can`t find h264 encoder!";
        return -1;
    }
    //b 创建编码器上下文
    m_pVc = avcodec_alloc_context3(codec);
    if (!m_pVc){
        qDebug() << "avcodec_alloc_context3 failed!";
        return -1;
    }
    //c 配置编码器参数
    m_pVc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //全局参数
    m_pVc->codec_id = codec->id;
    m_pVc->thread_count = 8;

    m_pVc->bit_rate = 50 * 1024 * 8;//压缩后每秒视频的bit位大小 50kB
    m_pVc->width = inWidth;
    m_pVc->height = inHeight;
    m_pVc->time_base = { 1,fps };
    m_pVc->framerate = { fps,1 };
    //画面组的大小，多少帧一个关键帧
    m_pVc->gop_size = 50;
    m_pVc->max_b_frames = 0;
    m_pVc->pix_fmt = AV_PIX_FMT_YUV420P;
    //d 打开编码器上下文
    ret = avcodec_open2(m_pVc, nullptr, nullptr);
    if (ret != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        qDebug() << QString("%1").arg(buf);
    }
    qDebug() << "avcodec_open2 success!" << endl;
    //5 输出封装器和视频流配置
    //a 创建输出封装器上下文
    ret = avformat_alloc_output_context2(&m_pIc, nullptr, "flv", m_rtmpUrl.toLocal8Bit());
    if (ret != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        qDebug() << QString("%1").arg(buf);
    }
    //b 添加视频流
    m_pVs = avformat_new_stream(m_pIc, nullptr);
    if (!m_pVs)
    {
        qDebug() << "avformat_new_stream failed";
    }
    m_pVs->codecpar->codec_tag = 0;
    //从编码器复制参数
    avcodec_parameters_from_context(m_pVs->codecpar, m_pVc);
    av_dump_format(m_pIc, 0, m_rtmpUrl.toLocal8Bit(), 1);

    //打开rtmp 的网络输出IO
    ret = avio_open(&m_pIc->pb, m_rtmpUrl.toLocal8Bit(), AVIO_FLAG_WRITE);
    if (ret != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        qDebug() << QString("%1").arg(buf) << m_rtmpUrl;
        return -1;
    }

    //写入封装头
    ret = avformat_write_header(m_pIc, nullptr);
    if (ret != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        qDebug() << QString("%1").arg(buf);
        return -1;
    }

    memset(&m_Pack, 0, sizeof(m_Pack));
    m_Vpts = 0;

    return 0;
}

int VideoProcess::VideoRtmpRun(Mat &frame)
{
#if 0
    ///读取rtsp视频帧，解码视频帧
    if (!cam.grab())
    {
        continue;
    }
    ///yuv转换为rgb
    if (!cam.retrieve(frame))
    {
        continue;
    }
    imshow("video", frame);
    waitKey(20);
#endif
    int ret = 0;
    ///rgb to yuv
    //输入的数据结构
    uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
    //indata[0] bgrbgrbgr
    //plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr
    indata[0] = frame.data;
    int insize[AV_NUM_DATA_POINTERS] = { 0 };
    //一行（宽）数据的字节数
    insize[0] = frame.cols * frame.elemSize();
    int h = sws_scale(m_pVsc, indata, insize, 0, frame.rows,m_pYuv->data, m_pYuv->linesize); //源数据
    if (h <= 0)
        return 0;

    //cout << h << " " << flush;
    ///h264编码
    m_pYuv->pts = m_Vpts;
    m_Vpts++;
    ret = avcodec_send_frame(m_pVc, m_pYuv);
    if (ret != 0)
        return 0;

    ret = avcodec_receive_packet(m_pVc, &m_Pack);
    if (ret != 0 || m_Pack.size > 0)
    {
        //cout << "*" << pack.size << flush;
    }
    else
    {
        return 0;
    }
    //推流
    m_Pack.pts = av_rescale_q(m_Pack.pts, m_pVc->time_base, m_pVs->time_base);
    m_Pack.dts = av_rescale_q(m_Pack.dts, m_pVc->time_base, m_pVs->time_base);
    m_Pack.duration = av_rescale_q(m_Pack.duration, m_pVc->time_base, m_pVs->time_base);
    ret = av_interleaved_write_frame(m_pIc, &m_Pack);
    if (ret == 0)
    {
        cout << "#" << flush;
    }
    return 0;
}

void VideoProcess::run()
{
    //ADD 20201220
    std::chrono::steady_clock::time_point record_start_time = std::chrono::steady_clock::now();
    const int RECORD_TIME_INTERVAL = 3600; // record video into file per hour

    Mat frame,rgbImageFrame;
    //--- GRAB AND WRITE LOOP
    while (m_RunFlag)
    {
        // check if we succeeded
        if (!m_VideoCapture->read(frame)) {
            qDebug() << "ERROR! blank frame grabbed\n";
        }
#if 1
        //contex change Detection
        if(VideoChangeDetection(frame) == true)
        {
            if(m_RecoderRun == false)
            {
                //start recoder
                QString filename = m_VideoDir + "/" +QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss") + ".avi";
                emit sigRecoderMsg(filename + "...");
                VideoFileOpen(filename.toStdString(),m_VideoCapture->get(CAP_PROP_FPS),frame);
                record_start_time = std::chrono::steady_clock::now();
            }
            VideoFileWrite(frame);

            //ADD 20201220
            std::chrono::duration<double> record_duration = std::chrono::steady_clock::now() - record_start_time;
            if((int)record_duration.count() > RECORD_TIME_INTERVAL)
            {
                VideoFileClose();
            }
        }
        // else
        // {
        //     //close recoder
        //     VideoFileClose();
        // }       
#endif
        cvtColor(frame, rgbImageFrame, COLOR_BGR2RGB);//BGR convert to RGB);
        emit sigVideoNewFrame(rgbImageFrame);
        if(m_rtmpReady)
            VideoRtmpRun(rgbImageFrame);

        // show live and wait for a key with timeout long enough to show images
        //imshow("Live", frame);
        //if (waitKey(5) >= 0)
        //  break;
    }
    VideoFileClose();
    VideoDeInit();
}

double VideoProcess::diffentpixelHash(Mat &matSrc1, Mat &matSrc2)
{
    double diffent_pixel_sum = 0;
    Mat gray, gray2, gray_diff;

    //COLOR_BGR2Luv
    cvtColor(matSrc1, gray, COLOR_BGR2BGRA);
    cvtColor(matSrc2, gray2, COLOR_BGR2BGRA);
    //cvtColor(frame3, gray3, CV_BGR2BGRA);
    subtract(gray, gray2, gray_diff);
    //subtract(gray3, gray2, gray_diff2);//第三帧减第二帧
    for (int i = 0; i < gray_diff.rows; i++){
        for (int j = 0; j < gray_diff.cols; j++)
        {
            if (fabs(gray_diff.at<unsigned char>(i, j)) >= 20){//这里模板参数一定要用unsigned char，否则就一直报错
                //gray_diff.at<unsigned char>(i, j) = 255;            //第一次相减阈值处理
                diffent_pixel_sum += fabs(gray_diff.at<unsigned char>(i, j));
            }
            //else gray_diff.at<unsigned char>(i, j) = 0;
            //if (abs(gray_diff2.at<unsigned char>(i, j)) >= 20){//第二次相减阈值处理
            //	gray_diff2.at<unsigned char>(i, j) = 255;
            //	diffent_pixel_sum += abs(gray_diff.at<unsigned char>(i, j));
            //}
            //else gray_diff2.at<unsigned char>(i, j) = 0;
        }
        //bitwise_and(gray_diff, gray_diff2, gray);
        //imshow("foreground", gray);
    }
    return diffent_pixel_sum;
}

//平均哈希算法 实现步骤
//缩小尺寸：将图像缩小到8*8的尺寸，总共64个像素。这一步的作用是去除图像的细节，只保留结构/明暗等基本信息，摒弃不同尺寸/比例带来的图像差异；
//简化色彩：将缩小后的图像，转为64级灰度，即所有像素点总共只有64种颜色；
//计算平均值：计算所有64个像素的灰度平均值；
//比较像素的灰度：将每个像素的灰度，与平均值进行比较，大于或等于平均值记为1，小于平均值记为0；
//计算哈希值：将上一步的比较结果，组合在一起，就构成了一个64位的整数，这就是这张图像的指纹。组合的次序并不重要，只要保证所有图像都采用同样次序就行了；
//得到指纹以后，就可以对比不同的图像，看看64位中有多少位是不一样的。在理论上，这等同于”汉明距离”(Hamming distance,在信息论中，两个等长字符串之间的汉明距离是两个字符串对应位置的不同字符的个数)。如果不相同的数据位数不超过5，就说明两张图像很相似；如果大于10，就说明这是两张不同的图像。
int VideoProcess::aHash(Mat matSrc1, Mat matSrc2)
{
    Mat matDst1, matDst2;
    cv::resize(matSrc1, matDst1, cv::Size(8, 8), 0, 0, cv::INTER_CUBIC);
    cv::resize(matSrc2, matDst2, cv::Size(8, 8), 0, 0, cv::INTER_CUBIC);

    cv::cvtColor(matDst1, matDst1, COLOR_BGR2GRAY);
    cv::cvtColor(matDst2, matDst2, COLOR_BGR2GRAY);

    int iAvg1 = 0, iAvg2 = 0;
    int arr1[64], arr2[64];

    for (int i = 0; i < 8; i++)
    {
        uchar* data1 = matDst1.ptr<uchar>(i);
        uchar* data2 = matDst2.ptr<uchar>(i);

        int tmp = i * 8;

        for (int j = 0; j < 8; j++)
        {
            int tmp1 = tmp + j;

            arr1[tmp1] = data1[j] / 4 * 4;
            arr2[tmp1] = data2[j] / 4 * 4;

            iAvg1 += arr1[tmp1];
            iAvg2 += arr2[tmp1];
        }
    }

    iAvg1 /= 64;
    iAvg2 /= 64;

    for (int i = 0; i < 64; i++)
    {
        arr1[i] = (arr1[i] >= iAvg1) ? 1 : 0;
        arr2[i] = (arr2[i] >= iAvg2) ? 1 : 0;
    }

    int iDiffNum = 0;

    for (int i = 0; i < 64; i++)
        if (arr1[i] != arr2[i])
            ++iDiffNum;

    return iDiffNum;
}

//感知哈希算法
//平均哈希算法过于严格，不够精确，更适合搜索缩略图，为了获得更精确的结果可以选择感知哈希算法，它采用的是DCT（离散余弦变换）来降低频率的方法
//一般步骤
//缩小图片：32 * 32是一个较好的大小，这样方便DCT计算
//转化为灰度图
//计算DCT：利用Opencv中提供的dct()方法，注意输入的图像必须是32位浮点型
//缩小DCT：DCT计算后的矩阵是32 * 32，保留左上角的8 * 8，这些代表的图片的最低频率
//计算平均值：计算缩小DCT后的所有像素点的平均值
//大于平均值记录为1，反之记录为0
//得到信息指纹
int VideoProcess::pHash(Mat matSrc1, Mat matSrc2)
{
    Mat matDst1, matDst2;
    cv::resize(matSrc1, matDst1, cv::Size(32, 32), 0, 0, cv::INTER_CUBIC);
    cv::resize(matSrc2, matDst2, cv::Size(32, 32), 0, 0, cv::INTER_CUBIC);

    cv::cvtColor(matDst1, matDst1, COLOR_BGR2GRAY);
    cv::cvtColor(matDst2, matDst2, COLOR_BGR2GRAY);

    matDst1.convertTo(matDst1, CV_32F);
    matDst2.convertTo(matDst2, CV_32F);
    dct(matDst1, matDst1);
    dct(matDst2, matDst2);

    int iAvg1 = 0, iAvg2 = 0;
    int arr1[64], arr2[64];

    for (int i = 0; i < 8; i++)
    {
        uchar* data1 = matDst1.ptr<uchar>(i);
        uchar* data2 = matDst2.ptr<uchar>(i);

        int tmp = i * 8;

        for (int j = 0; j < 8; j++)
        {
            int tmp1 = tmp + j;

            arr1[tmp1] = data1[j];
            arr2[tmp1] = data2[j];

            iAvg1 += arr1[tmp1];
            iAvg2 += arr2[tmp1];
        }
    }

    iAvg1 /= 64;
    iAvg2 /= 64;

    for (int i = 0; i < 64; i++)
    {
        arr1[i] = (arr1[i] >= iAvg1) ? 1 : 0;
        arr2[i] = (arr2[i] >= iAvg2) ? 1 : 0;
    }

    int iDiffNum = 0;

    for (int i = 0; i < 64; i++)
        if (arr1[i] != arr2[i])
            ++iDiffNum;

    return iDiffNum;
}

//平均结构相似性 MSSIM 算法
double VideoProcess::getMSSIM(const Mat& i1, const Mat& i2)
{
    const double C1 = 6.5025, C2 = 58.5225;
    /***************************** INITS **********************************/
    int d = CV_32F;

    Mat I1, I2;
    i1.convertTo(I1, d);           // cannot calculate on one byte large values
    i2.convertTo(I2, d);

    Mat I2_2 = I2.mul(I2);        // I2^2
    Mat I1_2 = I1.mul(I1);        // I1^2
    Mat I1_I2 = I1.mul(I2);        // I1 * I2

    /*************************** END INITS **********************************/

    Mat mu1, mu2;   // PRELIMINARY COMPUTING
    GaussianBlur(I1, mu1, Size(11, 11), 1.5);
    GaussianBlur(I2, mu2, Size(11, 11), 1.5);

    Mat mu1_2 = mu1.mul(mu1);
    Mat mu2_2 = mu2.mul(mu2);
    Mat mu1_mu2 = mu1.mul(mu2);

    Mat sigma1_2, sigma2_2, sigma12;

    GaussianBlur(I1_2, sigma1_2, Size(11, 11), 1.5);
    sigma1_2 -= mu1_2;

    GaussianBlur(I2_2, sigma2_2, Size(11, 11), 1.5);
    sigma2_2 -= mu2_2;

    GaussianBlur(I1_I2, sigma12, Size(11, 11), 1.5);
    sigma12 -= mu1_mu2;

    ///////////////////////////////// FORMULA ////////////////////////////////
    Mat t1, t2, t3;

    t1 = 2 * mu1_mu2 + C1;
    t2 = 2 * sigma12 + C2;
    t3 = t1.mul(t2);              // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

    t1 = mu1_2 + mu2_2 + C1;
    t2 = sigma1_2 + sigma2_2 + C2;
    t1 = t1.mul(t2);               // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

    Mat ssim_map;
    divide(t3, t1, ssim_map);      // ssim_map =  t3./t1;

    Scalar mssim = mean(ssim_map); // mssim = average of ssim map
    return (mssim[0] + mssim[1] + mssim[2])/3;
}

