FROM node:18-slim

RUN apt-get update && apt-get install -y git && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/ViktorOlausson/BuildYourOwnX.git /app

WORKDIR /app/NodeJsWebServer

EXPOSE 5000

CMD ["node", "server.js"]