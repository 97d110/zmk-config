FROM alpine:latest

RUN apk add --no-cache bash ripgrep

WORKDIR /workspace

CMD ["bash", "./scripts/agentic/verify.sh"]
