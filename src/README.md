# Minecraft beta 1.7.3 client in C

> Meant to run on old consoles like Playstation 2, and Playstation Portable (PSP)


# Screenshots

<img width="1600" height="900" alt="image" src="https://github.com/user-attachments/assets/8e6d5074-b01a-4c16-8b39-03ac63e8538a" />




# Building

You must have Go installed. Why? Because the codebase is written in [Solod](https://solod.dev) which is
a variant of Go that compiles to readable C code.




```
go install solod.dev/cmd/so@main
go run build.go --bootstrap=native-vendored


You will need `libcurl` findable by CMake. Just look up "how to install libcurl devel {distro name}"


The output binary will be at `_build/MagicBetaClient`
```

# Dependencies

- SDL3 (vendored)
- SDL3_Mixer (vendored)
- libcurl
- gl4es (optional, vendored)
