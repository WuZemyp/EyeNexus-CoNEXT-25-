# EyeNexus

**EyeNexus: Adaptive Gaze-Driven Quality and Bitrate Streaming for Seamless VR Cloud Gaming Experiences**

This repository contains the source code for the system described in our **CoNEXT 2025** paper. EyeNexus is an adaptive streaming system that leverages eye tracking to optimize visual quality and bitrate efficiency for VR cloud gaming.

📄 **Paper:** [EyeNexus: Adaptive Gaze-Driven Quality and Bitrate Streaming for Seamless VR Cloud Gaming Experiences](https://dl.acm.org/doi/abs/10.1145/3768989)

## Base System
This project is built upon the open-source **ALVR** codebase.
*   Original Repo: [alvr-org/alvr](https://github.com/alvr-org/alvr)
*   Build Wiki: [Building From Source](https://github.com/alvr-org/ALVR/wiki/Building-From-Source)

> **Note:** The original ALVR repo is a great resource for compatibility information and general troubleshooting.

## Eye Tracking Support
*   **Recommended Headset:** Meta Quest Pro.
*   **Customization:** You can modify the raw data collection part to support other eye-tracking headsets.
*   **If you  use a Non-Eye-Tracking Headsets:** The project functions with fixed foveated rendering (assuming gaze is always at the center of the frame).

---

## Getting Started

### Requirements

1.  **VR Headset:** A supported standalone VR headset (e.g., Meta Quest Pro, Quest 2/3).
2.  **SteamVR:** Installed and running on the host PC.
3.  **High-End Gaming PC:**
    *   **GPU:**
        *   **NVIDIA:** GTX 1000 series or newer (NVENC support required).
4.  **Network:**
    *   5 GHz Wi-Fi (802.11ac/ax) for the headset.
    *   Devices must be on the same local network.

### Installation

#### 1. Clone the Project
```bash
git clone https://github.com/YourUsername/EyeNexus.git
cd EyeNexus
```

#### 2. Build Instructions
We follow the standard ALVR build process [Building From Source](https://github.com/alvr-org/ALVR/wiki/Building-From-Source). Since EyeNexus currently supports a Windows server, we provide Windows-specific instructions below.

**Common Prerequisites:**
*   [Rustup](https://rust-lang.org/tools/install/) (Install Rust)
*   [Chocolatey](https://chocolatey.org/install) (Package manager for Windows)
*   *(Optional)* Visual Studio Code with `rust-analyzer` extension.

**Step A: Build Streamer (Server)**
Run the following commands in the project root terminal:

```powershell
# Prepare dependencies
cargo xtask prepare-deps --platform windows --gpl

# Build streamer
cargo xtask build-streamer --release --gpl
```

**Step B: Build Android Client (Headset App)**

1.  **Install Dependencies:**
    *   Android Studio (or `sdkmanager`)
    *   Android SDK Platform-Tools 29 (Android 10)
    *   Android NDK

2.  **Set Environment Variables (Windows):**
    *   `JAVA_HOME`: e.g., `C:\Program Files\Android\Android Studio\jre`
    *   `ANDROID_HOME`: e.g., `%LOCALAPPDATA%\Android\Sdk`
    *   `ANDROID_NDK_HOME`: e.g., `%LOCALAPPDATA%\Android\Sdk\ndk\25.1.8937393`

3.  **Build:**
    ```powershell
    # Prepare Android dependencies
    cargo xtask prepare-deps --platform android

    # Build APK
    cargo xtask build-client --release
    ```

## Settings

To ensure EyeNexus functions correctly, please configure the following settings in the ALVR Dashboard:

1.  **Eye Tracking:**
    *   Go to `Settings` -> `Eye and face tracking`.
    *   Select **VRChat Eye OSC**.
2.  **Resolution:**
    *   Go to `Settings` -> `Resolution` to adjust the streaming resolution.

---

## Documentation

We have separated the implementation details into specific documentation files, linked below. These documents reference the corresponding sections in our CoNEXT 2025 paper.

| Topic | Description | Paper Section |
| :--- | :--- | :--- |
| **[Gaze Mapping](docs/GAZE_MAPPING.md)** | How raw VR gaze angles are converted to screen space coordinates. | Sec 3.2.1 |
| **[Dynamic FSC](docs/DYNAMIC_FSC.md)** | Algorithm for adapting the foveal region size based on gaze. | Sec 3.2.2 |
| **[Gaze Mapping (FSC)](docs/GAZE_MAPPING_FSC.md)** | Re-mapping gaze coordinates from the screen to the compressed FSC frame. | Sec 3.3.1 |
| **[Gaze-Driven Encoding](docs/VIDEO_ENCODING_QP.md)** | Generating the Non-Uniform QP Map using a Gaussian model. | Sec 3.3 |
| **[Client Rendering](docs/CLIENT_RENDERING_FSD.md)** | Decoding and Foveated Spatial Decompression (FSD) on the headset. | Sec 3.4 |
| **[Network Monitoring](docs/NETWORK_MONITORING.md)** | Detecting congestion via Queuing Delay Gradient ($\nabla D$) and Feedback Timeout. | Sec 3.5 |
| **[Rate Control](docs/GAZE_CONTINGENT_RATE_CONTROL.md)** | AIMD algorithm for the Foveation Controller ($C$). | Sec 3.6 |

## Hyperparameter Tuning

For optimal performance in your specific environment, please refer to **Appendix G** in our [paper](https://dl.acm.org/doi/abs/10.1145/3768989). It details the tuning process for key parameters such as $\gamma_{delay}$, $\gamma_{fd}$, $\alpha$, $\beta$, $\beta_t$, $C_{min}$, and $C_{max}$.

---

## Statistics & Data

EyeNexus generates CSV logs for analysis. By default, these are saved in the root directory of your SteamVR installation (or the project root if running from source), but the location can be modified in the code.

*   **`eyegaze.csv`**
    *   Contains collected gaze locations (X, Y) during the gaming session.
*   **`statistics_mtp.csv`**
    *   Provides full **Motion-to-Photon (MTP) pipeline latency** records for each frame.
    *   Includes detailed breakdown: Game Latency, Composite Latency, Encode Latency, Network Latency, Decode Latency, etc.

