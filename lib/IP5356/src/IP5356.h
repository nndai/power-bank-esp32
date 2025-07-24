#ifndef IP5356_H
#define IP5356_H

#include <Wire.h>
#include <log.h>

/**
 * @brief Registers for I2C address 0x74
 */
enum IP5356_0X74_REG {
    SYS_CTL0 = 0x00,              // Section 4.1 . REG 0x00 | R/W: Boost and charger enable register
    SYS_CTL1 = 0x03,              // Section 4.2 . REG 0x03 | R/W: Light load shutdown control register
    CHG_CTL0 = 0x0B,              // Section 4.3 . REG 0x0B | R/W: 12V input charging undervoltage loop voltage control
    CHG_CTL1 = 0x0C,              // Section 4.4 . REG 0x0C | R/W: 9V input charging undervoltage loop voltage control
    CHG_CTL2 = 0x0D,              // Section 4.5 . REG 0x0D | R/W: 5V input charging undervoltage loop voltage control
    SYS_CTL2_KEY = 0x0F,          // Section 4.6 . REG 0x0F | R/W: Key control for lighting and low power modes
    VOUT1_CTL0 = 0x10,            // Section 4.7 . REG 0x10 | R/W: VOUT1 control register
    VOUT1_CTL1 = 0x13,            // Section 4.8 . REG 0x13 | R/W: VOUT1 light load shutdown time setting
    VOUT2_CTL0 = 0x14,            // Section 4.10. REG 0x14 | R/W: VOUT2 control register
    VOUT2_CTL1 = 0x17,            // Section 4.11. REG 0x17 | R/W: VOUT2 light load shutdown time setting
    VBUS_CTL0 = 0x18,             // Section 4.13. REG 0x18 | R/W: VBUS control register
    VBUS_CTL1 = 0x1B,             // Section 4.14. REG 0x1B | R/W: VBUS light load shutdown time setting
    VOUT1_CTL2 = 0x1C,            // Section 4.9 . REG 0x1C | R/W: VOUT1 DM DP load detection enable
    VOUT2_CTL2 = 0x1D,            // Section 4.12. REG 0x1D | R/W: VOUT2 DM DP load detection enable
    VBUS_CTL2 = 0x1E,             // Section 4.15. REG 0x1E | R/W: VBUS DM DP load detection enable
    PPATH_CTL0 = 0x24,            // Section 4.16. REG 0x24 | R/W: VBUS 5V charge-discharge path control
    SYS_CTL3_SINK_QC = 0x25,      // Section 4.18. REG 0x25 | R/W: Input fast charge enable (VBUS/VIN)
    CHG_CTL3_PPATH_ISET = 0x26,   // Section 4.19. REG 0x26 | R/W: Charge-discharge path input current setting
    CHG_CTL4_VIN12V_ISET = 0x27,  // Section 4.20. REG 0x27 | R/W: 12V VIN input current setting
    CHG_CTL5_VIN5V_ISET = 0x28,   // Section 4.21. REG 0x28 | R/W: 5V VIN input current setting
    CHG_CTL6_VBUS5V_ISET = 0x29,  // Section 4.22. REG 0x29 | R/W: 5V VBUS input current setting
    CHG_CTL7_VIN9V_ISET = 0x2B,   // Section 4.23. REG 0x2B | R/W: 9V VIN input current setting
    SYS_CTL4_KEY_SHUTDOWN = 0x31, // Section 4.26. REG 0x31 | R/W: Key shutdown settings
    SYS_CTL5_LOWCUR_VSET = 0x33,  // Section 4.27. REG 0x33 | R/W: Constant-on N hours and VSET detection enable
    BATLOW_ADC_TH = 0x41,         // Section 4.28. REG 0x41 | R/W: Low battery shutdown ADC threshold setting
    POW_LOW_TH = 0x44,            // Section 4.29. REG 0x44 | R/W: Light load shutdown power threshold setting
    ISYS_LOW_TH = 0x45,           // Section 4.30. REG 0x45 | R/W: Light load shutdown current threshold setting
    VOUT1_IMOSLOW_TH = 0x49,      // Section 4.31. REG 0x49 | R/W: VOUT1 port light load MOS shutdown current threshold
    VOUT2_IMOSLOW_TH = 0x4A,      // Section 4.32. REG 0x4A | R/W: VOUT2 port light load MOS shutdown current threshold
    VBUS_IMOSLOW_TH = 0x4B,       // Section 4.33. REG 0x4B | R/W: VBUS port light load MOS shutdown current threshold
    FCAP_SET = 0x4C,              // Section 4.34. REG 0x4C | R/W: Fuel gauge total capacity setting
    MFP_CTL0_LED4_LED5 = 0x65,    // Section 4.35. REG 0x65 | R/W: LED4/LED5 GPIO function selection
    MFP_CTL1_LED6 = 0x66,         // Section 4.36. REG 0x66 | R/W: LED6 GPIO function selection
    CHG_CTL8_VBUS12V_ISET = 0x6E, // Section 4.24. REG 0x6E | R/W: 12V VBUS input current setting
    CHG_CTL9_VBUS9V_ISET = 0x6F,  // Section 4.25. REG 0x6F | R/W: 9V VBUS input current setting
    EN_FCAP_EXT = 0x78,           // Section 4.37. REG 0x78 | R/W: External resistor for capacity setting enable
    SINK_QC_EN = 0x81,            // Section 4.40. REG 0x81 | R/W: Input fast charge protocol enable
    LINE_COMP_CTL = 0x84,         // Section 4.41. REG 0x84 | R/W: Line compensation and Apple DCP mode enable
    SRC_QC_EN = 0x85,             // Section 4.42. REG 0x85 | R/W: Output fast charge protocol enable
    SRC_QC_EN2 = 0x86,            // Section 4.43. REG 0x86 | R/W: Output DCP mode setting during charge-discharge
    BST_VSET_H = 0xAA,            // Section 4.44. REG 0xAA | R/W: Boost output voltage setting (high bits)
    BST_VSET_L = 0xAB,            // Section 4.45. REG 0xAB | R/W: Boost output voltage setting (low bits)
    BST_5V_VSET = 0xAD,           // Section 4.46. REG 0xAD | R/W: 5V output voltage setting
    BST_12V_9V_VSET = 0xAE,       // Section 4.47. REG 0xAE | R/W: 9V/12V output voltage setting
    VOUT1_5V_ISET = 0xB1,         // Section 4.48. REG 0xB1 | R/W: VOUT1 5V output current limit setting
    VOUT1_9V_ISET = 0xB3,         // Section 4.49. REG 0xB3 | R/W: VOUT1 9V output current limit setting
    VOUT1_12V_ISET = 0xB4,        // Section 4.50. REG 0xB4 | R/W: VOUT1 12V output current limit setting
    VOUT2_5V_ISET = 0xB5,         // Section 4.51. REG 0xB5 | R/W: VOUT2 5V output current limit setting
    VOUT2_9V_ISET = 0xB7,         // Section 4.52. REG 0xB7 | R/W: VOUT2 9V output current limit setting
    VOUT2_12V_ISET = 0xB8,        // Section 4.53. REG 0xB8 | R/W: VOUT2 12V output current limit setting
    VBUS_5V_ISET = 0xB9,          // Section 4.54. REG 0xB9 | R/W: VBUS 5V output current limit setting
    VBUS_9V_ISET = 0xBB,          // Section 4.55. REG 0xBB | R/W: VBUS 9V output current limit setting
    VBUS_12V_ISET = 0xBC,         // Section 4.56. REG 0xBC | R/W: VBUS 12V output current limit setting
    TYPEC_CTL0 = 0xD1,            // Section 4.57. REG 0xD1 | R/W: Type-C CC mode selection
    TYPEC_CTL1 = 0xD5,            // Section 4.58. REG 0xD5 | R/W: PD sink maximum input voltage setting
    PPATH_CTL1 = 0xF5,            // Section 4.17. REG 0xF5 | R/W: VIN 5V charge-discharge path control
    NTC_CTL0_ISET = 0xF6,         // Section 4.38. REG 0xF6 | R/W: NTC pin current source selection
    NTC_CTL1_EN = 0xFD,           // Section 4.39. REG 0xFD | R/W: NTC function enable/disable control
};


/**
 * @brief Registers for I2C address 0x75
 */
enum IP5356_0X75_REG {
    VIN_VBUS_OV_CTL = 0x01,       // Section 4.59. REG 0x01 | R/W: VIN/VBUS input overvoltage setting
    BATLOW_SET = 0x03,            // Section 4.60. REG 0x03 | R/W: Analog low battery shutdown voltage setting
    GPIO_20UA_EN_CTL = 0x19,      // Section 4.61. REG 0x19 | R/W: GPIO 20uA current output enable
    VOUT2_PD_CTL = 0x1F,          // Section 4.62. REG 0x1F | R/W: VOUT2 PD output CL pin pull-up enable
    IC_TEMP_LOOP_SET = 0x27,      // Section 4.63. REG 0x27 | R/W: IC internal temperature loop setting
    LINE_COMP_SET = 0x30,         // Section 4.64. REG 0x30 | R/W: Line compensation voltage selection
    CHG_CTL10_CV_VSET = 0x3A,     // Section 3.3, 4.65. REG 0x3A | R/W: Constant voltage charging voltage setting
    CHG_CTL11_ISTOP1 = 0x3B,      // Section 4.66. REG 0x3B | R/W: 5V input charge termination current setting
    CHG_CTL12_ISTOP2 = 0x42,      // Section 4.67. REG 0x42 | R/W: >5V input charge termination current setting
    BATVADC_DAT0 = 0x50,          // Section 4.1. REG 0x50 | R: VBAT voltage ADC data low 8 bits
    BATVADC_DAT1 = 0x51,          // Section 4.2. REG 0x51 | R: VBAT voltage ADC data high 8 bits
    VSYSVADC_DAT0 = 0x52,         // Section 4.3. REG 0x52 | R: VSYS voltage ADC data low 8 bits
    VSYSVADC_DAT1 = 0x53,         // Section 4.4. REG 0x53 | R: VSYS voltage ADC data high 8 bits
    I_IN_ADC_DAT0 = 0x54,         // Section 4.5. REG 0x54 | R: Input current (VIN/VBUS) ADC data low 8 bits
    I_IN_ADC_DAT1 = 0x55,         // Section 4.6. REG 0x55 | R: Input current (VIN/VBUS) ADC data high 8 bits
    IOUT1_ADC_DAT0 = 0x56,        // Section 4.7. REG 0x56 | R: VOUT1 output current ADC data low 8 bits
    IOUT1_ADC_DAT1 = 0x57,        // Section 4.8. REG 0x57 | R: VOUT1 output current ADC data high 8 bits
    IOUT2_ADC_DAT0 = 0x58,        // Section 4.9. REG 0x58 | R: VOUT2 output current ADC data low 8 bits
    IOUT2_ADC_DAT1 = 0x59,        // Section 4.10. REG 0x59 | R: VOUT2 output current ADC data high 8 bits
    IBUS_OUT_ADC_DAT0 = 0x5A,     // Section 4.11. REG 0x5A | R: VBUS output current ADC data low 8 bits
    IBUS_OUT_ADC_DAT1 = 0x5B,     // Section 4.12. REG 0x5B | R: VBUS output current ADC data high 8 bits
    VINVADC_DAT0 = 0x5C,          // Section 4.13. REG 0x5C | R: VIN voltage ADC data low 8 bits
    VINVADC_DAT1 = 0x5D,          // Section 4.14. REG 0x5D | R: VIN voltage ADC data high 8 bits
    VOUT1VADC_DAT0 = 0x5E,        // Section 4.15. REG 0x5E | R: VOUT1 voltage ADC data low 8 bits
    VOUT1VADC_DAT1 = 0x5F,        // Section 4.16. REG 0x5F | R: VOUT1 voltage ADC data high 8 bits
    VOUT2VADC_DAT0 = 0x60,        // Section 4.17. REG 0x60 | R: VOUT2 voltage ADC data low 8 bits
    VOUT2VADC_DAT1 = 0x61,        // Section 4.18. REG 0x61 | R: VOUT2 voltage ADC data high 8 bits
    VBUSVADC_DAT0 = 0x62,         // Section 4.19. REG 0x62 | R: VBUS voltage ADC data low 8 bits
    VBUSVADC_DAT1 = 0x63,         // Section 4.20. REG 0x63 | R: VBUS voltage ADC data high 8 bits
    NTCVADC_DAT0 = 0x64,          // Section 3.6, 4.21. REG 0x64 | R: NTC voltage ADC data low 8 bits
    NTCVADC_DAT1 = 0x65,          // Section 3.6, 4.22. REG 0x65 | R: NTC voltage ADC data high 8 bits
    LED4VADC_DAT0 = 0x68,         // Section 4.23. REG 0x68 | R: LED4 voltage ADC data low 8 bits
    LED4VADC_DAT1 = 0x69,         // Section 4.24. REG 0x69 | R: LED4 voltage ADC data high 8 bits
    LED5VADC_DAT0 = 0x6A,         // Section 4.25. REG 0x6A | R: LED5 voltage ADC data low 8 bits
    LED5VADC_DAT1 = 0x6B,         // Section 4.26. REG 0x6B | R: LED5 voltage ADC data high 8 bits
    LED6VADC_DAT0 = 0x6C,         // Section 4.27. REG 0x6C | R: LED6 voltage ADC data low 8 bits
    LED6VADC_DAT1 = 0x6D,         // Section 4.28. REG 0x6D | R: LED6 voltage ADC data high 8 bits
    IBAT_ADC_DAT0 = 0x6E,         // Section 4.29. REG 0x6E | R: Battery current ADC data low 8 bits
    IBAT_ADC_DAT1 = 0x6F,         // Section 4.30. REG 0x6F | R: Battery current ADC data high 8 bits
    ISYS_ADC_DAT0 = 0x70,         // Section 4.31. REG 0x70 | R: System current ADC data low 8 bits
    ISYS_ADC_DAT1 = 0x71,         // Section 4.32. REG 0x71 | R: System current ADC data high 8 bits
    VSYS_POW_ADC_DAT0 = 0x74,     // Section 4.33. REG 0x74 | R: VSYS power ADC data low 8 bits
    VSYS_POW_ADC_DAT1 = 0x75,     // Section 4.34. REG 0x75 | R: VSYS power ADC data high 8 bits
    SOC_CAP_DATA = 0x7B,          // Section 3.2, 4.35. REG 0x7B | R: State of Charge (SoC) percentage data
    FORCE_STANDBY = 0x86,         // Section 3.4, 4.37. REG 0x86 | R/W: Force IP5356 to enter standby/sleep mode
    SOC_CAP_SET = 0x87,           // Section 4.36. REG 0x87 | R/W: State of Charge (SoC) percentage control
    STATE_CTL0_SINK_DCP = 0x90,   // Section 4.38. REG 0x90 | R/W: Input sink DCP fast charge control
    STATE_CTL1_SINK_PD = 0x91,    // Section 3.4, 4.39. REG 0x91 | R/W: Input sink PD fast charge control
    STATE_CTL2_CHG_PATH = 0x92,   // Section 4.40. REG 0x92 | R/W: Input charging path selection
    STATE_CTL3_SRC_PD = 0x97,     // Section 3.4, 4.41. REG 0x97 | R/W: VBUS PD source fast charge control
    FCP_STATUS = 0xA1,            // Section 4.42. REG 0xA1 | R: Output FCP protocol voltage status
    STATUS_SRC0 = 0xA4,           // Section 4.43. REG 0xA4 | R: VOUT1/VOUT2 output fast charge protocol status
    STATUS_SRC1 = 0xA5,           // Section 4.44. REG 0xA5 | R: VBUS output fast charge protocol status
    STATUS_SRC2 = 0xA8,           // Section 4.45. REG 0xA8 | R: Low voltage fast charge status
    AFC_STATUS = 0xAF,            // Section 4.46. REG 0xAF | R: Output AFC protocol voltage status
    PD_STATE0_SINK_OK = 0xB1,     // Section 4.47. REG 0xB1 | R: PD sink connection status
    PD_STATE1_SRC_OK = 0xC2,      // Section 4.48. REG 0xC2 | R: PD source connection status
    PD_STATE2_PPS_OK = 0xC3,      // Section 4.49. REG 0xC3 | R: PPS source connection status
    SYS_STATE0 = 0xC4,            // Section 3.1, 4.50. REG 0xC4 | R: System status (VINOK, VBUSOK, VBUSOV)
    SYS_STATE1 = 0xC5,            // Section 4.51. REG 0xC5 | R: System status (BATLOW, VSYS_OV, VIN_OV)
    SYS_STATE2 = 0xCD,            // Section 4.52. REG 0xCD | R: Output fast charge active status
    SYS_STATE3 = 0xD0,            // Section 3.1, 4.53. REG 0xD0 | R: System state machine status
    SYS_STATE4 = 0xD4,            // Section 4.54. REG 0xD4 | R: Input charge fast charge state
    LOWCUR_STATE = 0xE1,          // Section 4.55. REG 0xE1 | R: Constant-on N hours mode status
    CHG_STATE1 = 0xE8,            // Section 4.56. REG 0xE8 | R: Charging voltage and input MOS status
    CHG_STATE2 = 0xE9,            // Section 3.1, 4.57. REG 0xE9 | R: Charging process state (trickle, CC, CV)
    MOS_STATE = 0xEB,             // Section 4.58. REG 0xEB | R: Output MOS status
    ILOW_STATE = 0xF2,            // Section 4.59. REG 0xF2 | R: System light load status (current/power)
    TYPEC_STATE = 0xF3,           // Section 4.60. REG 0xF3 | R: Type-C connection state (SRC/SINK)
    KEYIN_STATE = 0xF4,           // Section 4.61. REG 0xF4 | R/W: Key press status (short, long, double)
    NTC_STATE = 0xFB,             // Section 4.62. REG 0xFB | R: NTC temperature and output MOS current status
    OCP_STATE = 0xFC,             // Section 4.63. REG 0xFC | R/W: Overcurrent and short-circuit status
};


/**
 *
 *
 */
class IP5356 {
private:
    static IP5356* _ip5356;
    FileLogger* _log = nullptr;

    IP5356(const IP5356&) = delete;
    IP5356& operator=(const IP5356&) = delete;

    IP5356();

    /**
        * @brief Read 1 byte data from a specific register of a given I2C address.
        * @param addr The I2C address to read from.
        * @param reg The register to read from.
        * @param data Pointer to the buffer where the read data will be stored.
    */
    void _read(uint8_t addr, uint8_t reg, uint8_t* data);

    /**
    * @brief Read 2 byte data from a specific register of a given I2C address.
    * @param addr The I2C address to read from.
    * @param reg The register to read from.
    * @param data Pointer to the buffer where the read data will be stored.
    */
    void _read(uint8_t addr, uint8_t reg, uint16_t* data);

    /**
        * @brief Write 1 byte data to a specific register of a given I2C address.
        * @param addr The I2C address to write to.
        * @param reg The register to write to.
        * @param data Pointer to the data to be written.
    */
    void _write(uint8_t addr, uint8_t reg, uint8_t* data);

    /// Overloaded read functions for specific registers
    void _read(IP5356_0X74_REG reg, uint8_t* data);
    void _read(IP5356_0X74_REG reg, uint16_t* data);
    void _read(IP5356_0X75_REG reg, uint8_t* data);
    void _read(IP5356_0X75_REG reg, uint16_t* data);

    /// Overloaded write functions for specific registers
    void _write(IP5356_0X74_REG reg, uint8_t* data);
    void _write(IP5356_0X75_REG reg, uint8_t* data);


public:
    static IP5356* getInstance();
    void begin();

    uint8_t getPercentage();
};




#endif // IP5356_H