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



变量值示例：

engine：ali_tts

voice：aixia

text：您好，有什么我可以帮助您的吗？



如果不指定engine和voice，可以通过通道变量 tts_engine 和 tts_voice 来指定引擎和发言人。

<action application="set" data="tts_engine=ali_tts"/>

<action application="set" data="tts_voice=aixia"/>



如果需要设置语速、语调、音量可以在text前设置

 比如：{volume=80}您好，有什么我可以帮助您的吗？


