# Description
This repo contains X different projects:<br/>
An text editor in C

### To run TextEditorInC(amd64):
A(docker needed):<br/>
  - Pull this repo
  - run: `Docker build -t texteditor-image "{absolut path}" <br/>` or <br/> `from git diractory in TextEditorInC sub folder run: Docker build -t texteditor "./"` or<br/> 
  - run: `docker run -d --name {your name} --mount type=bind,source="{absolut path}",target=/data -w /data --entrypoint sleep texteditor-image infinity`
  - run: `docker exec -it {your name} /app/TextEditorInC/TextEditor /data/{path}`<br/>
###
B(docker needed)
  - Run: `docker pull viktorolausson/texteditorinc`
  - Run: `docker run --rm -it --mount type=bind,source="{path}",target=/data/ viktorolausson/texteditorinc:latest /data/{path}`
