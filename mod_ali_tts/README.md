# mod_tts
FreeSwitch扩展模块，基于阿里云的语音合成功能



# 安装

```sh
# vim ali_tts.xml 修改阿里云ASR配置

make

cp mod_ali_tts.so  /usr/local/freeswitch/lib/freeswitch/mod/

cp ali_tts.xml /usr/local/freeswitch/etc/freeswitch/autoload_configs

cp NlsSdkCpp3.X/lib/libalibabacloud-idst-speech.* /usr/lib/

# fs_cli 客户端中 load mod_ali_tts

mkdir /tmp/ali_tts/
chmod 775 /tmp/ali_tts/
```



# 使用

application 使用方式：

speak  <engine>|<voice>|<text>

示例：

speak ali_tts|aixia|您好，有什么我可以帮助您的吗？



也支持通过通道变量指定：

<action application="set" data="tts_engine=ali_tts"/>

<action application="set" data="tts_voice=aixia"/>

<action application="speak" data="您好，有什么我可以帮助您的吗？"/>



如果需要设置音量、语速、语调可以在text前设置

 比如：{volume=80,speech_rate=0,pitch_rate=0}您好，有什么我可以帮助您的吗？


