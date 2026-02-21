#ifndef CHARGE_LOGGER_H
#define CHARGE_LOGGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <vector>

class ChargeLogger {
public:
    static constexpr const char* LOG_DIR = "/charge_logs";
    static constexpr int MAX_FILES = 100;
    static constexpr int MIN_ENTRIES = 3;

    // Column indices for CSV: time,current,capacity,voltage,percent
    enum Column : uint8_t {
        COL_TIME     = 0,
        COL_CURRENT  = 1,
        COL_CAPACITY = 2,
        COL_VOLTAGE  = 3,
        COL_PERCENT  = 4,
        COL_COUNT    = 5
    };

    ChargeLogger();

    /** Initialize: ensure directory exists, cleanup old files. */
    void begin();

    /**
     * Called when battery percent increases during charging.
     * Logs a row: time, current, capacity, voltage, percent.
     * Automatically opens a new file on first call of a charge session.
     */
    void onPercentChanged(uint8_t percent, float current, float capacity, float voltage);

    /**
     * Called when charging stops (status becomes NOT_CHARGING or FULLY_CHARGED).
     * Closes the current log file. Deletes if < MIN_ENTRIES rows.
     */
    void onChargeStop();

    /** Check if currently logging (in a charge session). */
    bool isLogging() const { return _logging; }

    /** List available log filenames (basename only, e.g. "001.csv"). */
    void listFiles(std::vector<String>& out);

    /**
     * Load one column from a CSV file into a float array.
     * @param filename  Basename of the file (e.g. "001.csv")
     * @param col       Column index (0-4)
     * @param out       Output float array (must be pre-allocated)
     * @param maxCount  Max entries to read
     * @return          Number of entries actually read
     */
    int loadColumn(const String& filename, Column col, float* out, int maxCount);

    /**
     * Count number of data entries in a CSV file.
     */
    int getEntryCount(const String& filename);

private:
    bool _logging;
    int _entryCount;
    String _currentFile; // Full path of current log file
    uint32_t _sessionStartMs; // millis() when session started

    /** Find the next available file number by scanning existing files. */
    int getNextFileNumber();

    /** Delete oldest files to keep count <= MAX_FILES. */
    void cleanupOldFiles();

    /** Parse one float value at column index from a CSV line. */
    static float parseColumn(const String& line, int colIndex);
};

#endif // CHARGE_LOGGER_H
