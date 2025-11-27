# Client-Side Rendering: Decoding & Foveated Spatial Decompression (FSD) (Sec 3.4)

This document explains how the client (VR headset) handles the incoming compressed video stream to reconstruct the original game frame.

## Concept

The server sends a video stream that has been:
1.  **Spatially Compressed (FSC):** Warped to prioritize the foveal region.
2.  **Quantization Optimized (FVE):** Encoded with varying quality (QP) based on gaze.

The client needs to decode this stream and then reverse the spatial compression to display the correct image to the user. This reverse process is called **Foveated Spatial Decompression (FSD)**.

## 1. Video Decoding

The client uses a standard hardware decoder (e.g., MediaCodec on Android) to decode the H.264/HEVC stream.
*   **QP Agnostic:** The decoder does not need to know about the Gaze-Driven QP map used on the server. It simply decodes the macroblocks as per the standard video spec.
*   **Output:** The output of the decoder is the **FSC Frame** (the warped image).

## 2. Foveated Spatial Decompression (FSD)

To display the image correctly in the headset, the FSC frame must be unwarped back to the original game frame dimensions and geometry.

### Mechanism
The server injects the `centerShiftX` and `centerShiftY` values used for that specific frame into the **Video Packet Header**.

1.  **Header Extraction:**
    In `alvr/client_core/src/connection.rs`, the client reads the packet header.
    ```rust
    stats.report_frame_fr_shift(header.timestamp, header.centerShiftX, header.centerShiftY);
    ```

2.  **Shader Re-initialization:**
    The rendering pipeline (`alvr/client_core/cpp/ffr.cpp`) receives the new center shift values.
    *   `FFR::Reinit` is called with the new shift coordinates.
    *   It recalculates the foveation variables (boundaries, scales).
    *   It rebuilds the OpenGL shader string (`DECOMPRESS_AXIS_ALIGNED_FRAGMENT_SHADER`) to match the server's compression parameters.

3.  **Rendering:**
    The fragment shader samples the decoded FSC texture and maps the pixels back to their original screen-space positions, effectively "stretching" the compressed peripheral regions back to their correct size.

    *   **Foveal Region:** Preserves high fidelity.
    *   **Peripheral Region:** Stretched, resulting in lower effective resolution (which matches the lower acuity of peripheral vision).

## Visual Result
The final rendered image on the headset display is geometrically correct but has varying resolution density across the field of view, matching the user's gaze.

