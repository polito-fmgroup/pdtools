# Container image for pdtrav and pdtapp.
#
# Two stages: "builder" compiles the tools with the full toolchain, and the
# final image keeps only the two executables plus the shared libraries they
# need at run time.
#
# Build:  docker build -t pdtrav .
# Run:    docker run --rm pdtrav -version
#         docker run --rm -v "$PWD":/data pdtrav [options] /data/model.aig
#         docker run --rm --entrypoint pdtapp pdtrav [options]

# --- Stage 1: build -----------------------------------------------------

FROM ubuntu:24.04 AS builder

# Build dependencies: the compiler toolchain, flex and bison (used to
# generate the expression parser) and the readline and zlib headers. `make check-deps`
# verifies the same set.
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        flex \
        bison \
        libreadline-dev \
        zlib1g-dev \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# check-deps fails the build early if the packages above stop matching what
# the Makefile needs. The build runs serially (-j1); parallel builds have not
# been validated. A local config.local.mk is excluded by .dockerignore so that
# the compiler is not taken from the host.
RUN make check-deps && make -j1

# --- Stage 2: runtime ---------------------------------------------------

FROM ubuntu:24.04 AS runtime

# Run-time libraries, as reported by `ldd pdtrav/bin/pdtrav`. 
# Note, on Ubuntu 24.04 the package that provides libreadline.so.8 is libreadline8t64
RUN apt-get update && apt-get install -y --no-install-recommends \
        libreadline8t64 \
        libtinfo6 \
        libstdc++6 \
        zlib1g \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --create-home --shell /usr/sbin/nologin pdtrav

COPY --from=builder /src/pdtrav/bin/pdtrav /usr/local/bin/pdtrav
COPY --from=builder /src/pdtrav/bin/pdtapp /usr/local/bin/pdtapp

# Mount point for model files (`docker run -v "$PWD":/data`), owned by the
# unprivileged user the container runs as.
RUN mkdir -p /data && chown pdtrav:pdtrav /data
WORKDIR /data
USER pdtrav

ENTRYPOINT ["pdtrav"]
CMD ["-version"]
