# Gaze-Driven Video Encoding (QP Map Generation) (Sec 3.3)

This document explains the Gaze-Driven Video Encoding mechanism, which optimizes video bitrate by allocating visual quality based on the user's gaze.

## Concept

Traditional video encoding allocates bitrate uniformly or based on image complexity. EyeNexus uses **Non-uniform Quantization** aligned with the Human Visual System (HVS). We generate a Quantization Parameter (QP) map where:
*   **Foveal Region:** Low QP (high quality, low compression).
*   **Peripheral Region:** High QP (lower quality, high compression).

This is implemented using NVIDIA NVENC's `QP_MAP_DELTA` mode (or `CONSTQP` mode with offsets).

## Mathematical Model

### 1. Gaze to Macroblock Mapping
First, the gaze point $(X_r, Y_r)$ in the FSC frame is mapped to the macroblock coordinate system. Each macroblock is $16 \times 16$ pixels.

$$
X_{QP} = \lceil X_r / 16 \rceil, \quad Y_{QP} = \lceil Y_r / 16 \rceil
$$

### 2. Gaussian Quality Assignment
We model the quality falloff as a 2D Gaussian function centered at the gaze macroblock $(X_{QP}, Y_{QP})$. For any macroblock $(i, j)$, the Quantization Offset (QO) is calculated as:

$$
QO(i, j) = QO_{max} - QO_{max} \times \exp\left(-\frac{(Distance(i, j))^2}{2C^2}\right)
$$

Where:
*   $Distance(i, j) = \sqrt{(i - X_{QP})^2 + (j - Y_{QP})^2}$ is the Euclidean distance from the gaze center.
*   $QO_{max} = QP_{max} - QP_{const}$: The maximum quantization offset allowed.
*   $C$: The **Foveation Controller** parameter, which determines the spread of the Gaussian (width of the high-quality region).

## Implementation

The logic is located in `alvr/server/cpp/platform/win32/NvEncoder.cpp`:

*   **Function:** `GenQPDeltaMap`
*   **Process:**
    1.  Initializes the `qp_map` array.
    2.  Iterates through every macroblock $(i, j)$ in the frame.
    3.  Calls `EyeNexus_CalculateQPOffsetValue_leftEye` (and `_rightEye`) to compute the Gaussian-based QP offset.
    4.  Selects the minimum QP (best quality) if the macroblock falls within the influence of both eyes (relevant for stereo overlapping regions).
    5.  Passes the generated `qp_map` to the NVENC encoder via `NV_ENC_PIC_PARAMS`.

### Foveation Controller (C)
The parameter $C$ allows dynamic adjustment of the foveation area. A larger $C$ expands the high-quality region (beneficial for good network conditions), while a smaller $C$ shrinks it (saving bitrate under poor network conditions).

## Encoder Configuration

To enable foveated encoding, the NVENC encoder must be configured specifically to accept external QP maps and use a constant base QP. This configuration is handled in `alvr/server/cpp/platform/win32/VideoEncoderNVENC.cpp`.

### Key Settings

1.  **Rate Control Mode:** `NV_ENC_PARAMS_RC_CONSTQP`
    *   Disables automatic bitrate targeting.
    *   Uses a fixed base QP for the entire frame, which we then modify per-block.

2.  **QP Map Mode:** `NV_ENC_QP_MAP_DELTA`
    *   Tells the encoder to interpret the provided map as *offsets* (deltas) from the base QP, rather than absolute QP values.
    *   This allows for smoother gradients and easier relative quality control.

3.  **Base QP:** `rcParams.constQP = {23, 23, 23}`
    *   Sets the baseline quality (e.g., QP=23) for I, P, and B frames.
    *   The Gaussian function calculates positive offsets to *increase* QP (reduce quality) in the periphery relative to this base.

4.  **Disable Automatic Features:**
    *   `enableAQ = 0` (Spatial Adaptive Quantization disabled to avoid conflict with our map).
    *   `enableTemporalAQ = 0` (Temporal Adaptive Quantization disabled).

### Code Reference

```cpp
// EyeNexus-Gaze-Driven Video Encoding: Setup NVENC for Constant QP mode with Delta Map
encodeConfig.rcParams.qpMapMode = NV_ENC_QP_MAP_DELTA;
encodeConfig.rcParams.enableAQ = 0;
encodeConfig.rcParams.enableTemporalAQ = 0;
encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
encodeConfig.rcParams.constQP = {23, 23, 23}; 
```
