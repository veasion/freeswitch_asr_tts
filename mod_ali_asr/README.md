FreeSwitch ASR 模块 [NlsSdkCpp3.x]

[阿里云ASR](https://help.aliyun.com/product/30413.html?spm=a2c4g.11186623.2.10.6b634c07NBBDiY) 和 FreeSwitch 直接对接，识别结果通过ESL输出  



# 安装

```sh
# vim ali_asr.xml 修改阿里云ASR配置

make

cp mod_ali_asr.so  /usr/local/freeswitch/lib/freeswitch/mod/

cp ali_asr.xml /usr/local/freeswitch/etc/freeswitch/autoload_configs

cp NlsSdkCpp3.X/lib/libalibabacloud-idst-speech.* /usr/lib/

# fs_cli 客户端中 load mod_ali_tts
```



# 使用

fs_cli 测试:
```
originate user/1000 'start_asr,echo' inline
```



dialplan执行

```
<extension name="asr">
    <condition field="destination_number" expression="^.*$">
        <action application="answer"/>
        <action application="start_asr" data=""/>
        <action application="echo"/>
    </condition>
</extension>
```



# ESL 订阅

eventName:  CUSTOM

Event-Subclass:  start_asr

Channel-Name: sofia/internal/1001@192.168.0.100

Caller-Unique-ID: 7s23b863-e6fc-46e1-401b-6b1931c47163

Caller-Caller-ID-Number: 1001

Caller-Destination-Number: 1000
ASR-Response: {"header":{"namespace":"SpeechTranscriber","name":"SentenceEnd","status":20000000,"message_id":"0ca84cbeed884ca39c88c0c5ae4edbb4","task_id":"97f77f8f53f14eef8be5469375051d81","status_text":"Gateway:SUCCESS:Success."},"payload":{"index":3,"time":4950,"result":"你好。","confidence":0.38665345311164856,"words":[],"status":20000000,"gender":"","begin_time":3600,"stash_result":{"sentenceId":0,"beginTime":0,"text":"","currentTime":0},"audio_extra_info":"","sentence_id":"92887f8e4444437598baeb3768e87035","gender_score":0.0}}



Event-Subclass: update_asr

ASR-Response: {"header":{"namespace":"SpeechTranscriber","name":"TranscriptionResultChanged","status":20000000,"message_id":"06bf1659fc904abcb95c727e7fb143a2","task_id":"3d7563f486a74aa28b3d50256eae0958","status_text":"Gateway:SUCCESS:Success."},"payload":{"index":1,"time":6340,"result":"你是谁？","confidence":0.45189642906188965,"words":[],"status":20000000}}



Event-Subclass: stop_asr

