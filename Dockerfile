FROM emscripten/emsdk:2.0.34

RUN apt-get update && \
	DEBIAN_FRONTEND=noninteractive apt-get install -y \
	python3-pillow nasm

RUN useradd -m user && mkdir /work && chown user:user /work
USER user
WORKDIR /work
COPY --chown=user . .

RUN git submodule update --init --recursive
RUN npm install
RUN make
