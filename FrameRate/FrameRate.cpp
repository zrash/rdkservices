/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#include "FrameRate.h"
#include "host.hpp"
#include "exception.hpp"
#include "dsMgr.h"
#include "libIBus.h"
#include "tptimer.h"

// Methods
#define METHOD_SET_COLLECTION_FREQUENCY "setCollectionFrequency"
#define METHOD_START_FPS_COLLECTION "startFpsCollection"
#define METHOD_STOP_FPS_COLLECTION "stopFpsCollection"
#define METHOD_UPDATE_FPS_COLLECTION "updateFps"
#define METHOD_SET_FRAME_MODE "setFrmMode"
#define METHOD_GET_FRAME_MODE "getFrmMode"
#define METHOD_GET_DISPLAY_FRAME_RATE "getDisplayFrameRate"
#define METHOD_SET_DISPLAY_FRAME_RATE "setDisplayFrameRate"

// Events
#define EVENT_FPS_UPDATE "onFpsEvent"
#define EVENT_FRAMERATE_PRECHANGE  "onDisplayFrameRateChanging"
#define EVENT_FRAMERATE_POSTCHANGE    "onDisplayFrameRateChanged"

//Defines
#define DEFAULT_FPS_COLLECTION_TIME_IN_MILLISECONDS 10000
#define MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS 100
#define DEFAULT_MIN_FPS_VALUE 60
#define DEFAULT_MAX_FPS_VALUE -1

/**
 * from utils.h
 * TODO: cannot use utils.h because it has too many include-s
 */
#include <syscall.h>
#define LOGINFO(fmt, ...) do { fprintf(stderr, "[%d] INFO [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)
#define LOGWARN(fmt, ...) do { fprintf(stderr, "[%d] WARN [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)
#define LOGERR(fmt, ...) do { fprintf(stderr, "[%d] ERROR [%s:%d] %s: " fmt "\n", (int)syscall(SYS_gettid), WPEFramework::Core::FileNameOnly(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr); } while (0)
#define LOGINFOMETHOD() { std::string json; parameters.ToString(json); LOGINFO( "params=%s", json.c_str() ); }
#define LOGTRACEMETHODFIN() { std::string json; response.ToString(json); LOGINFO( "response=%s", json.c_str() ); }
#define LOG_DEVICE_EXCEPTION0() LOGWARN("Exception caught: code=%d message=%s", err.getCode(), err.what());
#define LOG_DEVICE_EXCEPTION1(param1) LOGWARN("Exception caught" #param1 "=%s code=%d message=%s", param1.c_str(), err.getCode(), err.what());
#define LOG_DEVICE_EXCEPTION2(param1, param2) LOGWARN("Exception caught " #param1 "=%s " #param2 "=%s code=%d message=%s", param1.c_str(), param2.c_str(), err.getCode(), err.what());
#define returnResponse(success) \
    { \
        response["success"] = success; \
        LOGTRACEMETHODFIN(); \
        return (Core::ERROR_NONE); \
    }
#define returnIfParamNotFound(param, name) \
    if (!param.HasLabel(name)) \
    { \
        LOGERR("No argument '%s'", name); \
        returnResponse(false); \
    }
#define IARM_CHECK(FUNC) { \
    if ((res = FUNC) != IARM_RESULT_SUCCESS) { \
        LOGINFO("IARM %s: %s", #FUNC, \
            res == IARM_RESULT_INVALID_PARAM ? "invalid param" : ( \
            res == IARM_RESULT_INVALID_STATE ? "invalid state" : ( \
            res == IARM_RESULT_IPCCORE_FAIL ? "ipcore fail" : ( \
            res == IARM_RESULT_OOM ? "oom" : "unknown")))); \
    } \
    else \
    { \
        LOGINFO("IARM %s: success", #FUNC); \
    } \
}

namespace Utils {
struct IARM {
    static bool init();
    static bool isConnected();

    static const char* NAME;
};

const char* IARM::NAME = "Thunder_Plugins";

bool IARM::isConnected() {
    IARM_Result_t res;
    int isRegistered = 0;
    res = IARM_Bus_IsConnected(NAME, &isRegistered);
    LOGINFO("IARM_Bus_IsConnected: %d (%d)", res, isRegistered);

    return (isRegistered == 1);
}

bool IARM::init() {
    IARM_Result_t res;
    bool result = false;

    if (isConnected()) {
        LOGINFO("IARM already connected");
        result = true;
    } else {
        res = IARM_Bus_Init(NAME);
        LOGINFO("IARM_Bus_Init: %d", res);
        if (res == IARM_RESULT_SUCCESS ||
            res == IARM_RESULT_INVALID_STATE /* already inited or connected */) {

            res = IARM_Bus_Connect();
            LOGINFO("IARM_Bus_Connect: %d", res);
            if (res == IARM_RESULT_SUCCESS ||
                res == IARM_RESULT_INVALID_STATE /* already connected or not inited */) {

                result = isConnected();
            } else {
                LOGERR("IARM_Bus_Connect failure: %d", res);
            }
        } else {
            LOGERR("IARM_Bus_Init failure: %d", res);
        }
    }

    return result;
}
}

namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(FrameRate, 1, 0);

        FrameRate* FrameRate::_instance = nullptr;

        FrameRate::FrameRate()
            : PluginHost::JSONRPC()
            , m_fpsCollectionFrequencyInMs(DEFAULT_FPS_COLLECTION_TIME_IN_MILLISECONDS)
            , m_minFpsValue(DEFAULT_MIN_FPS_VALUE)
            , m_maxFpsValue(DEFAULT_MAX_FPS_VALUE)
            , m_totalFpsValues(0)
            , m_numberOfFpsUpdates(0)
            , m_fpsCollectionInProgress(false)
            , m_reportFpsTimer(Core::ProxyType<TpTimer>::Create())
            , m_lastFpsValue(-1)
        {
            FrameRate::_instance = this;

            Register(METHOD_SET_COLLECTION_FREQUENCY, &FrameRate::setCollectionFrequencyWrapper, this);
            Register(METHOD_START_FPS_COLLECTION, &FrameRate::startFpsCollectionWrapper, this);
            Register(METHOD_STOP_FPS_COLLECTION, &FrameRate::stopFpsCollectionWrapper, this);
            Register(METHOD_UPDATE_FPS_COLLECTION, &FrameRate::updateFpsWrapper, this);
            CreateHandler({2});
            GetHandler(2)->Register<JsonObject, JsonObject>(METHOD_SET_FRAME_MODE, &FrameRate::setFrmMode, this);
            GetHandler(2)->Register<JsonObject, JsonObject>(METHOD_GET_FRAME_MODE, &FrameRate::getFrmMode, this);
            GetHandler(2)->Register<JsonObject, JsonObject>(METHOD_GET_DISPLAY_FRAME_RATE, &FrameRate::getDisplayFrameRate, this);
            GetHandler(2)->Register<JsonObject, JsonObject>(METHOD_SET_DISPLAY_FRAME_RATE, &FrameRate::setDisplayFrameRate, this);

            m_reportFpsTimer->connect(std::bind(&FrameRate::onReportFpsTimer, this));
        }

        FrameRate::~FrameRate()
        {
            Unregister(METHOD_SET_COLLECTION_FREQUENCY);
            Unregister(METHOD_START_FPS_COLLECTION);
            Unregister(METHOD_STOP_FPS_COLLECTION);
            Unregister(METHOD_UPDATE_FPS_COLLECTION);
            GetHandler(2)->Unregister(METHOD_SET_FRAME_MODE);
            GetHandler(2)->Unregister(METHOD_GET_FRAME_MODE);
            GetHandler(2)->Unregister(METHOD_GET_DISPLAY_FRAME_RATE);
            GetHandler(2)->Unregister(METHOD_SET_DISPLAY_FRAME_RATE);
        }

	const string FrameRate::Initialize(PluginHost::IShell * /* service */)
        {
		InitializeIARM();
                return "";
        }

	void FrameRate::InitializeIARM()
        {
            LOGWARN("FrameRate::InitializeIARM");
	    if(Utils::IARM::init())
	    {

		    IARM_Result_t res;
		    IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME,IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, FrameRatePreChange) );
		    IARM_CHECK( IARM_Bus_RegisterEventHandler(IARM_BUS_DSMGR_NAME,IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, FrameRatePostChange) );
	    }
	}

	void FrameRate::DeinitializeIARM()
	{
            if (Utils::IARM::isConnected())
            {
                IARM_Result_t res;
                IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_DSMGR_NAME,IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE) );
                IARM_CHECK( IARM_Bus_UnRegisterEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE) );
            }
        }

        void FrameRate::Deinitialize(PluginHost::IShell* /* service */)
        {
		DeinitializeIARM();
    		FrameRate::_instance = nullptr;
        }

        string FrameRate::Information() const
        {
            return (string());
        }

        uint32_t FrameRate::setCollectionFrequencyWrapper(const JsonObject& parameters, JsonObject& response)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFOMETHOD();
            
            int fpsFrequencyInMilliseconds = DEFAULT_FPS_COLLECTION_TIME_IN_MILLISECONDS;
            if (parameters.HasLabel("frequency"))
            {
                fpsFrequencyInMilliseconds = std::stod(parameters["frequency"].String());
            }
            setCollectionFrequency(fpsFrequencyInMilliseconds);
            
            returnResponse(true);
        }
        
        uint32_t FrameRate::startFpsCollectionWrapper(const JsonObject& parameters, JsonObject& response)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFOMETHOD();

            returnResponse(startFpsCollection());
        }
        
        uint32_t FrameRate::stopFpsCollectionWrapper(const JsonObject& parameters, JsonObject& response)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFOMETHOD();

            returnResponse(stopFpsCollection());
        }
        
        uint32_t FrameRate::updateFpsWrapper(const JsonObject& parameters, JsonObject& response)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFOMETHOD();
            
            if (!parameters.HasLabel("newFpsValue"))
            {
                returnResponse(false);
            }
            
            updateFps(parameters["newFpsValue"].Number());

            returnResponse(true);
        }
        
	uint32_t FrameRate::setFrmMode(const JsonObject& parameters, JsonObject& response)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFOMETHOD();
            returnIfParamNotFound(parameters, "frmmode");

            string sPortId = parameters["frmmode"].String();
            int frfmode = 0;
            try {
                frfmode = stoi(sPortId);
            }catch (const device::Exception& err) {
                LOG_DEVICE_EXCEPTION1(sPortId);
                returnResponse(false);
            }

            bool success = true;
            try
            {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.setFRFMode(frfmode);
            }
            catch (const device::Exception& err)
            {
                LOG_DEVICE_EXCEPTION1(sPortId);
                success = false;
            }
            returnResponse(success);
        }

        uint32_t FrameRate::getFrmMode(const JsonObject& parameters, JsonObject& response)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFOMETHOD();

            int frmmode = dsHDRSTANDARD_NONE;
            bool success = true;
            try
            {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.getFRFMode(&frmmode);
            }
            catch(const device::Exception& err)
            {
                LOG_DEVICE_EXCEPTION0();
                success = false;
            }

            response["auto-frm-mode"] = frmmode;
            returnResponse(success);
        }

        uint32_t FrameRate::setDisplayFrameRate(const JsonObject& parameters, JsonObject& response)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFOMETHOD();
            returnIfParamNotFound(parameters, "framerate");

            string sFramerate = parameters["framerate"].String();

            bool success = true;
            try
            {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.setDisplayframerate(sFramerate.c_str());
            }
            catch (const device::Exception& err)
            {
                LOG_DEVICE_EXCEPTION1(sFramerate);
                success = false;
            }
            returnResponse(success);
        }

        uint32_t FrameRate::getDisplayFrameRate(const JsonObject& parameters, JsonObject& response)
        {
            std::lock_guard<std::mutex> guard(m_callMutex);

            LOGINFOMETHOD();
            char sFramerate[20] ={0};
            bool success = true;
            try
            {
                device::VideoDevice &device = device::Host::getInstance().getVideoDevices().at(0);
                device.getCurrentDisframerate(sFramerate);
            }
            catch (const device::Exception& err)
            {
                LOG_DEVICE_EXCEPTION1(std::string(sFramerate));
                success = false;
            }

            response["framerate"] = std::string(sFramerate);
            returnResponse(success);
        }

        /**
        * @brief This function is used to get the amount of collection interval per milliseconds.
        *
        * @return Integer value of Amount of milliseconds per collection interval .
        */
        int FrameRate::getCollectionFrequency()
        {
            return m_fpsCollectionFrequencyInMs;
        }

        /**
        * @brief This function is used to set the amount of collection interval per milliseconds.
        *
        * @param[in] frequencyInMs Amount of milliseconds per collection interval.
        * @ingroup SERVMGR_ABSFRAMERATE_API
        */
        void FrameRate::setCollectionFrequency(int frequencyInMs)
        {
            m_fpsCollectionFrequencyInMs = frequencyInMs;
        }

        /**
        * @brief This function is used to start the fps collection. Stop the fps timer before
        * start the fps collection frequency. Fps collection frequency is updated to minimum fps
        * collection time if the fps collection frequency is less than the minimum fps collection time
        * and start the fps timer.
        *
        * @return true on success else false if there was an error.
        * @ingroup SERVMGR_ABSFRAMERATE_API
        */
        bool FrameRate::startFpsCollection()
        {
            if (m_fpsCollectionInProgress)
            {
                return false;
            }
            if (m_reportFpsTimer->isActive())
            {
                m_reportFpsTimer->stop();
            }
            m_minFpsValue = DEFAULT_MIN_FPS_VALUE;
            m_maxFpsValue = DEFAULT_MAX_FPS_VALUE;
            m_totalFpsValues = 0;
            m_numberOfFpsUpdates = 0;
            m_fpsCollectionInProgress = true;
            int fpsCollectionFrequency = m_fpsCollectionFrequencyInMs;
            if (fpsCollectionFrequency < MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS)
            {
                fpsCollectionFrequency = MINIMUM_FPS_COLLECTION_TIME_IN_MILLISECONDS;
            }
            m_reportFpsTimer->start(fpsCollectionFrequency);
            enableFpsCollection();
            return true;
        }

        /**
        * @brief This function is used to stops the fps collection. Stop the fps timer before disable the
        * fps collection. If the number of fps updates is greater than 0, update the fps collection by
        * passing the minimum fps, maximum fps and average fps values  and disable the fps collection.
        *
        * @return true on success or false if there was an error.
        * @ingroup SERVMGR_ABSFRAMERATE_API
        */
        bool FrameRate::stopFpsCollection()
        {
            if (m_reportFpsTimer->isActive())
            {
                m_reportFpsTimer->stop();
            }
            if (m_fpsCollectionInProgress)
            {
                m_fpsCollectionInProgress = false;
                int averageFps = -1;
                int minFps = -1;
                int maxFps = -1;
                if (m_numberOfFpsUpdates > 0)
                {
                averageFps = (m_totalFpsValues / m_numberOfFpsUpdates);
                minFps = m_minFpsValue;
                maxFps = m_maxFpsValue;
                fpsCollectionUpdate(averageFps, minFps, maxFps);
                }
                disableFpsCollection();
            }
            return true;
        }

        /**
        * @brief This function is used to update the FPS value.
        *
        * @param[in] newFpsValue Latest amount of milliseconds per collection interval.
        * @ingroup SERVMGR_ABSFRAMERATE_API
        */
        void FrameRate::updateFps(int newFpsValue)
        {
            if (newFpsValue > m_maxFpsValue)
            {
                m_maxFpsValue = newFpsValue;
            }
            if (newFpsValue < m_minFpsValue)
            {
                m_minFpsValue = newFpsValue;
            }
            m_totalFpsValues += newFpsValue;
            m_numberOfFpsUpdates++;
            m_lastFpsValue = newFpsValue;
        }
        
        void FrameRate::fpsCollectionUpdate( int averageFps, int minFps, int maxFps )
        {
            JsonObject params;
            params["average"] = averageFps;
            params["min"] = minFps;
            params["max"] = maxFps;

            std::string json;
            params.ToString(json);
            LOGINFO("Notify %s %s", EVENT_FPS_UPDATE, json.c_str());
            Notify(EVENT_FPS_UPDATE, params);
            GetHandler(2)->Notify(EVENT_FPS_UPDATE, params);
        }
        
        void FrameRate::onReportFpsTimer()
        {
            std::lock_guard<std::mutex> guard(m_callMutex);
            
            int averageFps = -1;
            int minFps = -1;
            int maxFps = -1;
            if (m_numberOfFpsUpdates > 0)
            {
                averageFps = (m_totalFpsValues / m_numberOfFpsUpdates);
                minFps = m_minFpsValue;
                maxFps = m_maxFpsValue;
            }
            fpsCollectionUpdate(averageFps, minFps, maxFps);
            if (m_lastFpsValue >= 0)
            {
                // store the last fps value just in case there are no updates
                m_minFpsValue = m_lastFpsValue;
                m_maxFpsValue = m_lastFpsValue;
                m_totalFpsValues = m_lastFpsValue;
                m_numberOfFpsUpdates = 1;
            }
            else
            {
                m_minFpsValue = DEFAULT_MIN_FPS_VALUE;
                m_maxFpsValue = DEFAULT_MAX_FPS_VALUE;
                m_totalFpsValues = 0;
                m_numberOfFpsUpdates = 0;
            }
        }

	void FrameRate::FrameRatePreChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if(FrameRate::_instance)
            {
                FrameRate::_instance->frameRatePreChange();
            }
        }

        void FrameRate::frameRatePreChange()
        {
            LOGINFO("Notify %s", EVENT_FRAMERATE_PRECHANGE);
            Notify(EVENT_FRAMERATE_PRECHANGE, JsonObject());
            GetHandler(2)->Notify(EVENT_FRAMERATE_PRECHANGE, JsonObject());
        }

        void FrameRate::FrameRatePostChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
        {
            if(FrameRate::_instance)
            {
                FrameRate::_instance->frameRatePostChange();
            }
        }

        void FrameRate::frameRatePostChange()
        {
            LOGINFO("Notify %s", EVENT_FRAMERATE_POSTCHANGE);
            Notify(EVENT_FRAMERATE_POSTCHANGE, JsonObject());
            GetHandler(2)->Notify(EVENT_FRAMERATE_POSTCHANGE, JsonObject());
        }

        
    } // namespace Plugin
} // namespace WPEFramework
