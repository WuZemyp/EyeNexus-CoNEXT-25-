# Dynamic Foveated Spatial Compression (FSC) (Sec 3.2.2)

This document explains the Dynamic Foveated Spatial Compression (FSC) algorithm, which adapts the video encoding to the user's real-time eye gaze location.

## Concept

Standard FSC algorithms assume the user is always looking at the center of the screen. In VR, especially in non-first-person views, this assumption is incorrect. 
Dynamic FSC moves the high-quality "foveal" region to match the user's gaze $(X_o, Y_o)$, applying spatial compression (downsampling) only to the peripheral areas.

## Mathematical Model

### FSC Frame Dimensions

We first compute the dimensions of the compressed FSC frame ($W_r, H_r$) based on the original game frame ($W_o, H_o$), the size of the foveal Region of Interest ($X_{size}, Y_{size}$), and the compression ratios ($X_{comp}, Y_{comp}$).

$$
W_r = W_o \left( X_{size} + \frac{1 - X_{size}}{X_{comp}} \right)
$$

$$
H_r = H_o \left( Y_{size} + \frac{1 - Y_{size}}{Y_{comp}} \right)
$$

### Coordinate Mapping

For every pixel ($i, j$) in the compressed FSC frame, we map it to a corresponding source pixel ($i', j'$) in the original game frame.

The mapping logic for the X-axis is defined as follows (Y-axis is analogous):

1.  **Calculate Boundaries:**
    $$bound_{left} = \frac{2(1-X_{size})}{(X_{comp}-1)X_{size}+1} \cdot \frac{X_o}{W_o}$$
    $$bound_{right} = \frac{1-X_{size}}{X_{size}(X_{comp}-1)+1} \cdot \frac{X_o-W_o}{W_o} + 1$$

2.  **Mapping Logic:**
    *   **Left Periphery ($i < bound_{left}$):**
        $$i' = X_{comp} \cdot i$$
    *   **Right Periphery ($i > bound_{right}$):**
        $$c_p = (1 - X_{comp}) \cdot X_{size}$$
        $$i' = X_{comp} \cdot i + W_o \cdot c_p$$
    *   **Foveal Region (Center):**
        $$c_f = \frac{X_{comp}-1}{X_{comp}} \cdot (1-X_{size}) \cdot \left(\frac{X_o}{W_o}\right)$$
        $$i' = i + c_f \cdot W_o$$

## Implementation

The logic is implemented across three C++ files:

1.  **`CEncoder.cpp`**:
    *   Retrieves the latest gaze coordinates.
    *   Calculates the normalized center shift $(centerShiftX, centerShiftY)$.
    *   Updates the render pipeline every $N$ frames (controlled by `FSC_frequency`).

2.  **`FrameRender.cpp`**:
    *   Acts as a bridge, calling the `Reinit` method of the FFR (Fixed Foveated Rendering) controller.

3.  **`FFR.cpp`**:
    *   Recalculates the foveation variables (`CalculateFoveationVars`).
    *   Rebuilds the DirectX shader pipeline with the new compression parameters to warp the image centered on the gaze point.

