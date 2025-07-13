FROM emscripten/emsdk:2.0.34

RUN apt-get update && \
	DEBIAN_FRONTEND=noninteractive apt-get install -y \
	python3-pillow nasm

RUN useradd -m user && mkdir /work && chown user:user /work
USER user
WORKDIR /work

COPY --chown=user package.json package-lock.json ./
RUN npm install

# Pre-build the engine portion using a limited set of dependencies
COPY --chown=user library library
COPY --chown=user original original
COPY --chown=user Makefile ./
COPY --chown=user src/engine src/engine
COPY --chown=user src/assets/check-originals.py src/assets/
COPY --chown=user src/assets/fs-packer.py src/assets/
COPY --chown=user src/assets/showfile-repacker.py src/assets/
RUN make build/original
RUN make build/engine.js

COPY --chown=user . .
RUN make
