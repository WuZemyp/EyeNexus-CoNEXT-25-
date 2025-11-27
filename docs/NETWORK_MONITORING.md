# Network Monitoring (Sec 3.5)

EyeNexus monitors network conditions to detect congestion and available bandwidth, using this information to adapt the video stream.

## 1. Queuing Delay Gradient ($\nabla D$)

To detect congestion, we track the **Queuing Delay Gradient**. We measure the trend of how packet delay changes over time.

### Mathematical Model
For a sequence of frames, we calculate the delay $D_i$ for the $i$-th frame:

$$
D_i = (t_{arr}^i - t_{arr}^{i-1}) - (t_{send}^i - t_{send}^{i-1})
$$

We then compute the gradient $\nabla D$ by performing a **linear regression** (LinearFitSlope) on a moving window of these delay values (size = 5-8 frames).

*   **Overuse:** $\nabla D > \gamma_{delay}$ (Delay is increasing, queue is building up).
*   **Underuse:** $\nabla D < -\gamma_{delay}$ (Delay is decreasing, queue is emptying).
*   **Normal:** $-\gamma_{delay} \le \nabla D \le \gamma_{delay}$ (Stable).

### Implementation
*   **File:** `alvr/server/src/congestion_controller.rs`
*   **Struct:** `TrendlineEstimator`
*   **Function:** `LinearFitSlope` and `Detect`

## 2. Feedback Timeout

We also monitor the arrival of feedback packets from the client. If feedback is missing for too long, it indicates severe congestion or packet loss.

*   **Timeout Condition:** $\delta t_{fd} > \gamma_{fd}$
*   **Action:** Triggers a sharp reduction in bitrate (see Rate Control).

### Implementation
*   **File:** `alvr/server/src/lib.rs`
*   **Function:** `get_controller_c` check for `(now - last_change)`.

