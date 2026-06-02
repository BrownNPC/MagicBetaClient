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
		return Command("psp-cmake", "-B", BuildDir, "-G", "Ninja")
	case "native":
		return Command("cmake", "-B", BuildDir, "-G", "Ninja")
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
