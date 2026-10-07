# Minecraft beta 1.7.3 client in C

> Meant to run on old consoles like Playstation 2, and Playstation Portable (PSP)


# Screenshots
<p align="center">
  <img width="32%" src="https://github.com/user-attachments/assets/8e6d5074-b01a-4c16-8b39-03ac63e8538a" />
  <img width="32%" src="https://github.com/user-attachments/assets/addfb319-247d-43be-8973-92ffd20f3ff2" />
  <img width="32%" src="https://github.com/user-attachments/assets/079f2c95-6dfe-48c6-870c-d3cf8a04132c" />
</p>
# Running
For instructions, see the [release](https://github.com/BrownNPC/MagicBetaClient/releases/tag/0.0.1)
This application is Linux only for now.

# Roadmap
see [Todo.md](./TODO.md) to get an idea of the roadmap.


# Building

You must have Go installed. Why? Because the codebase is written in [Solod](https://solod.dev) which is
a variant of Go that compiles to readable C code.

```
git submodule update --init --recursive vendored/SDL
```

```
go install solod.dev/cmd/so@latest
go run build.go --bootstrap=native-vendored
```
You will need `libcurl` findable by CMake. Just look up "how to install libcurl devel {distro name}"


The output binary will be at `_build-native-vendored/MagicBetaClient`

# Dependencies

- SDL3 (vendored)
- SDL3_Mixer (vendored)
- libcurl

