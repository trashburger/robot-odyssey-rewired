FROM emscripten/emsdk:2.0.34

RUN apt-get update && \
	DEBIAN_FRONTEND=noninteractive apt-get install -y \
	python3-pillow nasm

RUN useradd -m user
USER user
WORKDIR /home/user
COPY --chown=user . .

RUN git submodule update --init --recursive
RUN npm install
RUN make
