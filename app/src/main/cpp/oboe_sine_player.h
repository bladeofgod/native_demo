//
// Created by 45010 on 2021/6/6.
//

#ifndef NATIVEDEMO_OBOE_SINE_PLAYER_H
#define NATIVEDEMO_OBOE_SINE_PLAYER_H


#include <math.h>
#include <algorithm>
#include <oboe/Oboe.h>


/*
 * This class is responsible for creating an audio stream and starting it.
 * It specifies a callback function onAudioReady which is called each time
 * the audio stream needs more data.
 * Inside this callback either silence is rendered or if the isOn variable
 * is true a sine wave will be rendered.
 * The sine wave's frequency is hardcoded to 440Hz inside kFrequency.
 */

class OboeSinePlayer : public oboe::AudioStreamCallback {

private:
    //可以释放资源
    oboe::ManagedStream outStream;

    std::atomic_bool isOn {false};

    int channelCount;
    double mPhaseIncrement;

    /*
        对于修饰Object来说，
        const并未区分出编译期常量和运行期常量
        constexpr限定在了编译期常量
     */
    static float constexpr kAmplitude = 0.5f;
    static float constexpr kFrequency = 440;

    float mPhase = 0.0;

    static double constexpr kTwoPi = M_PI * 2;

public:
    OboeSinePlayer() {
        oboe::AudioStreamBuilder builder;
        // The builder set methods can be chained for convenience.
        builder.setSharingMode(oboe::SharingMode::Exclusive);
        //低延迟
        builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
        builder.setFormat(oboe::AudioFormat::Float);
        builder.setCallback(this);
        builder.openManagedStream(outStream);

        channelCount = outStream->getChannelCount();
        mPhaseIncrement = kFrequency * kTwoPi / outStream->getSampleRate();
        outStream->requestStart();

    }

    // This class will also be used for the callback
    // For more complicated callbacks create a separate class
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream,
                                          void *audioData,int32_t numFrames) override {
        float *floatData = static_cast<float*>(audioData);
        if(isOn) {
            //generate sine wave values
            for(int i=0; i< numFrames; ++i) {
                float smapleValue = kAmplitude * sinf(mPhase);
                for(int j=0;j< channelCount;j++ ) {
                    floatData[i * channelCount +j] = smapleValue;
                }
                mPhase += mPhaseIncrement;
                if(mPhase >= kTwoPi) mPhase -= kTwoPi;
            }
        } else {
            //输出静音
            /*
             * fill函数的作用是：将一个区间的元素都赋予val值。函数参数：fill(vec.begin(), vec.end(), val); val为将要替换的值；
             * fill_n函数的作用是：参数包括 : 一个迭代器，一个计数器以及一个值。该函数从迭代器指向的元素开始，将指定数量的元素设置为给定的值。
             */
            std::fill_n(floatData,numFrames * channelCount ,0);
        }
        return oboe::DataCallbackResult::Continue;
    }

    void enable(bool toEnable) {
        isOn.store(toEnable);
    }



};





#endif //NATIVEDEMO_OBOE_SINE_PLAYER_H
