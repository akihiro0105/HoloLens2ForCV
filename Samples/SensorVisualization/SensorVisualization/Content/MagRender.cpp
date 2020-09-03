#include "pch.h"
#include "MagRender.h"
#include "Common\DirectXHelper.h"
#include <mutex>

using namespace BasicHologram;
using namespace DirectX;
using namespace winrt::Windows::Foundation::Numerics;
using namespace winrt::Windows::UI::Input::Spatial;

void MagRender::MagUpdateLoop()
{
    uint64_t lastSocTick = 0;
    uint64_t lastHupTick = 0;
    LARGE_INTEGER qpf;
    uint64_t lastQpcNow = 0;

    // Cache the QueryPerformanceFrequency
    QueryPerformanceFrequency(&qpf);

    winrt::check_hresult(m_pMagSensor->OpenStream());

    while (!m_fExit)
    {
        char printString[1000];

        IResearchModeSensorFrame* pSensorFrame = nullptr;
        ResearchModeSensorTimestamp timeStamp;
        size_t BufferOutLength;
        IResearchModeMagFrame* pSensorMagFrame = nullptr;
        const MagDataStruct* pMagBuffer = nullptr;

        

        winrt::check_hresult(m_pMagSensor->GetNextBuffer(&pSensorFrame));


        winrt::check_hresult(pSensorFrame->QueryInterface(IID_PPV_ARGS(&pSensorMagFrame)));

        {
            std::lock_guard<std::mutex> guard(m_sampleMutex);

            winrt::check_hresult(pSensorMagFrame->GetMagnetometer(&m_magSample));
        }

        winrt::check_hresult(pSensorMagFrame->GetMagnetometerSamples(
            &pMagBuffer,
            &BufferOutLength));

        lastHupTick = 0;
        std::string hupTimeDeltas = "";

        for (UINT i = 0; i < BufferOutLength; i++)
        {
            pSensorFrame->GetTimeStamp(&timeStamp);
            if (lastHupTick != 0)
            {
                if (pMagBuffer[i].VinylHupTicks < lastHupTick)
                {
                    sprintf(printString, "####MAG BAD HUP ORDERING\n");
                    OutputDebugStringA(printString);
                    DebugBreak();
                }
                sprintf(printString, " %I64d", (pMagBuffer[i].VinylHupTicks - lastHupTick) / 1000); // Microseconds

                hupTimeDeltas = hupTimeDeltas + printString;

            }
            lastHupTick = pMagBuffer[i].VinylHupTicks;
        }

        hupTimeDeltas = hupTimeDeltas + "\n";
        //OutputDebugStringA(hupTimeDeltas.c_str());

        pSensorFrame->GetTimeStamp(&timeStamp);
        LARGE_INTEGER qpcNow;
        uint64_t uqpcNow;
        QueryPerformanceCounter(&qpcNow);
        uqpcNow = qpcNow.QuadPart;

        if (lastSocTick != 0)
        {
            uint64_t timeInMilliseconds =
                (1000 *
                    (uqpcNow - lastQpcNow)) /
                qpf.QuadPart;

            if (timeStamp.HostTicks < lastSocTick)
            {
                DebugBreak();
            }

            sprintf(printString, "####Mag: % 3.4f % 3.4f % 3.4f %I64d %I64d\n",
                m_magSample.x,
                m_magSample.y,
                m_magSample.z,
                (((timeStamp.HostTicks - lastSocTick) * 1000) / timeStamp.HostTicksPerSecond), // Milliseconds
                timeInMilliseconds);
            OutputDebugStringA(printString);
        }
        lastSocTick = timeStamp.HostTicks;
        lastQpcNow = uqpcNow;

        if (pSensorFrame)
        {
            pSensorFrame->Release();
        }

        if (pSensorMagFrame)
        {
            pSensorMagFrame->Release();
        }
    }

    winrt::check_hresult(m_pMagSensor->CloseStream());
}

void MagRender::GetMagSample(DirectX::XMFLOAT3* pAccleSample)
{
    std::lock_guard<std::mutex> guard(m_sampleMutex);

    *pAccleSample = m_magSample;
}

// This function uses a SpatialPointerPose to position the world-locked hologram
// two meters in front of the user's heading.
void MagRender::PositionHologram(SpatialPointerPose const& pointerPose)
{
}

// Called once per frame. Rotates the cube, and calculates and sets the model matrix
// relative to the position transform indicated by hologramPositionTransform.
void MagRender::Update(DX::StepTimer const& timer)
{
}

void MagRender::SetSensorFrame(IResearchModeSensorFrame* pSensorFrame)
{

}

void MagRender::MagUpdateThread(MagRender* pMagRenderer, HANDLE hasData, ResearchModeSensorConsent* pCamAccessConsent)
{
    HRESULT hr = S_OK;

    if (hasData != nullptr)
    {
        DWORD waitResult = WaitForSingleObject(hasData, INFINITE);

        if (waitResult == WAIT_OBJECT_0)
        {
            switch (*pCamAccessConsent)
            {
            case ResearchModeSensorConsent::Allowed:
                OutputDebugString(L"Access is granted");
                break;
            case ResearchModeSensorConsent::DeniedBySystem:
                OutputDebugString(L"Access is denied by the system");
                hr = E_ACCESSDENIED;
                break;
            case ResearchModeSensorConsent::DeniedByUser:
                OutputDebugString(L"Access is denied by the user");
                hr = E_ACCESSDENIED;
                break;
            case ResearchModeSensorConsent::NotDeclaredByApp:
                OutputDebugString(L"Capability is not declared in the app manifest");
                hr = E_ACCESSDENIED;
                break;
            case ResearchModeSensorConsent::UserPromptRequired:
                OutputDebugString(L"Capability user prompt required");
                hr = E_ACCESSDENIED;
                break;
            default:
                OutputDebugString(L"Access is denied by the system");
                hr = E_ACCESSDENIED;
                break;
            }
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }

    if (FAILED(hr))
    {
        return;
    }

    pMagRenderer->MagUpdateLoop();
}

void MagRender::UpdateSample()
{
    HRESULT hr = S_OK;
}
