#include "LineChart.h"

// =============================================================================
// Constructor
// =============================================================================

LineChart::LineChart(TFT_eSprite* sprite) : _sprite(sprite),
    _chartX(0), _chartY(0), _chartW(0), _chartH(0),
    _xMin(0), _xMax(1), _yMin(0), _yMax(1),
    _xDivisor(1), _yDivisor(1) {}

// =============================================================================
// Public: draw
// =============================================================================

void LineChart::draw(const float* xData, const float* yData, int dataCount,
                     const Config& config) {
    if (!_sprite || dataCount < 2 || !xData || !yData) return;

    computeDataRange(xData, yData, dataCount, config);
    computeChartArea(config);
    drawBackground(config);
    drawGrid(config);
    drawAxes(config);
    drawAxisLabels(config);

    if (config.linearX) {
        drawDataLineLinear(xData, yData, dataCount, config);
    } else {
        drawDataLine(xData, yData, dataCount, config);
    }
}

// =============================================================================
// Private: compute data range (with unit conversion + scaling)
// =============================================================================

void LineChart::computeDataRange(const float* xData, const float* yData, int count,
                                  const Config& cfg) {
    // Find raw min/max
    float rawXMin = xData[0], rawXMax = xData[0];
    float rawYMin = yData[0], rawYMax = yData[0];

    for (int i = 1; i < count; i++) {
        if (xData[i] < rawXMin) rawXMin = xData[i];
        if (xData[i] > rawXMax) rawXMax = xData[i];
        if (yData[i] < rawYMin) rawYMin = yData[i];
        if (yData[i] > rawYMax) rawYMax = yData[i];
    }

    // Convert by unit first
    float xMinConverted = convertByUnit(rawXMin, cfg.xUnit);
    float xMaxConverted = convertByUnit(rawXMax, cfg.xUnit);
    float yMinConverted = convertByUnit(rawYMin, cfg.yUnit);
    float yMaxConverted = convertByUnit(rawYMax, cfg.yUnit);

    // Compute divisors for display scaling
    _xDivisor = getScaleDivisor(max(fabsf(xMinConverted), fabsf(xMaxConverted)));
    _yDivisor = getScaleDivisor(max(fabsf(yMinConverted), fabsf(yMaxConverted)));

    // Scale to display values
    _xMin = xMinConverted / _xDivisor;
    _xMax = xMaxConverted / _xDivisor;
    _yMin = yMinConverted / _yDivisor;
    _yMax = yMaxConverted / _yDivisor;

    // Clamp min to 0 if real min >= 0
    if (rawYMin >= 0 && _yMin < 0) _yMin = 0;
    if (rawXMin >= 0 && _xMin < 0) _xMin = 0;

    // Avoid zero range
    if (_xMax - _xMin < 0.001f) _xMax = _xMin + 1.0f;
    if (_yMax - _yMin < 0.001f) _yMax = _yMin + 1.0f;
}

// =============================================================================
// Private: compute chart area (dynamic margins based on label widths)
// =============================================================================

void LineChart::computeChartArea(const Config& cfg) {
    // Font size 1 = 6px per char width, 8px height
    const int charW = 6;
    const int charH = 8;

    // Y axis: 3 labels (max at top, mid, min at bottom)
    String yMaxStr = formatAxisValue(_yMax);
    String yMidStr = formatAxisValue((_yMin + _yMax) / 2.0f);
    String yMinStr = formatAxisValue(_yMin);
    
    int maxYLabelLen = max(yMaxStr.length(), max(yMidStr.length(), yMinStr.length()));

    // Margins — no extra top/right, maximize chart area
    int16_t marginLeft   = maxYLabelLen * charW + 2;
    int16_t marginBottom  = charH + 2;  // X labels row
    int16_t marginTop     = 2;
    int16_t marginRight   = 2;

    _chartX = cfg.x + marginLeft;
    _chartY = cfg.y + marginTop;
    _chartW = cfg.w - marginLeft - marginRight;
    _chartH = cfg.h - marginTop - marginBottom;

    if (_chartW < 10) _chartW = 10;
    if (_chartH < 10) _chartH = 10;
}

// =============================================================================
// Private: draw background
// =============================================================================

void LineChart::drawBackground(const Config& cfg) {
    _sprite->fillRect(cfg.x, cfg.y, cfg.w, cfg.h, cfg.bgColor);
}

// =============================================================================
// Private: draw grid (dashed lines)
// =============================================================================

void LineChart::drawGrid(const Config& cfg) {
    // Horizontal grid lines
    for (uint8_t i = 1; i < cfg.gridRows; i++) {
        int16_t y = _chartY + (int32_t)_chartH * i / cfg.gridRows;
        drawDashedHLine(_chartX, y, _chartW, cfg.gridColor);
    }

    // Vertical grid lines
    for (uint8_t i = 1; i < cfg.gridCols; i++) {
        int16_t x = _chartX + (int32_t)_chartW * i / cfg.gridCols;
        drawDashedVLine(x, _chartY, _chartH, cfg.gridColor);
    }
}

// =============================================================================
// Private: draw axes
// =============================================================================

void LineChart::drawAxes(const Config& cfg) {
    _sprite->drawFastVLine(_chartX, _chartY, _chartH, cfg.axisColor);
    _sprite->drawFastHLine(_chartX, _chartY + _chartH, _chartW, cfg.axisColor);
}

// =============================================================================
// Private: draw axis labels (3 per axis)
// =============================================================================

void LineChart::drawAxisLabels(const Config& cfg) {
    _sprite->setTextSize(1);
    _sprite->setFreeFont();
    _sprite->setTextColor(cfg.textColor);

    const int charW = 6;
    const int charH = 8;

    // Build unit strings
    const char* ySuffix = getScaleSuffix(_yMax * _yDivisor);
    const char* yUnitStr = getUnitSuffix(cfg.yUnit);
    String yUnitFull = String(ySuffix) + String(yUnitStr);

    const char* xSuffix = getScaleSuffix(_xMax * _xDivisor);
    const char* xUnitStr = getUnitSuffix(cfg.xUnit);
    String xUnitFull = String(xSuffix) + String(xUnitStr);

    // --- Y axis: 3 labels at top, middle, bottom ---
    float yVals[3] = { _yMax, (_yMin + _yMax) / 2.0f, _yMin };
    int16_t yPositions[3] = {
        _chartY,
        (int16_t)(_chartY + _chartH / 2 - charH / 2),
        (int16_t)(_chartY + _chartH - charH)
    };

    for (int i = 0; i < 3; i++) {
        String label = formatAxisValue(yVals[i]);
        int16_t labelX = _chartX - 2 - (int16_t)(label.length() * charW);
        if (labelX < cfg.x) labelX = cfg.x;
        _sprite->drawString(label, labelX, yPositions[i]);
    }

    // Y unit label below the first (top) Y-axis value
    int16_t labelY = _chartY + _chartH + 2;
    if (yUnitFull.length() > 0) {
        _sprite->drawString(yUnitFull, 0, labelY);
    }

    // --- X axis: 3 labels at left, middle, right (bottom) ---
    float xVals[3] = { _xMin, (_xMin + _xMax) / 2.0f, _xMax };
    

    // Left label
    String xLabelLeft = formatAxisValue(xVals[0]);
    _sprite->drawString(xLabelLeft, _chartX, labelY);

    // Middle label (centered)
    String xLabelMid = formatAxisValue(xVals[1]);
    int16_t midX = _chartX + _chartW / 2 - (int16_t)(xLabelMid.length() * charW) / 2;
    _sprite->drawString(xLabelMid, midX, labelY);

    // Right label with X unit appended (e.g., "17Ns")
    String xLabelRight = formatAxisValue(xVals[2]) + xUnitFull;
    int16_t rightX = _chartX + _chartW - (int16_t)(xLabelRight.length() * charW);
    if (rightX < _chartX) rightX = _chartX;
    _sprite->drawString(xLabelRight, rightX, labelY);
}

// =============================================================================
// Private: draw data line (point-to-point, no interpolation)
// =============================================================================

void LineChart::drawDataLine(const float* xData, const float* yData, int count,
                             const Config& cfg) {
    int16_t prevPx = mapX(convertByUnit(xData[0], cfg.xUnit) / _xDivisor);
    int16_t prevPy = mapY(convertByUnit(yData[0], cfg.yUnit) / _yDivisor);

    for (int i = 1; i < count; i++) {
        int16_t px = mapX(convertByUnit(xData[i], cfg.xUnit) / _xDivisor);
        int16_t py = mapY(convertByUnit(yData[i], cfg.yUnit) / _yDivisor);
        _sprite->drawLine(prevPx, prevPy, px, py, cfg.lineColor);
        prevPx = px;
        prevPy = py;
    }
}

// =============================================================================
// Private: draw data line with linear X-axis interpolation
// For each pixel column, find the X value it corresponds to.
// If data exists near that X, interpolate Y and draw.
// =============================================================================

void LineChart::drawDataLineLinear(const float* xData, const float* yData, int count,
                                    const Config& cfg) {
    float rawXMin = xData[0];
    float rawXMax = xData[count - 1];
    float xRange = rawXMax - rawXMin;
    if (xRange < 0.001f) return;

    int16_t prevPx = -1, prevPy = -1;
    int dataIdx = 0; // Track position in sorted data

    for (int16_t px = 0; px < _chartW; px++) {
        // X value corresponding to this pixel column (in raw units)
        float xAtPixel = rawXMin + (xRange * px) / (_chartW - 1);

        // Advance dataIdx to find the segment containing xAtPixel
        while (dataIdx < count - 1 && xData[dataIdx + 1] < xAtPixel) {
            dataIdx++;
        }

        // Check if we have a valid segment
        if (dataIdx >= count - 1) break;

        float x0 = xData[dataIdx];
        float x1 = xData[dataIdx + 1];
        float segLen = x1 - x0;

        // Only interpolate if xAtPixel is within this segment
        if (xAtPixel >= x0 && xAtPixel <= x1 && segLen > 0.001f) {
            float t = (xAtPixel - x0) / segLen;
            float yInterp = yData[dataIdx] + t * (yData[dataIdx + 1] - yData[dataIdx]);

            // Convert and scale
            float yScaled = convertByUnit(yInterp, cfg.yUnit) / _yDivisor;
            float xScaled = convertByUnit(xAtPixel, cfg.xUnit) / _xDivisor;

            int16_t screenX = _chartX + px;
            int16_t screenY = mapY(yScaled);

            if (prevPx >= 0) {
                _sprite->drawLine(prevPx, prevPy, screenX, screenY, cfg.lineColor);
            }
            prevPx = screenX;
            prevPy = screenY;
        }
        // If xAtPixel is in a gap (no data), don't draw — break the line
        else if (xAtPixel > x1) {
            prevPx = -1;
            prevPy = -1;
        }
    }
}

// =============================================================================
// Private: map display value to pixel coordinate
// =============================================================================

int16_t LineChart::mapX(float value) const {
    if (_xMax - _xMin < 0.001f) return _chartX;
    float ratio = (value - _xMin) / (_xMax - _xMin);
    return _chartX + (int16_t)(ratio * (_chartW - 1));
}

int16_t LineChart::mapY(float value) const {
    if (_yMax - _yMin < 0.001f) return _chartY + _chartH - 1;
    float ratio = (value - _yMin) / (_yMax - _yMin);
    return _chartY + _chartH - 1 - (int16_t)(ratio * (_chartH - 1));
}

// =============================================================================
// Private: dashed line drawing
// =============================================================================

void LineChart::drawDashedHLine(int16_t x, int16_t y, int16_t w,
                                 uint16_t color, uint8_t dashLen, uint8_t gapLen) {
    uint8_t period = dashLen + gapLen;
    for (int16_t i = 0; i < w; i++) {
        if ((i % period) < dashLen) {
            _sprite->drawPixel(x + i, y, color);
        }
    }
}

void LineChart::drawDashedVLine(int16_t x, int16_t y, int16_t h,
                                 uint16_t color, uint8_t dashLen, uint8_t gapLen) {
    uint8_t period = dashLen + gapLen;
    for (int16_t i = 0; i < h; i++) {
        if ((i % period) < dashLen) {
            _sprite->drawPixel(x, y + i, color);
        }
    }
}

// =============================================================================
// Static utility methods
// =============================================================================

const char* LineChart::getScaleSuffix(float maxValue) {
    float absVal = fabsf(maxValue);
    if (absVal >= 1e6f) return "Tr";  // Triệu
    if (absVal >= 1e3f) return "N";   // Nghìn
    return "";
}

float LineChart::getScaleDivisor(float maxValue) {
    float absVal = fabsf(maxValue);
    if (absVal >= 1e6f) return 1e6f;
    if (absVal >= 1e3f) return 1e3f;
    return 1.0f;
}

String LineChart::formatScaledValue(float value) {
    float divisor = getScaleDivisor(value);
    const char* suffix = getScaleSuffix(value);
    float scaled = value / divisor;
    String result = formatAxisValue(scaled);
    result += suffix;
    return result;
}

String LineChart::formatAxisValue(float value) {
    // If < 10, allow 1 decimal place; otherwise integer
    if (fabsf(value) < 10.0f) {
        // 1 decimal, but trim ".0" if whole number
        if (value == (int)value) {
            return String((int)value);
        }
        return String(value, 1);
    }
    return String((int)roundf(value));
}

const char* LineChart::getUnitSuffix(AxisUnit unit) {
    switch (unit) {
        case AxisUnit::SECOND:    return "s";
        case AxisUnit::HOUR:      return "h";
        case AxisUnit::DAY:       return "d";
        case AxisUnit::WATT:      return "W";
        case AxisUnit::MILLIAMP:  return "A";
        case AxisUnit::MILLIVOLT: return "V";
        case AxisUnit::NORMAL:
        default:                  return "";
    }
}

float LineChart::convertByUnit(float value, AxisUnit unit) {
    switch (unit) {
        case AxisUnit::HOUR:      return value / 3600.0f;
        case AxisUnit::DAY:       return value / 86400.0f;
        case AxisUnit::WATT:      return value / 1000.0f;
        case AxisUnit::MILLIAMP:  return value / 1000.0f;
        case AxisUnit::MILLIVOLT: return value / 1000.0f;
        case AxisUnit::SECOND:
        case AxisUnit::NORMAL:
        default:                  return value;
    }
}
