FROM emscripten/emsdk:2.0.34

# Install debian dependencies
RUN apt-get update && \
	DEBIAN_FRONTEND=noninteractive apt-get install -y \
	nasm python3-venv

# Switch to non-root user
RUN useradd -m user && mkdir /work && chown user:user /work
USER user
WORKDIR /work

# Pre-build emscripten system libraries
RUN embuilder.py build --lto libembind-rtti libc libcompiler_rt libc++-noexcept libc++abi-noexcept libemmalloc libc_rt_wasm-optz libsockets

# Install python dependencies in a venv
COPY --chown=user requirements.txt requirements.txt ./
RUN python3 -m venv venv; . venv/bin/activate; pip3 install -r requirements.txt

# Install npm dependencies
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
RUN . venv/bin/activate; make build/original
RUN . venv/bin/activate; make build/engine.js

COPY --chown=user . .
RUN . venv/bin/activate; make
