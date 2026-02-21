#include "ChargeLogger.h"

ChargeLogger::ChargeLogger()
    : _logging(false), _entryCount(0), _sessionStartMs(0) {}

void ChargeLogger::begin() {
    // Ensure log directory exists
    if (!LittleFS.exists(LOG_DIR)) {
        LittleFS.mkdir(LOG_DIR);
    }
    cleanupOldFiles();
}

void ChargeLogger::onPercentChanged(uint8_t percent, float current, float capacity, float voltage) {
    // Start a new session if not already logging
    if (!_logging) {
        _logging = true;
        _entryCount = 0;
        _sessionStartMs = millis();

        int fileNum = getNextFileNumber();
        char fname[32];
        snprintf(fname, sizeof(fname), "%s/%03d.csv", LOG_DIR, fileNum);
        _currentFile = String(fname);

        // Clean up old files before creating new one
        cleanupOldFiles();
    }

    // Calculate time in seconds since session start
    float timeSec = (millis() - _sessionStartMs) / 1000.0f;

    // Build CSV line: time,current,capacity,voltage,percent
    String line = String(timeSec, 0) + "," +
                  String(current, 0) + "," +
                  String(capacity, 0) + "," +
                  String(voltage, 0) + "," +
                  String(percent);

    // Append to file
    fs::File f = LittleFS.open(_currentFile, "a", true);
    if (f) {
        f.println(line);
        f.close();
        _entryCount++;
    }
}

void ChargeLogger::onChargeStop() {
    if (!_logging) return;

    _logging = false;

    // If fewer than MIN_ENTRIES, delete the file
    if (_entryCount < MIN_ENTRIES) {
        LittleFS.remove(_currentFile);
    }

    _entryCount = 0;
    _currentFile = "";
}

void ChargeLogger::listFiles(std::vector<String>& out) {
    out.clear();
    fs::File dir = LittleFS.open(LOG_DIR);
    if (!dir || !dir.isDirectory()) return;

    // Collect all filenames
    for (fs::File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        if (f.isDirectory()) continue;
        String name = String(f.name());
        // Extract basename only
        int lastSlash = name.lastIndexOf('/');
        if (lastSlash >= 0) name = name.substring(lastSlash + 1);
        if (name.endsWith(".csv")) {
            out.push_back(name);
        }
    }

    // Sort by filename (numeric order since zero-padded)
    for (size_t i = 0; i < out.size(); i++) {
        for (size_t j = i + 1; j < out.size(); j++) {
            if (out[i] > out[j]) {
                String tmp = out[i];
                out[i] = out[j];
                out[j] = tmp;
            }
        }
    }
}

int ChargeLogger::loadColumn(const String& filename, Column col, float* out, int maxCount) {
    String fullPath = String(LOG_DIR) + "/" + filename;
    fs::File f = LittleFS.open(fullPath, "r");
    if (!f) return 0;

    int count = 0;
    while (f.available() && count < maxCount) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;
        out[count] = parseColumn(line, col);
        count++;
    }
    f.close();
    return count;
}

int ChargeLogger::getEntryCount(const String& filename) {
    String fullPath = String(LOG_DIR) + "/" + filename;
    fs::File f = LittleFS.open(fullPath, "r");
    if (!f) return 0;

    int count = 0;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) count++;
    }
    f.close();
    return count;
}

int ChargeLogger::getNextFileNumber() {
    int maxNum = 0;
    fs::File dir = LittleFS.open(LOG_DIR);
    if (!dir || !dir.isDirectory()) return 1;

    for (fs::File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        if (f.isDirectory()) continue;
        String name = String(f.name());
        int lastSlash = name.lastIndexOf('/');
        if (lastSlash >= 0) name = name.substring(lastSlash + 1);
        // Parse number from "NNN.csv"
        int dotIdx = name.indexOf('.');
        if (dotIdx > 0) {
            int num = name.substring(0, dotIdx).toInt();
            if (num > maxNum) maxNum = num;
        }
    }
    return maxNum + 1;
}

void ChargeLogger::cleanupOldFiles() {
    std::vector<String> files;
    listFiles(files);

    // Delete oldest files (sorted ascending) until count <= MAX_FILES
    while (files.size() > MAX_FILES) {
        String fullPath = String(LOG_DIR) + "/" + files.front();
        LittleFS.remove(fullPath);
        files.erase(files.begin());
    }
}

float ChargeLogger::parseColumn(const String& line, int colIndex) {
    int start = 0;
    for (int i = 0; i < colIndex; i++) {
        int comma = line.indexOf(',', start);
        if (comma < 0) return 0.0f;
        start = comma + 1;
    }
    int end = line.indexOf(',', start);
    if (end < 0) end = line.length();
    return line.substring(start, end).toFloat();
}
