package main

import (
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
)

var TranspileDir = File("./_build/transpiled/")
var BuildDir = File("./_build/")

var Bootstrap = flag.String("bootstrap", "none", " -bootstrap=<psp,native>")

func RunCmakeForTarget(target string) bool {
	switch target {
	case "psp":
		return Command("psp-cmake",
			"-DUSE_VENDORED_SDL3=OFF", "-DUSE_VENDORED_MIXER=ON", "-DUSE_GL4ES=OFF",
			"-B", BuildDir, "-G", "Ninja")
	case "native":
		return Command("cmake",
			"-DUSE_VENDORED_SDL3=OFF", "-DUSE_VENDORED_MIXER=OFF", "-DUSE_GL4ES=OFF",
			"-B", BuildDir, "-G", "Ninja")
	case "android":
		ndk := os.Getenv("ANDROID_NDK")
		if ndk == "" {
			panic("ndk is not found in env var $ANDROID_NDK")
		}
		toolchainFilePath := File(ndk, "/build/cmake/android.toolchain.cmake")
		return Command("cmake",
			"-DCMAKE_TOOLCHAIN_FILE="+toolchainFilePath,
			"-DCMAKE_FIND_ROOT_PATH="+"buildscripts",
			"-DANDROID_ABI="+"armeabi-v7a",
			"-DANDROID_PLATFORM="+"android-28",
			"-DANDROID_STL="+"c++_shared ",
			"-DUSE_VENDORED_SDL3=ON", "-DUSE_VENDORED_MIXER=ON",
			"-B", BuildDir, "-G", "Ninja")
	}
	return false
}

func main() {
	flag.Parse()
	if *Bootstrap != "none" {
		Command("rm", "-fr", BuildDir)
	} else {
		Command("rm", "-fr", TranspileDir)
		Command("mkdir", "-p", TranspileDir)
	}
	if !Command("so", "translate", "-o", TranspileDir, "src") {
		return
	}
	RunCmakeForTarget(*Bootstrap)
	Command("cmake", "--build", BuildDir, "--parallel")
}

var File = filepath.Join

func Command(e string, args ...string) bool {
	cmd := exec.Command(e, args...)
	fmt.Println("RUNNING:", cmd.String())
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run() == nil
}
