# Gaze Mapping: VR to Screen Space Coordinates (Sec 3.2.1)

This document explains the mathematical model used to map eye gaze from the VR 3D space to the 2D screen space coordinates used for Foveated Spatial Compression (FSC).

## Concept

The VR headset tracks the user's eye gaze, providing the orientation as Yaw ($\theta_{yaw}$) and Pitch ($\theta_{pitch}$). To optimize video encoding, we need to determine where this gaze vector intersects with the 2D video frame being rendered.

Since the video frame corresponds to the user's field of view (FOV), we can project the gaze angles onto the 2D plane using the FOV tangents.

## Mathematical Model

The mapping converts the spherical gaze direction into normalized device coordinates (NDC) and then to pixel coordinates $(X_o, Y_o)$.

### Formula

$$
X_o = \frac{(\tan(|\text{fov}_{left}|) - \tan(\theta_{yaw})) \cdot W_o}{\tan(|\text{fov}_{left}|) + \tan(|\text{fov}_{right}|)}
$$

$$
Y_o = \frac{(\tan(|\text{fov}_{up}|) - \tan(\theta_{pitch})) \cdot H_o}{\tan(|\text{fov}_{up}|) + \tan(|\text{fov}_{down}|)}
$$

Where:
*   $W_o, H_o$: Width and Height of the eye's frame.
*   $\text{fov}_{left}, \text{fov}_{right}, \text{fov}_{up}, \text{fov}_{down}$: The field of view angles for the specific eye.
*   $\theta_{yaw}, \theta_{pitch}$: The gaze orientation angles.

### Coordinate Systems

1.  **Yaw ($\theta_{yaw}$):** Positive values indicate looking to the Left (in the projection logic used).
2.  **Pitch ($\theta_{pitch}$):** Positive values indicate looking Up.
3.  **Screen Y ($Y_o$):**
    *   The Paper's formula calculates $Y_o$ where **0 is the Top edge**.
    *   The Rust implementation (`compute_eye_gaze_location`) calculates $Y_o$ where **0 is the Bottom edge** (equivalent to $\tan(\text{fov}_{down}) + \tan(\theta_{pitch})$).
    *   The C++ Video Encoder (`CEncoder.cpp`) expects Top-Left origin, so it flips the Y coordinate: `Final_Y = Height - Rust_Y`.

## Implementation

The function `compute_eye_gaze_location` in `alvr/server/src/connection.rs` implements this logic. It handles both left and right eyes, offsetting the right eye's X coordinate for Side-by-Side (SBS) rendering.

### Usage

This function is called within the tracking loop in `connection.rs`. It takes the raw gaze data received from the client, computes the screen coordinates, and passes them to the `BitrateManager` and the `FaceTrackingSink`.

