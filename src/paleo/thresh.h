#ifndef __paleo_thresh_h__
#define __paleo_thresh_h__

#define PALEO_THRESH_A   0.5      // Tail-removing thresh.
#define PALEO_THRESH_B   5.0      // Min points for tail removal.
#define PALEO_THRESH_C  70.0      // Min px_length for tail removal.
#define PALEO_THRESH_D   1.31     // Overtraced revolution percentage.
#define PALEO_THRESH_E   0.16     // Closedness dist/len ratio.
#define PALEO_THRESH_F   0.75     // Closedness min revolutions.
#define PALEO_THRESH_G   2.0      // Line seg straightness.
#define PALEO_THRESH_H  10.25     // Line FA/len max ratio.
#define PALEO_THRESH_I   0.0036   // Pline LSE max.
#define PALEO_THRESH_J   6.0      // Min DCR for Pline/Arc/Curve.
#define PALEO_THRESH_K   0.8      // Ellipse/Arc/Spiral NDDE min.
#define PALEO_THRESH_L  30.0      // Ellipse maj-axis len req.
#define PALEO_THRESH_M   0.33     // Max FA error for ellipse.
#define PALEO_THRESH_N  16.0      // Circle/Arc radius len req.
#define PALEO_THRESH_O   0.425    // Ellipse/Circle tie-breaker.
#define PALEO_THRESH_P   0.35     // Max FAE for Circle.
#define PALEO_THRESH_Q   0.4      // Max FAE for Arc.
#define PALEO_THRESH_R   0.37     // Max LSE for Bézier curves.
#define PALEO_THRESH_S   0.9      // Spiral avg / bbox r max.
#define PALEO_THRESH_T   0.25     // Spiral sub-center max difference.
#define PALEO_THRESH_U   0.2      // Spiral max ep_dist / px_len.
#define PALEO_THRESH_V   0.1
#define PALEO_THRESH_W   9.0
#define PALEO_THRESH_X  10.0
#define PALEO_THRESH_Y   0.99    // Corner detection.
#define PALEO_THRESH_Z   0.06    // Corner merge percentage.

#endif  // __paleo_thresh_h__
