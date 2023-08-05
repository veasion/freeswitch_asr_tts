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



ESL 开发

订阅`CUSTOM asr_start` `CUSTOM asr_update` `CUSTOM asr_stop` 事件
fs_cli可以通过`/event Custom asr_start  asr_update asr_stop`订阅事件
识别结果通过esl输出
```
RECV EVENT
Event-Subclass: start_asr
Event-Name: CUSTOM
Core-UUID: dbc6fb6a-16e6-44cb-8be8-a49397cc3c5f
FreeSWITCH-Hostname: telegant
FreeSWITCH-Switchname: telegant
FreeSWITCH-IPv4: 10.10.16.180
FreeSWITCH-IPv6: ::1
Event-Date-Local: 2021-01-25 16:33:20
Event-Date-GMT: Mon, 25 Jan 2021 08:33:20 GMT
Event-Date-Timestamp: 1611563600014063
Event-Calling-File: mod_asr.cpp
Event-Calling-Function: onAsrSentenceEnd
Event-Calling-Line-Number: 215
Event-Sequence: 2485
UUID: 8a53b863-e6fc-46e1-902b-7b1931c47164
ASR-Response: {"header":{"namespace":"SpeechTranscriber","name":"SentenceEnd","status":20000000,"message_id":"0ca84cbeed884ca39c88c0c5ae4edbb4","task_id":"97f77f8f53f14eef8be5469375051d81","status_text":"Gateway:SUCCESS:Success."},"payload":{"index":3,"time":4950,"result":"你好。","confidence":0.38665345311164856,"words":[],"status":20000000,"gender":"","begin_time":3600,"stash_result":{"sentenceId":0,"beginTime":0,"text":"","currentTime":0},"audio_extra_info":"","sentence_id":"92887f8e4444437598baeb3768e87035","gender_score":0.0}}
Channel: sofia/internal/8000@cc.com


RECV EVENT
Event-Subclass: stop_asr
Event-Name: CUSTOM
Core-UUID: dbc6fb6a-16e6-44cb-8be8-a49397cc3c5f
FreeSWITCH-Hostname: telegant
FreeSWITCH-Switchname: telegant
FreeSWITCH-IPv4: 10.10.16.180
FreeSWITCH-IPv6: ::1
Event-Date-Local: 2021-01-25 16:34:13
Event-Date-GMT: Mon, 25 Jan 2021 08:34:13 GMT
Event-Date-Timestamp: 1611563653232813
Event-Calling-File: mod_asr.cpp
Event-Calling-Function: onAsrChannelClosed
Event-Calling-Line-Number: 331
Event-Sequence: 2495

RECV EVENT
Event-Subclass: update_asr
Event-Name: CUSTOM
Core-UUID: dbc6fb6a-16e6-44cb-8be8-a49397cc3c5f
FreeSWITCH-Hostname: telegant
FreeSWITCH-Switchname: telegant
FreeSWITCH-IPv4: 10.10.16.180
FreeSWITCH-IPv6: ::1
Event-Date-Local: 2021-01-25 16:34:50
Event-Date-GMT: Mon, 25 Jan 2021 08:34:50 GMT
Event-Date-Timestamp: 1611563690152634
Event-Calling-File: mod_asr.cpp
Event-Calling-Function: onAsrTranscriptionResultChanged
Event-Calling-Line-Number: 249
Event-Sequence: 2512
UUID: 8a53b863-e6fc-46e1-902b-7b1931c47164
ASR-Response: {"header":{"namespace":"SpeechTranscriber","name":"TranscriptionResultChanged","status":20000000,"message_id":"06bf1659fc904abcb95c727e7fb143a2","task_id":"3d7563f486a74aa28b3d50256eae0958","status_text":"Gateway:SUCCESS:Success."},"payload":{"index":1,"time":6340,"result":"结果是100还是150 ？","confidence":0.45189642906188965,"words":[],"status":20000000}}
Channel: sofia/internal/8000@cc.com

```

ASR-Response: asr识别返回结果 Channel: 当前Channel Name 

