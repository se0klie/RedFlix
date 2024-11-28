# Redflix
REDFLIX es un sistema diseñado para la reproducción de videos. Consiste en varios componentes que trabajan juntos utilizando variables compartidas para procesar y entregar el contenido con el mejor bitrate posible. Cada componente juega un rol específico para la codificación, streaming y visualización del video.

## Componentes
1. global_server:
* Recibe las conexiones del cliente y las envía a logic_side, que maneja la parte lógica para el procesamiento del video.
* Actúa como el punto de entrada para requests de clientes.

## Instalación.
1. Clona el repositorio:
```
git clone https://github.com/se0klie/RedFlix.git
```
2. Navega hasta el directorio REDFLIX en tu escritorio.
3. Compila el código.
```
make
```
## Uso.
1. Inicia el server global con cualquier puerto permitido:
```
./global_server <puerto>
```
Ej: ./global_server 8080
2. Inicia la conexión como cliente, conectándote a la misma dirección y puerto en el que el servidor está escuchando.
```
./client <hostname> <puerto>
```
Ej: ./client 127.0.0.1 8080

## Comandos.
Desde el cliente, puedes enviar los siguientes mensajes:
1. -L, -M, -H dedicados a la calidad del video. (Baja, media o alta).
2. STOP. Pausa y cierra la conexión.
3. PLAY. Inicia el video. Deberás utilizar este comando apenas te conectes al servidor o hayas pausado el video previamente.
4. PAUSE. Pausa el vídeo.
5. REPLAY. En cualquier punto, si deseas resetear el video, usa este comando.
