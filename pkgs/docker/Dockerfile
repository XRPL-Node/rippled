FROM ubuntu:jammy

COPY rippled_*_amd64.deb /tmp/
RUN apt-get update && \
    apt-get install -y --no-install-recommends /tmp/rippled_*_amd64.deb && \
    rm -f /tmp/*.deb && \
    rm -rf /var/lib/apt/lists/*

RUN <<EOF
useradd \
    --system \
    --no-create-home \
    --shell /usr/sbin/nologin \
    rippled
chown -R \
    rippled:rippled \
    /var/lib/rippled \
    /var/log/rippled \
    /opt/ripple
EOF
EXPOSE 51235 6006
USER rippled

ENTRYPOINT ["/opt/ripple/bin/rippled"]
