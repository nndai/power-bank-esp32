#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <FS.h>
#include <vector>
#include <JPEGDEC.h>
#include "Globals.h"

class VideoPlayer {
private:
    fs::File _file;
    String _path;
    uint32_t _frameCount = 0;
    uint16_t _fps = 15;
    uint32_t _framesStartOffset = 0;
    bool _loop = false;
    volatile bool _stopRequested = false;
    TaskHandle_t _task = nullptr;
    JPEGDEC _jpeg;

    static int _jpegDrawCallback(JPEGDRAW* pDraw);
    static void _taskEntry(void* arg);
    bool _readHeader();
    bool _readFrame(std::vector<uint8_t>& buffer);

public:
    bool open(String path);
    void setLoop(bool loop);
    bool play();
    void stop();
    bool isPlaying() const;
};

#endif // VIDEO_PLAYER_H

