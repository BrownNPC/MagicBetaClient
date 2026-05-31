package main

import (
	"io/fs"
	"os"
	"path/filepath"
	"slices"
	"strings"
	"text/template"
	"unicode"
	"unicode/utf8"
)

var AllowedExt = []string{".png", ".ogg"}

type Asset struct {
	Name, Path string
}

func main() {
	Assets := []Asset{}
	err := filepath.WalkDir("assets/", func(path string, d fs.DirEntry, err error) error {
		path = strings.TrimPrefix(path, "assets")
		ext := filepath.Ext(path)
		if !slices.Contains(AllowedExt, ext) || d.IsDir() {
			return nil
		}
		trimmedPath := strings.ReplaceAll(path, " ", "_")
		assetID := ToAssetID(strings.TrimSuffix(trimmedPath, ext))
		Assets = append(Assets, Asset{Name: assetID, Path: path})
		return nil
	})
	if err != nil {
		panic(err)
	}
	type Template struct {
		Total  int
		Assets []Asset
	}
	var templateData = Template{
		Total:  len(Assets),
		Assets: Assets,
	}
	t, err := template.ParseFiles("cmd/assetIDgen/asset_id.go.tmpl")
	if err != nil {
		panic(err)
	}
	f, err := os.Create("src/gfx/assets/asset_id.go")
	if err != nil {
		panic(err)
	}
	defer f.Close()
	err = t.Execute(f, templateData)
	if err != nil {
		panic(err)
	}
}

var b strings.Builder

func ToAssetID(path string) string {
	b.Reset()
	first := true
	for s := range strings.SplitSeq(path, "/") {
		if s == "" {
			continue
		}
		if first {
			b.WriteString(uppercaseFirst(s))
			first = false
			b.WriteByte('_')
			continue
		}
		b.WriteString(s)
		b.WriteByte('_')
	}

	s := b.String()
	return s[:len(s)-1] // remove last underscore
}

func uppercaseFirst(s string) string {
	if s == "" {
		return ""
	}

	// Decode the first UTF-8 character (rune) safely
	r, size := utf8.DecodeRuneInString(s)

	// Convert the single rune to uppercase and combine with the rest of the string
	return string(unicode.ToUpper(r)) + s[size:]
}
