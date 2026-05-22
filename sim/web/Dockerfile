# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

FROM alpine:latest

RUN apk add --no-cache build-base python3

WORKDIR /workspace

COPY . /workspace

EXPOSE 8080

CMD ["python3", "sim/web/app.py"]
