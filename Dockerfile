FROM ubuntu:20.04 AS builder

# The version number  will be handled automatically by "make docker" but not by Docker Hub
ARG FVERSION=2.0.2
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt-get install -y make sudo
# These are not necessary HERE, but copied from Makefile and help caching when testing locally
RUN apt-get install -y --no-install-recommends devscripts build-essential fakeroot

# Now do the "magic"
ADD ./flint++.tar /workspace/
WORKDIR /workspace/flint++-${FVERSION}
RUN make deb
RUN cp -l /workspace/flint++-${FVERSION}/flint*.deb /workspace/

# Now we build the actual image we are distributing so minimize layers
FROM ubuntu:latest
LABEL maintainer="Aaron D. Marasco <github-flintplusplus@marascos.net>"

COPY --from=builder /workspace/flint*.deb /
RUN apt-get update && \
    apt-get install -y --no-install-recommends /flint*.deb && \
    rm -rf /var/lib/apt/lists/*

# Set our default executable
ENTRYPOINT ["/usr/bin/flint++", "-r", "/src/"]
