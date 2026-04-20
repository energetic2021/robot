/**
 * AIUI 语音交互节点 (AIUI Voice Interaction Node)
 * 功能：作为 ROS 与科大讯飞 AIUI 云端交互的桥梁，实现语音唤醒、ASR、NLP 语义分析以及 TTS 语音合成。
 */

#include <ros/ros.h>
#include <std_msgs/String.h>
#include <std_msgs/Bool.h>
#include <std_msgs/Int32.h>
#include <std_srvs/SetBool.h>
#include <std_srvs/Trigger.h>

// 注释掉audio_common_msgs相关头文件
// #include <audio_common_msgs/AudioData.h>
// #include <audio_common_msgs/AudioInfo.h>


// 在包含所有头文件之前，添加这行
#define aiui_va aiui_v2  // 将 aiui_va 映射到 aiui_v2

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <functional>
#include <signal.h>

// AIUI相关头文件
#include "aiui/AIUI_V2.h"
#include "aiui/PcmPlayer_C.h"
#include "json/json.h"
#include "utils/StreamNlpTtsHelper.h"
#include "utils/IatResultUtil.h"
#include "utils/Base64Util.h"

// using namespace aiui_va;
using namespace aiui_v2;

// ==================== 配置宏 ====================
#define AIUI_VER 3

#ifdef WIN32
    #include <windows.h>
    #define _HAS_STD_BYTE 0
    #define AIUI_SLEEP Sleep
#else
    #include <unistd.h>
    #define AIUI_SLEEP(x) usleep(x * 1000)
#endif

#undef AIUI_LIB_COMPILING

#define CHECK_FUNC_IF_SUCCESS(func) \
do {\
    int __ret = func; \
    if (__ret) { ROS_ERROR("call %s failed, return %d", #func, __ret); } \
} while(0)

// ==================== 全局变量 ====================
std::string gSyncSid;
std::string gVoiceCloneResId;
int gPcmPlayerIndex = -1;
int gCustomizeWakeUpWordId = -1;
const char* gVpMemberId = "ros_member";
std::string gVpFeatureId;

// ==================== ROS相关全局变量 ====================
ros::Publisher result_pub;
// ros::Publisher tts_audio_pub;  // 注释掉不需要的publisher
ros::Publisher state_pub;
ros::Publisher asr_result_pub;
ros::Publisher nlp_result_pub;

ros::ServiceServer wakeup_srv;
ros::ServiceServer sleep_srv;
ros::ServiceServer tts_srv;
ros::ServiceServer reset_srv;
ros::ServiceServer start_record_srv;
ros::ServiceServer stop_record_srv;
ros::ServiceServer write_text_srv;

ros::Subscriber text_input_sub;  // 文本输入话题

// ros::Subscriber audio_sub;  // 注释掉不需要的subscriber

// std::mutex audio_mutex;  // 注释掉音频处理相关变量
// std::condition_variable audio_cv;
// std::queue<std::vector<uint8_t>> audio_queue;
bool recording_active = false;
// std::thread audio_process_thread;  // 注释掉音频处理线程

// AIUI全局变量
class AIUIListener;
AIUIListener* g_pListener = nullptr;
IAIUIAgent* g_pAgent = nullptr;
std::string g_work_dir;

// ==================== 工具函数 ====================
bool parseAsJson(const std::string& s, Json::Value& json)
{
    Json::Reader reader;
    return reader.parse(s, json, false);
}

std::string readFileAsString(const std::string& path)
{
    std::fstream t(path, std::ios::in | std::ios::binary);
    if (!t.is_open()) {
        ROS_ERROR("Error open file: %s", path.c_str());
        return "";
    }
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    return str;
}

bool saveToFile(const std::string& content, const std::string& path)
{
    std::fstream fs;
    fs.open(path, std::ios::binary | std::ios::out);
    if (!fs.is_open()) {
        return false;
    }
    fs.write(content.c_str(), content.length());
    fs.close();
    return true;
}

// ==================== 播放回调函数 ====================
void onStarted()
{
    ROS_INFO("PcmPlayer started");
    std_msgs::String msg;
    msg.data = "tts_started";
    state_pub.publish(msg);
}

void onPaused()
{
    ROS_INFO("PcmPlayer paused");
}

void onResumed()
{
    ROS_INFO("PcmPlayer resumed");
}

void onStopped()
{
    ROS_INFO("PcmPlayer stopped");
    std_msgs::String msg;
    msg.data = "tts_stopped";
    state_pub.publish(msg);
}

void onError(int error, const char* des)
{
    ROS_ERROR("PcmPlayer error: %d, %s", error, des);
    std_msgs::String msg;
    msg.data = "tts_error";
    state_pub.publish(msg);
}

void onProgress(int streamId, int progress, const char* audio, int len, bool isCompleted)
{
    if (isCompleted) {
        ROS_INFO("TTS playback completed");
        std_msgs::String msg;
        msg.data = "tts_completed";
        state_pub.publish(msg);
    }
}

// ==================== AIUI消息发送宏 ====================
#define SEND_AIUIMESSAGE(cmd, arg1, arg2, params, data)                               \
    do {                                                                         \
        if (!g_pAgent) break;                                                       \
        IAIUIMessage* msg = IAIUIMessage::create(cmd, arg1, arg2, params, data); \
        g_pAgent->sendMessage(msg);                                                 \
        msg->destroy();                                                          \
    } while (false)

#define SEND_AIUIMESSAGE4(cmd, arg1, arg2, params) SEND_AIUIMESSAGE(cmd, arg1, arg2, params, nullptr)
#define SEND_AIUIMESSAGE3(cmd, arg1, arg2)              SEND_AIUIMESSAGE4(cmd, arg1, arg2, "")
#define SEND_AIUIMESSAGE2(cmd, arg1)                         SEND_AIUIMESSAGE3(cmd, arg1, 0)
#define SEND_AIUIMESSAGE1(cmd)                                    SEND_AIUIMESSAGE2(cmd, 0)

// ==================== AIUI函数声明 ====================
void startTTS(const std::string& text, const std::string& tag = "");

// ==================== AIUI监听器类 ====================
class AIUIListener : public IAIUIListener
{
private:
    class TtsHelperListener : public StreamNlpTtsHelper::Listener
    {
    public:
        void onText(const StreamNlpTtsHelper::OutTextSeg& textSeg) override {
            if (textSeg.isBegin() || textSeg.isEnd()) {
                if (aiui_pcm_player_get_state() != PCM_PLAYER_STATE_STARTED) {
                    CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_start());
                }
                if (textSeg.isBegin()) {
                    CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_clear());
                }
            }
            startTTS(textSeg.mText, textSeg.mTag);
        }

        void onFinish(const std::string& fullText) override {
            ROS_INFO("TTS finished, full text: %s", fullText.c_str());
        }

        void onTtsData(const Json::Value& bizParamJson, const char* audio, int len) override {
            const Json::Value& data = (bizParamJson["data"])[0];
            const Json::Value& content = (data["content"])[0];
            int dts = content["dts"].asInt();
            int progress = content["text_percent"].asInt();

            if (len > 1) {
                CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_write(0, audio, len, dts, progress));
            } else if (dts == AIUIConstant::DTS_ONE_BLOCK || dts == AIUIConstant::DTS_BLOCK_LAST) {
                char data[2] = {0};
                CHECK_FUNC_IF_SUCCESS(
                    aiui_pcm_player_write(0, data, 2, AIUIConstant::DTS_BLOCK_LAST, 100));
            }
        }
    };

private:
    std::shared_ptr<StreamNlpTtsHelper> m_pTtsHelper;
    int mStreamTtsIndex{0};
    std::string mCurIatSid;
    std::string mIatTextBuffer;
    std::string mStreamNlpAnswerBuffer;
    int mIntentCnt{0};

public:
    AIUIListener()
    {
        // 初始化PCM播放器
        CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_create());

        int count = aiui_pcm_player_get_output_device_count();
        for (int i = 0; i < count; i++) {
            ROS_INFO("PCM device %d: %s", i, aiui_pcm_player_get_device_name(i));
        }
        ROS_INFO("Using PCM device index: %d", gPcmPlayerIndex);

        CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_init(gPcmPlayerIndex));
        aiui_pcm_player_set_callbacks(
            onStarted, onPaused, onResumed, onStopped, onProgress, onError);

        std::shared_ptr<TtsHelperListener> listener = std::make_shared<TtsHelperListener>();
        m_pTtsHelper = std::make_shared<StreamNlpTtsHelper>(listener);
        m_pTtsHelper->setTextMinLimit(20);
    }

    ~AIUIListener()
    {
        aiui_pcm_player_destroy();
    }

    void onEvent(const IAIUIEvent& event) override
    {
        try {
            handleEvent(event);
        } catch (std::exception& e) {
            ROS_ERROR("Exception in onEvent: %s", e.what());
        }
    }

private:
    /**
     * AIUI 事件分发处理中心
     */
    void handleEvent(const IAIUIEvent& event)
    {
        switch (event.getEventType()) {
            case AIUIConstant::EVENT_STATE: { // 状态机状态改变
                handleStateEvent(event);
            } break;

            case AIUIConstant::EVENT_WAKEUP: { // 语音唤醒事件
                handleWakeupEvent(event);
            } break;

            case AIUIConstant::EVENT_SLEEP: { // 自动休眠事件
                handleSleepEvent(event);
            } break;

            case AIUIConstant::EVENT_VAD: { // 语音活动检测（开始说话/停止说话）
                handleVadEvent(event);
            } break;

            case AIUIConstant::EVENT_RESULT: { // 语义解析/识别结果返回
                handleResultEvent(event);
            } break;

            case AIUIConstant::EVENT_CMD_RETURN: {
                handleCmdReturnEvent(event);
            } break;

            case AIUIConstant::EVENT_START_RECORD: {
                ROS_INFO("EVENT_START_RECORD");
                std_msgs::String msg;
                msg.data = "recording_started";
                state_pub.publish(msg);
            } break;

            case AIUIConstant::EVENT_STOP_RECORD: {
                ROS_INFO("EVENT_STOP_RECORD");
                std_msgs::String msg;
                msg.data = "recording_stopped";
                state_pub.publish(msg);
            } break;

            case AIUIConstant::EVENT_ERROR: {
                ROS_ERROR("AIUI Error: %d, %s", event.getArg1(), event.getInfo());
                std_msgs::String msg;
                msg.data = "error";
                state_pub.publish(msg);
            } break;

            case AIUIConstant::EVENT_CONNECTED_TO_SERVER: {
                std::string uid = event.getData()->getString("uid", "");
                ROS_INFO("Connected to server, uid=%s", uid.c_str());
                std_msgs::String msg;
                msg.data = "connected";
                state_pub.publish(msg);
            } break;

            case AIUIConstant::EVENT_SERVER_DISCONNECTED: {
                ROS_INFO("Disconnected from server");
                std_msgs::String msg;
                msg.data = "disconnected";
                state_pub.publish(msg);
            } break;

            default:
                break;
        }
    }

    void handleStateEvent(const IAIUIEvent& event)
    {
        std_msgs::String msg;
        switch (event.getArg1()) {
            case AIUIConstant::STATE_IDLE:
                ROS_INFO("AIUI State: IDLE");
                msg.data = "idle";
                break;
            case AIUIConstant::STATE_READY:
                ROS_INFO("AIUI State: READY");
                msg.data = "ready";
                break;
            case AIUIConstant::STATE_WORKING:
                ROS_INFO("AIUI State: WORKING");
                msg.data = "working";
                break;
        }
        state_pub.publish(msg);
    }

    void handleWakeupEvent(const IAIUIEvent& event)
    {
        int eventType = 0;
        Json::Value wakeupJson;
        if (parseAsJson(event.getInfo(), wakeupJson)) {
            Json::Value ivwResult = wakeupJson["ivw_result"];
            if (ivwResult.isMember("event_type")) {
                eventType = atoi(ivwResult["event_type"].asString().c_str());
            }
        }
        ROS_INFO("AIUI Wakeup, event_type=%d", eventType);
        
        std_msgs::String msg;
        msg.data = "wakeup";
        state_pub.publish(msg);
        
        if (0 == eventType) {
            CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_stop());
        }
    }

    void handleSleepEvent(const IAIUIEvent& event)
    {
        ROS_INFO("AIUI Sleep, arg1=%d", event.getArg1());
        std_msgs::String msg;
        msg.data = "sleep";
        state_pub.publish(msg);
    }

    void handleVadEvent(const IAIUIEvent& event)
    {
        std_msgs::String msg;
        switch (event.getArg1()) {
            case AIUIConstant::VAD_BOS:
                ROS_INFO("VAD: BOS - 开始说话");
                msg.data = "vad_bos";
                state_pub.publish(msg);
                break;
            case AIUIConstant::VAD_EOS:
                ROS_INFO("VAD: EOS - 说话结束");
                msg.data = "vad_eos";
                state_pub.publish(msg);
                break;
            case AIUIConstant::VAD_VOL:
                break;
        }
    }

    void handleResultEvent(const IAIUIEvent& event)
    {
        Json::Value bizParamJson;
        if (!parseAsJson(event.getInfo(), bizParamJson)) {
            ROS_ERROR("Parse result info error: %s", event.getInfo());
            return;
        }

        Json::Value& data = (bizParamJson["data"])[0];
        Json::Value& params = data["params"];
        Json::Value& content = (data["content"])[0];

        std::string sub = params["sub"].asString();
        std::string sid = event.getData()->getString("sid", "");

        Json::Value empty;
        std::string cnt_id = content.get("cnt_id", empty).asString();
        int dataLen = 0;
        const char* buffer = event.getData()->getBinary(cnt_id.c_str(), &dataLen);

        if (sub == "iat") {
            handleIatResult(buffer, dataLen, params, sid);
        } else if (sub == "nlp") {
            handleNlpResult(buffer, dataLen, params, sid, event);
        } else if (sub == "tts") {
            handleTtsResult(buffer, dataLen, content, sid, event);
        } else if (sub == "cbm_tidy") {
            handleCbmTidyResult(buffer, dataLen);
        }
    }

    void handleIatResult(const char* buffer, int dataLen, Json::Value& params, const std::string& sid)
    {
        std::string resultStr = std::string(buffer, dataLen);
        Json::Value resultJson;
        
        if (parseAsJson(resultStr, resultJson)) {
            Json::Value textJson = resultJson["text"];
            
            if (textJson.isMember("pgs")) {
                mIatTextBuffer = IatResultUtil::parsePgsIatText(textJson);
            } else {
                mIatTextBuffer.append(IatResultUtil::parseIatResult(textJson));
            }

            bool isLast = textJson["ls"].asBool();
            if (isLast) {
                ROS_INFO("ASR Final Result: %s", mIatTextBuffer.c_str());
                
                std_msgs::String msg;
                msg.data = mIatTextBuffer;
                result_pub.publish(msg);
                asr_result_pub.publish(msg);
                
                mIatTextBuffer.clear();
            } else {
                std_msgs::String msg;
                msg.data = mIatTextBuffer;
                asr_result_pub.publish(msg);
            }
        }
    }

    void handleNlpResult(const char* buffer, int dataLen, Json::Value& params, 
                         const std::string& sid, const IAIUIEvent& event)
    {
        std::string resultStr = std::string(buffer, dataLen);
        Json::Value resultJson;
        
        if (parseAsJson(resultStr, resultJson)) {
            if (resultJson.isMember("intent") && resultJson["intent"].isMember("rc")) {
                Json::Value intentJson = resultJson["intent"];
                std::string answer = intentJson["answer"]["text"].asString();
                
                ROS_INFO("NLP Intent: rc=%d, answer=%s", 
                         intentJson["rc"].asInt(), answer.c_str());
                
                std_msgs::String msg;
                msg.data = answer;
                result_pub.publish(msg);
                nlp_result_pub.publish(msg);
                
#ifndef USE_RAPID_INTERACTION
                if (!answer.empty()) {
                    startTTS(answer);
                }
#endif
            } else if (resultJson.isMember("nlp")) {
                handleLLMResult(resultJson, params, sid, event);
            }
        }
    }

    void handleLLMResult(Json::Value& resultJson, Json::Value& params, 
                         const std::string& sid, const IAIUIEvent& event)
    {
        Json::Value nlpJson = resultJson["nlp"];
        std::string text = nlpJson["text"].asString();
        int seq = nlpJson["seq"].asInt();
        int status = nlpJson["status"].asInt();
        
        if (status == 0) {
            mStreamTtsIndex = 0;
        }
        
        m_pTtsHelper->addText(text, mStreamTtsIndex++, status);
        mStreamNlpAnswerBuffer.append(text);
        
        ROS_INFO("LLM Result: seq=%d, status=%d, partial=%s", 
                 seq, status, text.c_str());
        
        std_msgs::String msg;
        msg.data = text;
        nlp_result_pub.publish(msg);
        
        if (status == 2) {
            ROS_INFO("LLM Full Answer: %s", mStreamNlpAnswerBuffer.c_str());
            
            std_msgs::String full_msg;
            full_msg.data = mStreamNlpAnswerBuffer;
            result_pub.publish(full_msg);
            
            mStreamNlpAnswerBuffer.clear();
        }
    }

    void handleTtsResult(const char* buffer, int dataLen, Json::Value& content,
                         const std::string& sid, const IAIUIEvent& event)
    {
        Json::Value empty;
        Json::Value&& isUrl = content.get("url", empty);
        
        if (isUrl.asString() == "1") {
            std::string url = std::string(buffer, dataLen);
            ROS_INFO("TTS URL: %s", url.c_str());
        } else {
            int dts = content["dts"].asInt();
            std::string tag = event.getData()->getString("tag", "");
            
            if (dts == AIUIConstant::DTS_BLOCK_FIRST || dts == AIUIConstant::DTS_ONE_BLOCK) {
                if (aiui_pcm_player_get_state() != PCM_PLAYER_STATE_STARTED) {
                    CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_start());
                }
            }
            
            if (dataLen > 1) {
                CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_write(0, buffer, dataLen, 1, 0));
            }
            
            if (dts == AIUIConstant::DTS_ONE_BLOCK || dts == AIUIConstant::DTS_BLOCK_LAST) {
                char data[2] = {0};
                CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_write(
                    0, data, 2, AIUIConstant::DTS_BLOCK_LAST, 100));
            }
        }
    }

    void handleCbmTidyResult(const char* buffer, int dataLen)
    {
        std::string intentStr = std::string(buffer, dataLen);
        Json::Value tmpJson;
        if (parseAsJson(intentStr, tmpJson)) {
            Json::Value intentTextJson = tmpJson["cbm_tidy"]["text"];
            if (!intentTextJson.empty() && parseAsJson(intentTextJson.asString(), tmpJson)) {
                mIntentCnt = tmpJson["intent"].size();
                ROS_INFO("cbm_intent_cnt: %d", mIntentCnt);
            }
        }
    }

    void handleCmdReturnEvent(const IAIUIEvent& event)
    {
        if (AIUIConstant::CMD_SYNC == event.getArg1()) {
            int dtype = event.getData()->getInt("sync_dtype", -1);
            int retCode = event.getArg2();

            if (AIUIConstant::SUCCESS == retCode) {
                gSyncSid = event.getData()->getString("sid", "");
                std::string tag = event.getData()->getString("tag", "");
                long timeSpent = event.getData()->getLong("time_spent", -1);
                
                ROS_INFO("Sync success, sid=%s, tag=%s, time=%ldms", 
                         gSyncSid.c_str(), tag.c_str(), timeSpent);
            } else {
                ROS_ERROR("Sync failed, error=%d", retCode);
            }
        }
    }
};

// ==================== AIUI函数实现 ====================
void createAgent(const std::string& cfgPath)
{
    if (g_pAgent) return;
    
    std::string aiuiParams = readFileAsString(cfgPath);
    
    Json::Value paramJson;
    Json::Reader reader;
    if (reader.parse(aiuiParams, paramJson, false)) {
        ROS_INFO("AIUI Params: %s", paramJson.toStyledString().c_str());
        g_pAgent = IAIUIAgent::createAgent(paramJson.toStyledString().c_str(), g_pListener);
    }
    
    if (!g_pAgent) {
        ROS_ERROR("Failed to create AIUI agent: %s", cfgPath.c_str());
    } else {
        ROS_INFO("AIUI agent created successfully");
    }
}

void destroyAgent()
{
    if (g_pAgent) {
        g_pAgent->destroy();
        g_pAgent = nullptr;
        ROS_INFO("AIUI agent destroyed");
    }
}

void wakeup()
{
    if (!g_pAgent) {
        ROS_WARN("AIUI agent not initialized");
        return;
    }
    SEND_AIUIMESSAGE4(AIUIConstant::CMD_WAKEUP, 0, 0, "clear_data=true");
    ROS_INFO("Wakeup command sent");
}

void resetWakeup()
{
    if (!g_pAgent) return;
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_RESET_WAKEUP);
    ROS_INFO("Reset wakeup command sent");
}

void start()
{
    if (!g_pAgent) return;
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_START);
    ROS_INFO("Start command sent");
}

void stop()
{
    if (!g_pAgent) return;
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_STOP);
    ROS_INFO("Stop command sent");
}

void resetAIUI()
{
    if (!g_pAgent) return;
    SEND_AIUIMESSAGE(AIUIConstant::CMD_RESET, 0, 0, "", nullptr);
    ROS_INFO("Reset AIUI command sent");
}

void startRecordAudio()
{
    if (!g_pAgent) {
        ROS_WARN("AIUI agent not initialized");
        return;
    }
    
    recording_active = true;
    
    SEND_AIUIMESSAGE4(
        AIUIConstant::CMD_START_RECORD, 0, 0, 
        "data_type=audio,pers_param={\"uid\":\"\"},tag=record-tag");
    ROS_INFO("Start record command sent");
}

void stopRecordAudio()
{
    if (!g_pAgent) return;
    
    recording_active = false;
    
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_STOP_RECORD);
    ROS_INFO("Stop record command sent");
}

void writeText(const std::string& text, bool needWakeup = true)
{
    if (!g_pAgent) {
        ROS_WARN("AIUI agent not initialized");
        return;
    }
    
    AIUIBuffer textData = aiui_create_buffer_from_data(text.c_str(), text.length());
    
    if (needWakeup) {
        SEND_AIUIMESSAGE(AIUIConstant::CMD_WRITE, 0, 0, 
                         "data_type=text,pers_param={\"uid\":\"\"}", textData);
    } else {
        SEND_AIUIMESSAGE(AIUIConstant::CMD_WRITE, 0, 0, 
                         "data_type=text,need_wakeup=false", textData);
    }
    
    ROS_INFO("Write text command sent: %s", text.c_str());
}

void startTTS(const std::string& text, const std::string& tag)
{
    if (!g_pAgent) {
        ROS_WARN("AIUI agent not initialized");
        return;
    }
    
    AIUIBuffer textData = aiui_create_buffer_from_data(text.c_str(), text.length());
    std::string params = "voice_name=x5_lingxiaoyue_flow";
    if (!tag.empty()) {
        params.append(",tag=").append(tag);
    }
    
    SEND_AIUIMESSAGE(AIUIConstant::CMD_TTS, AIUIConstant::START, 0, params.c_str(), textData);
    ROS_INFO("TTS command sent: %s", text.c_str());
}

// ==================== ROS服务回调 ====================
bool wakeupService(std_srvs::Trigger::Request& req, std_srvs::Trigger::Response& res)
{
    ROS_INFO("Wakeup service called");
    
    if (!g_pAgent) {
        res.success = false;
        res.message = "AIUI agent not initialized";
        return true;
    }
    
    wakeup();
    
    res.success = true;
    res.message = "Wakeup command sent";
    return true;
}

bool sleepService(std_srvs::Trigger::Request& req, std_srvs::Trigger::Response& res)
{
    ROS_INFO("Sleep service called");
    
    if (!g_pAgent) {
        res.success = false;
        res.message = "AIUI agent not initialized";
        return true;
    }
    
    resetWakeup();
    
    res.success = true;
    res.message = "Sleep command sent";
    return true;
}

bool ttsService(std_srvs::SetBool::Request& req, std_srvs::SetBool::Response& res)
{
    ROS_INFO("TTS service called");
    
    if (!g_pAgent) {
        res.success = false;
        res.message = "AIUI agent not initialized";
        return true;
    }
    
    if (req.data) {
        startTTS("你好，我是你的语音助手", "tts-service");
    }
    
    res.success = true;
    res.message = "TTS command sent";
    return true;
}

bool resetService(std_srvs::Trigger::Request& req, std_srvs::Trigger::Response& res)
{
    ROS_INFO("Reset service called");
    
    if (!g_pAgent) {
        res.success = false;
        res.message = "AIUI agent not initialized";
        return true;
    }
    
    resetAIUI();
    
    res.success = true;
    res.message = "Reset command sent";
    return true;
}

bool startRecordService(std_srvs::Trigger::Request& req, std_srvs::Trigger::Response& res)
{
    ROS_INFO("Start record service called");
    
    if (!g_pAgent) {
        res.success = false;
        res.message = "AIUI agent not initialized";
        return true;
    }
    
    startRecordAudio();
    
    res.success = true;
    res.message = "Start record command sent";
    return true;
}

bool stopRecordService(std_srvs::Trigger::Request& req, std_srvs::Trigger::Response& res)
{
    ROS_INFO("Stop record service called");
    
    if (!g_pAgent) {
        res.success = false;
        res.message = "AIUI agent not initialized";
        return true;
    }
    
    stopRecordAudio();
    
    res.success = true;
    res.message = "Stop record command sent";
    return true;
}

bool writeTextService(std_srvs::SetBool::Request& req, std_srvs::SetBool::Response& res)
{
    ROS_INFO("Write text service called");
    
    if (!g_pAgent) {
        res.success = false;
        res.message = "AIUI agent not initialized";
        return true;
    }
    
    writeText("你好，我叫小飞飞", req.data);
    
    res.success = true;
    res.message = "Write text command sent";
    return true;
}


// ==================== 话题回调函数 ====================
void textInputCallback(const std_msgs::String::ConstPtr& msg)
{
    ROS_INFO("Received text input: %s", msg->data.c_str());
    
    if (!g_pAgent) {
        ROS_WARN("AIUI agent not initialized");
        return;
    }
    
    // 发送文本到AIUI进行交互
    AIUIBuffer textData = aiui_create_buffer_from_data(msg->data.c_str(), msg->data.length());
    
    // 使用need_wakeup=true，需要先唤醒
    SEND_AIUIMESSAGE(AIUIConstant::CMD_WRITE, 0, 0, 
                     "data_type=text,pers_param={\"uid\":\"\"}", textData);
    
    ROS_INFO("Text sent to AIUI: %s", msg->data.c_str());
}

// 带唤醒控制的文本输入回调
void textInputWithWakeupCallback(const std_msgs::String::ConstPtr& msg)
{
    ROS_INFO("Received text input (no wakeup needed): %s", msg->data.c_str());
    
    if (!g_pAgent) {
        ROS_WARN("AIUI agent not initialized");
        return;
    }
    
    // 发送文本到AIUI进行交互，不需要唤醒
    AIUIBuffer textData = aiui_create_buffer_from_data(msg->data.c_str(), msg->data.length());
    
    SEND_AIUIMESSAGE(AIUIConstant::CMD_WRITE, 0, 0, 
                     "data_type=text,need_wakeup=false", textData);
    
    ROS_INFO("Text sent to AIUI (no wakeup): %s", msg->data.c_str());
}

// ==================== 初始化设置 ====================
void initSetting(bool log = true)
{
    AIUISetting::setAIUIDir(g_work_dir.c_str());
    
    std::string msc_dir = g_work_dir + "/msc";
    AIUISetting::setMscDir(msc_dir.c_str());
    
    AIUISetting::setNetLogLevel(log ? aiui_debug : aiui_none);
    
    char sn[64] = {0};
    sprintf(sn, "ROS_AIUI_%d", getpid());
    
    AIUISetting::setSystemInfo(AIUI_KEY_SERIAL_NUM, sn);
    
    ROS_INFO("AIUI initialized with work dir: %s", g_work_dir.c_str());
}

// ==================== 信号处理 ====================
void sigintHandler(int sig)
{
    ROS_INFO("Shutting down AIUI node...");
    
    recording_active = false;
    
    destroyAgent();
    if (g_pListener) {
        delete g_pListener;
        g_pListener = nullptr;
    }
    
    ros::shutdown();
}

// ==================== 主函数 ====================
int main(int argc, char** argv)
{
    ros::init(argc, argv, "aiui_node");
    ros::NodeHandle nh;
    ros::NodeHandle private_nh("~");
    
    private_nh.param("pcm_device_index", gPcmPlayerIndex, -1);
    
    std::string cfg_path;
    private_nh.param("cfg_path", cfg_path, std::string("cfg/aiui.cfg"));
    
    bool debug;
    private_nh.param("debug", debug, false);
    
    private_nh.param("work_dir", g_work_dir, std::string("."));
    
    if (cfg_path[0] != '/') {
        cfg_path = g_work_dir + "/" + cfg_path;
    }
    
    ROS_INFO("AIUI Node starting with:");
    ROS_INFO("  PCM device index: %d", gPcmPlayerIndex);
    ROS_INFO("  Config path: %s", cfg_path.c_str());
    ROS_INFO("  Work dir: %s", g_work_dir.c_str());
    
    // 创建Publisher
    result_pub = nh.advertise<std_msgs::String>("aiui/result", 10);
    asr_result_pub = nh.advertise<std_msgs::String>("aiui/asr_result", 10);
    nlp_result_pub = nh.advertise<std_msgs::String>("aiui/nlp_result", 10);
    state_pub = nh.advertise<std_msgs::String>("aiui/state", 10);


    
    // 创建Subscriber
    // audio_sub = nh.subscribe("audio_input", 10, audioCallback);  // 如果有音频输入可以取消注释

    // 添加文本输入话题订阅
    text_input_sub = nh.subscribe("aiui/text_input", 10, textInputCallback);
    // 如果需要不唤醒就交互，可以添加另一个话题
    // text_input_nowakeup_sub = nh.subscribe("aiui/text_input_nowakeup", 10, textInputWithWakeupCallback);

    
    // 创建Service
    wakeup_srv = nh.advertiseService("aiui/wakeup", wakeupService);
    sleep_srv = nh.advertiseService("aiui/sleep", sleepService);
    tts_srv = nh.advertiseService<std_srvs::SetBool::Request, std_srvs::SetBool::Response>(
        "aiui/tts", boost::bind(ttsService, _1, _2));
    reset_srv = nh.advertiseService("aiui/reset", resetService);
    start_record_srv = nh.advertiseService("aiui/start_record", startRecordService);
    stop_record_srv = nh.advertiseService("aiui/stop_record", stopRecordService);
    write_text_srv = nh.advertiseService<std_srvs::SetBool::Request, std_srvs::SetBool::Response>(
        "aiui/write_text", boost::bind(writeTextService, _1, _2));
    
    // 打印SDK版本
    ROS_INFO("AIUI Version: %s", getVersion());
    
    // 初始化设置
    initSetting(debug);
    
    // 创建监听器和Agent
    g_pListener = new AIUIListener();
    createAgent(cfg_path);
    
    // 自动启动
    start();
    
    // 设置信号处理
    signal(SIGINT, sigintHandler);
    
    ROS_INFO("==========================================");
    ROS_INFO("AIUI Node started successfully!");
    ROS_INFO("Services:");
    ROS_INFO("  /aiui/wakeup - Wake up AIUI");
    ROS_INFO("  /aiui/sleep - Put AIUI to sleep");
    ROS_INFO("  /aiui/tts - Text to speech");
    ROS_INFO("  /aiui/reset - Reset AIUI");
    ROS_INFO("  /aiui/start_record - Start recording");
    ROS_INFO("  /aiui/stop_record - Stop recording");
    ROS_INFO("  /aiui/write_text - Write text for interaction");
    ROS_INFO("Topics:");
    ROS_INFO("  Published:");
    ROS_INFO("    /aiui/result - Final result");
    ROS_INFO("    /aiui/asr_result - ASR intermediate results");
    ROS_INFO("    /aiui/nlp_result - NLP/LLM results");
    ROS_INFO("    /aiui/state - AIUI state changes");
    ROS_INFO("  Subscribed:");
    ROS_INFO("    /aiui/text_input - Text input for interaction (requires wakeup)");
    // ROS_INFO("    /aiui/text_input_nowakeup - Text input without wakeup");
    ROS_INFO("==========================================");
    
    ros::spin();
    
    destroyAgent();
    if (g_pListener) {
        delete g_pListener;
        g_pListener = nullptr;
    }
    
    ROS_INFO("AIUI Node shutdown complete");
    
    return 0;
}