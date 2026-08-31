# Como compilar y probar NOCTIS

En Windows no existe binario nativo del toolchain de PSP, asi que se compila
dentro de **WSL2** (Linux integrado en Windows) y se prueba en **PPSSPP**
(emulador de Windows). La PSP fisica se prueba copiando el `EBOOT.PBP`.

## 1. Instalar WSL2 (una sola vez, requiere administrador + reinicio)

Abre **PowerShell como Administrador** (clic derecho > Ejecutar como
administrador) y corre:

    wsl --install -d Ubuntu

Reinicia el PC si te lo pide. Al terminar, Ubuntu te pedira crear un usuario y
contrasenia. Si tu PC da error de "virtualizacion", hay que activarla en la
BIOS (Intel VT-x / AMD-V).

## 2. Instalar el toolchain pspdev dentro de WSL (una sola vez)

Abre "Ubuntu" desde el menu inicio y corre:

    sudo apt-get update
    sudo apt-get install -y build-essential cmake curl
    curl -L https://github.com/pspdev/pspdev/releases/latest/download/pspdev-ubuntu-latest-x86_64.tar.gz -o /tmp/pspdev.tar.gz
    sudo tar -xzf /tmp/pspdev.tar.gz -C /usr/local
    echo 'export PSPDEV=/usr/local/pspdev' >> ~/.bashrc
    echo 'export PATH=$PSPDEV/bin:$PATH'  >> ~/.bashrc
    source ~/.bashrc
    psp-config --pspdev-path   # verificacion: debe imprimir /usr/local/pspdev

## 3. Compilar

Desde WSL, entra a la carpeta del proyecto (el disco C: es /mnt/c):

    cd /mnt/c/Users/Usuario/Desktop/Proyectos_Claude/noctis-psp
    bash tools/build.sh

Genera `build/EBOOT.PBP`.

## 4. Probar

- **PPSSPP**: abre PPSSPP y carga `build/EBOOT.PBP` (o crea la carpeta
  `PSP/GAME/NOCTIS/` en la memstick de PPSSPP y copia ahi el EBOOT.PBP).
- **PSP real**: copia el EBOOT.PBP a `PSP/GAME/NOCTIS/EBOOT.PBP` en la
  memoria de la PSP.
