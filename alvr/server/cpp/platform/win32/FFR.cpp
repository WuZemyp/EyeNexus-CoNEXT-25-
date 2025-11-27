#include "FFR.h"

#include "alvr_server/Settings.h"
#include "alvr_server/Utils.h"
#include "alvr_server/bindings.h"
#include <fstream>
#include <chrono>
using Microsoft::WRL::ComPtr;
using namespace d3d_render_utils;

namespace {

	struct FoveationVars {
		uint32_t targetEyeWidth;
		uint32_t targetEyeHeight;
		uint32_t optimizedEyeWidth;
		uint32_t optimizedEyeHeight;

		float eyeWidthRatio;
		float eyeHeightRatio;

		float centerSizeX;
		float centerSizeY;
		float centerShiftX;
		float centerShiftY;
		float edgeRatioX;
		float edgeRatioY;
	};

	FoveationVars CalculateFoveationVars(float centerShiftX_input = 0.4, float centerShiftY_input = 0.1) {
		float targetEyeWidth = (float)Settings::Instance().m_renderWidth / 2;//2144
		float targetEyeHeight = (float)Settings::Instance().m_renderHeight;//2336

		float centerSizeX = (float)Settings::Instance().m_foveationCenterSizeX;//0.45
		float centerSizeY = (float)Settings::Instance().m_foveationCenterSizeY;//0.4
		float centerShiftX = centerShiftX_input;//(float)Settings::Instance().m_foveationCenterShiftX;//0.4
		float centerShiftY = centerShiftY_input;//(float)Settings::Instance().m_foveationCenterShiftY;//0.1
		float edgeRatioX = (float)Settings::Instance().m_foveationEdgeRatioX;//4.0
		float edgeRatioY = (float)Settings::Instance().m_foveationEdgeRatioY;//5.0

		float edgeSizeX = targetEyeWidth-centerSizeX*targetEyeWidth;//1179.2
		float edgeSizeY = targetEyeHeight-centerSizeY*targetEyeHeight;//1401.6

		float centerSizeXAligned = 1.-ceil(edgeSizeX/(edgeRatioX*2.))*(edgeRatioX*2.)/targetEyeWidth;//0.447761194
		float centerSizeYAligned = 1.-ceil(edgeSizeY/(edgeRatioY*2.))*(edgeRatioY*2.)/targetEyeHeight;//0.3964041096

		float edgeSizeXAligned = targetEyeWidth-centerSizeXAligned*targetEyeWidth;//1184
		float edgeSizeYAligned = targetEyeHeight-centerSizeYAligned*targetEyeHeight;//1410

		float centerShiftXAligned = ceil(centerShiftX*edgeSizeXAligned/(edgeRatioX*2.))*(edgeRatioX*2.)/edgeSizeXAligned;//0.4054054054
		float centerShiftYAligned = ceil(centerShiftY*edgeSizeYAligned/(edgeRatioY*2.))*(edgeRatioY*2.)/edgeSizeYAligned;//0.1063829787

		float foveationScaleX = (centerSizeXAligned+(1.-centerSizeXAligned)/edgeRatioX);//0.5858208955
		float foveationScaleY = (centerSizeYAligned+(1.-centerSizeYAligned)/edgeRatioY);//0.5171232877

		float optimizedEyeWidth = foveationScaleX*targetEyeWidth;//1256
		float optimizedEyeHeight = foveationScaleY*targetEyeHeight;//1208

		// round the frame dimensions to a number of pixel multiple of 32 for the encoder
		auto optimizedEyeWidthAligned = (uint32_t)ceil(optimizedEyeWidth / 32.f) * 32;//1280
		auto optimizedEyeHeightAligned = (uint32_t)ceil(optimizedEyeHeight / 32.f) * 32;//1216

		float eyeWidthRatioAligned = optimizedEyeWidth/optimizedEyeWidthAligned;//0.98125
		float eyeHeightRatioAligned = optimizedEyeHeight/optimizedEyeHeightAligned;//0.9934210526

		return { (uint32_t)targetEyeWidth, (uint32_t)targetEyeHeight, optimizedEyeWidthAligned, optimizedEyeHeightAligned,
			eyeWidthRatioAligned, eyeHeightRatioAligned,
			centerSizeXAligned, centerSizeYAligned, centerShiftXAligned, centerShiftYAligned, edgeRatioX, edgeRatioY };
	}
}


void FFR::GetOptimizedResolution(uint32_t* width, uint32_t* height) {
	auto fovVars = CalculateFoveationVars();
	*width = fovVars.optimizedEyeWidth * 2;
	*height = fovVars.optimizedEyeHeight;
}

FFR::FFR(ID3D11Device* device) : mDevice(device) {}

void FFR::Initialize(ID3D11Texture2D* compositionTexture, float centerShiftX, float centerShiftY) {
	auto now = std::chrono::system_clock::now();
	m_compositionTexture = compositionTexture;
    // Convert to milliseconds since epoch
    auto milliseconds_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

	auto fovVars = CalculateFoveationVars(centerShiftX, centerShiftY);
	std::ofstream testOut("fovVar.txt", std::ios::app);
	testOut << "targetEyeWidth" << fovVars.targetEyeWidth << std::endl;
	testOut << "targetEyeHeight" << fovVars.targetEyeHeight << std::endl;
	testOut << "OptimizedEyeWidth" << fovVars.optimizedEyeWidth << std::endl;
	testOut << "OptimizedEyeHeight" << fovVars.optimizedEyeHeight << std::endl;
	testOut << "eyeWidthRatio" << fovVars.eyeWidthRatio << std::endl;
	testOut << "eyeHeightRatio" << fovVars.eyeHeightRatio << std::endl;
	testOut << "centerSizeX" << fovVars.centerSizeX << std::endl;
	testOut << "centerSizeY" << fovVars.centerSizeY << std::endl;
	testOut << "centerShiftX" << fovVars.centerShiftX << std::endl;
	testOut << "centerShiftY" << fovVars.centerShiftY << std::endl;
	testOut << "edgeRatioX" << fovVars.edgeRatioX << std::endl;
	testOut << "edgeRatioY" << fovVars.edgeRatioY << std::endl;



	ComPtr<ID3D11Buffer> foveatedRenderingBuffer = CreateBuffer(mDevice.Get(), fovVars);

	std::vector<uint8_t> quadShaderCSO(QUAD_SHADER_CSO_PTR, QUAD_SHADER_CSO_PTR + QUAD_SHADER_CSO_LEN);
	mQuadVertexShader = CreateVertexShader(mDevice.Get(), quadShaderCSO);

	mOptimizedTexture = CreateTexture(mDevice.Get(), fovVars.optimizedEyeWidth * 2,
		fovVars.optimizedEyeHeight, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

	if (Settings::Instance().m_enableFoveatedEncoding) {
		std::vector<uint8_t> compressAxisAlignedShaderCSO(COMPRESS_AXIS_ALIGNED_CSO_PTR, COMPRESS_AXIS_ALIGNED_CSO_PTR + COMPRESS_AXIS_ALIGNED_CSO_LEN);
		auto compressAxisAlignedPipeline = RenderPipeline(mDevice.Get());
		compressAxisAlignedPipeline.Initialize({ compositionTexture }, mQuadVertexShader.Get(),
			compressAxisAlignedShaderCSO, mOptimizedTexture.Get(), foveatedRenderingBuffer.Get());

		mPipelines.push_back(compressAxisAlignedPipeline);
	} else {
		mOptimizedTexture = compositionTexture;
	}
	auto now1 = std::chrono::system_clock::now();

    // Convert to milliseconds since epoch
    auto milliseconds_since_epoch1 = std::chrono::duration_cast<std::chrono::milliseconds>(now1.time_since_epoch()).count();
	testOut << "computation time in ms: " << (milliseconds_since_epoch1-milliseconds_since_epoch) << std::endl;
}

void FFR::Reinit(float centerShiftX, float centerShiftY)
{
	// EyeNexus-Dynamic FSC: Recalculate foveation variables based on new center shift
	auto fovVars = CalculateFoveationVars(centerShiftX, centerShiftY);// Ever time the centershift changes, the fovVars changes.
	
	// create the new buffer and shader code for the new fovVars
	ComPtr<ID3D11Buffer> foveatedRenderingBuffer = CreateBuffer(mDevice.Get(), fovVars);
	std::vector<uint8_t> compressAxisAlignedShaderCSO(COMPRESS_AXIS_ALIGNED_CSO_PTR, COMPRESS_AXIS_ALIGNED_CSO_PTR + COMPRESS_AXIS_ALIGNED_CSO_LEN);
		auto compressAxisAlignedPipeline = RenderPipeline(mDevice.Get());
	compressAxisAlignedPipeline.Initialize({ m_compositionTexture }, mQuadVertexShader.Get(),
			compressAxisAlignedShaderCSO, mOptimizedTexture.Get(), foveatedRenderingBuffer.Get());
	// remove the old pipeline and add the new pipeline
	mPipelines.pop_back();
	mPipelines.push_back(compressAxisAlignedPipeline);
}

void FFR::Render() {
	for (auto &p : mPipelines) {
		p.Render();
	}
}

ID3D11Texture2D* FFR::GetOutputTexture() {
	return mOptimizedTexture.Get();
}