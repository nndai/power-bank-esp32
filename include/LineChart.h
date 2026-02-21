#ifndef LINE_CHART_H
#define LINE_CHART_H

#include <Arduino.h>
#include <TFT_eSPI.h>

/**
 * @brief Standalone line chart renderer for TFT_eSprite.
 * 
 * Renders a line chart with auto-scaled axes, dashed grid lines,
 * and axis labels into a specified region of a sprite.
 * Supports linear X-axis interpolation and N-based scale suffixes.
 */
class LineChart {
public:
    /**
     * @brief Axis unit type for value conversion and display.
     */
    enum class AxisUnit : uint8_t {
        NORMAL,     // No conversion, raw numbers
        SECOND,     // Seconds (suffix "s")
        HOUR,       // Seconds -> Hours (suffix "h")
        DAY,        // Seconds -> Days (suffix "d")
        WATT,       // milliwatts -> Watts (suffix "W")
        MILLIAMP,   // milliamps -> Amps (suffix "A")
        MILLIVOLT   // millivolts -> Volts (suffix "V")
    };

    /**
     * @brief Chart configuration.
     */
    struct Config {
        int16_t  x       = 0;          // Top-left X on sprite
        int16_t  y       = 0;          // Top-left Y on sprite
        int16_t  w       = 160;        // Width of total area
        int16_t  h       = 80;         // Height of total area
        uint16_t lineColor = TFT_GREEN;
        uint16_t gridColor = 0x2945;   // Dark gray
        uint16_t axisColor = 0x7BEF;   // Medium gray
        uint16_t bgColor   = TFT_BLACK;
        uint16_t textColor = 0xAD55;   // Light gray
        uint8_t  gridRows  = 4;        // Number of horizontal grid divisions
        uint8_t  gridCols  = 5;        // Number of vertical grid divisions
        AxisUnit yUnit     = AxisUnit::NORMAL;
        AxisUnit xUnit     = AxisUnit::NORMAL;
        bool     linearX   = true;     // Linear X-axis interpolation
    };

    explicit LineChart(TFT_eSprite* sprite);

    /**
     * @brief Draw a line chart.
     * @param xData     Array of X-axis values (must be sorted ascending for linearX).
     * @param yData     Array of Y-axis values.
     * @param dataCount Number of data points.
     * @param config    Chart configuration.
     */
    void draw(const float* xData, const float* yData, int dataCount, const Config& config);

    // ==================== Utility Methods ====================

    /** @brief Get the N-based suffix: "", "N" (nghìn), "Tr" (triệu). */
    static const char* getScaleSuffix(float maxValue);

    /** @brief Get the divisor: 1, 1000, 1000000. */
    static float getScaleDivisor(float maxValue);

    /** @brief Format a value with auto scale (e.g., 5168 -> "5.2N"). */
    static String formatScaledValue(float value);

    /** @brief Format axis value: <10 -> 1 decimal, else integer. */
    static String formatAxisValue(float value);

    /** @brief Get the display suffix string for an AxisUnit. */
    static const char* getUnitSuffix(AxisUnit unit);

    /** @brief Convert a raw value according to AxisUnit (e.g., mA -> A). */
    static float convertByUnit(float value, AxisUnit unit);

private:
    TFT_eSprite* _sprite;

    // Chart plotting area (pixel coordinates)
    int16_t _chartX, _chartY, _chartW, _chartH;

    // Display range (after unit conversion & scaling)
    float _xMin, _xMax, _yMin, _yMax;
    // Scale divisors
    float _xDivisor, _yDivisor;

    void computeDataRange(const float* xData, const float* yData, int count, const Config& cfg);
    void computeChartArea(const Config& cfg);
    void drawBackground(const Config& cfg);
    void drawGrid(const Config& cfg);
    void drawAxes(const Config& cfg);
    void drawAxisLabels(const Config& cfg);
    void drawDataLine(const float* xData, const float* yData, int count, const Config& cfg);
    void drawDataLineLinear(const float* xData, const float* yData, int count, const Config& cfg);

    int16_t mapX(float value) const;
    int16_t mapY(float value) const;

    void drawDashedHLine(int16_t x, int16_t y, int16_t w, uint16_t color, uint8_t dashLen = 2, uint8_t gapLen = 2);
    void drawDashedVLine(int16_t x, int16_t y, int16_t h, uint16_t color, uint8_t dashLen = 2, uint8_t gapLen = 2);
};

#endif // LINE_CHART_H
