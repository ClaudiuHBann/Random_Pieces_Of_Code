// The approximative ratios between the client window rect and the accept button rect

// As an example the client window will have the resolution 1366 x 768
// then the accept button will be positioned on 1366 * BUTTON_RATIO_POS_X and 768 * BUTTON_RATIO_POS_Y
// with a size of 1366 * BUTTON_RATIO_SIZE_W and 768 * BUTTON_RATIO_SIZE_H and the result are
//
// posApprox = (607, 290) -> posActual = (600, 290)
// sizeApprox = (160, 60) -> sizeActual = (165, 60)

#define BUTTON_RATIO_POS_X (0.444f)
#define BUTTON_RATIO_POS_Y (0.377f)

#define BUTTON_RATIO_SIZE_W (0.1171f)
#define BUTTON_RATIO_SIZE_H (0.078f)
