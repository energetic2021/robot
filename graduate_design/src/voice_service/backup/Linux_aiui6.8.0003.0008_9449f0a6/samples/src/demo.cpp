#ifdef WIN32
    #include <windows.h>

    #define _HAS_STD_BYTE 0
    #define AIUI_SLEEP Sleep
#else
    #include <unistd.h>

    #define AIUI_SLEEP(x) usleep(x * 1000)
#endif

#undef AIUI_LIB_COMPILING

#include <cstring>
#include <fstream>
#include <iostream>

#include "aiui/AIUI_V2.h"
#include "aiui/PcmPlayer_C.h"
#include "json/json.h"
#include "utils/StreamNlpTtsHelper.h"
#include "utils/IatResultUtil.h"
#include "utils/Base64Util.h"

// 使用的AIUI服务版本：1（语义技能），2（语义技能 + 大模型），3（语义技能 + 大模型 + 极速交互）
#define AIUI_VER 3

#if AIUI_VER == 3
    // 是否使用语义后合成。当在AIUI平台应用配置页面打开"语音合成"开关时，需要打开该宏
    #define USE_POST_SEMANTIC_TTS
#endif

#define CHECK_FUNC_IF_SUCCESS(func) \
do {\
    int __ret = func; \
    if (__ret) { std::cout << "[" << __FILE__ << " line:" << __LINE__ << "]" << "call " \
                 << #func << " failed, return " << __ret << std::endl; } \
} while(0)

using namespace aiui_va;
using namespace aiui_v2;

/*********************JSON处理函数************************/
bool parseAsJson(const std::string& s, Json::Value& json)
{
    Json::Reader reader;
    return reader.parse(s, json, false);
}

/*********************播放回调函数************************/
void onStarted()
{
    std::cout << "PcmPlayer, onStarted" << std::endl;
}

void onPaused()
{
    std::cout << "PcmPlayer, onPaused" << std::endl;
}

void onResumed()
{
    std::cout << "PcmPlayer, onResumed" << std::endl;
}

void onStopped()
{
    std::cout << "PcmPlayer, onStopped" << std::endl;
}

void onError(int error, const char* des)
{
    std::cout << "PcmPlayer, onError, error=" << error << ", des=" << des << std::endl;
}

void onProgress(int streamId, int progress, const char* audio, int len, bool isCompleted)
{
//    std::cout << "PcmPlayer, onProgress, streamId=" << streamId << ", progress=" << progress
//         << ", len=" << len << ", isCompleted=" << isCompleted << std::endl;
}

/*********************全局变量定义************************/
std::string gSyncSid;
std::string gVoiceCloneResId;

int gPcmPlayerIndex = -1;
// 自定义唤醒词的id vtn3.0添加唤醒词会返回这个id后期删除需要传入这个id
int gCustomizeWakeUpWordId = -1;

// 声纹成员id
const char* gVpMemberId = "abc123";
// 声纹特征id
std::string gVpFeatureId;

/*********************函数与类定义************************/
bool saveToFile(const std::string& content, const std::string& path);

void startTTS(const std::string& text, const std::string& tag = "");

// 从IAIUIListener派生自己的结果监听器
class DemoListener : public IAIUIListener
{
private:
    // 从StreamNlpTtsHelper::Listener派生流式合成监听器，用于监听大模型结果的合成
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

            // 调用合成
            startTTS(textSeg.mText, textSeg.mTag);
        }

        void onFinish(const std::string& fullText) override {
            // 文本合成完成回调
            std::cout << "tts, fullText=" << fullText << std::endl;
        }

        void onTtsData(const Json::Value& bizParamJson, const char* audio, int len) override {
            const Json::Value& data = (bizParamJson["data"])[0];
            const Json::Value& content = (data["content"])[0];
            int dts = content["dts"].asInt();
            int progress = content["text_percent"].asInt();

            if (len > 1) {
                // 将合成数据写入播放器
                CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_write(0, audio, len, dts, progress));
            } else if (dts == AIUIConstant::DTS_ONE_BLOCK || dts == AIUIConstant::DTS_BLOCK_LAST) {
                // 人为的插入尾帧
                char data[2] = {0};
                CHECK_FUNC_IF_SUCCESS(
                    aiui_pcm_player_write(0, data, 2, AIUIConstant::DTS_BLOCK_LAST, 100));
            }
        }
    };

private:
    std::shared_ptr<StreamNlpTtsHelper> m_pTtsHelper;

    int mStreamTtsIndex{0};

public:
    DemoListener()
    {
        // 创建内置的pcm播放器，并初始化，设置回调，启动起来
        CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_create());

        int count = aiui_pcm_player_get_output_device_count();
        for (int i = 0; i < count; i++) {
            std::cout << "pcm player index: " << i
                 << " device name: " << aiui_pcm_player_get_device_name(i) << std::endl;
        }
        std::cout << "user pcm player index: " << gPcmPlayerIndex << std::endl;

        CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_init(gPcmPlayerIndex));
        aiui_pcm_player_set_callbacks(
            onStarted, onPaused, onResumed, onStopped, onProgress, onError);
        //aiui_pcm_player_start();

        std::shared_ptr<TtsHelperListener> listener = std::make_shared<TtsHelperListener>();
        m_pTtsHelper = std::make_shared<StreamNlpTtsHelper>(listener);
        m_pTtsHelper->setTextMinLimit(20);
    }

    ~DemoListener()
    {
        // 析构时销毁播放器，释放资源
        aiui_pcm_player_destroy();
    }

    /**
     * 重写onEvent方法，SDK通过回调该方法抛出各种事件，在这里针对事件做对应的处理。
     *
     * @param event
     */
    void onEvent(const IAIUIEvent& event) override
    {
        try {
            handleEvent(event);
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    }

private:
    std::fstream mFs;

    // 当前合成sid
    std::string mCurTtsSid;

    // 当前识别sid
    std::string mCurIatSid;

    // 识别结果缓存
    std::string mIatTextBuffer;

    // 流式nlp的应答语缓存
    std::string mStreamNlpAnswerBuffer;

    // 当前收到了tts音频的长度
    int mTtsLen = 0;

    // 意图的数量
    int mIntentCnt = 0;

private:
    static void processIntentJson(Json::Value& params,
                           Json::Value& intentJson,
                           std::string& resultStr,
                           int eosRsltTime,
                           std::string& sid)
    {
        int rc = intentJson["rc"].asInt();

        Json::Value answerJson = intentJson["answer"];
        std::string answer = answerJson["text"].asString();

        // 正常nlp结果（AIUI通用语义模型返回的语义结果）
        std::cout << "----------------------------------" << std::endl;
        std::cout << "params: " << params.asString() << std::endl;
        std::cout << "nlp: " << resultStr << std::endl;
        std::cout << "eos_result=" << eosRsltTime << "ms" << std::endl;
        std::cout << "结果解析：" << std::endl;
        std::cout << "sid=" << sid << std::endl;
        std::cout << "text（请求文本）: " << intentJson["text"].asString() << std::endl;
        std::cout << "rc=" << rc << ", answer（应答语）: " << answer << std::endl;

        // 极速交互不支持主动合成，只有非主动交互才支持
#ifndef USE_RAPID_INTERACTION
        if (!answer.empty()) {
            startTTS(answer);
        }
#endif
    }

    void handleEvent(const IAIUIEvent& event)
    {
        switch (event.getEventType()) {
            // SDK状态
            case AIUIConstant::EVENT_STATE: {
                switch (event.getArg1()) {
                    case AIUIConstant::STATE_IDLE: {
                        // 空闲状态，即最初始的状态
                        std::cout << "EVENT_STATE: STATE_IDLE" << std::endl;
                    } break;

                    case AIUIConstant::STATE_READY: {
                        // 准备好状态（待唤醒），可以进行唤醒
                        std::cout << "EVENT_STATE: STATE_READY" << std::endl;
                    } break;

                    case AIUIConstant::STATE_WORKING: {
                        // 工作状态（即已唤醒状态），可以语音交互，也可以再次唤醒
                        std::cout << "EVENT_STATE: STATE_WORKING" << std::endl;
                    } break;
                }
            } break;

            // 唤醒事件
            case AIUIConstant::EVENT_WAKEUP: {
                // 正式唤醒值为０, 预唤醒的值为1
                int eventType = 0;
                Json::Value wakeupJson;
                if (parseAsJson(event.getInfo(), wakeupJson)) {
                    Json::Value ivwResult = wakeupJson["ivw_result"];
                    if (ivwResult.isMember("event_type")) {
                        eventType = atoi(ivwResult["event_type"].asString().c_str());
                    }
                }

                std::cout << "EVENT_WAKEUP event_type = " << eventType << " info: " << event.getInfo() << std::endl;
                if (0 ==  eventType) {
                    // 唤醒时停止播放
                    CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_stop());
                }
            } break;

            // 休眠事件，即一段时间无有效交互或者外部主动要求，SDK会自动进入STATE_READY状态
            case AIUIConstant::EVENT_SLEEP: {
                // arg1用来区分休眠类型，是自动休眠还是外部要求，可参考AIUIConstant.h中EVENT_SLEEP的注释
                std::cout << "EVENT_SLEEP: arg1=" << event.getArg1() << std::endl;
            } break;

            // VAD事件，如语音活动检测
            case AIUIConstant::EVENT_VAD: {
                // arg1为活动类型
                switch (event.getArg1()) {
                    case AIUIConstant::VAD_BOS_TIMEOUT: {
                        std::cout << "EVENT_VAD: VAD_BOS_TIMEOUT" << std::endl;
                    } break;

                    // 检测到前端点，即开始说话
                    case AIUIConstant::VAD_BOS: {
                        std::cout << "EVENT_VAD: BOS" << std::endl;
                    } break;

                    // 检测到后端点，即说话结束
                    case AIUIConstant::VAD_EOS: {
                        std::cout << "EVENT_VAD: EOS" << std::endl;
                    } break;

                    // 音量，arg2为音量级别（0-30）
                    case AIUIConstant::VAD_VOL: {
                        //std::cout << "EVENT_VAD: vol=" << event.getArg2() << std::endl;
                    } break;
                }
            } break;

            // 结果事件
            case AIUIConstant::EVENT_RESULT: {
                Json::Value bizParamJson;
                if (!parseAsJson(event.getInfo(), bizParamJson)) {
                    std::cout << "parse error! info=" << event.getInfo() << std::endl;
                    break;
                }

                Json::Value& data = (bizParamJson["data"])[0];
                Json::Value& params = data["params"];
                Json::Value& content = (data["content"])[0];

                std::string sub = params["sub"].asString();
                if (sub != "iat" && sub != "nlp" && sub != "tts" && sub != "cbm_tidy" && sub != "cbm_semantic") {
                    return;
                }

                // sid即唯一标识一次会话的id
                std::string sid = event.getData()->getString("sid", "");
                if (sub == "iat") {
                    if (sid != mCurIatSid) {
                        std::cout << "**********************************" << std::endl;
                        std::cout << "sid=" << sid << std::endl;

                        mCurIatSid = sid;

                        // 新的会话，清空之前识别缓存
                        mIatTextBuffer.clear();
                        mStreamNlpAnswerBuffer.clear();
                        m_pTtsHelper->clear();
                        mIntentCnt = 0;
                    }
                } else if (sub == "tts") {
                    if (sid != mCurTtsSid) {
                        std::cout << "**********************************" << std::endl;
                        std::cout << "sid=" << sid << std::endl;
                        mTtsLen = 0;
                        mCurTtsSid = sid;
                    }
                }

                Json::Value empty;
                std::string cnt_id = content.get("cnt_id", empty).asString();

                int dataLen = 0;

                // 注意：当buffer里存字符串时也不是以0结尾，当使用C语言时，转成字符串则需要自已在末尾加0
                const char* buffer = event.getData()->getBinary(cnt_id.c_str(), &dataLen);

                if (sub == "tts") {
                    // 语音合成结果，返回url或者pcm音频
                    std::cout << "tts: " << content.toString() << std::endl;

#ifdef USE_POST_SEMANTIC_TTS //使用语义后合成
                    //多意图的语义后合成的tts信息,可以知晓当前的tts音频流是哪个意图的。
                    if (mIntentCnt > 1) {
                        int ttsOtherDataLen = 0;
                        const char* ttsOtherBuffer = event.getData()->getBinary("1", &ttsOtherDataLen);
                        std::string ttsOtherResultStr = std::string(ttsOtherBuffer, ttsOtherDataLen);
                        Json::Value ttsOtherResultJson;

                        if (parseAsJson(ttsOtherResultStr, ttsOtherResultJson)) {
                            Json::Value metaTtsJson;
                            Json::Value textJson =
                                ttsOtherResultJson["cbm_meta"].get("text", metaTtsJson);

                            if (parseAsJson(textJson.asString(), metaTtsJson)) {
                                int intentIndex = metaTtsJson["tts"]["intent"].asInt();
                                std::cout << "tts intent index: " << intentIndex
                                     << " , des: " << ttsOtherResultJson.asString().c_str() << std::endl;
                            }
                        }
                    }
#endif

                    Json::Value&& isUrl = content.get("url", empty);
                    if (isUrl.asString() == "1") {
                        // 云端返回的是url链接，可以用播放器播放
                        std::cout << "tts_url=" << std::string(buffer, dataLen) << std::endl;
                    } else {
                        // 云端返回的是pcm音频，分成一块块流式返回
                        int progress = 0;
                        int dts = content["dts"].asInt();

                        std::string tag = event.getData()->getString("tag", "");
                        if (tag.find("stream_nlp_tts") == 0) {
                            // 流式语义应答的合成
                            m_pTtsHelper->onOriginTtsData(tag, bizParamJson, buffer, dataLen);
                        } else {
                            // 只有碰到开始块和(特殊情况:合成字符比较少时只有一包tts，dts = 2)，开启播放器
                            if (dts == AIUIConstant::DTS_BLOCK_FIRST
                                || dts == AIUIConstant::DTS_ONE_BLOCK ||
                                (dts == AIUIConstant::DTS_BLOCK_LAST && 0 == mTtsLen)) {

                                if (aiui_pcm_player_get_state() != PCM_PLAYER_STATE_STARTED) {
                                    CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_start());
                                }
                            }

                            mTtsLen += dataLen;
                            std::cout << "mTtsLen: " << mTtsLen << " currentLen:" << dataLen << std::endl;
                            // 收到sdk的tts数据dts固定传入1
							if (dataLen > 1) {
                                CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_write(0, buffer, dataLen, 1, 0));
							}
                            // 插入结束帧可以确保收到播放器stop的回调和isCompleted=true的onProgress回调
                            if (dts == AIUIConstant::DTS_ONE_BLOCK ||
                                dts == AIUIConstant::DTS_BLOCK_LAST) {
                                char data[2] = {0};
                                CHECK_FUNC_IF_SUCCESS(aiui_pcm_player_write(
                                    0, data, 2, AIUIConstant::DTS_BLOCK_LAST, 100));
                            }

                            // 若要保存合成音频，请打开以下开关
#if 0
                            // 音频开始
                            if (dts == AIUIConstant::DTS_BLOCK_FIRST ||
                                dts == AIUIConstant::DTS_ONE_BLOCK ||
                                /* 特殊情况:合成字符比较少时只有一包数据而且状态是２ */
                                (dts == AIUIConstant::DTS_BLOCK_LAST && 0 == mTtsLen)) {
                                mFs.open("tts.pcm", ios::binary | ios::out);
                            }

                            if (dataLen > 1) {
                                mFs.write(buffer, dataLen);
                            }

                            // 音频结束
                            if (dts == AIUIConstant::DTS_BLOCK_LAST || dts == AIUIConstant::DTS_ONE_BLOCK) {
                                mFs.close();
                            }
#endif
                        }
                    }
                } else if (sub == "iat") {
                    // 语音识别结果
                    std::string resultStr = std::string(buffer, dataLen);     // 注意：这里不能用std::string resultStr = buffer，因为buffer不一定以0结尾
                    Json::Value resultJson;
                    if (parseAsJson(resultStr, resultJson)) {
                        Json::Value textJson = resultJson["text"];

                        bool isWpgs = textJson.isMember("pgs");
                        if (isWpgs) {
                            mIatTextBuffer = IatResultUtil::parsePgsIatText(textJson);
                        } else {
                            // 结果拼接起来
                            mIatTextBuffer.append(IatResultUtil::parseIatResult(textJson));
                        }

                        // 是否是该次会话最后一个识别结果
                        bool isLast = textJson["ls"].asBool();
                        if (isLast) {
                            std::cout << "params: " << params.asString() << std::endl;
                            std::cout << "iat: " << mIatTextBuffer << std::endl;

                            mIatTextBuffer.clear();
                        }

                        // 需要在平台打开声纹开关，并在基础配置中选择“通用-中文-方言融合”识别引擎才有该字段
                        if (resultJson.isMember("extra")) {
                            Json::Value extraJson = resultJson["extra"];

                            std::cout << "extra: " << extraJson.toString() << std::endl;
                        }
                    }
                } else if (sub == "nlp") {
                    // 语义理解结果
                    // 注意：这里不能用std::string resultStr = buffer，因为buffer不一定以0结尾
                    std::string resultStr = std::string(buffer, dataLen);

                    // 从说完话到语义结果返回的时长
                    long eosRsltTime = event.getData()->getLong("eos_rslt", -1);

                    Json::Value resultJson;
                    if (parseAsJson(resultStr, resultJson)) {
                        // 判断是否为有效结果
                        if (resultJson.isMember("intent") &&
                            resultJson["intent"].isMember("rc")) {
                            // AIUI v1的语义结果
                            Json::Value intentJson = resultJson["intent"];
                            processIntentJson(params, intentJson, resultStr, eosRsltTime, sid);
                        } else if (resultJson.isMember("nlp")) {
                            // AIUI v2的语义结果
                            Json::Value nlpJson = resultJson["nlp"];
                            std::string text = nlpJson["text"].asString();

                            if (text.find("{\"intent\":") == 0) {
                                // 通用语义结果
                                Json::Value textJson;
                                if (parseAsJson(text, textJson)) {
                                    Json::Value intentJson = textJson["intent"];
                                    processIntentJson(params, intentJson, resultStr, eosRsltTime, sid);
                                }
                            } else {
                                // 大模型语义结果
                                // 流式nlp结果里面有seq和status字段
                                int seq = nlpJson["seq"].asInt();
                                int status = nlpJson["status"].asInt();

                                if (status == 0) {
                                    mStreamTtsIndex = 0;
                                }

                                /* 多意图取最后一次问题的结果进行tts合成 */
                                if (mIntentCnt > 1) {
                                    int currentIntentIndex;
                                    Json::Value metaNlpJson;
                                    Json::Value textJson = resultJson["cbm_meta"].get("text", metaNlpJson);
                                    if (parseAsJson(textJson.asString(), metaNlpJson)) {
                                        currentIntentIndex = metaNlpJson["nlp"]["intent"].asInt();
                                        if ((mIntentCnt - 1) != currentIntentIndex) {
                                            std::cout << "ignore nlp:" << resultStr << std::endl;
                                            return;
                                        }
                                    } else {
                                        std::cout << "ignore nlp:" << resultStr << std::endl;
                                        return;
                                    }
                                }

#ifndef USE_POST_SEMANTIC_TTS
                                // 如果使用应用的语义后合成不需要在调用下面的函数否则tts的播报会重复
                                m_pTtsHelper->addText(text, mStreamTtsIndex++, status);
#endif

                                std::cout << "----------------------------------" << std::endl;
                                std::cout << "params: " << params.asString() << std::endl;
                                std::cout << "nlp: " << resultStr << std::endl;

                                if (seq == 0) {
                                    long eosRsltTime = event.getData()->getLong("eos_rslt", -1);
                                    std::cout << "eos_result=" << eosRsltTime << "ms" << std::endl;
                                }

                                std::cout << "结果解析：" << std::endl;
                                std::cout << "sid=" << sid << std::endl;
                                std::cout << "seq=" << seq << ", status=" << status << ", answer（应答语）: " << text << std::endl;
                                std::cout << "fullAnswer=" << (mStreamNlpAnswerBuffer.append(text)) << std::endl;

                                if (status == 2) {
                                    mStreamNlpAnswerBuffer.clear();
                                }
                            }
                        } else {
                            // 无效结果，把原始结果打印出来
                            std::cout << "----------------------------------" << std::endl;
                            std::cout << "nlp: " << resultStr << std::endl;
                            std::cout << "sid=" << sid << std::endl;
                        }
                    }
                } else if (sub == "cbm_tidy") {
                    // 意图拆分的结果
                    std::string intentStr = std::string(buffer, dataLen); // 注意：这里不能用std::string resultStr = buffer，因为buffer不一定以0结尾
                    Json::Value tmpJson;
                    if (parseAsJson(intentStr, tmpJson)) {
                        Json::Value intentTextJson = tmpJson["cbm_tidy"]["text"];
                        if (!intentTextJson.empty() &&
                            parseAsJson(intentTextJson.asString(), tmpJson)) {
                            mIntentCnt = tmpJson["intent"].size();
                            std::cout << "cbm_intent_cnt: " << mIntentCnt
                                 << " text: " << tmpJson.toString() << std::endl;
                        }
                    }
                } else {
                    // 其他结果
                    std::string resultStr = std::string(buffer, dataLen);     // 注意：这里不能用std::string resultStr = buffer，因为buffer不一定以0结尾

                    std::cout << sub << ": " << event.getInfo() << std::endl << resultStr << std::endl;
                }
            } break;

            // 与CMD命令对应的返回结果，arg1为CMD类型，arg2为错误码
            case AIUIConstant::EVENT_CMD_RETURN: {
                if (AIUIConstant::CMD_BUILD_GRAMMAR == event.getArg1()) {
                    // 语法构建命令的结果
                    // 注：需要集成本地esr引擎才能构建语法
                    if (event.getArg2() == 0) {
                        std::cout << "build grammar success." << std::endl;
                    } else {
                        std::cout << "build grammar, error=" << event.getArg2() << ", des=" << event.getInfo() << std::endl;
                    }
                } else if (AIUIConstant::CMD_UPDATE_LOCAL_LEXICON == event.getArg1()) {
                    // 更新本地语法槽的结果
                    if (event.getArg2() == 0) {
                        std::cout << "update lexicon success" << std::endl;
                    } else {
                        std::cout << "update lexicon, error=" << event.getArg2() << "des=" << event.getInfo() << std::endl;
                    }
                } else if (AIUIConstant::CMD_CLONE_VOICE == event.getArg1()) {
                    //声音复刻
                    int dtype = event.getData()->getInt("sync_dtype", -1);
                    int retCode = event.getArg2();
                    std::string dataTypeStr;
                    if (dtype == AIUIConstant::VOICE_CLONE_REG) { //注册资源
                        dataTypeStr = "注册音频资源";
                    } else if (dtype == AIUIConstant::VOICE_CLONE_DEL) { //删除资源
                        dataTypeStr = "删除资源";
                    } else if (dtype == AIUIConstant::VOICE_CLONE_RES_QUERY) { //查询资源
                        dataTypeStr = "查询资源";
                    }

                    if (AIUIConstant::SUCCESS == retCode ) {
                        // 上传成功，会话的唯一id，用于反馈问题的日志索引字段，注意留存
                        // 注：上传成功立即生效
                        std::string sid = event.getData()->getString("sid", "");
                        // 获取上传调用时设置的自定义tag
                        std::string tag = event.getData()->getString("tag", "");
                        // 获取上传调用耗时，单位：ms
                        long timeSpent = event.getData()->getLong("time_spent", -1);
                        std::cout << "声音复刻" << dataTypeStr << "成功"
                             << "，耗时：" << timeSpent
                             << "ms, sid=" + sid + "，tag=" + tag;
                        if (dtype == AIUIConstant::VOICE_CLONE_REG) {
                            std::string resId = event.getData()->getString("res_id", "");
                            std::cout << "，res id = " << resId << std::endl;

                            //保存声音复刻的的res id
                            gVoiceCloneResId = resId;
                            saveToFile(resId, "./voice_clone_reg_id.txt");
                        } else if (dtype == AIUIConstant::VOICE_CLONE_RES_QUERY) {
                            std::string result = event.getData()->getString("result", "");
                            std::cout << ", result:" << std::endl;
                            std::cout << result << std::endl;
                        } else {
                            std::cout << std::endl;
                        }
                    } else {
                        std::string result = event.getData()->getString("result", "");
                        std::cout << "声音复刻" << dataTypeStr << "失败，错误码：" <<
                            retCode << " info:" << event.getInfo() << " result:" << result << std::endl;
                    }
                } else if (AIUIConstant::CMD_VOICE_PRINT == event.getArg1()) {
                    // 声纹操作
                    int dtype = event.getData()->getInt("sync_dtype", -1);
                    int retCode = event.getArg2();
                    long timeSpent = event.getData()->getLong("time_spent", -1);

                    // 获取sid，用于反馈问题的日志索引字段，注意留存
                    std::string sid = event.getData()->getString("sid", "");
                    // 获取上传调用时设置的自定义tag
                    std::string tag = event.getData()->getString("tag", "");
                    // 获取上传调用耗时，单位：ms
                    std::string result = event.getData()->getString("result", "");

                    std::string dataTypeStr;
                    if (dtype == AIUIConstant::VOICE_PRINT_MEMBER_ADD) {
                        dataTypeStr = "添加成员信息";
                    } else if (dtype == AIUIConstant::VOICE_PRINT_MEMBER_UPDATE) {
                        dataTypeStr = "更新成员信息";
                    } else if (dtype == AIUIConstant::VOICE_PRINT_MEMBER_QUERY) {
                        dataTypeStr = "查询成员信息";
                    } else if (dtype == AIUIConstant::VOICE_PRINT_MEMBER_DEL) {
                        dataTypeStr = "删除成员信息";
                    } else if (dtype == AIUIConstant::VOICE_PRINT_MEMBER_CLEAN) {
                        dataTypeStr = "清空成员信息";
                    } else if (dtype == AIUIConstant::VOICE_PRINT_FEATURE_REG) {
                        dataTypeStr = "注册声纹特征";

                        if (AIUIConstant::SUCCESS == retCode) {
                            Json::Value resultJson;
                            if (parseAsJson(result, resultJson)) {
                                Json::Value& featureJson = resultJson["data"];
                                gVpFeatureId = featureJson["feature_id"].asString();
                            }
                        }
                    } else if (dtype == AIUIConstant::VOICE_PRINT_FEATURE_UPDATE) {
                        dataTypeStr = "更新声纹特征";
                    } else if (dtype == AIUIConstant::VOICE_PRINT_FEATURE_QUERY) {
                        dataTypeStr = "查询声纹特征";

                        if (AIUIConstant::SUCCESS == retCode) {
                            Json::Value resultJson;
                            if (parseAsJson(result, resultJson)) {
                                Json::Value& dataArray = resultJson["data"];
                                if (!dataArray.empty()) {
                                    Json::Value& memberJson = dataArray[0];
                                    Json::Value& featureArray = memberJson["features"];

                                    if (!featureArray.empty()) {
                                        Json::Value& featureJson = featureArray[0];

                                        gVpFeatureId = featureJson["feature_id"].asString();
                                    } else {
                                        gVpFeatureId = "";
                                    }
                                }
                            }
                        }
                    } else if (dtype == AIUIConstant::VOICE_PRINT_FEATURE_DEL) {
                        dataTypeStr = "删除声纹特征";
                    } else if (dtype == AIUIConstant::VOICE_PRINT_FEATURE_SEARCH) {
                        dataTypeStr = "检索声纹特征";
                    }

                    if (AIUIConstant::SUCCESS == retCode) {
                        std::cout << "声纹操作，" << dataTypeStr << "成功，耗时：" << timeSpent
                             << "ms, sid=" + sid + ", tag=" + tag
                             << ", result:" << std::endl << result << std::endl;
                    } else {
                        std::cout << "声纹操作，" << dataTypeStr << "失败，错误码：" << retCode
                             << " info: " << event.getInfo() << ", result: " << result << std::endl;
                    }
                } else if (AIUIConstant::CMD_SYNC == event.getArg1()) {
                    // 数据同步的返回
                    int dtype = event.getData()->getInt("sync_dtype", -1);
                    int retCode = event.getArg2();

                #if AIUI_VER == 2 || AIUI_VER == 3
                    std::string dataTypeStr;
                    std::string text;

                    if (dtype == AIUIConstant::SYNC_DATA_UPLOAD) {
                        dataTypeStr = "上传实体";
                    } else if (dtype == AIUIConstant::SYNC_DATA_DELETE) {
                        dataTypeStr = "删除实体";
                    } else if (dtype == AIUIConstant::SYNC_DATA_DOWNLOAD) {
                        dataTypeStr = "下载实体";
                    } else if (dtype == AIUIConstant::SYNC_DATA_SEE_SAY) {
                        dataTypeStr = "所见即可说";
                    }

                    if (AIUIConstant::SUCCESS == retCode ) {
                        // 上传成功，会话的唯一id，用于反馈问题的日志索引字段，注意留存
                        // 注：上传成功立即生效
                        gSyncSid = event.getData()->getString("sid", "");
                        // 获取上传调用时设置的自定义tag
                        std::string tag = event.getData()->getString("tag", "");
                        // 获取上传调用耗时，单位：ms
                        long timeSpent = event.getData()->getLong("time_spent", -1);
                        std::cout << "同步" << dataTypeStr << "成功"
                             << "，耗时：" << timeSpent
                             << "ms, sid=" + gSyncSid + "，tag=" + tag;
                        if (dtype == AIUIConstant::SYNC_DATA_UPLOAD) {
                            std::cout << "，你可以试着说“打电话给刘德华“" << std::endl;
                        } else {
                            std::cout << std::endl;
                        }
                        // 实体内容
                        if (dtype == AIUIConstant::SYNC_DATA_DOWNLOAD) {
                            text = event.getData()->getString("text", "");
                            std::cout << "下载的实体内容:\n" << Base64Util::decode(text) << std::endl;
                        }
                    } else {
                        gSyncSid = "";
                        std::string result = event.getData()->getString("result", "");
                        std::cout << "同步" << dataTypeStr << "失败，错误码：" <<
                            retCode << " info:" << event.getInfo() << " result:" << result << std::endl;
                    }
                #else
                    if (dtype == AIUIConstant::SYNC_DATA_SCHEMA) {
                        if (AIUIConstant::SUCCESS == retCode) {
                            // 上传成功，记录上传会话的sid，以用于查询数据打包状态
                            // 注：上传成功并不表示数据打包成功，打包成功与否应以同步状态查询结果为准，数据只有打包成功后才能正常使用
                            gSyncSid = event.getData()->getString("sid", "");

                            // 获取上传调用时设置的自定义tag
                            std::string tag = event.getData()->getString("tag", "");

                            // 获取上传调用耗时，单位：ms
                            long timeSpent = event.getData()->getLong("time_spent", -1);

                            std::cout << "同步成功，"
                                 << "耗时：" << timeSpent
                                 << "ms, sid=" + gSyncSid + "，tag=" + tag +
                                        "，你可以试着说“打电话给刘德华“"
                                 << std::endl;
                        } else {
                            gSyncSid = "";
                            std::cout << "同步失败，错误码：" << retCode << std::endl;
                        }
                    }
                #endif
                }
            #if AIUI_VER == 1
                else if (AIUIConstant::CMD_QUERY_SYNC_STATUS == event.getArg1()) {
                    // 数据同步状态查询的返回
                    // 获取同步类型
                    int syncType = event.getData()->getInt("sync_dtype", -1);
                    if (AIUIConstant::SYNC_DATA_QUERY == syncType) {
                        // 若是同步数据查询，则获取查询结果，结果中error字段为0则表示上传数据打包成功，否则为错误码
                        std::string result = event.getData()->getString("result", "");

                        std::cout << "查询结果：" << result << std::endl;
                    }
                }
            #endif
            } break;

            // 开始录音事件
            case AIUIConstant::EVENT_START_RECORD: {
                std::cout << "EVENT_START_RECORD " << std::endl;
            } break;

            // 停止录音事件
            case AIUIConstant::EVENT_STOP_RECORD: {
                std::cout << "EVENT_STOP_RECORD " << std::endl;
            } break;

            // 出错事件
            case AIUIConstant::EVENT_ERROR: {
                // 打印错误码和描述信息
                std::cout << "EVENT_ERROR: error=" << event.getArg1() << ", des=" << event.getInfo() << std::endl;
            } break;

            // 连接到服务器
            case AIUIConstant::EVENT_CONNECTED_TO_SERVER: {
                // 获取uid（为客户端在云端的唯一标识）并打印
                std::string uid = event.getData()->getString("uid", "");

                std::cout << "EVENT_CONNECTED_TO_SERVER, uid=" << uid << std::endl;
            } break;

            // 与服务器断开连接
            case AIUIConstant::EVENT_SERVER_DISCONNECTED: {
                std::cout << "EVENT_SERVER_DISCONNECTED " << std::endl;
            } break;

            //唤醒词操作的结果
            case AIUIConstant::EVENT_CAE_WAKEUP_WORD_RESULT: {
                int type = event.getArg1();
                int ret = event.getArg2();

                //保存唤醒词资源
                if (0 == ret && (type == AIUIConstant::CAE_GEN_WAKEUP_WORD ||
                                 type == AIUIConstant::CAE_ADD_WAKEUP_WORD)) {
                    int dataLen = 0;
                    const char* buffer = event.getData()->getBinary("data", &dataLen);

                    std::string resData(buffer, dataLen);
                    saveToFile(resData, "./wakeup_word_res.bin");

                    if (type == AIUIConstant::CAE_ADD_WAKEUP_WORD) {
                        const char* buffer = event.getData()->getBinary("id", &dataLen);
                        if (buffer) {
                            gCustomizeWakeUpWordId = atoi(buffer);
                            std::cout << "add customize wake up word s id = " << gCustomizeWakeUpWordId
                                 << std::endl;
                        }
                    }
                }

                if (ret != 0) {
                    int dataLen = 0;
                    const char* buffer = event.getData()->getBinary("error_msg", &dataLen);
                    // 注意：这里不能用std::string errorMsg = buffer，因为buffer不一定以0结尾
                    std::string errorMsg = std::string(buffer, dataLen);
                    std::cout << "wakeup word operate fail, type = " << type << ", error code = " << ret
                         << ", error msg: " << errorMsg << std::endl;
                } else {
                    std::cout << "wakeup word operate success, type = " << type << std::endl;
                }

            } break;

            //唤醒音频事件
            case AIUIConstant::EVENT_CAE_WAKEUP_AUDIO: {
                int type = event.getArg1();
                //保存唤醒音频
                if (type == AIUIConstant::CAE_WAKEUP_AUDIO) {
                    int dataLen = 0;
                    const char* buffer = event.getData()->getBinary("audio", &dataLen);

                    std::string audioData(buffer, dataLen);
                    saveToFile(audioData, "wakeup_audio.pcm");
                }
            } break;
        }
    }
};

DemoListener* g_pListener = nullptr; // NOLINT

IAIUIAgent* g_pAgent = nullptr;

#ifdef AIUI_ANDROID
    #define TEST_ROOT_DIR "/sdcard/AIUI/"
    #
    #ifdef TURING_UNIT_SUPPORT
        #define CFG_FILE_PATH "/sdcard/AIUI/cfg/turing.cfg"
    #else
        #define CFG_FILE_PATH "/sdcard/AIUI/cfg/aiui.cfg"
    #endif
    #
    #define TEST_AUDIO_PATH "/sdcard/AIUI/audio/test.pcm"
    #define LOG_DIR         "/sdcard/AIUI/log/"
    #define MSC_DIR         "/sdcard/AIUI/msc/"
    #define TEST_TTS_PATH   "/sdcard/AIUI/text/tts.txt"
    #define TEST_SEE_SAY_PATH "/sdcard/AIUI/text/see_say.txt"
    #define VOICE_CLONE_AUDIO_PATH "/sdcard/AIUI/audio/voice_clone_1ch24K16bit.pcm"
    #define VOICE_CLONE_RES_ID_PATH "/sdcard/AIUI/voice_clone_reg_id.txt"
    #define WAKE_WORD_TEXT_PATH "/sdcard/AIUI/wakeup_wake.txt"
    #define VOICE_PRINT_AUDIO_PATH "/sdcard/AIUI/audio/voice_print_1ch16k16bit.pcm"
#else
    #define TEST_ROOT_DIR "./AIUI/"
    #
    #ifdef TURING_UNIT_SUPPORT
        #define CFG_FILE_PATH "./AIUI/cfg/turing.cfg"
    #else
        #define CFG_FILE_PATH "./AIUI/cfg/aiui.cfg"
    #endif
    #
    #define TEST_AUDIO_PATH "./AIUI/audio/test.pcm"
    #define LOG_DIR         "./AIUI/log/"
    #define MSC_DIR         "./AIUI/msc/"
    #define TEST_TTS_PATH   "./AIUI/text/tts.txt"
    #define TEST_SEE_SAY_PATH "./AIUI/text/see_say.txt"
    #define VOICE_CLONE_AUDIO_PATH "./AIUI/audio/voice_clone_1ch24K16bit.pcm"
    #define VOICE_CLONE_RES_ID_PATH "./voice_clone_reg_id.txt"
    #define WAKE_WORD_TEXT_PATH "./AIUI/text/wakeup_wake.txt"
    #define VOICE_PRINT_AUDIO_PATH "./AIUI/audio/voice_print_1ch16k16bit.pcm"
#endif

// 通讯录同步示例内容
std::string SYNC_CONTACT_CONTENT =       // NOLINT
    R"({"name":"刘得华", "phoneNumber":"13512345671"})" "\n"
    R"({"name":"张学诚", "phoneNumber":"13512345672"})" "\n"
    R"({"name":"张右兵", "phoneNumber":"13512345673"})" "\n"
    R"({"name":"吴羞波", "phoneNumber":"13512345674"})" "\n"
    R"({"name":"黎晓", "phoneNumber":"13512345675"})";

/**
 * 读取文件内容存到字符串。
 *
 * @param path
 * @return
 */
std::string readFileAsString(const std::string& path)
{
    std::fstream t(path, std::ios::in | std::ios::binary);
    if (!t.is_open()) {
        std::cout << "Error open file: " << path << " fail." << std::endl;
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

/**
 * 创建AIUIAgent对象。
 *
 * @param more
 * @param cfgPath
 */
void createAgent(bool more = true, const char* cfgPath = CFG_FILE_PATH)
{
    if (g_pAgent) {
        return;
    }

    std::string aiuiParams = readFileAsString(cfgPath);

    Json::Value paramJson;
    Json::Reader reader;
    if (reader.parse(aiuiParams, paramJson, false)) {
        if (more) {
            std::cout << paramJson.toString() << std::endl;
        }

        g_pAgent = IAIUIAgent::createAgent(paramJson.toString().c_str(), g_pListener);
    }

    if (!g_pAgent) {
        std::cout << std::string(cfgPath) << ", " << reader.getFormatedErrorMessages() << std::endl;
        return;
    }
}

/**
 * 销毁AIUIAgent对象。
 */
void destroyAgent()
{
    if (g_pAgent) {
        g_pAgent->destroy();
        g_pAgent = nullptr;
    }
}

/**
 * 唤醒AIUI。
 */
void wakeup()
{
    // 可以通过clear_data来控制是否要清除唤醒之前的数据（默认会清除），清除则唤醒之前的会话结果（tts除外）会被丢弃从而不再继续抛出
    // arg1为波束编号
    SEND_AIUIMESSAGE4(AIUIConstant::CMD_WAKEUP, 0, 0, "clear_data=true");
}

/**
 * 重置唤醒，即回到待唤醒状态。
 */
void resetWakeup()
{
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_RESET_WAKEUP);
}

/**
 * 开启AIUI服务，此接口是与stop()对应，调用stop()之后必须调用此接口才能继续与SDK交互。
 *
 * 注：AIUIAgent创建成功之后AIUI会自动开启，故若非调用过stop()则不需要调用start()。
 */
void start()
{
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_START);
}

/**
 * 停止AIUI服务。
 */
void stop()
{
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_STOP);
}

/**
 * 重置AIUI服务，相当于先调用stop()再调用start()。一般用不到。
 */
void resetAIUI()
{
    SEND_AIUIMESSAGE(AIUIConstant::CMD_RESET, 0, 0, "", nullptr);
}

/**
 * 从文件读音频写入SDK，即用文件数据模型实时录音数据。
 *
 * @param repeat
 */
void writeAudioFromLocal(bool repeat)
{
    if (!g_pAgent) {
        return;
    }

    std::string audioData = readFileAsString(TEST_AUDIO_PATH);
    if (!audioData.empty()) {
        int offset = 0;
        int left = audioData.size();
        const int frameLen = 1280;
        char buff[frameLen];

        while (true) {
            if (left < frameLen) {
                if (repeat) {
                    offset = 0;
                    left = audioData.size();
                    continue;
                } else {
                    break;
                }
            }

            memset(buff, '\0', frameLen);
            memcpy(buff, audioData.data() + offset, frameLen);

            offset += frameLen;
            left -= frameLen;

            // frameData内存会在Message在内部处理完后自动release掉
            AIUIBuffer frameData = aiui_create_buffer_from_data(buff, frameLen);
            SEND_AIUIMESSAGE(AIUIConstant::CMD_WRITE, 0, 0, "data_type=audio,tag=audio-tag", frameData);

            // 必须暂停一会儿模拟人停顿，太快的话后端报错。1280字节16k采样16bit编码的pcm数据对应40ms时长
            AIUI_SLEEP(40);
        }

        // 音频写完后，要发CMD_STOP_WRITE停止写入消息
        SEND_AIUIMESSAGE4(AIUIConstant::CMD_STOP_WRITE, 0, 0, "data_type=audio");
    } else {
        std::cout << "open file failed, path=" << TEST_AUDIO_PATH << std::endl;
    }

    std::cout << "write finish" << std::endl;
}

/**
 * 开启录音。
 */
void startRecordAudio()
{
    SEND_AIUIMESSAGE4(
        AIUIConstant::CMD_START_RECORD, 0, 0, "data_type=audio,pers_param={\"uid\":\"\"},tag=record-tag");
}

/**
 * 停止录音。
 */
void stopRecordAudio()
{
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_STOP_RECORD);
}

/**
 * 写入文本进行交互。
 *
 * @param text 文本内容
 * @param needWakeup 是否需要唤醒
 */
void writeText(const std::string& text, bool needWakeup = true)
{
    AIUIBuffer textData = aiui_create_buffer_from_data(text.c_str(), text.length());

    if (needWakeup) {
        SEND_AIUIMESSAGE(AIUIConstant::CMD_WRITE, 0, 0, "data_type=text,pers_param={\"uid\":\"\"}", textData);
    } else {
        SEND_AIUIMESSAGE(AIUIConstant::CMD_WRITE, 0, 0, "data_type=text,need_wakeup=false", textData);
    }
}

/**
 * 测试语音合成，返回pcm数据。
 *
 * @param text
 */
void startTTS(const std::string& text, const std::string& tag)
{
    AIUIBuffer textData = aiui_create_buffer_from_data(text.c_str(), text.length());
    std::string params = "voice_name=x5_lingxiaoyue_flow";
    if (!tag.empty()) {
        params.append(",tag=").append(tag);
    }

    // 使用发音人x4_lingxiaoying_em_v2合成，也可以使用其他发音人
    SEND_AIUIMESSAGE(AIUIConstant::CMD_TTS, AIUIConstant::START, 0, params.c_str(), textData);
}

/**
 * 测试流式语音合成，即分段上传文本，返回pcm数据。
 *
 * @param text 待合成文本
 * @param dts 数据流状态，取值：0（第一块数据），1（中间数据），2（最后一块数据）3(只有一块数据)
 */
void startTTS(const std::string& text, int dts, const std::string& tag)
{
    AIUIBuffer textData = aiui_create_buffer_from_data(text.c_str(), text.length());
    char buffer[10] = {0};
    snprintf(buffer, sizeof(buffer), "%d", dts);

    std::string params = "voice_name=x5_lingxiaoyue_flow,data_status=" + std::string(buffer);
    if (!tag.empty()) {
        params.append(",tag=").append(tag);
    }

    // 使用发音人x4_lingxiaoying_em_v2合成，也可以使用其他发音人
    SEND_AIUIMESSAGE(AIUIConstant::CMD_TTS, AIUIConstant::START, 0, params.c_str(), textData);
}

/**
 * 测试超拟人语音合成，返回pcm数据。
 *
 * @param text
 */
void startHTS(const std::string& text, const std::string& tag)
{
    AIUIBuffer textData = aiui_create_buffer_from_data(text.c_str(), text.length());

    // 超拟人合成需要先给发音人开通授权，再设置scene=IFLYTEK.hts
    // 口语化等级oral_level: high, mid（默认值）, low
    // 情感强度emotion_scale: [-20, 20]，默认值: 0
    // 情感类型emotion: 0（中立，默认）, 1（调皮）, 2（安慰）, 3（可爱）, 4（鼓励）, 5（高兴）, 6（抱歉）, 7（撒娇）, 8（宠溺）, 9（严肃）
    //      10（困惑）, 11（害怕）, 12（悲伤）, 13（生气）
    std::string params = "voice_name=x4_lingxiaoxuan_oral,scene=IFLYTEK.hts,oral_level=mid,emotion_scale=0,emotion=0";
    if (!tag.empty()) {
        params.append(",tag=").append(tag);
    }

    SEND_AIUIMESSAGE(AIUIConstant::CMD_TTS, AIUIConstant::START, 0, params.c_str(), textData);
}

/**
 * 测试语音合成，返回url。
 *
 * @param text
 */
void startTTSUrl(const std::string& text)
{
    AIUIBuffer textData = aiui_create_buffer_from_data(text.c_str(), text.length());

    // 使用发音人chongchong合成，也可以使用其他发音人
    SEND_AIUIMESSAGE(AIUIConstant::CMD_TTS, AIUIConstant::START, 0,
                     "text_encoding=utf-8,tts_res_type=url,vcn=x2_xiaojuan", textData);
}

/**
 * 构建asr语法。
 *
 * 注：当前版本已废弃，只有历史版本支持。
 */
void buildAsrGrammar()
{
    std::string grammar = readFileAsString("AIUI/asr/call.bnf");

    SEND_AIUIMESSAGE4(AIUIConstant::CMD_BUILD_GRAMMAR, 0, 0, grammar.c_str());
}

/**
 * 构建esr语法。
 *
 * 注：新版本SDK都只支持esr。
 */
void buildEsrGrammar()
{
    std::string grammar = readFileAsString("AIUI/esr/message.fsa");

    SEND_AIUIMESSAGE4(AIUIConstant::CMD_BUILD_GRAMMAR, 0, 0, grammar.c_str());
}

/**
 * 将麦克风参数切换到单麦设置。
 */
void changeMicTypeToMic1()
{
    constexpr const char* mic1_params = R"(
{
	"ivw": {
		"mic_type": "mic1"
	},
	"recorder": {
		"channel_filter": "0,-1"
	}
}
)";
    SEND_AIUIMESSAGE(AIUIConstant::CMD_SET_PARAMS, 0, 0, mic1_params, nullptr);
}

/**
 * 清除语义对话历史。
 */
void cleanDialogHistory()
{
    std::cout << "cleanDialogHistory" << std::endl;

    SEND_AIUIMESSAGE1(AIUIConstant::CMD_CLEAN_DIALOG_HISTORY);
}

/**
 * 同步动态实体。
 */
void syncSchemaData(int type = AIUIConstant::SYNC_DATA_SCHEMA)
{
    std::string dataStrBase64 = Base64Util::encode(SYNC_CONTACT_CONTENT);

    Json::Value syncSchemaJson;
    Json::Value dataParamJson;

    // 设置id_name为uid，即用户级个性化资源
    // 个性化资源使用方法可参见http://doc.xfyun.cn/aiui_mobile/的用户个性化章节
    dataParamJson["id_name"] = "uid";

    // 设置res_name为联系人
    dataParamJson["res_name"] = "IFLYTEK.telephone_contact";

#ifdef AIUI_VER == 2 || AIUI_VER == 3
    // aiui开放平台的命名空间，在「技能工作室-我的实体-动态实体密钥」中查看
    dataParamJson["name_space"] = "OS13360977719";
#endif

    syncSchemaJson["param"] = dataParamJson;
    if (AIUIConstant::SYNC_DATA_SCHEMA == type || AIUIConstant::SYNC_DATA_UPLOAD == type) {
        syncSchemaJson["data"] = dataStrBase64;
    }

    std::string jsonStr = syncSchemaJson.toString();

    // 传入的数据一定要为utf-8编码
    AIUIBuffer syncData = aiui_create_buffer_from_data(jsonStr.c_str(), jsonStr.length());

    // 给该次同步加上自定义tag，在返回结果中可通过tag将结果和调用对应起来
    Json::Value paramJson;
    paramJson["tag"] = "sync-tag";

    // 用schema数据同步上传联系人
    // 注：数据同步请在连接服务器之后进行，否则可能失败
    SEND_AIUIMESSAGE(AIUIConstant::CMD_SYNC, type, 0,
                     paramJson.toString().c_str(), syncData);
}

#if AIUI_VER == 2 || AIUI_VER == 3
/**
 * 同步所见即可说的内容。
 */
void syncV2SeeSayData()
{
    std::string seeSayContent = readFileAsString(TEST_SEE_SAY_PATH);
    Json::Value contentJson;
    if (!parseAsJson(seeSayContent, contentJson)) {
        std::cout << "syncV2SeeSayData parse error! info=" << seeSayContent << std::endl;
        return;
    }
    /*注意：传入的数据一定要为utf-8编码
     * 　　可见即可说的内容为json数组
     [{
     　 //资源名称
　　　　"res_name": "IFLYTEK.telephone_contact",
       //内容要进行base64编码
　　　　"data": "base64"　
　　　}]
     */

    std::cout << "see say content: " << contentJson.asString() << std::endl;
    //整个json在进行base64编码
    std::string dataStrBase64 = Base64Util::encode(contentJson.asString());
    Json::Value syncSeeSayJson;
    syncSeeSayJson["data"] = dataStrBase64;
    std::string jsonStr = syncSeeSayJson.toString();
    AIUIBuffer syncData = aiui_create_buffer_from_data(jsonStr.c_str(), jsonStr.length());

    // 给该次同步加上自定义tag，在返回结果中可通过tag将结果和调用对应起来
    Json::Value paramJson;
    paramJson["tag"] = "sync_see_say_tag";

    // 注：数据同步请在连接服务器之后进行，否则可能失败
    SEND_AIUIMESSAGE(AIUIConstant::CMD_SYNC, AIUIConstant::SYNC_DATA_SEE_SAY, 0,
                     paramJson.toString().c_str(), syncData);
}

/**
 * 上传需要复刻的音频资源
 * @param resPath 音频资源的路径
 * 音频格式:　
 *    采样率: 24000　
 *    通道数: 1　
 *    位深: 16　
 *    编码格式: 裸音频pcm
 * @parm engineVersion 取值：v4,omni_v1
         不设置默认取值：v4 （对应初始版本）
 */
void voiceCloneReg(const std::string& resPath = VOICE_CLONE_AUDIO_PATH,
                   const std::string& engineVersion = "v4")
{
    Json::Value paramJson;
    paramJson["tag"] = "voice_clone_tag_0";
    paramJson["res_path"] = resPath;
    paramJson["engine_version"] = engineVersion;

    std::cout << "[func:" << __FUNCTION__ << " line:" << __LINE__ << "] "
              << "上传声音复刻的资源,资源路径: " << resPath << std::endl;

    // 注：数据同步请在连接服务器之后进行，否则可能失败
    SEND_AIUIMESSAGE(AIUIConstant::CMD_CLONE_VOICE,
                     AIUIConstant::VOICE_CLONE_REG,
                     0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * 删除声音复刻的资源
 */
void voiceCloneDelRes()
{
    std::string resId = gVoiceCloneResId;
    if (resId.empty()) {
        resId = readFileAsString(VOICE_CLONE_RES_ID_PATH);
    }

    if (resId.empty()) {
        std::cout << "[fail func:"  << __FUNCTION__  << " line:" <<__LINE__  << "] "
             << "删除声音复刻的资源失败，资源ID为NULL" << std::endl;
        return;
    }

    std::cout << "[func:"  << __FUNCTION__  << " line:" <<__LINE__  << "] "
         << "删除声音复刻的资源,res id = " << resId <<  std::endl;

    Json::Value paramJson;
    paramJson["tag"] = "voice_clone_tag_１";
    paramJson["res_id"] = resId;

    // 注：数据同步请在连接服务器之后进行，否则可能失败
    SEND_AIUIMESSAGE(AIUIConstant::CMD_CLONE_VOICE, AIUIConstant::VOICE_CLONE_DEL, 0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * 测试声音复刻的tts
 *
 * @param text
 */
void startVoiceCloneTTS(const std::string& text, const std::string& tag, const std::string& vcn)
{
    std::string resId = gVoiceCloneResId;
    if (resId.empty()) {
        resId = readFileAsString(VOICE_CLONE_RES_ID_PATH);
    }

    if (resId.empty()) {
        std::cout << "[fail func:"  << __FUNCTION__  << " line:" <<__LINE__  << "] "
             << "请求声音复刻的tts失败，资源ID为NULL" << std::endl;
        return;
    }

    AIUIBuffer textData = aiui_create_buffer_from_data(text.c_str(), text.length());
    // v4版本的发言人必须是x5_clone, omni_v1版本的发言人必须是x6_clone
    std::string params;
    params.append("voice_name=").append(vcn);
    params.append(",res_id=").append(resId);
    if (!tag.empty()) {
        params.append(",tag=").append(tag);
    }

    SEND_AIUIMESSAGE(AIUIConstant::CMD_TTS, AIUIConstant::START, 0, params.c_str(), textData);
}

/**
 * 查询声音复刻注册的资源信息
 */
void voiceCloneQueryRes()
{

    Json::Value paramJson;
    paramJson["tag"] = "voice_clone_tag_2";

    // 注：数据同步请在连接服务器之后进行，否则可能失败
    SEND_AIUIMESSAGE(AIUIConstant::CMD_CLONE_VOICE, AIUIConstant::VOICE_CLONE_RES_QUERY, 0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * 添加声纹成员信息。注意：一个设备下默认最多有10个成员（数量限制后台可修改）。
 */
void addV3VoicePrintMember(const std::string& memberId)
{
    Json::Value paramJson;
    // 成员标识，非必需，不超过32位，不传云端默认生成32位uuid（纯小写）返回
    paramJson["member_id"] = memberId;
    // 名称，必需，不超过16个字符，不支持特殊符号
    paramJson["name"] = "小飞飞";
    // 性别年龄，非必需，取值：child（儿童），male（青年男），female（青年女），oldmale（老年男），oldfemale（老年女）
    paramJson["age_sex"] = "child";
    // 描述，非必需，不超过32个字符，不支持特殊符号
    paramJson["desc"] = "描述信息";

    SEND_AIUIMESSAGE(AIUIConstant::CMD_VOICE_PRINT,
                     AIUIConstant::VOICE_PRINT_MEMBER_ADD,
                     0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * 更新声纹成员信息。
 */
void updateV3VoicePrintMember(const std::string& memberId)
{
    Json::Value paramJson;
    // 成员标识，必需，由服务端返回
    paramJson["member_id"] = memberId;
    // 名称，必需，不超过16个字符，不支持特殊符号
    paramJson["name"] = "小飞飞2";
    // 性别年龄，非必需，取值：child（儿童），male（青年男），female（青年女），oldmale（老年男），oldfemale（老年女）
    paramJson["age_sex"] = "male";
    // 描述，非必需，不超过32个字符，不支持特殊符号
    paramJson["desc"] = "描述信息2";

    SEND_AIUIMESSAGE(AIUIConstant::CMD_VOICE_PRINT,
                     AIUIConstant::VOICE_PRINT_MEMBER_UPDATE,
                     0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * 删除声纹成员信息，成员注册的声纹特征也将被删除。
 */
void delV3VoicePrintMember(const std::string& memberId)
{
    Json::Value paramJson;
    // 成员标识，必需
    paramJson["member_id"] = memberId;

    SEND_AIUIMESSAGE(AIUIConstant::CMD_VOICE_PRINT,
                     AIUIConstant::VOICE_PRINT_MEMBER_DEL,
                     0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * 清空声纹成员信息，成员注册的声纹特征也将被删除。
 */
void cleanV3VoicePrintMember()
{
    SEND_AIUIMESSAGE(AIUIConstant::CMD_VOICE_PRINT,
                     AIUIConstant::VOICE_PRINT_MEMBER_CLEAN,
                     0,
                     "",
                     nullptr);
}

/**
 * 查询声纹成员信息。
 */
void queryV3VoicePrintMember()
{
    SEND_AIUIMESSAGE(AIUIConstant::CMD_VOICE_PRINT,
                     AIUIConstant::VOICE_PRINT_MEMBER_QUERY,
                     0,
                     "",
                     nullptr);
}

/**
 * 注册声纹特征。
 */
void regV3VoicePrintFeature(const std::string& memberId, const std::string& resPath = VOICE_PRINT_AUDIO_PATH)
{
    Json::Value paramJson;
    // 成员标识，非必需，不超过32位，不传云端默认生成32位uuid（纯小写）返回
    paramJson["member_id"] = memberId;
    // 音频文件路径，必需，16k采样16bit编码的pcm音频，时长大于1s小于2min
    paramJson["res_path"] = resPath;
    // 描述，非必需，不超过32个字符，不支持特殊符号
    paramJson["feature_info"] = "描述信息";
    paramJson["tag"] = "voice_print_tag_0";

    std::cout << "[func:" << __FUNCTION__ << " line:" << __LINE__ << "] "
         << "注册声纹，音频路径: " << resPath << std::endl;

    SEND_AIUIMESSAGE(AIUIConstant::CMD_VOICE_PRINT,
                     AIUIConstant::VOICE_PRINT_FEATURE_REG,
                     0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * 更新声纹特征。
 */
void updateV3VoicePrintFeature(const std::string& featureId, const std::string& resPath = VOICE_PRINT_AUDIO_PATH)
{
    Json::Value paramJson;
    // 声纹标识，必需，注册成功后由云端返回
    paramJson["feature_id"] = featureId;
    // 音频文件路径，必需，16k采样16bit编码的pcm音频，时长大于1s小于2min
    paramJson["res_path"] = resPath;
    // 是否覆盖原有，必需，取值：true（覆盖原有特征），false（与原有特征合并）
    paramJson["cover"] = "true";

    std::cout << "[func:" << __FUNCTION__ << " line:" << __LINE__ << "] "
         << "更新声纹，音频路径: " << resPath << std::endl;

    SEND_AIUIMESSAGE(AIUIConstant::CMD_VOICE_PRINT,
                     AIUIConstant::VOICE_PRINT_FEATURE_UPDATE,
                     0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * 查询声纹特征。
 */
void queryV3VoicePrintFeature(const std::string& memberId = "")
{
    Json::Value paramJson;
    // 成员标识，非必需，不传则会返回所有成员注册的声纹特征
    if (!memberId.empty()) {
        paramJson["member_id"] = memberId;
    }

    SEND_AIUIMESSAGE(AIUIConstant::CMD_VOICE_PRINT,
                     AIUIConstant::VOICE_PRINT_FEATURE_QUERY,
                     0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * 检索声纹特征。
 */
void searchV3VoicePrintFeature(const std::string& resPath = VOICE_PRINT_AUDIO_PATH)
{
    Json::Value paramJson;
    // 音频文件路径，必需，16k采样16bit编码的pcm音频，时长大于1s小于2min
    paramJson["res_path"] = resPath;

    std::cout << "[func:" << __FUNCTION__ << " line:" << __LINE__ << "] "
         << "检索声纹，音频路径: " << resPath << std::endl;

    SEND_AIUIMESSAGE(AIUIConstant::CMD_VOICE_PRINT,
                     AIUIConstant::VOICE_PRINT_FEATURE_SEARCH,
                     0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * 删除声纹特征。
 */
void delV3VoicePrintFeature(const std::string& featureId)
{
    Json::Value paramJson;
    // 声纹标识，必需，注册成功后由云端返回
    paramJson["feature_id"] = featureId;

    SEND_AIUIMESSAGE(AIUIConstant::CMD_VOICE_PRINT,
                     AIUIConstant::VOICE_PRINT_FEATURE_DEL,
                     0,
                     paramJson.toString().c_str(),
                     nullptr);
}

/**
 * AIUI登录操作，只有AIUI_V2才支持。
 */
void aiuiLogin()
{
    SEND_AIUIMESSAGE1(AIUIConstant::CMD_AIUI_LOGIN);
}

#endif

/**
 * 查询动态实体同步状态。
 */
void querySyncSchemaStatus()
{
    // 构造查询json字符串，填入同步schema数据返回的sid
    Json::Value queryJson;
    queryJson["sid"] = gSyncSid;

    // 发送同步数据状态查询消息，设置arg1为schema数据类型，params为查询字符串
    SEND_AIUIMESSAGE4(AIUIConstant::CMD_QUERY_SYNC_STATUS,
                      AIUIConstant::SYNC_DATA_SCHEMA,
                      0,
                      queryJson.toString().c_str());
}

/**
 * 同步可见即可说数据。
 */
void syncSpeakableData() {}

/**
 * 唤醒词的操作
 * @param op  0-生成唤醒词资源, 1-添加唤醒词
 */
void operateWakeupWord(int op)
{
    std::string text;
    int arg2 = 0;
    if (op != AIUIConstant::CAE_DEL_WAKEUP_WORD) {
        //获取唤醒词名称
        text = readFileAsString(WAKE_WORD_TEXT_PATH);
        std::cout << "wakeup word process text: " << text << ", operate type:" << op << std::endl;
    } else {
        std::cout << "wakeup word process operate type:" << op << " id = " << gCustomizeWakeUpWordId
             << std::endl;
        arg2 = gCustomizeWakeUpWordId;
    }

    SEND_AIUIMESSAGE(AIUIConstant::CMD_CAE_OPERATE_WAKEUP_WORD, op, arg2, text.c_str(), nullptr);
}

void help()
{
    static std::string s =
        "demo示例为输入命令，调用对应的函数，使用AIUI "
        "SDK完成文本理解，语义理解，文本合成等功能，如：\r\n"
        "c命令，创建AIUI代理，与AIUI SDK交互都是通过代理发送消息的方式进行, "
        "所以第一步必须是输入该命令；\r\n"
        "s命令，初始化内部组件 第二步；\r\n"
        "w命令，发送外部唤醒命令唤醒AIUI，AIUI只有在唤醒过后才可以交互；第三步\r\n"
        "rw命令，发送外部休眠命令 AIUI进入休眠状态\r\n"
        "wrt命令，字符串文本写入sdk，sdk会返回云端识别的语义结果；\r\n"
        "wrtn命令，不需要先唤醒，字符串文本写入sdk，sdk会返回云端识别的语义结果；\r\n"
        "wra命令，音频文件写入sdk，sdk会返回云端识别的语义结果；\r\n"
        "wrr命令，跟wra一样写音频，但文件写完后不会停止，而是从头开始重新写；\r\n"
        "sr命令，启用内部录音，注意 aiui.cfg配置中audio_captor字段；\r\n"
        "str命令，停止内部录音；\r\n"
        "tts命令，单合成示例，返回合成的pcm音频；\r\n"
        "hts命令，超拟人单合成示例，返回合成的pcm音频；\r\n"
        "tts2命令，单合成示例，返回合成的音频的url；\r\n"
        "tts3命令，流式单合成示例，返回合成的pcm音频；\r\n"
        "be命令，构建语法；\r\n"
        "cl命令，消除语义历史；\r\n"
        "ssch命令，同步动态实体；\r\n"

#if AIUI_VER == 2 || AIUI_VER == 3
        "dsch命令，下载态实体；\r\n"
        "csch命令，删除态实体；\r\n"
        "sssay, 同步所见即可说内容；\r\n"
        "vcr, 使用v4版本上传声音复刻的资源；\r\n"
        "vcr2, 使用omni_v1版本上传声音复刻的资源；\r\n"
        "vcd, 删除声音复刻的资源；\r\n"
        "vcq, 查询声音复刻注册的资源；\r\n"
        "vc_tts, 使用v4版本复刻声音请求tts；\r\n"
        "vc_tts2, 使用omni_v1版本复刻声音请求tts；\r\n"
        "l, AIUI登录操作；\r\n"
#else
        "qsch命令，查询动态实体同步状态\r\n；"
#endif

#if AIUI_VER == 3
        "vpma, 添加声纹成员信息；\r\n"
        "vpmu, 更新声纹成员信息；\r\n"
        "vpmd, 删除声纹成员信息；\r\n"
        "vpmq, 查询声纹成员信息；\r\n"
        "vpmc, 清空声纹成员信息；\r\n"
        "vpfr, 注册声纹特征；\r\n"
        "vpfu, 更新声纹特征；\r\n"
        "vpfd, 删除声纹特征；\r\n"
        "vpfq, 查询声纹特征；\r\n"
        "vpfs, 检索声纹特征；\r\n"
#endif

        "sspk命令，同步所见即可说数据；\r\n"
        "kr命令，添加唤醒词；\r\n"
        "kg命令，生成唤醒词资源；\r\n"
        "kd命令，清除之前添加的唤醒词；\r\n"
        "help命令，显示本demo提供的示例的介绍；\r\n"
        "输入c命令后，正常情况返回结果为：\r\n"
        "EVENT_STATE: READY\r\n"
        "输入w命令后，正常情况返回结果为: \r\n"
        "EVENT_WAKEUP\r\n"
        "EVENT_STATE: WORKING\r\n"
        "听写，语义，合成等结果在onEvent函数，该函数是结果回调，请仔细研究。\r\n"
        "流程 c -> w -> wra\r\n"
        "input cmd:";

    std::cout << s << std::endl;
}

#if defined(__linux) || defined(__ANDROID__)
    #include <sys/socket.h>
    #include <net/if.h>
    #include <sys/ioctl.h>

/**
 * 获取mac地址。
 *
 * @param mac
 */
static void GenerateMACAddress(char* mac)
{
    // reference: https://stackoverflow.com/questions/1779715/how-to-get-mac-address-of-your-machine-using-a-c-program/35242525
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return;
    }

    struct ifconf ifc{};
    char buf[1024];
    int success = 0;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        return;
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
    struct ifreq ifr{};

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (!(ifr.ifr_flags & IFF_LOOPBACK)) {    // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = 1;
                    break;
                }
            }
        } else {
            return;
        }
    }

    unsigned char mac_address[6];
    if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);

    sprintf(mac,
            "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_address[0],
            mac_address[1],
            mac_address[2],
            mac_address[3],
            mac_address[4],
            mac_address[5]);
    close(sock);
}
#elif defined(WIN32)
    #include <stdio.h>
    #include <IPHlpApi.h>
    #pragma comment(lib, "IPHLPAPI.lib")

static void GenerateMACAddress(char* mac)
{
    ULONG ulSize = 0;
    PIP_ADAPTER_INFO adapterInfo = NULL;
    PIP_ADAPTER_INFO adapterInfoTmp = NULL;

    GetAdaptersInfo(adapterInfo, &ulSize);

    if (0 == ulSize) {
        return;
    }

    adapterInfo = (PIP_ADAPTER_INFO)malloc(ulSize);

    if (adapterInfo == NULL) {
        return;
    }

    memset(adapterInfo, 0, ulSize);

    adapterInfoTmp = adapterInfo;

    GetAdaptersInfo(adapterInfo, &ulSize);

    while (adapterInfo) {
        _snprintf(mac,
                  64,
                  "%02x:%02x:%02x:%02x:%02x:%02x",
                  adapterInfo->Address[0],
                  adapterInfo->Address[1],
                  adapterInfo->Address[2],
                  adapterInfo->Address[3],
                  adapterInfo->Address[4],
                  adapterInfo->Address[5]);

        if (std::strcmp(adapterInfo->GatewayList.IpAddress.String, "0.0.0.0") != 0) break;

        adapterInfo = adapterInfo->Next;
    }

    free(adapterInfoTmp);

    adapterInfoTmp = NULL;
}
#endif

/**
 * 初始化设置。
 *
 * @param log
 */
static void initSetting(bool log = true)
{
    AIUISetting::setAIUIDir(TEST_ROOT_DIR);
    AIUISetting::setMscDir(MSC_DIR);
    AIUISetting::setNetLogLevel(log ? aiui_debug : aiui_none);

    char mac[64] = {0};
    GenerateMACAddress(mac);

    // 为每一个设备设置唯一对应的序列号SN（最好使用设备硬件信息(mac地址，设备序列号等）生成），以便正确统计装机量，
    // 避免刷机或者应用卸载重装导致装机量重复计数
    AIUISetting::setSystemInfo(AIUI_KEY_SERIAL_NUM, mac);

    // 6.6.xxxx.xxxx版本设置用户唯一标识uid（可选，AIUI后台服务需要，不设置则会使用上面的SN作为uid）
    // 5.6.xxxx.xxxx版本SDK不能也不需要设置uid
    // AIUISetting::setSystemInfo(AIUI_KEY_UID, "1234567890");
}

int main(int argc, char* argv[])
{
#ifdef WIN32
    system("chcp 65001 >nul");
    freopen("nul", "w", stderr);
#else
//    freopen("/dev/null", "w", stderr);
#endif

    if (argc > 1) {
        gPcmPlayerIndex = atoi(argv[1]);
    }

    //g_pListener = new DemoListener;

    // 打印SDK版本
    std::cout << "Version: " << getVersion() << std::endl;

    initSetting();
    help();

    std::string cmd;
    while (true) {
        std::cin >> cmd;

        if (cmd == "c") {
            std::cout << "createAgent" << std::endl;
            if (nullptr == g_pListener)
                g_pListener = new DemoListener;
           createAgent();
        } else if (cmd == "w") {
            std::cout << "wakeup" << std::endl;
            wakeup();
        } else if (cmd == "rw") {
            std::cout << "resetWakeup" << std::endl;
            resetWakeup();
        } else if (cmd == "s") {
            std::cout << "start" << std::endl;
            start();
        } else if (cmd == "st") {
            std::cout << "stop" << std::endl;
            stop();
        } else if (cmd == "d") {
            std::cout << "destroyAgent" << std::endl;
            destroyAgent();
            if (nullptr != g_pListener) {
                delete g_pListener;
                g_pListener = nullptr;
            }
        } else if (cmd == "e") {
            std::cout << "exit" << std::endl;
            break;
        } else if (cmd == "sr") {
            std::cout << "startRecordAudio" << std::endl;
            startRecordAudio();
        } else if (cmd == "str") {
            std::cout << "stopRecordAudio" << std::endl;
            stopRecordAudio();
        } else if (cmd == "wrt") {
            std::cout << "writeText" << std::endl;
            writeText("你叫什么名字？");
        } else if (cmd == "wrtn") {
            std::cout << "writeText, wakeup not needed" << std::endl;
            //writeText("今天天气怎么样？", false);
            writeText("我想唱歌，我想看刘德华的电影", false);
        } else if (cmd == "wra") {
            std::cout << "writeAudio" << std::endl;
            writeAudioFromLocal(false);
        } else if (cmd == "wrr") {
            std::cout << "writeAudio repeatedly" << std::endl;
            writeAudioFromLocal(true);
        } else if (cmd == "tts2") {
            startTTSUrl("我叫小飞飞，是你的好朋友");
        } else if (cmd == "tts") {
            startTTS("这几天心里颇不宁静。今晚在院子里坐着乘凉，忽然想起日日走过的荷塘，在这满月的夜里，总该另有一 番样子吧。月亮渐渐地升高了，墙外马路上孩子们的欢笑，已经听不见了;妻在屋里拍着闰儿，迷迷糊糊地哼着眠歌。我悄悄地披了大衫，带上门出去。", "tts-tag");
        } else if (cmd == "hts") {
            startHTS("这几天心里颇不宁静。今晚在院子里坐着乘凉，忽然想起日日走过的荷塘，在这满月的夜里，总该另有一 番样子吧。月亮渐渐地升高了，墙外马路上孩子们的欢笑，已经听不见了;妻在屋里拍着闰儿，迷迷糊糊地哼着眠歌。我悄悄地披了大衫，带上门出去。", "hts-tag");
        } else if (cmd == "tts3") {
            startTTS("这几天心里颇不宁静。今晚在院子里坐着乘凉，忽然想起日日走过的荷塘，在这满月的夜里，总该另有一 番样子吧。", 0, "tts3-tag");
            startTTS("月亮渐渐地升高了，墙外马路上孩子们的欢笑，已经听不见了;妻在屋里拍着闰儿，迷迷糊糊地哼着眠歌。我悄悄地披了大衫，带上门出去。", 2, "tts3-tag");
        } else if (cmd == "be") {
            buildEsrGrammar();
        } else if (cmd == "bg") {
            buildAsrGrammar();
        } else if (cmd == "q") {
            break;
        } else if (cmd == "mic1") {
            changeMicTypeToMic1();
        } else if (cmd == "rs") {
            resetAIUI();
        } else if (cmd == "cl") {
            cleanDialogHistory();
        }
#if AIUI_VER == 2 || AIUI_VER == 3
        else if (cmd == "ssch") {
            std::cout << "upload SchemaData" << std::endl;
            syncSchemaData(AIUIConstant::SYNC_DATA_UPLOAD);
        } else if (cmd == "dsch") {
            std::cout << "download SchemaData" << std::endl;
            syncSchemaData(AIUIConstant::SYNC_DATA_DOWNLOAD);
        } else if (cmd == "csch") {
            std::cout << "delete SchemaData" << std::endl;
            syncSchemaData(AIUIConstant::SYNC_DATA_DELETE);
        } else if (cmd == "sssay") {
            std::cout << "sync v2 see say" << std::endl;
            syncV2SeeSayData();
        } else if (cmd == "vcr") {
            voiceCloneReg();
        } else if (cmd == "vcr2") {
            voiceCloneReg(VOICE_CLONE_AUDIO_PATH, "omni_v1");
        } else if (cmd == "vcd") {
            voiceCloneDelRes();
        } else if (cmd == "vcq") {
            voiceCloneQueryRes();
        } else if (cmd == "vc_tts") {
            startVoiceCloneTTS("这几天心里颇不宁静。今晚在院子里坐着乘凉，忽然想起日日走过的荷塘，在这满月的夜里，总该另有一 番样子吧。月亮渐渐地升高了，墙外马路上孩子们的欢笑，已经听不见了;妻在屋里拍着闰儿，迷迷糊糊地哼着眠歌。我悄悄地披了大衫，带上门出去。", "tts_vc-tag", "x5_clone");
        } else if (cmd == "vc_tts2") {
            startVoiceCloneTTS("这几天心里颇不宁静。今晚在院子里坐着乘凉，忽然想起日日走过的荷塘，在这满月的夜里，总该另有一 番样子吧。月亮渐渐地升高了，墙外马路上孩子们的欢笑，已经听不见了;妻在屋里拍着闰儿，迷迷糊糊地哼着眠歌。我悄悄地披了大衫，带上门出去。", "tts_vc-tag", "x6_clone");
        } else if (cmd == "l") {
            aiuiLogin();
        }
#else
        else if (cmd == "ssch") {
            std::cout << "syncSchemaData" << std::endl;
            syncSchemaData();
        } else if (cmd == "qsch") {
            std::cout << "querySyncSchemaStatus" << std::endl;
            querySyncSchemaStatus();
        }
#endif

#if AIUI_VER == 3
        else if (cmd == "vpma") {
            std::cout << "add voicePrintMember, member_id=" << gVpMemberId << std::endl;
            addV3VoicePrintMember(gVpMemberId);
        } else if (cmd == "vpmu") {
            std::cout << "update voicePrintMember, member_id=" << gVpMemberId << std::endl;
            updateV3VoicePrintMember(gVpMemberId);
        } else if (cmd == "vpmd") {
            std::cout << "delete voicePrintMember, member_id=" << gVpMemberId << std::endl;
            delV3VoicePrintMember(gVpMemberId);
        } else if (cmd == "vpmq") {
            std::cout << "query voicePrintMember" << std::endl;
            queryV3VoicePrintMember();
        } else if (cmd == "vpmc") {
            std::cout << "clean voicePrintMember" << std::endl;
            cleanV3VoicePrintMember();
        } else if (cmd == "vpfr") {
            std::cout << "register voicePrintFeature, member_id=" << gVpMemberId << std::endl;
            regV3VoicePrintFeature(gVpMemberId);
        } else if (cmd == "vpfu") {
            if (gVpFeatureId.empty()) {
                std::cout << "feature_id为空，请先运行vpfq查询或者vpfr注册" << std::endl;
                continue;
            }

            std::cout << "update voicePrintFeature, feature_id=" << gVpFeatureId << std::endl;
            updateV3VoicePrintFeature(gVpFeatureId);
        } else if (cmd == "vpfd") {
            if (gVpFeatureId.empty()) {
                std::cout << "feature_id为空，请先运行vpfq查询或者vpfr注册" << std::endl;
                continue;
            }

            std::cout << "delete voicePrintFeature, feature_id=" << gVpFeatureId << std::endl;
            delV3VoicePrintFeature(gVpFeatureId);
        } else if (cmd == "vpfq") {
            std::cout << "query voicePrintFeature, member_id=" << gVpMemberId << std::endl;
            queryV3VoicePrintFeature(gVpMemberId);
        } else if (cmd == "vpfs") {
            std::cout << "search voicePrintFeature" << std::endl;
            searchV3VoicePrintFeature();
        }
#endif

        else if (cmd == "sspk") {
            std::cout << "syncSpeakableData" << std::endl;
            syncSpeakableData();
        }  else if (cmd == "kr") {
            operateWakeupWord(AIUIConstant::CAE_ADD_WAKEUP_WORD);
        } else if (cmd == "kg") {
            operateWakeupWord(AIUIConstant::CAE_GEN_WAKEUP_WORD);
        } else if (cmd == "kd") {
            operateWakeupWord(AIUIConstant::CAE_DEL_WAKEUP_WORD);
        } else if (cmd == "h") {
            help();
        } else {
            std::cout << "invalid cmd, input again." << std::endl;
        }
    }
}
