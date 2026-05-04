package main

import (
	"embed"
	"io/fs"
)

var playPageHTML string

var wasmDist embed.FS

func wasmGameSubFS() fs.FS {
	sub, err := fs.Sub(wasmDist, "wasmdist")
	if err != nil {
		panic("wasmdist embed: " + err.Error())
	}
	return sub
}
