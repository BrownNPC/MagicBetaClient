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

var Target = flag.String("target", "none", " -target=<psp,psp-vendored,native,native-vendored>")

func RunCmakeForTarget(target string) bool {
	switch target {
	case "psp":
		BuildDir = "./_build-psp"
		return Command("psp-cmake",
			"-DUSE_VENDORED_SDL3=OFF", "-DUSE_VENDORED_MIXER=ON",
			"-B", BuildDir, "-G", "Ninja")
	case "psp-vendored":
		BuildDir = "./_build-psp-vendored"
		return Command("psp-cmake",
			"-DUSE_VENDORED_SDL3=ON", "-DUSE_VENDORED_MIXER=ON",
			"-B", BuildDir, "-G", "Ninja")
	case "native":
		BuildDir = "./_build-native"
		return Command("cmake",
			"-DUSE_VENDORED_SDL3=OFF", "-DUSE_VENDORED_MIXER=OFF",
			"-B", BuildDir, "-G", "Ninja")
	case "native-vendored":
		BuildDir = "./_build-native-vendored"
		return Command("cmake",
			"-DUSE_VENDORED_SDL3=ON", "-DUSE_VENDORED_MIXER=ON",
			"-B", BuildDir, "-G", "Ninja")
	}
	return false
}

func main() {
	flag.Parse()
	Command("rm", "-fr", TranspileDir)
	Command("mkdir", "-p", TranspileDir)
	if !Command("so", "translate", "-o", TranspileDir, "src") {
		return
	}
	RunCmakeForTarget(*Target)
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
