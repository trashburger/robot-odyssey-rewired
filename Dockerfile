# Using debian experimental so we can have clang >= 20
FROM debian:rc-buggy

# Install debian dependencies
RUN apt-get update && \
	DEBIAN_FRONTEND=noninteractive apt-get install -y \
	clang-22 lld-22 npm nasm python3-zstd python3-pillow \
	bzip2 make

ENV CLANG_VER=22

# Switch to non-root user
RUN useradd -m user && mkdir /work && chown user:user /work
USER user
WORKDIR /work

# Install npm dependencies
COPY --chown=user package.json package-lock.json ./
COPY --chown=user packages/engine/package.json packages/engine/
COPY --chown=user packages/cli/package.json packages/cli/
COPY --chown=user packages/web/package.json packages/web/
RUN npm install

# Pre-build the engine using a limited set of dependencies
COPY --chown=user library library
COPY --chown=user original original
COPY --chown=user notes notes
COPY --chown=user packages/engine packages/engine
RUN npm run build:engine

# Full build
COPY --chown=user . .
RUN npm run build

# Run tests
RUN npm run test
