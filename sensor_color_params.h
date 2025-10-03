// Override color thresholds for sender. Placed at repo root.
// adjustParams.h will include this file if present.

// NOTE: The struct ColorThreshold is defined in adjustParams.h before this include.

const ColorThreshold RED_THD = {
    .rMin = 140, .rMax = 255, .gMin = 0, .gMax = 70, .bMin = 0, .bMax = 70};

const ColorThreshold BLUE_THD = {
    .rMin = 30, .rMax = 70, .gMin = 0, .gMax = 90, .bMin = 120, .bMax = 255};

const ColorThreshold YELLOW_THD = {
    .rMin = 100, .rMax = 159, .gMin = 50, .gMax = 100, .bMin = 0, .bMax = 60};


