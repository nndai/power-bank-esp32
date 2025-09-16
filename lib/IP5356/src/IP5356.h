#ifndef IP5356_H
#define IP5356_H

#include <Wire.h>
#include <Log.h>

/**
 * @brief IP5356 Power Management IC (PMIC) driver
 *
 */
class IP5356 {
public:
    const uint8_t address1 = 0x74; // I2C address for IP5356
    const uint8_t address2 = 0x75; // I2C address for IP5356

    /**
     * @brief Registers for I2C address 0x74 (Write: 0xE8, Read: 0xE9)
     */
    enum IP5356_0X74_REG {
        SYS_CTLO = 0x00,          // Section 4.1 | REG 0x00 | R/W: Boost and charger enable register
        SYS_CTL1 = 0x03,          // Section 4.2 | REG 0x03 | R/W: Light load shutdown control register
        CHG_CTLO = 0x0B,          // Section 4.3 | REG 0x0B | R/W: 12V input charging undervoltage loop voltage control
        CHG_CTL1 = 0x0C,          // Section 4.4 | REG 0x0C | R/W: 9V input charging undervoltage loop voltage control
        CHG_CTL2 = 0x0D,          // Section 4.5 | REG 0x0D | R/W: 5V input charging undervoltage loop voltage control
        SYS_CTL2 = 0x0F,          // Section 4.6 | REG 0x0F | R/W: Light load shutdown control register
        VOUT1_CTLO = 0x10,        // Section 3.5, 4.7 | REG 0x10 | R/W: VOUT1 control register 0
        VOUT1_CTL1 = 0x13,        // Section 4.8 | REG 0x13 | R/W: VOUT1 control register 1
        VOUT2_CTLO = 0x14,        // Section 3.5, 4.9 | REG 0x14 | R/W: VOUT2 control register 0
        VOUT2_CTL1 = 0x17,        // Section 4.11 | REG 0x17 | R/W: VOUT2 control register 1
        VBUS_CTLO = 0x18,         // Section 3.5, 4.13 | REG 0x18 | R/W: VBUS control register 0
        VBUS_CTL1 = 0x1B,         // Section 4.14 | REG 0x1B | R/W: VBUS control register 1
        VOUT1_CTL2 = 0x1C,        // Section 4.9 | REG 0x1C | R/W: VOUT1 control register 2
        VOUT2_CTL2 = 0x1D,        // Section 4.12 | REG 0x1D | R/W: VOUT2 control register 2
        VBUS_CTL2 = 0x1E,         // Section 4.15 | REG 0x1E | R/W: VBUS control register 2
        PPATH_CTL0 = 0x24,        // Section 4.16 | REG 0x24 | R/W: Pass-through charging control register
        SYS_CTL3 = 0x25,          // Section 3.5, 4.18 | REG 0x25 | R/W: Input fast charge and MOS control register
        CHG_CTL3 = 0x26,          // Section 4.19 | REG 0x26 | R/W: Pass-through charging current control register
        CHG_CTL4 = 0x27,          // Section 4.20 | REG 0x27 | R/W: VIN 12V charging current control register
        CHG_CTL5 = 0x28,          // Section 4.21 | REG 0x28 | R/W: VIN 5V charging current control register
        CHG_CTL6 = 0x29,          // Section 4.22 | REG 0x29 | R/W: VBUS 5V charging current control register
        CHG_CTL7 = 0x2B,          // Section 4.23 | REG 0x2B | R/W: VIN 9V charging current control register
        SYS_CTL4 = 0x31,          // Section 4.26 | REG 0x31 | R/W: Key press shutdown control
        SYS_CTL5_VSET = 0x33,     // Section 3.3, 4.27 | REG 0x33 | R/W: Always-on N hours and VSET detection enable
        BATLOW_ADC = 0x41,        // Section 4.28 | REG 0x41 | R/W: Low battery voltage setting register
        POW_LOW = 0x44,           // Section 4.29 | REG 0x44 | R/W: Light load shutdown power setting register
        ISYS_LOW = 0x45,          // Section 4.30 | REG 0x45 | R/W: Light load shutdown current setting register
        VOUT1_IMOSLOW = 0x49,     // Section 4.31 | REG 0x49 | R/W: VOUT1 port light load current setting register
        VOUT2_IMOSLOW = 0x4A,     // Section 4.32 | REG 0x4A | R/W: VOUT2 port light load current setting register
        VBUS_IMOSLOW = 0x4B,      // Section 4.33 | REG 0x4B | R/W: VBUS port light load current setting register
        FCAP_REG = 0x4C,          // Section 4.34 | REG 0x4C | R/W: Fuel gauge capacity setting register
        MFP_CTL0 = 0x65,          // Section 4.35 | REG 0x65 | R/W: LED4/LED5 function selection
        MFP_CTL1 = 0x66,          // Section 4.36 | REG 0x66 | R/W: LED6 function selection
        CHG_CTL8 = 0x6E,          // Section 4.24 | REG 0x6E | R/W: VBUS 12V charging current control register
        CHG_CTL9 = 0x6F,          // Section 4.25 | REG 0x6F | R/W: VBUS 9V charging current control register
        EN_FCAP = 0x78,           // Section 4.37 | REG 0x78 | R/W: External capacity enable register
        SINK_QC_EN = 0x81,        // Section 3.4, 4.40 | REG 0x81 | R/W: Input fast charge enable
        SYS_CTL5_LC = 0x84,       // Section 4.41 | REG 0x84 | R/W: Line compensation enable register
        SRC_QC_EN = 0x85,         // Section 3.4, 4.42 | REG 0x85 | R/W: Output fast charge enable
        SRC_QC_EN2 = 0x86,        // Section 4.43 | REG 0x86 | R/W: Output fast charge enable 2
        BST_VSET_H = 0xAA,        // Section 4.44 | REG 0xAA | R/W: Output voltage setting register (High bits)
        BST_VSET_L = 0xAB,        // Section 4.45 | REG 0xAB | R/W: Output voltage setting register (Low bits)
        BST_5V = 0xAD,            // Section 4.46 | REG 0xAD | R/W: 5V output voltage setting register
        BST_12V_9V = 0xAE,        // Section 4.47 | REG 0xAE | R/W: 12V/9V output voltage setting register
        VOUT1_5V = 0xB1,          // Section 4.48 | REG 0xB1 | R/W: VOUT1 5V output current limit setting
        VOUT1_9V = 0xB3,          // Section 4.49 | REG 0xB3 | R/W: VOUT1 9V output current limit setting
        VOUT1_12V = 0xB4,         // Section 4.50 | REG 0xB4 | R/W: VOUT1 12V output current limit setting
        VOUT2_5V = 0xB5,          // Section 4.51 | REG 0xB5 | R/W: VOUT2 5V output current limit setting
        VOUT2_9V = 0xB7,          // Section 4.52 | REG 0xB7 | R/W: VOUT2 9V output current limit setting
        VOUT2_12V = 0xB8,         // Section 4.53 | REG 0xB8 | R/W: VOUT2 12V output current limit setting
        VBUS_5V = 0xB9,           // Section 4.54 | REG 0xB9 | R/W: VBUS 5V output current limit setting
        VBUS_9V = 0xBB,           // Section 4.55 | REG 0xBB | R/W: VBUS 9V output current limit setting
        VBUS_12V = 0xBC,          // Section 4.56 | REG 0xBC | R/W: VBUS 12V output current limit setting
        TYPEC_CTL0 = 0xD1,        // Section 3.4, 4.57 | REG 0xD1 | R/W: PD control register 0
        TYPEC_CTL1 = 0xD5,        // Section 4.58 | REG 0xD5 | R/W: PD control register 1
        PPATH_CTL1 = 0xF5,        // Section 4.17 | REG 0xF5 | R/W: Pass-through charging control register 1
        NTC_CTL0 = 0xF6,          // Section 3.6, 4.38 | REG 0xF6 | R/W: NTC current selection
        NTC_CTL1 = 0xFD,          // Section 3.6, 4.39 | REG 0xFD | R/W: NTC control register
    };

    /**
     * @brief Registers for I2C address 0x75 (Write: 0xEA, Read: 0xEB)
     */
    enum IP5356_0X75_REG {
        VIN_VBUS_OV = 0x01,       // Section 4.59 | REG 0x01 | R/W: VIN/VBUS input overvoltage setting
        BATLOW = 0x03,            // Section 4.60 | REG 0x03 | R/W: Analog cell low voltage shutdown voltage setting
        GPIO_20UA_EN = 0x19,      // Section 4.61 | REG 0x19 | R/W: GPIO 20uA current output enable register
        SYS_CTL5_PD = 0x1F,       // Section 4.62 | REG 0x1F | R/W: VOUT2 PD fast charge register
        TEMP_LP = 0x27,           // Section 4.63 | REG 0x27 | R/W: IC internal temperature loop setting
        LC_SET = 0x30,            // Section 4.64 | REG 0x30 | R/W: Line compensation selection register
        CHG_CTL10 = 0x3A,         // Section 3.3, 4.65 | REG 0x3A | R/W: Constant voltage charging voltage setting register
        CHG_CTL11 = 0x3B,         // Section 4.66 | REG 0x3B | R/W: Charging termination current setting register
        CHG_CTL12 = 0x42,         // Section 4.67 | REG 0x42 | R/W: Charging termination current setting register
        BATVADC_DAT0 = 0x50,      // Section 4.1 | REG 0x50 | R: VBAT voltage ADC data low 8 bits
        BATVADC_DAT1 = 0x51,      // Section 4.2 | REG 0x51 | R: VBAT voltage ADC data high 8 bits
        VSYSVADC_DAT0 = 0x52,     // Section 4.3 | REG 0x52 | R: VSYS voltage ADC data low 8 bits
        VSYSVADC_DAT1 = 0x53,     // Section 4.4 | REG 0x53 | R: VSYS voltage ADC data high 8 bits
        IVIN_IVBUS_IADC_DAT0 = 0x54, // Section 4.5 | REG 0x54 | R: Input current ADC data low 8 bits
        IVIN_IVBUS_IADC_DAT1 = 0x55, // Section 4.6 | REG 0x55 | R: Input current ADC data high 8 bits
        IVOUT1_IADC_DAT0 = 0x56,  // Section 4.7 | REG 0x56 | R: VOUT1 output current ADC data low 8 bits
        IVOUT1_IADC_DAT1 = 0x57,  // Section 4.8 | REG 0x57 | R: VOUT1 output current ADC data high 8 bits
        IVOUT2_IADC_DAT0 = 0x58,  // Section 4.9 | REG 0x58 | R: VOUT2 output current ADC data low 8 bits
        IVOUT2_IADC_DAT1 = 0x59,  // Section 4.10 | REG 0x59 | R: VOUT2 output current ADC data high 8 bits
        IBUS_IADC_DAT0 = 0x5A,    // Section 4.11 | REG 0x5A | R: VBUS output current ADC data low 8 bits
        IBUS_IADC_DAT1 = 0x5B,    // Section 4.12 | REG 0x5B | R: VBUS output current ADC data high 8 bits
        VINVADC_DAT0 = 0x5C,      // Section 4.13 | REG 0x5C | R: VIN voltage ADC data low 8 bits
        VINVADC_DAT1 = 0x5D,      // Section 4.14 | REG 0x5D | R: VIN voltage ADC data high 8 bits
        VOUT1VADC_DAT0 = 0x5E,    // Section 4.15 | REG 0x5E | R: VOUT1 voltage ADC data low 8 bits
        VOUT1VADC_DAT1 = 0x5F,    // Section 4.16 | REG 0x5F | R: VOUT1 voltage ADC data high 8 bits
        VOUT2VADC_DAT0 = 0x60,    // Section 4.17 | REG 0x60 | R: VOUT2 voltage ADC data low 8 bits
        VOUT2VADC_DAT1 = 0x61,    // Section 4.18 | REG 0x61 | R: VOUT2 voltage ADC data high 8 bits
        VBUSVADC_DAT0 = 0x62,     // Section 4.19 | REG 0x62 | R: VBUS voltage ADC data low 8 bits
        VBUSVADC_DAT1 = 0x63,     // Section 4.20 | REG 0x63 | R: VBUS voltage ADC data high 8 bits
        NTCVADC_DAT0 = 0x64,      // Section 3.6, 4.21 | REG 0x64 | R: NTC voltage ADC data low 8 bits
        NTCVADC_DAT1 = 0x65,      // Section 3.6, 4.22 | REG 0x65 | R: NTC voltage ADC data high 8 bits
        LED4VADC_DAT0 = 0x68,     // Section 4.23 | REG 0x68 | R: LED4 pin voltage ADC data low 8 bits
        LED4VADC_DAT1 = 0x69,     // Section 4.24 | REG 0x69 | R: LED4 pin voltage ADC data high 8 bits
        LED5VADC_DAT0 = 0x6A,     // Section 4.25 | REG 0x6A | R: LED5 pin voltage ADC data low 8 bits
        LED5VADC_DAT1 = 0x6B,     // Section 4.26 | REG 0x6B | R: LED5 pin voltage ADC data high 8 bits
        LED6VADC_DAT0 = 0x6C,     // Section 4.27 | REG 0x6C | R: LED6 pin voltage ADC data low 8 bits
        LED6VADC_DAT1 = 0x6D,     // Section 4.28 | REG 0x6D | R: LED6 pin voltage ADC data high 8 bits
        IBATIADC_DAT0 = 0x6E,     // Section 4.29 | REG 0x6E | R: Battery terminal current ADC data low 8 bits
        IBATIADC_DAT1 = 0x6F,     // Section 4.30 | REG 0x6F | R: Battery terminal current ADC data high 8 bits
        ISYS_IADC_DAT0 = 0x70,    // Section 4.31 | REG 0x70 | R: IVSYS terminal current ADC data low 8 bits
        IVSYS_IADC_DAT1 = 0x71,   // Section 4.32 | REG 0x71 | R: IVSYS terminal current ADC data high 8 bits
        VSYS_POW_DAT0 = 0x74,     // Section 4.33 | REG 0x74 | R: VSYS terminal power ADC data low 8 bits
        VSYS_POW_DAT1 = 0x75,     // Section 4.34 | REG 0x75 | R: VSYS terminal power ADC data high 8 bits
        SOC_CAP_DATA = 0x7B,      // Section 3.2, 4.35 | REG 0x7B | R: Cell battery percentage data (%)
        FORCE_STANDBY = 0x86,     // Section 3.4, 4.37 | REG 0x86 | R/W: Force IP5356 to enter sleep mode
        SOC_CAP_SET = 0x87,       // Section 4.36 | REG 0x87 | R/W: Cell battery percentage control register
        STATE_CTL0 = 0x90,        // Section 4.38 | REG 0x90 | R/W: Charging state control register
        STATE_CTL1 = 0x91,        // Section 3.4, 4.39 | REG 0x91 | R/W: Input PD state control register
        STATE_CTL2 = 0x92,        // Section 4.40 | REG 0x92 | R/W: Charging path control register
        STATE_CTL3 = 0x97,        // Section 3.4, 4.41 | REG 0x97 | R/W: PD output control register
        FCP_STATUS = 0xA1,        // Section 4.42 | REG 0xA1 | R: Output FCP status indicator register
        STATUS_SRC0 = 0xA4,       // Section 4.43 | REG 0xA4 | R: Output fast charge status indicator register 0
        STATUS_SRC1 = 0xA5,       // Section 4.44 | REG 0xA5 | R: Output fast charge status indicator register 1
        STATUS_SRC2 = 0xA8,       // Section 4.45 | REG 0xA8 | R: Output fast charge status indicator register 2
        AFC_STATUS = 0xAF,        // Section 4.46 | REG 0xAF | R: Output AFC status indicator register
        PD_STATE0 = 0xB1,         // Section 4.47 | REG 0xB1 | R: System status indicator register (PD_SINK_OK)
        PD_STATE1 = 0xC2,         // Section 4.48 | REG 0xC2 | R: System status indicator register (PD_SRC_OK)
        PD_STATE2 = 0xC3,         // Section 4.49 | REG 0xC3 | R: System status indicator register (PPS_SRC_OK)
        SYS_STATE0 = 0xC4,        // Section 3.1, 4.50 | REG 0xC4 | R: System status indicator register 0
        SYS_STATE1 = 0xC5,        // Section 4.51 | REG 0xC5 | R: System status indicator register 1
        SYS_STATE2 = 0xCD,        // Section 4.52 | REG 0xCD | R: System status indicator register 2
        SYS_STATE3 = 0xD0,        // Section 3.1, 4.53 | REG 0xD0 | R: System status indicator register 3
        SYS_STATE4 = 0xD4,        // Section 4.54 | REG 0xD4 | R: System status indicator register 4
        LOWCUR_STATE = 0xE1,      // Section 4.55 | REG 0xE1 | R: System status indicator register (Always-on N hours)
        CHG_STATE1 = 0xE8,        // Section 4.56 | REG 0xE8 | R: System status indicator register (Charging state 1)
        CHG_STATE2 = 0xE9,        // Section 3.1, 4.57 | REG 0xE9 | R: System status indicator register (Charging state 2)
        MOS_STATE = 0xEB,         // Section 4.58 | REG 0xEB | R: Output MOS status indicator register
        ILOW_STATE = 0xF2,        // Section 4.59 | REG 0xF2 | R: System light load status indicator register
        TYPEC_STATE = 0xF3,       // Section 4.60 | REG 0xF3 | R: TYPE-C connection status indicator register
        KEYIN_STATE = 0xF4,       // Section 4.61 | REG 0xF4 | R/W: Key press status indicator register
        NTC_STATE = 0xFB,         // Section 4.62 | REG 0xFB | R: NTC and output MOS current status indicator
        OCP_STATE = 0xFC,         // Section 4.63 | REG 0xFC | R/W: System over-current status indicator
    };

    /**
     * @brief Array check register in address 0x74
     * 0: not a register in ip5356
     * 1: R
     * 2: R/W
     */
    static const uint8_t checkReg0x74Arr[256];
    
    /**
     * @brief Array check register in address 0x75
     * 0: not a register in ip5356
     * 1: R
     * 2: R/W
     */
    static const uint8_t checkReg0x75Arr[256];

    /**
     * @brief Delete any existing key press task
     */
    void deletePressTask();

private:
    TwoWire* _i2c;
    const float _iNTC = 8e-5f; // 80μA
    const float _rNTC = 10000; // 10kΩ

    int8_t _ntcDiePin;
    int8_t _keyPin;
    int8_t _intPin;

    TaskHandle_t _shortPressTaskHandle = NULL;
    TaskHandle_t _longPressTaskHandle = NULL;
    TaskHandle_t _doublePressTaskHandle = NULL;
    TaskHandle_t _resetWholeSystemTaskHandle = NULL;



public:
    
    /**
     * @brief Read 1 byte data from a specific register of a given I2C address.
     * @param addr The I2C address to read from.
     * @param reg The register to read from.
     * @param data Pointer to the buffer where the read data will be stored.
     * @return true if success
    */
    bool _read(uint8_t addr, uint8_t reg, uint8_t* data);

    /**
     * @brief Read 2 byte data from a specific register of a given I2C address. [15:8][7:0]
     * @param addr The I2C address to read from.
     * @param reg The register to read from.
     * @param data Pointer to the buffer where the read data will be stored.
     * @return true if success
    */
    bool _read(uint8_t addr, uint8_t reg, uint16_t* data);

    /**
     * @brief Write 1 byte data to a specific register of a given I2C address.
     * @param addr The I2C address to write to.
     * @param reg The register to write to.
     * @param data Pointer to the data to be written.
     * @return true if success
    */
    bool _write(uint8_t addr, uint8_t reg, uint8_t* data);


    /// Overloaded read functions for specific registers
    bool _read(IP5356_0X74_REG reg, uint8_t* data);
    bool _read(IP5356_0X74_REG reg, uint16_t* data);
    bool _read(IP5356_0X75_REG reg, uint8_t* data);
    bool _read(IP5356_0X75_REG reg, uint16_t* data);

    /// Overloaded write functions for specific registers
    bool _write(IP5356_0X74_REG reg, uint8_t* data);
    bool _write(IP5356_0X75_REG reg, uint8_t* data);

    /**
     * @brief Check if a register can be read from a specific I2C address.
     * @param addr The I2C address to check.
     * @param reg The register to check.
     * @return true if the register can be read
    */
    bool checkRegRead(uint8_t addr, uint8_t reg);

    /**
     * @brief Check if a register can be written to a specific I2C address.
     * @param addr The I2C address to check.
     * @param reg The register to check.
     * @return true if the register can be written
    */
    bool checkRegWrite(uint8_t addr, uint8_t reg);


public:
    IP5356(TwoWire* wire = &Wire, int8_t ntcDiePin = -1, int8_t keyPin = -1, int8_t intPin = -1);

    void init();

    String loadRegData(uint8_t addr, uint8_t reg);
    String loadAllRegData();

    /**
     * @brief Check if the IP5356 is I2C connected
     */
    bool isConnected();

    bool isWork();

    void powerOff();

    void isysLowAutoOff(bool on = false);

    bool checkIsysLow();

    void set8sAutoOff();

    /**
     * @brief Read battery information
     */
    uint8_t getBatteryPercentage(); // %
    float getBatteryVoltage();      // mV
    float getBatteryCurrent();
    float getBatteryTemperature();
    float getBatteryPower();

    /**
     * @brief Read VSYS information
     */
    float getSystemVoltage();
    float getSystemCurrent();
    float getSystemPower();

    /**
     * @brief Read VOUT1 information
     */
    float getUSB1Voltage();
    float getUSB1Current();
    float getUSB1Power();

    /**
     * @brief Read VOUT2 information
     */
    float getUSB2Voltage();
    float getUSB2Current();
    float getUSB2Power();

    /**
     * @brief Read VBUS(typeC) information
     */
    float getVBUSVoltage();
    float getVBUSCurrent();
    float getVBUSPower();

    /**
     * @brief Read VIN information
     */
    float getVINVoltage();
    float getVINCurrent();
    float getVINPower();

    /**
     * @brief Read Internal Temperature
     */
    float getInternalTemperature();

    /**
     * @brief Key press handling
     */
    void shortPressKey();
    void longPressKey();
    void doublePressKey();
    void resetWholeSystem();


public:
    /**
     * @enum 
     */
    enum class NTC_STATE_FLAG {
        NORMAL_TEMPERATURE = 0,
        HIGH_TEMPERATURE,
        MEDIUM_HIGH_TEMPERATURE,
        MEDIUM_LOW_TEMPERATURE,
        LOW_TEMPERATURE,
    };

    enum class CHARGE_STATUS {
        NOT_CHARGING,
        TRICKLE_CHARGE,
        CONSTANT_CURRENT,
        CONSTANT_VOLTAGE,
        FULLY_CHARGED,
        CHARGE_TIMEOUT,
    };

    enum class SYSTEM_MODE {
        STANDBY,
        BOOST_START_DELAY,
        BOOST_ACTIVE,
        SHUTDOWN_DELAY,
        CHARGE_START_DELAY,
        CHARGE_ACTIVE,
        CHARGE_TO_BOOST_DELAY,
        CHARGE_TO_BOOST_WAIT,
    };

    enum class FAST_CHARGE_PROTOCOL {
        NONE = 0,
        DCP,
        QC_HANDSHAKE_OK,
        QC2,
        QC3,
        FCP_SCP,
        AFC,
        SFCP,
        PD,
        PPS,
    };

    enum class KEY_PRESS {
        NO_PRESS,
        SHORT_PRESS,
        LONG_PRESS,
        DOUBLE_PRESS
    };

    enum class PORT_STATE {
        NONE = 0,
        SINK,
        SOURCE,
    };


    /**
     * 
     */
    NTC_STATE_FLAG getNtcState();

    String protocolToString(FAST_CHARGE_PROTOCOL protocol);
    FAST_CHARGE_PROTOCOL convertValueToProtocol(uint8_t value);

    FAST_CHARGE_PROTOCOL getVBUSSourceProtocol();
    FAST_CHARGE_PROTOCOL getUSB1SourceProtocol();
    FAST_CHARGE_PROTOCOL getUSB2SourceProtocol();

    PORT_STATE getVBUSPortStatus();
    PORT_STATE getUSB2PortStatus();//

    CHARGE_STATUS getChargeStatus();

    

};




#endif // IP5356_H