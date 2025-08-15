# Description
This repo contains X different projects:<br/>
An text editor in C

## To run TextEditorInC:
A(docker needed):<br/>
  - Pull this repo
  - run:Docker build -t texteditor-image "{absolut path}" <br/> or <br/> from git diractory in TextEditorInC sub folder run: Docker build -t texteditor "./"
  - run: docker run -it -v {absolut path}:/data texteditor-image /data/{filename}
