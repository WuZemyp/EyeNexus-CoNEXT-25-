# Gaze-Contingent Rate Control (AIMD) (Sec 3.6)

EyeNexus uses a novel rate control mechanism that adjusts the **Foveation Controller ($C$)** instead of directly setting a bitrate target. This $C$ value controls the spread of the high-quality foveal region in the video encoder.

## Foveation Controller ($C$)

*   **High $C$:** Large foveal region (high quality, high bitrate).
*   **Low $C$:** Small foveal region (lower quality periphery, low bitrate).
*   **Range:** $C_{min}$ to $C_{max}$ (Resolution dependent).

## AIMD Algorithm

We use an **Additive Increase, Multiplicative Decrease (AIMD)** algorithm to adapt $C$ based on the network state detected by the Network Monitoring module.

### 1. Additive Increase (Normal / Underuse)
When the network is stable or underused (bandwidth is available), we slowly increase $C$ to improve visual quality.

$$
C_{new} = C_{old} + \alpha
$$

*   $\alpha = 0.1$ or $0.2$ (depending on link capacity estimate).

### 2. Multiplicative Decrease (Overuse)
When congestion is detected ($\nabla D > \gamma_{delay}$), we quickly reduce $C$ to relieve network pressure.

$$
C_{new} = C_{old} \times \beta
$$

*   $\beta = 0.9$ (Standard congestion).

### 3. Timeout Decrease
If a feedback timeout occurs (severe congestion), we apply a sharper decrease.

$$
C_{new} = C_{old} \times \beta_t
$$

*   $\beta_t = 0.85$.

## Implementation

The core logic resides in `alvr/server/src/congestion_controller.rs`:

*   **Struct:** `EyeNexus_Controller`
*   **Function:** `Update`
    *   Updates trendline.
    *   Determines state (Normal, Overusing).
    *   Applies AIMD to `controller_c`.
    *   Clamps `controller_c` between 2.0 and 80.0.

The timeout logic is handled in `alvr/server/src/lib.rs` inside `get_controller_c`.

