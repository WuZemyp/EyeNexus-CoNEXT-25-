# Gaze Mapping: Screen to FSC Frame Coordinates (Sec 3.3.1)

This document explains the logic for mapping the gaze point from Screen Space Coordinates to FSC Frame Coordinates, which is necessary for accurate Gaze-Driven Video Encoding.

## Concept

Before video encoding, the frame undergoes Dynamic Foveated Spatial Compression (FSC), which warps the image to allocate more resolution to the foveal region. Because the frame dimensions and pixel distribution change, the gaze point $(X_o, Y_o)$ in the original screen space no longer corresponds to the foveal center in the compressed FSC frame.

To perform accurate non-uniform quantization (Gaze-Driven Video Encoding), we must find the gaze point $(X_r, Y_r)$ within the FSC frame. This is achieved by applying the inverse of the FSC mapping function, known as Foveated Spatial Decompression (FSD).

## Mathematical Model

We define the Foveated Spatial Decompression (FSD) function as the inverse of the FSC function:

$$
FSC(X_r, Y_r) = (X_o, Y_o) \quad \Rightarrow \quad (X_r, Y_r) = FSD(X_o, Y_o)
$$

The implementation uses a numerical inverse or piecewise inverse function to map the original screen coordinates $(X_o, Y_o)$ back to the compressed frame coordinates $(X_r, Y_r)$.

## Implementation

This logic is implemented in `alvr/server/cpp/platform/win32/NvEncoder.cpp` within the `GenQPDeltaMap` function.

1.  **Update Decompression Parameters:**
    The function `Update_decompress_params` calculates the boundaries and coefficients ($c_1, c_2$, etc.) required for the inverse mapping based on the current foveation center shift.

2.  **Project Gaze Point:**
    The functions `decompress_x` and `decompress_y` take the screen coordinates $(X_o, Y_o)$ and return the corresponding coordinates $(X_r, Y_r)$ in the FSC frame.

    ```cpp
    int r_leftX = (decompress_x(leftX) + 15) / 16;
    int r_leftY = (decompress_y(leftY) + 15) / 16;
    ```
    *Note: The result is converted to macroblock coordinates (divided by 16) for use in the QP map generation.*

