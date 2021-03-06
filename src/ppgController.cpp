#include "ppgController.h"

Controller::Controller(QString videoPath)
{
    m_playMode = VIDEO_MODE_CAMERA;
    m_exitState = false;
    m_videoPath = videoPath;
}

// This function will run in new thread when "start()" is called.
void Controller::run()
{
    VideoCapture video;
    Detector detector;
    Mat frame;
    system_clock::time_point ctime;
    duration<double> time_span;

    while(1){
        while(!m_exitState) waitKey(100);

        // reset variables
        clearVector(m_ppgTime);
        clearVector(m_ppgValue);

        // Camera mode
        if(m_playMode == VIDEO_MODE_CAMERA){
            video = VideoCapture(0);
            m_fps = 30;
        }else{
            video = VideoCapture(m_videoPath.toStdString());
            m_fps = video.get(CV_CAP_PROP_FPS);
        }
        
        Filter filter(m_fps);
        // Handle frame one by one
        m_time = 0;
        int cnt = 0;
        while(m_exitState && video.read(frame)){
            cnt++;
            ctime = system_clock::now();

            // Detect: show frame in label as picture
            Scalar value = detector.detect(frame);
            Mat rgb;
            cvtColor(frame, rgb, CV_BGR2RGB);
            QImage img = QImage((const uchar*)(rgb.data), rgb.cols, rgb.rows,
                                rgb.cols * rgb.channels(), QImage::Format_RGB888);

            emit signal_showImage(img);

            // Filter
            filter.filterInput(value.val[1]);
            emit signal_returnPPGValue(m_time, 255-value.val[1], 0);

            std::vector<int> peak_pos = filter.getPeakPos();
            std::vector<double> hr_val = filter.getFrequency();
            std::vector<double> signal = filter.getSignal();
            filter.clearTmpData();
            for(int i = 0; i < signal.size(); i++){
                emit signal_returnPPGValue(m_time-float(signal.size()-1-i+10)/m_fps, 255-signal[i], 1);
            }
            for(int i = 0; i < hr_val.size(); i++){
                emit signal_returnHRValue(double(peak_pos[i])/m_fps, hr_val[i]);
            }

            // Save results
            time_span = duration_cast<duration<double>>(system_clock::now() - ctime); /* DevSkim: ignore DS154189 */
            printf("%.2f\t%.3f\t%.3f\t%.3f\t%.1f\t%.2f\n", m_time, value[0], value[1],
                    value[2], m_fps, 1 / time_span.count());
            m_time += 1 / m_fps;

            m_ppgTime.push_back(m_time);
            m_ppgValue.push_back(255-value.val[1]);
        }
        video.release();
    }

    detector.initialize();
    fclose(stdout);
}

// SLOT: set video path
void Controller::slot_setVideoPath(QString newVideoPath)
{
    printf("slot_setVideoPath\n"); /* DevSkim: ignore DS154189 */
    m_videoPath = newVideoPath;
}

// SLOT: set playing state
void Controller::slot_setPlayingState(bool state)
{
    printf("slot_setPlayingState\n"); /* DevSkim: ignore DS154189 */
    m_exitState = state;
}

// SLOT: set video mode
void Controller::slot_setVideoMode(int mode)
{
    printf("slot_setVideoMode\n"); /* DevSkim: ignore DS154189 */
    m_playMode = mode;
}

// SLOT: set video mode
void Controller::slot_saveData(QString saveDataPath)
{
    printf("slot_saveData\n"); /* DevSkim: ignore DS154189 */
    FILE* fpt = fopen(saveDataPath.toStdString().c_str(),"w"); /* DevSkim: ignore DS154189 */
    for(int i=0; i<m_ppgValue.size(); i++){
        fprintf(fpt,"%10.4f,", m_ppgTime[i]);  /* DevSkim: ignore DS154189 */
        fprintf(fpt,"%10.4f\n",m_ppgValue[i]); /* DevSkim: ignore DS154189 */
    }
    fclose(fpt);
}

// FUNC: get fps
double Controller::getFps()
{
    return m_fps;
}