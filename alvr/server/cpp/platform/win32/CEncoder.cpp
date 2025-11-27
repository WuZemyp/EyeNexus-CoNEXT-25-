#include "CEncoder.h"
#include "alvr_server/Settings.h"
#include <random>
#include <chrono>

		CEncoder::CEncoder()
			: m_bExiting(false)
			, m_targetTimestampNs(0)
			, m_gaze_location_leftx(1072)
			, m_gaze_location_lefty(1168)
			, m_gaze_location_rightx(3216)
			, m_gaze_location_righty(1168)
		{
			m_encodeFinished.Set();
			last_centerShiftX = 0.4;
			last_centerShiftY = 0.1;
			times = 0;
		}

		
		CEncoder::~CEncoder()
		{
			if (m_videoEncoder)
			{
				m_videoEncoder->Shutdown();
				m_videoEncoder.reset();
			}
		}

		void CEncoder::Initialize(std::shared_ptr<CD3DRender> d3dRender) {
			m_FrameRender = std::make_shared<FrameRender>(d3dRender);
			m_FrameRender->Startup();
			uint32_t encoderWidth, encoderHeight;
			m_FrameRender->GetEncodingResolution(&encoderWidth, &encoderHeight);

			Exception vceException;
			Exception nvencException;
#ifdef ALVR_GPL
			Exception swException;
			if (Settings::Instance().m_force_sw_encoding) {
				try {
					Debug("Try to use VideoEncoderSW.\n");
					m_videoEncoder = std::make_shared<VideoEncoderSW>(d3dRender, encoderWidth, encoderHeight);
					m_videoEncoder->Initialize();
					return;
				}
				catch (Exception e) {
					swException = e;
				}
			}
#endif
			
			try {
				Debug("Try to use VideoEncoderAMF.\n");
				m_videoEncoder = std::make_shared<VideoEncoderAMF>(d3dRender, encoderWidth, encoderHeight);
				m_videoEncoder->Initialize();
				return;
			}
			catch (Exception e) {
				vceException = e;
			}
			try {
				Debug("Try to use VideoEncoderNVENC.\n");
				m_videoEncoder = std::make_shared<VideoEncoderNVENC>(d3dRender, encoderWidth, encoderHeight);
				m_videoEncoder->Initialize();
				return;
			}
			catch (Exception e) {
				nvencException = e;
			}
#ifdef ALVR_GPL
			try {
				Debug("Try to use VideoEncoderSW.\n");
				m_videoEncoder = std::make_shared<VideoEncoderSW>(d3dRender, encoderWidth, encoderHeight);
				m_videoEncoder->Initialize();
				return;
			}
			catch (Exception e) {
				swException = e;
			}
			throw MakeException("All VideoEncoder are not available. VCE: %s, NVENC: %s, SW: %s", vceException.what(), nvencException.what(), swException.what());
#else
			throw MakeException("All VideoEncoder are not available. VCE: %s, NVENC: %s", vceException.what(), nvencException.what());
#endif
		}

		bool CEncoder::CopyToStaging(ID3D11Texture2D *pTexture[][2], vr::VRTextureBounds_t bounds[][2], int layerCount, bool recentering
			, uint64_t presentationTime, uint64_t targetTimestampNs, const std::string& message, const std::string& debugText)
		{
			
			m_presentationTime = presentationTime;
			m_targetTimestampNs = targetTimestampNs;
			int frame_width = Settings::Instance().m_renderWidth/2;
			int frame_height = Settings::Instance().m_renderHeight;
			Info("Render Target: %d %d\n", frame_width, frame_height);

			// EyeNexus-Dynamic FSC: Retrieve gaze location in left frame and right frame
			
			m_gaze_location_leftx = int(GetEyeGazeLocationLeftX());
			m_gaze_location_lefty = int(frame_height-GetEyeGazeLocationLeftY());
			m_gaze_location_rightx = int(GetEyeGazeLocationRightX());
			m_gaze_location_righty = int(frame_height-GetEyeGazeLocationRightY());

			// EyeNexus-Dynamic FSC: applied gaze location to achieve foveated spatial compression (Sec 3.2.2)
			int FSC_frequency = 3; // FSC_frequency is the frequency of applying changes of foveated spatial compression. A lower value means more frequent changes, demands more computation (Should decide based on the hardware performance of server).
			
			if (times%FSC_frequency == 0){

				last_centerShiftX = (static_cast<float>(m_gaze_location_leftx) / (static_cast<float>(frame_width)) - 0.5)*2.0;
				last_centerShiftY = (static_cast<float>(m_gaze_location_lefty) / (static_cast<float>(frame_height)) - 0.5)*2.0;

			}
			times++;

			// apply default value if gaze location is not updated
			if (last_centerShiftX == 0.0){
				last_centerShiftX = 0.4;
			}
			if (last_centerShiftY == 0.0){
				last_centerShiftY = 0.1;
			}

			// EyeNexus-Dynamic FSC: Reinit the foveated spatial compression formula using gaze location as center
			m_FrameRender->Startup(last_centerShiftX, last_centerShiftY);
			m_FrameRender->Reinit_ffr(last_centerShiftX, last_centerShiftY);//Reinit the foveated spatial compression formula and shader code using gaze location as center.

			m_FrameRender->RenderFrame(pTexture, bounds, layerCount, recentering, message, debugText);
			return true;
		}

		void CEncoder::Run()
		{
			Debug("CEncoder: Start thread. Id=%d\n", GetCurrentThreadId());
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_MOST_URGENT);

			while (!m_bExiting)
			{
				m_newFrameReady.Wait();
				if (m_bExiting)
					break;

				if (m_FrameRender->GetTexture())
				{
					// EyeNexus Gaze-Driven video encoding: Transmit the texture to the video encoder with gaze location and foveated spatial compression formula.
					m_videoEncoder->Transmit(m_FrameRender->GetTexture().Get(), m_presentationTime, m_targetTimestampNs, m_scheduler.CheckIDRInsertion(),m_gaze_location_leftx,m_gaze_location_lefty,m_gaze_location_rightx,m_gaze_location_righty,last_centerShiftX,last_centerShiftY);
				}

				m_encodeFinished.Set();
			}
		}

		void CEncoder::Stop()
		{
			m_bExiting = true;
			m_newFrameReady.Set();
			Join();
			m_FrameRender.reset();
		}

		void CEncoder::NewFrameReady()
		{
			m_encodeFinished.Reset();
			m_newFrameReady.Set();
		}

		void CEncoder::WaitForEncode()
		{
			m_encodeFinished.Wait();
		}

		void CEncoder::OnStreamStart() {
			m_scheduler.OnStreamStart();
		}

		void CEncoder::OnPacketLoss() {
			m_scheduler.OnPacketLoss();
		}

		void CEncoder::InsertIDR() {
			m_scheduler.InsertIDR();
		}

		void CEncoder::CaptureFrame() {
		}
