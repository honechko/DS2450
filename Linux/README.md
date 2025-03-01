# 1-wire port expander driver for Linux

## Build Out-of-Tree (fast way)

* To build the module, you may need to install kernel headers and makefiles if you have not done so already:

    ```$ sudo yum install -y kernel-devel```  
    or  
    ```$ sudo apt-get install -y linux-headers-$(uname -r)```  

* Build module:

    ```$ cd kernel/module```  
    ```$ make```  

* Load module:

    ```$ sudo modprobe wire```  
    ```$ sudo insmod ./w1_ds2450.ko```  

## Build In-Tree (preferred way)

* Download and unpack [kernel source](http://kernel.org/) ([rpi kernel source](https://github.com/raspberrypi/linux/))

* Optionally apply patches from [patches directory](https://github.com/honechko/DS2450/raw/main/Linux/kernel/patches/) and [ser1wm patches directory](https://github.com/honechko/ser1wm/raw/main/kernel/patches/):

    ```$ patch -p1 < /path/to/enhancement_w1_touchblock.patch```  
    ```$ patch -p1 < /path/to/enhancement_w1_resetcounter.patch```  
    ```$ patch -p1 < /path/to/fix_w1_slavecount.patch```  
    ```$ patch -p1 < /path/to/fix_w1_loadmodule.patch```  
    ```$ patch -p1 < /path/to/fix_w1_thermpullup.patch```  
    ```$ patch -p1 < /path/to/fix_gpiolib.patch```  

* Add driver to source tree:

    ```$ cp /path/to/w1_ds2450.c drivers/w1/slaves```  
    ```$ patch -p1 < /path/to/intree-w1_ds2450.patch```  

* Configure, make and install modules as usual:

    ```$ make menuconfig```  
    ```$ make modules```  
    ```$ sudo make modules_install```  

## See also

* [Serial 1-wire master driver for Linux](https://github.com/honechko/ser1wm/)

