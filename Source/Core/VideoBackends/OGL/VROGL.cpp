// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#ifdef _WIN32
#include "VideoBackends/OGL/GLInterface/WGL.h"
#else
#include "VideoBackends/OGL/GLInterface/GLX.h"
#endif

#include "VideoBackends/OGL/FramebufferManager.h"
#include "VideoBackends/OGL/Render.h"
#include "VideoBackends/OGL/TextureConverter.h"
#include "VideoBackends/OGL/VROGL.h"
#include "VideoCommon/VideoConfig.h"
#include "VideoCommon/VR.h"

// Oculus Rift
#ifdef HAVE_OCULUSSDK
ovrGLTexture g_eye_texture[2];
#endif

namespace OGL
{

void VR_ConfigureHMD()
{
#ifdef HAVE_OCULUSSDK
	if (g_has_rift)
	{
		ovrGLConfig cfg;
		cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
#ifdef OCULUSSDK044
		cfg.OGL.Header.BackBufferSize.w = hmdDesc.Resolution.w;
		cfg.OGL.Header.BackBufferSize.h = hmdDesc.Resolution.h;
#else
		cfg.OGL.Header.RTSize.w = hmdDesc.Resolution.w;
		cfg.OGL.Header.RTSize.h = hmdDesc.Resolution.h;
#endif
		cfg.OGL.Header.Multisample = 0;
#ifdef _WIN32
		cfg.OGL.Window = (HWND)((cInterfaceWGL*)GLInterface)->m_window_handle;
		cfg.OGL.DC = GetDC(cfg.OGL.Window);
#ifndef OCULUSSDK042
		if (g_is_direct_mode) //If in Direct Mode
		{
			ovrHmd_AttachToWindow(hmd, cfg.OGL.Window, nullptr, nullptr); //Attach to Direct Mode.
		}
#endif
#else
		cfg.OGL.Disp = (Display*)((cInterfaceGLX*)GLInterface)->getDisplay();
#ifdef OCULUSSDK043
		cfg.OGL.Win = glXGetCurrentDrawable();
#endif
#endif
		int caps = 0;
		if (g_Config.bChromatic)
			caps |= ovrDistortionCap_Chromatic;
		if (g_Config.bTimewarp)
			caps |= ovrDistortionCap_TimeWarp;
		if (g_Config.bVignette)
			caps |= ovrDistortionCap_Vignette;
		if (g_Config.bNoRestore)
			caps |= ovrDistortionCap_NoRestore;
		if (g_Config.bFlipVertical)
			caps |= ovrDistortionCap_FlipInput;
		if (g_Config.bSRGB)
			caps |= ovrDistortionCap_SRGB;
		if (g_Config.bOverdrive)
			caps |= ovrDistortionCap_Overdrive;
		if (g_Config.bHqDistortion)
			caps |= ovrDistortionCap_HqDistortion;
		ovrHmd_ConfigureRendering(hmd, &cfg.Config, caps,
			g_eye_fov, g_eye_render_desc);
		ovrhmd_EnableHSWDisplaySDKRender(hmd, false);
	}
#endif
}

void VR_StartFramebuffer(int target_width, int target_height, GLuint left_texture, GLuint right_texture)
{
	if (g_has_vr920)
	{
#ifdef _WIN32
		VR920_StartStereo3D();
#endif
	}
#ifdef HAVE_OCULUSSDK
	else if (g_has_rift)
	{
		g_eye_texture[0].OGL.Header.API = ovrRenderAPI_OpenGL;
		g_eye_texture[0].OGL.Header.TextureSize.w = target_width;
		g_eye_texture[0].OGL.Header.TextureSize.h = target_height;
		g_eye_texture[0].OGL.Header.RenderViewport.Pos.x = 0;
		g_eye_texture[0].OGL.Header.RenderViewport.Pos.y = 0;
		g_eye_texture[0].OGL.Header.RenderViewport.Size.w = target_width;
		g_eye_texture[0].OGL.Header.RenderViewport.Size.h = target_height;
		g_eye_texture[0].OGL.TexId = left_texture;
		g_eye_texture[1] = g_eye_texture[0];
		if (g_ActiveConfig.iStereoMode == STEREO_OCULUS)
			g_eye_texture[1].OGL.TexId = right_texture;
	}
#endif
}

void VR_PresentHMDFrame()
{
#ifdef HAVE_OCULUSSDK
	if (g_has_rift)
	{
		//ovrHmd_EndEyeRender(hmd, ovrEye_Left, g_left_eye_pose, &FramebufferManager::m_eye_texture[ovrEye_Left].Texture);
		//ovrHmd_EndEyeRender(hmd, ovrEye_Right, g_right_eye_pose, &FramebufferManager::m_eye_texture[ovrEye_Right].Texture);

		// Let OVR do distortion rendering, Present and flush/sync.
		ovrHmd_EndFrame(hmd, g_eye_poses, &g_eye_texture[0].Texture);
	}
#endif
}

void VR_DrawTimewarpFrame()
{
#ifdef HAVE_OCULUSSDK
	if (g_has_rift)
	{
		ovrFrameTiming frameTime = ovrHmd_BeginFrame(hmd, ++g_ovr_frameindex);

		ovr_WaitTillTime(frameTime.NextFrameSeconds - g_ActiveConfig.fTimeWarpTweak);

		ovrHmd_EndFrame(hmd, g_eye_poses, &g_eye_texture[0].Texture);
	}
#endif
}

void VR_DrawAsyncTimewarpFrame()
{
#ifdef HAVE_OCULUSSDK
	if (g_has_rift)
	{
		auto frameTime = ovrHmd_BeginFrame(hmd, ++g_ovr_frameindex);
		g_vr_lock.unlock();

		if (0 == frameTime.TimewarpPointSeconds) {
			ovr_WaitTillTime(frameTime.TimewarpPointSeconds - 0.002);
		}
		else {
			ovr_WaitTillTime(frameTime.NextFrameSeconds - 0.008);
		}

		g_vr_lock.lock();
		// Grab the most recent textures
		for (int eye = 0; eye < 2; eye++)
		{
			((ovrGLTexture&)(g_eye_texture[eye])).OGL.TexId = FramebufferManager::m_frontBuffer[eye];
		}
#ifdef _WIN32
		//HANDLE thread_handle = g_video_backend->m_video_thread->native_handle();
		//SuspendThread(thread_handle);
#endif
		ovrHmd_EndFrame(hmd, g_front_eye_poses, &g_eye_texture[0].Texture);
#ifdef _WIN32
		//ResumeThread(thread_handle);
#endif
	}
#endif
}

}