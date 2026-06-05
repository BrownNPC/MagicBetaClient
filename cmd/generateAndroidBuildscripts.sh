#!/bin/bash

set -e

java -jar cmd/cli-2.1.0-all.jar \
  --platform android \
  --abi "armeabi-v7a" \
  --os-version 21 \
  --stl c++_shared \
  --ndk-version 23 \
  --build-system cmake \
  --output "buildscripts" \
  vendored/curl-aar/prefab/ \
  vendored/boringssl-aar/prefab/
