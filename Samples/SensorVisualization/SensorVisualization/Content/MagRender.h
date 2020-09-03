#pragma once
#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"
#include "ShaderStructures.h"
#include "researchmode\ResearchModeApi.h"
#include <Texture2D.h>

namespace BasicHologram
{
    // This sample renderer instantiates a basic rendering pipeline.
    class MagRender
    {
    public:
        MagRender(std::shared_ptr<DX::DeviceResources> const& deviceResources, IResearchModeSensor* pMagSensor, HANDLE hasData, ResearchModeSensorConsent* pCamAccessConsent)
        {
            m_pMagSensor = pMagSensor;
            m_pMagSensor->AddRef();
            m_pMagUpdateThread = new std::thread(MagUpdateThread, this, hasData, pCamAccessConsent);
        }
        virtual ~MagRender()
        {
            m_fExit = true;
            m_pMagUpdateThread->join();

            if (m_pMagSensor)
            {
                m_pMagSensor->CloseStream();
                m_pMagSensor->Release();
            }
        }
        void Update(DX::StepTimer const& timer);
        void UpdateSample();

        // Repositions the sample hologram.
        void PositionHologram(winrt::Windows::UI::Input::Spatial::SpatialPointerPose const& pointerPose);

        // Property accessors.
        void SetSensorFrame(IResearchModeSensorFrame* pSensorFrame);

        void GetMagSample(DirectX::XMFLOAT3* pMagSample);

    private:
        static void MagRender::MagUpdateThread(MagRender* pSpinningCube, HANDLE hasData, ResearchModeSensorConsent* pCamAccessConsent);
        void MagUpdateLoop();

        // If the current D3D Device supports VPRT, we can avoid using a geometry
        // shader just to set the render target array index.
        bool                                            m_usingVprtShaders = false;

        IResearchModeSensor* m_pMagSensor = nullptr;
        IResearchModeSensorFrame* m_pSensorFrame;
        DirectX::XMFLOAT3 m_magSample;
        std::mutex m_sampleMutex;

        std::thread* m_pMagUpdateThread;
        bool m_fExit = { false };
    };

}