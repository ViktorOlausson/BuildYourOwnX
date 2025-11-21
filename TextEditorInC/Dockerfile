FROM ubuntu:22.04

ENV TERM xterm-256color

RUN apt-get update && \
   apt-get install -y --no-install-recommends git build-essential ca-certificates && \ 
   rm -rf /var/lib/apt/lists/*

#RUN git clone https://github.com/ViktorOlausson/BuildYourOwnX.git /app

WORKDIR /app/TextEditorInC

COPY . .

#RUN gcc main.c -o TextEditor

RUN make

ENTRYPOINT [ "/app/TextEditorInC/text_editor"]