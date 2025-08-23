# Description
This repo contains X different projects:<br/>
An text editor in C

### To run TextEditorInC:
A(docker needed):<br/>
  - Pull this repo
  - run: `Docker build -t texteditor-image "{absolut path}"` <br/> or <br/> from git diractory in TextEditorInC sub folder run: `Docker build -t texteditor "./"` or<br/> 
  - run: `docker run -d --name {your name} --mount type=bind,source="{absolut path}",target=/data -w /data --entrypoint sleep texteditor-image infinity`
  - run: `docker exec -it {your name} /app/TextEditorInC/TextEditor /data/{path}`<br/>
###
One Click(run) script(only Windows amd64 supported)
  - Pull `https://github.com/ViktorOlausson/Scripts.git`
  - Change into diractory where you saved it and move into `BuildYourOwnX\TextEditorInC`
  - Run: `.\runTextEditorInC`
