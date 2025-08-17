FROM --platform=$TARGETPLATFORM ubuntu:22.04

ENV TERM xterm-256color

RUN apt-get update && \
   apt-get install -y --no-install-recommends git build-essential ca-certificates && \ 
   rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/ViktorOlausson/BuildYourOwnX.git /app

WORKDIR /app/TextEditorInC

RUN gcc main.c -o TextEditor

ENTRYPOINT [ "/app/TextEditorInC/TextEditor"]