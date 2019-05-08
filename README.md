# DMUTEX
Esta práctica plantea el desarrollo de un escenario experimental que implemente un algoritmo de exclusión mutua distribuida. En concreto se plantea la implementación de un algoritmo basado en marcas de tiempo.

# Objetivo de la práctica
La práctica consiste en el desarrollo de un prototipo que implemente la lógica de control para la gestión de relojes lógicos vectoriales así como el protocolo de acceso a una región con exclusión mutua. Para ello se deberá desarrollar un programa que:
- Mediante la entrada estándar (_stdin_) reciba la secuencia de acciones a realizar.
- Utilice el protocolo UDP y la librería de sockets para comunicarse con otros procesos
análogos.
- Genere mensajes de traza y control por medio de la salida estándar (_stdout_).
El programa solicitado sólo tendrá que tener un único hilo de ejecución (no es necesario implementar varios threads) puesto que la entrada de acciones será la que determinará la secuencia de sucesos que atenderá el proceso. Se ha de entender que esto es una implementación de un prototipo y que este mecanismo permite forzar secuencias de acciones que permitan estudiar el comportamiento de ambos algoritmos (es una simulación).

Para más información, consultar el [enunciado de la práctica](/doc/dmutex.2018.pdf).

## Autor
[Víctor Nieves Sánchez](https://twitter.com/VictorNS69)

## Licencia
[Licencia](/LICENSE).
