package main

import (
	"embed"
	"io/fs"
)

//go:embed play.html
var playPageHTML string

//go:embed wasmdist/game.html wasmdist/game.js wasmdist/game.wasm wasmdist/game.data
var wasmDist embed.FS

func wasmGameSubFS() fs.FS {
	sub, err := fs.Sub(wasmDist, "wasmdist")
	if err != nil {
		panic("wasmdist embed: " + err.Error())
	}
	return sub
}
