#include "VideoPlayer.h"

// Container format:
// [0xFE 0xFA] (2 bytes)
// [frameCount u32 LE] (4 bytes)
// [fps u16 LE] (2 bytes)
// Frames start immediately after header (total 8 bytes)
// Each frame: [length u32 LE] + [JPEG bytes]

static inline uint16_t readU16LE(File& f) {
    uint8_t b[2];
    if (f.read(b, 2) != 2) return 0;
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

static inline uint32_t readU32LE(File& f) {
    uint8_t b[4];
    if (f.read(b, 4) != 4) return 0;
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

int VideoPlayer::_jpegDrawCallback(JPEGDRAW* pDraw) {
    // pDraw->pPixels is 16-bit RGB565, width pDraw->iWidth, height pDraw->iHeight
    // Draw to sprite at position (pDraw->x, pDraw->y)
    if (!img) return 0;
    img->pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, (uint16_t*)pDraw->pPixels);
    return 1;
}

bool VideoPlayer::_readHeader() {
    if (!_file) return false;
    uint8_t magic[2];
    if (_file.read(magic, 2) != 2) return false;
    if (magic[0] != 0xFE || magic[1] != 0xFA) return false;

    _frameCount = readU32LE(_file);
    _fps = readU16LE(_file);
    if (_fps == 0) _fps = 15;
    _framesStartOffset = _file.position();
    return _frameCount > 0;
}

bool VideoPlayer::open(String path) {
    if (!LittleFS.begin(true)) {
        app_log_e("LittleFS mount failed");
        return false;
    }
    _file = LittleFS.open(path, "r");
    if (!_file) {
        app_log_e("Open file " + path + " error!!!");
        return false;
    }
    _path = path;
    if (!_readHeader()) {
        app_log_e("Bad header");
        _file.close();
        return false;
    }
    //app_log_i("Read header success");
    return true;
}

void VideoPlayer::setLoop(bool loop) { _loop = loop; }

bool VideoPlayer::_readFrame(std::vector<uint8_t>& buffer) {
    if (!_file) return false;
    uint32_t len = readU32LE(_file);
    if (len == 0 || len > 50UL * 1024UL) return false; // sanity (50KB max)
    buffer.resize(len);
    size_t rd = _file.read(buffer.data(), len);
    return rd == len;
}

void VideoPlayer::_taskEntry(void* arg) {
    VideoPlayer* self = static_cast<VideoPlayer*>(arg);
    const uint32_t periodMs = 1000UL / (self->_fps ? self->_fps : 15);

    for (;;) {
        if (self->_stopRequested) break;

        // Reset to start of frames
        self->_file.seek(self->_framesStartOffset, SeekSet);

        for (uint32_t i = 0; i < self->_frameCount; ++i) {
            if (self->_stopRequested) break;
            uint32_t tStart = millis();

            std::vector<uint8_t> frame;
            if (!self->_readFrame(frame)) {
                app_log_e("Read frame " + String(i) + " failed");
                break;
            }

            if (self->_jpeg.openRAM(frame.data(), frame.size(), _jpegDrawCallback)) {
                img->fillSprite(TFT_BLACK);
                self->_jpeg.decode(0, 0, 0);
                //img->setTextColor(TFT_BLACK);
                //img->fillRect(0, 0, 30, 7, TFT_WHITE);
                //img->drawNumber(millis() - tStart, 0, 0);
                img->pushSprite(0, 0);
            }

            uint32_t elapsed = millis() - tStart;
            if (elapsed < periodMs) {
                vTaskDelay(pdMS_TO_TICKS(periodMs - elapsed));
            }
            else {
                taskYIELD();
            }
        }

        if (!self->_loop) break;
    }

    self->_file.seek(0, SeekSet);
    self->_task = nullptr;
    vTaskDelete(nullptr);
}

bool VideoPlayer::play() {
    if (_task) return true;
    _stopRequested = false;
    //app_log("Play video start");
    xTaskCreatePinnedToCore(_taskEntry, "video_task", 4096, this, 1, &_task, 0);
    return _task != nullptr;
}

void VideoPlayer::stop() {
    if (!_task) return;
    _stopRequested = true;
    // busy wait briefly; alternatively event/notify
    while (_task) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (_file) _file.close();
}

bool VideoPlayer::isPlaying() const { return _task != nullptr; }


