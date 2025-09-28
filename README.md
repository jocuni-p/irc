
# ft_irc - Internet Relay Chat Server

![C++](https://img.shields.io/badge/C++-98-blue)
![42](https://img.shields.io/badge/42-Barcelona-00babc)


## ¿Qué es IRC?

IRC (Internet Relay Chat) es un protocolo de comunicación en tiempo real basado en texto que permite conversaciones grupales en canales y mensajes privados entre usuarios. Desarrollado en 1988, fue uno de los primeros sistemas de chat en Internet y sigue siendo popular en comunidades técnicas.

## Tabla de Contenidos

- [¿Qué aprenderemos?](#-qué-aprenderemos)
- [Características](#-características)
- [Instalación](#-instalación)
- [Uso](#-uso)
- [Comandos Implementados](#-comandos-implementados)
- [Estructura del Proyecto](#-estructura-del-proyecto)
- [Tecnologías](#-tecnologías)
- [Testing](#-testing)
- [Protocolo](#-protocolo)
- [Autores](#-autores)
- [Licencia](#-licencia)


## ¿Qué aprenderemos?

### La Importancia de los Sockets en la Comunicación entre Procesos

Este proyecto profundiza en uno de los conceptos fundamentales de la programación de sistemas: **la comunicación entre procesos mediante sockets**. Los sockets son la base de toda comunicación en red moderna y entender su funcionamiento es crucial para cualquier desarrollador de sistemas.

#### ¿Qué son los Sockets?
Los sockets son endpoints de comunicación que permiten que procesos diferentes (incluso en máquinas diferentes) se comuniquen entre sí. Actúan como puertas a través de las cuales los programas pueden enviar y recibir datos.

#### Conceptos Clave Aprendidos:

**1. Comunicación Cliente-Servidor**
```cpp
// Servidor - Creación y configuración
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
bind(server_fd, (struct sockaddr*)&address, sizeof(address));
listen(server_fd, BACKLOG);
```

**2. I/O No Bloqueante y Multiplexación**
- Uso de `poll()` para manejar múltiples conexiones simultáneas
- Evitar que el servidor se bloquee esperando datos de un cliente
- Gestión eficiente de recursos del sistema

**3. Protocolos de Comunicación**
- Implementación del protocolo IRC sobre TCP
- Formato de mensajes y parsing de comandos
- Gestión de estados de conexión

**4. Gestión de Conexiones de Red**
```cpp
// Aceptar nuevas conexiones
int new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
fcntl(new_socket, F_SETFL, O_NONBLOCK);
```

**5. Comunicación en Tiempo Real**
- Forwarding de mensajes entre múltiples clientes
- Sincronización de estado en canales
- Manejo de desconexiones inesperadas

#### Por qué es importante:
- **Fundamento de Internet**: HTTP, SSH, FTP, y todos los protocolos web usan sockets
- **Aplicaciones Distribuidas**: Sistemas modernos como microservicios dependen de la comunicación entre procesos
- **Performance**: El I/O no bloqueante es esencial para aplicaciones de alta concurrencia
- **Debugging**: Entender el flujo de datos ayuda a diagnosticar problemas complejos de red

Este proyecto no solo te enseña a implementar un chat, sino los **cimientos de toda aplicación distribuida moderna**.

## Características

- **Múltiples clientes simultáneos** usando I/O no bloqueante
- **Comunicación TCP/IP** con sockets BSD
- **Canales IRC locales** según el estándar moderno
- **Sistema de autenticación completo** (CAP, PASS, NICK, USER)
- **Mensajería** pública en canales y mensajes privados
- **Comandos de operador** para moderación de canales
- **Implementación basada en el protocolo IRC moderno**
- **Compatibilidad con HexChat y netcat**

## Instalación

### Prerrequisitos
- Compilador C++ (g++/clang++)
- Make
- Cliente IRC (orientado a clientes HexChat o netcat para testing)

### Compilación
```bash
git clone https://github.com/jocuni-p/irc.git
cd irc
make
```

El Makefile incluye las reglas estándar:
- `make` / `make all` - Compila el proyecto
- `make run` - Ejecuta el servidor con parametros de prueba (puerto:6667, contraseña:123456)
- `make clean` - Limpia los objetos
- `make fclean` - Limpia objetos y ejecutable  
- `make re` - Recompila completamente

## Uso

### Ejecución del servidor
```bash
./ircserv <puerto> <contraseña>
```

**Parámetros:**
- `<puerto>`: Puerto donde el servidor escuchará conexiones (ej: 6667)
- `<contraseña>`: Contraseña requerida para la autenticación de clientes

### Ejemplo
```bash
./ircserv 6667 mysecretpassword
```

## Comandos Implementados

### Autenticación y Registro
| Comando | Descripción | Uso |
|---------|-------------|-----|
| `CAP`   | Negociación de capacidades | `CAP LS` |
| `PASS`  | Autenticación con contraseña | `PASS <contraseña>` |
| `NICK`  | Establecer nickname | `NICK <apodo>` |
| `USER`  | Registrar usuario | `USER <usuario> <modo> <unused> <nombre_real>` |

### Comandos Básicos de Usuario
| Comando | Descripción | Uso |
|---------|-------------|-----|
| `JOIN`  | Unirse a canal | `JOIN #<canal>` |
| `PRIVMSG`| Mensaje privado/canal | `PRIVMSG <destino> :<mensaje>` |
| `TOPIC` | Cambiar/ver tema del canal | `TOPIC #<canal> [nuevo_tema]` |
| `QUIT`  | Desconectarse del servidor | `QUIT [mensaje]` |
| `WHO`   | Listar usuarios en canal | `WHO #<canal>` |

### Comandos de Operador de Canal
| Comando | Descripción | Uso |
|---------|-------------|-----|
| `KICK`  | Expulsar usuario del canal | `KICK #<canal> <usuario> [razón]` |
| `INVITE`| Invitar usuario a canal | `INVITE <usuario> #<canal>` |
| `MODE`  | Gestionar modos de canal | `MODE #<canal> <modo> [parámetro]` |

### Modos de Operador de Canal Implementados
- `i` - Canal solo por invitación
- `t` - Restricción de TOPIC a operadores
- `k` - Contraseña del canal
- `o` - Dar/quitar privilegios de operador
- `l` - Límite de usuarios en el canal

## Estructura del Proyecto

```
.
├── Makefile
├── include
│   ├── Channel.hpp      # Gestión de canales y usuarios
│   ├── Client.hpp       # Representación de clientes conectados
│   ├── Server.hpp       # Servidor principal y configuración
│   ├── Utils.hpp        # Funciones utilitarias
│   └── debug.hpp        # Herramientas de depuración
└── src
    ├── Channel.cpp           # Lógica de canales
    ├── Client.cpp            # Gestión de clientes
    ├── Server.cpp            # Servidor principal
    ├── ServerAuth.cpp        # Autenticación (CAP, PASS, NICK, USER)
    ├── ServerCommands.cpp    # Comandos IRC (JOIN, PRIVMSG, etc.)
    ├── ServerHelpers.cpp     # Funciones auxiliares del servidor
    ├── ServerIO.cpp          # Comunicación por sockets (I/O no bloqueante)
    ├── Utils.cpp             # Utilidades generales
    ├── debug.cpp             # Sistema de depuración
    └── main.cpp              # Punto de entrada
```

## Tecnologías

- **Lenguaje**: C++98
- **Sockets**: Implementación basada en [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/pdf/bgnet_a4_c_1.pdf)
- **I/O**: No bloqueante con `poll()`
- **Compilación**: Makefile con flags `-Wall -Wextra -Werror -std=c++98`
- **Protocolo**: Basado en [Modern IRC Documentation](https://modern.ircdocs.horse/#local-channels)

## Testing

### Con HexChat (Recomendado)
1. Abre HexChat
2. En "Network List", añade un nuevo servidor:
   - **Name**: `ft_irc Local`
   - **Server**: `127.0.0.1/6667`
3. En "Connect Commands", añade:
   ```
   PASS tu_contraseña
   ```
4. Conecta y usa los comandos IRC normalmente

### Con netcat (Testing básico)
```bash
# Conexión básica
nc -C 127.0.0.1 6667

# Autenticación completa
echo -e "CAP LS\r\nPASS mypassword\r\nNICK testuser\r\nUSER user 0 * :Real Name\r\nJOIN #test" | nc -C 127.0.0.1 6667
```

### Flujo de autenticación típico
```irc
CAP LS
PASS serverpassword
NICK mynickname  
USER myusername 0 * :My Real Name
JOIN #channel
PRIVMSG #channel :Hello world!
```

## Protocolo

La implementación sigue el estándar moderno de IRC documentado en:
- **[Modern IRC Documentation](https://modern.ircdocs.horse/)** - Protocolo completo
- **Canales locales** - Soporte para canales comenzando con `#`
- **Mensajes formateados** - Según especificación IRC
- **Gestión de errores** - Respuestas numéricas estándar

### Base de Sockets
La lógica de comunicación de sockets está basada en la guía clásica:
- **[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/pdf/bgnet_a4_c_1.pdf)**

## Autores

- **Sergio Maximiliano Pereyra** - [smaxpereyra](https://github.com/smaxpereyra)
- **Joan Cuní** - [jocuni-p](https://github.com/jocuni-p)

## Licencia

Este proyecto fue desarrollado como parte del currículo de [42 Barcelona](https://www.42barcelona.com/).

---


