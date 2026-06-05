#!/bin/bash
wget https://repo1.maven.org/maven2/io/github/vvb2060/ndk/curl/8.18.0/curl-8.18.0.aar
wget https://repo1.maven.org/maven2/io/github/vvb2060/ndk/boringssl/20251124/boringssl-20251124.aar

unzip -o curl-8.18.0.aar -d vendored/curl-aar 
unzip -o boringssl-20251124.aar -d vendored/boringssl-aar

rm *.aar
