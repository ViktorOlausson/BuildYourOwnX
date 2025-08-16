# Description
This repo contains X different projects:<br/>
An text editor in C

## To run TextEditorInC:
A(docker needed):<br/>
  - Pull this repo
  - run: `Docker build -t texteditor-image "{absolut path}" <br/> or <br/> from git diractory in TextEditorInC sub folder run: Docker build -t texteditor "./"`
  - run: `docker run -d --name {your name} --mount type=bind,source="{absolut path}",target=/data -w /data --entrypoint sleep texteditor-image infinity`
  - run `docker exec -it {your name} /app/TextEditorInC/TextEditor /data/{path}`<br/>
  <br/>
A(docker needed):<br/>
  - Pull this repo
  - run: `Docker build -t texteditor-image "{absolut path}" <br/> or <br/> from git diractory in TextEditorInC sub folder run: Docker build -t texteditor "./"`
  - run: `docker run -d --name {your name} --mount type=bind,source="{absolut path}",target=/data -w /data --entrypoint sleep texteditor-image infinity`
  - run `d