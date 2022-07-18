
#define BAR_REGISTER 1

#define FIFO_SIZE 0x800  /* FIFO Almost Full IRQ Threshold value (0 < FAF <= 0xFFF */
#define SAMPLES_PER_FIFO_ENTRY   2
#define BYTES_PER_FIFO_ENTRY     (4 * SAMPLES_PER_FIFO_ENTRY)
#define BYTES_PER_TRANSFER       (FIFO_SIZE * BYTES_PER_FIFO_ENTRY)
#define SAMPLES_PER_TRANSFER     (FIFO_SIZE * SAMPLES_PER_FIFO_ENTRY)
#define AdcBaseClock 10000000

/* Hardware registers */
#define ofsReset                0x00
    #define bmResetEverything       (1 << 0)
    #define bmResetAdc              (1 << 1)
    #define bmResetDac              (1 << 2)
    #define bmResetDio              (1 << 3)

#define ofsAdcRange             0x01 /* per-channel-group range is ofsAdcRange + channel group */
    #define bmGainX2                (1 << 1)
    #define bmGainX5                (1 << 2)
    #define bmBipolar               (1 << 0)
    #define bmUnipolar              (0 << 0)
    #define bmSingleEnded           (1 << 3)
    #define bmDifferential          (0 << 3)
    #define bmAdcRange_u10V         (bmUnipolar)
    #define bmAdcRange_b10V         (bmBipolar)
    #define bmAdcRange_u5V          (bmGainX2|bmUnipolar)
    #define bmAdcRange_b5V          (bmGainX2|bmBipolar)
    #define bmAdcRange_u2V          (bmGainX5|bmUnipolar)
    #define bmAdcRange_b2V          (bmGainX5|bmBipolar)
    #define bmAdcRange_u1V          (bmGainX2|bmGainX5|bmUnipolar)
    #define bmAdcRange_b1V          (bmGainX2|bmGainX5|bmBipolar)

#define ofsAdcCalibrationMode   0x11
    #define bmCalibrationOff        (0 << 0)
    #define bmCalibrationOn         (1 << 0)
    #define bmCalibrateVRef         (1 << 1)
    #define bmCalibrateGnd          (0 << 1)
    #define bmCalibrateBip          (1 << 2)
    #define bmCalibrateUnip         (0 << 2)

#define ofsAdcTriggerOptions    0x12
    #define bmAdcTriggerSoftware    (0b00 << 0)
    #define bmAdcTriggerTimer       (0b01 << 0)
    #define bmAdcTriggerExternal    (0b11 << 1)
    #define bmAdcTriggerTypeChannel (0 << 2)
    #define bmAdcTriggerTypeScan    (1 << 2)
    #define bmAdcTriggerEdgeRising  (0 << 3)
    #define bmAdcTriggerEdgeFalling (1 << 3)

#define ofsAdcStartChannel      0x13
#define ofsAdcStopChannel       0x14
#define ofsAdcOversamples       0x15
#define ofsAdcSoftwareStart     0x16
#define ofsAdcCrossTalkFeep     0x17
    #define bmAdcCrossTalk400k      (1 << 0)
    #define bmAdcCrossTalk500k      (0 << 0)

#define ofsAdcRateDivisor       0x18
#define ofsAdcDataFifo          0x1C
    #define bmAdcDataInvalid        (1 << 31)
    #define bmAdcDataChannelMask    (0x7F << 20)
    #define bmAdcDataGainMask       (0xF << 27)
    #define bmAdcDataMask           (0xFFFF)

#define ofsAdcFifoIrqThreshold  0x20
#define ofsAdcFifoCount         0x24
#define ofsIrqEnables           0x28
    #define bmIrqDmaDone            (1 << 0)
    #define bmIrqFifoAlmostFull     (1 << 1)
    #define bmIrqEvent              (1 << 3)

#define ofsIrqStatus_Clear      0x2C
//  #define bmIrqDmaDone            (1 << 0)
//  #define bmIrqFifoAlmostFull     (1 << 1)
//  #define bmIrqEvent              (1 << 3)

#define ofsDac                  0x30
#define ofsDacSleep             0x34
#define ofsDioDirections        0x3C
    #define bmDioInput              (1) // bit 0 is DIO#0, 1 is DIO#1 etc
    #define bmDioOutput             (0)

#define ofsDioOutputs           0x40
#define ofsDioInputs            0x44

#define ofsSubMuxSelect         0x9C    // used to override autodetect
    #define bmNoSubMux              0x03
    #define bmAIMUX64M              0x00
    #define bmAIMUX32               0x01

#define ofsFPGARevision         0xA0    // FPGA revision reports as 0xMMMMmmmm where M is major, and m is minor revision
#define ofsAutodetectPattern    0xA4
#define ofsDeviceID             0xA8

/* Flash is currently factory-use only and can out-of-warranty brick your unit */
#define ofsFlashAddress         0x70
#define ofsFlashData            0x74
#define ofsFlashErase           0x78

#define ofsAdcCalScale1         0xC0
#define ofsAdcCalOffset1        0xC4
#define ofsAdcCalScale2         0xC8
#define ofsAdcCalOffset2        0xCC
#define ofsAdcCalScale3         0xD0
#define ofsAdcCalOffset3        0xD4
#define ofsAdcCalScale4         0xD8
#define ofsAdcCalOffset4        0xDC

#define BAR_DMA 0

#define ofsDmaAddr32            0x0
#define ofsDmaAddr64            0x4
#define ofsDmaSize              0x8
#define ofsDmaControl           0xc
    #define DmaStart            0x4
    #define DmaAbortClear       0x8
    #define DmaEnableSctrGthr   0x10
