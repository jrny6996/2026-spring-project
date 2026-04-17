#!/bin/bash

/bin/sh -ec 'cd server && go run .' &

/bin/sh -ec '
cd client_wasm/build &&
emcmake cmake .. -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXECUTABLE_SUFFIX=.html &&
emmake make &&
emrun example.html
'"'"'' &

wait