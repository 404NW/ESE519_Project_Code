# Installation Guide of RP2040 SDK


## Machine Environment

** OS:** Ubuntu 22.04.1 LTS

** Processor:** Intel i5-11400H

** Laptop Model:** GigaByte G5


## Installed Software
**1. Terminal**
    Since the Ubuntu OS is a Linux OS, it contains a terminal. That's what we are gonna use.

**2. VIM / Text Editor**
    gVIM, a graphical VIM text editor which can be found from the ubuntu official software store.

**3. Serial Console**

(1) Firstly, the official guide of setting up the serial console and connect your RP2040 can be accessed by this [website](https://learn.adafruit.com/welcome-to-circuitpython/advanced-serial-console-on-linux).

(2) Secondly, to install screen, use `sudo apt install screen`

**4. Git**

Use ` sudo apt install git` to install git so that you could clone the repos from the github

**5. Cmake & GNU Embedded Toolchain for ARM**

Use ` sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build essential` to install all Cmake&GNU toolchain for ARM

## Detailed Guide from Zero to Hello.C
### Step 0: Environment Installation

1. Before starting, you need to install softwares #1-#4 in the installed software section.


### Step 1: Get the SDK and examples

1. Use git to clone the `pico-examples` repository (https://github.com/raspberrypi/pico-examples) and the `pico-sdk` repository (https://github.com/raspberrypi/pico-sdk)

2. Code

```
    git clone -b master https://github.com/raspberrypi/pico-examples
    cd pico-sdk
    git submodule update --init
    git pull
    git submodule update
    cd ..
    git clone -b master https://github.com/raspberrypi/pico-examples
```
Now, you have the local copy of the SDK and bunch of examples including the hello.c, which is what we are gonna use in the following steps

### Step 2: Install the Toolchain
1. Use the following code to install the toolchain for ARM

```
    sudo apt update
    sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build essential
```
2. Additionally, you may need to check if the libstd++-arm-none-eabi-newlib has been installed automatically by ` sudo apt install libstd++-arm-none-eabi-newlib`

** From this point, you are all set with the SDK environment setup and we are gonna start with the example: hello_world! **

### Step 3: Building "Hello_World"

1. From the examples repo we created earlier, create a build directory by `mkdir build` and enter this directory by `cd build`
2. Then, set the `PICO_SDK_PATH` variable by `export PICO_SDK_PATH=../../PICO-SDK`, so that the system can found the sdk for compilation
3. prepare your cmake build directory by running cmake by the code `cmake ..`
4. Now, enter the target project and build the project by `make -j4`. To be more detailed, `-j4` indicates the building will be processed in parallel by 4 processors.
5. As a result, after the building has finished, a `.uf2` file can be found from the `build/target_project_name` folder.

## Step 4: Load the compiled file to the board and make it work!
1. To do this, you need to connect your RP2040 board with the computer through USB and then drag the `.uf2` file to the RP2040 in the GUI of your file management system. Then, the RP2040 will be automatically unmounted from your computer and rebooted. After the reboot finished, the loaded program will start.
2. In order to monitor the output from your RP2040, open the serial console by steps below(assume you are in your home directory)

    (1) Firstly, find out the serial port number by ` ls /dev/ttyACM*` after you connect your RP2040 with your computer through USB.

    (2) Secondly, connect with screen by `screen /dev/ttyACM0 115200`

    (3) Finally, you should get connected with your RP2040 if you could see the REPL information if you use CTRL-C.
